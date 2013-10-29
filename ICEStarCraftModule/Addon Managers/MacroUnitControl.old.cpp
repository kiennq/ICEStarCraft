#include <BWAPI.h>
#include <BWTA.h>
#include <time.h>
#include "MacroUnitControl.h"
#include "MicroUnitControl.h"
#include "Vector2.h"

using namespace std;
using namespace BWAPI;
using namespace Helper;
using namespace ICEStarCraft;

MacroManager* theMacroManager = NULL;

MacroManager* MacroManager::create() 
{
	if (theMacroManager) return theMacroManager;
	else return theMacroManager = new MacroManager();
}

void MacroManager::destroy()
{
	if (theMacroManager) delete theMacroManager;
	theMacroManager = NULL;
	DropManager::destroy();
}

MacroManager::MacroManager()
{
	this->bom = NULL;
	this->mental = NULL;
	this->scm = NULL;
	this->mInfo = NULL;
	this->baseManager = BaseManager::create();
	this->terrainManager = TerrainManager::create();
	this->onceCommand = new issueOnce();
	this->dropManager = DropManager::create();
	this->mDefendTeam.clear();
	this->DefendTeamSize = 0;
	this->btCount = 0;
	this->receivedBTPositions.clear();
	this->allEnemyUnits.clear();
	//this->goOutGroup.clear();
	this->CCPosition = BWTA::getStartLocation(Broodwar->self())->getPosition();
	this->highGroundTankLimitation = 0;
	this->nextSetPoint = Positions::None;
	this->lastScanTime = 0;
	this->lastTourMapTime = 0;
	this->lastAttackPosition.clear();
	this->lastAllUnitAttack = 0;
	this->desEnemyMainBase = false;
	this->lastScanPosition = Positions::None;
	
	this->searchingUnits.clear(); 
	this->attackers.clear();
	this->myBattleUnits.clear();
	this->enemyBattleUnits.clear();
	std::set<BaseLocation*> aBL = BWTA::getBaseLocations();
	for each(BaseLocation* bl in aBL)
	{
		this->positionTOattack.insert(make_pair(bl->getPosition(),make_pair(false,0)));
	}
	this->scanTableFlag = false;
	this->groupCenter = NULL;
	this->gf = GameFlow::create();
	this->unitGroupCenterLimitation = 0;
	this->vultureGroup.clear();
	this->startTP = TilePositions::None;
	this->endTP = TilePositions::None;
	this->atkTar.first = "No_AtkTar";
	this->atkTar.second = Position(0,0);
	this->lastMineStart = TilePositions::None;
	this->lastMineEnd = TilePositions::None;
	this->TankDefBase.clear();
	this->theMarine = NULL;
	this->toGoIn = NULL;
	this->gatherPoint = Positions::None;
}

void MacroManager::setManagers(BuildOrderManager* bom)
{
	this->bom = bom;
	this->mental = MentalClass::create();
	this->terrainManager = TerrainManager::create();
	this->scm = ScoutManager::create();
	this->eInfo = EnemyInfoManager::create();
	this->mInfo = MyInfoManager::create();
	this->dropManager->setManagers(bom);
}

void MacroManager::allUnitAttack(Unit* e, Position p, bool needTank /*= true*/)
{
	if (this->lastAllUnitAttack == Broodwar->getFrameCount())
	{
		return;
	}

	this->lastAllUnitAttack = Broodwar->getFrameCount();

	//if the tile position is visible and enemy does not exist, then return
	if (e && !e->exists() && Broodwar->isVisible(e->getTilePosition()))
	{
		return;
	}

	// set gatherPoint and gather units before the attack
	if (this->mental->goAttack 
		  && (this->atkTar.first == "MainBase" || this->atkTar.first == "Only_MainBase" || this->atkTar.first == "Expansion" || this->atkTar.first == "Enemy_Army"))
	{
		BWTA::Region* atkReg = BWTA::getRegion(this->atkTar.second);
		if (atkReg)
		{
			int px = (atkReg->getCenter().x()/32 + Broodwar->mapWidth()/2)/2;
			int py = (atkReg->getCenter().y()/32 + Broodwar->mapHeight()/2)/2;
			this->gatherPoint = Position(this->terrainManager->getConnectedTilePositionNear(TilePosition(px,py)));

			Broodwar->drawCircleMap(gatherPoint.x(),gatherPoint.y(),20,Colors::Purple,true);
		}
		else
		{
			this->gatherPoint = Positions::None;
		}	
	}
	else
	{
		this->gatherPoint = Positions::None;
	}

	if (this->mental->shouldWaitBeforeAttack && this->gatherPoint != Positions::None)
	{
		if (attackers.inRadius(32*10,gatherPoint).size() < 0.8 * attackers.size())
		{ 
			// gather units and send 1 unit to scout
			/*if ((toGoIn || this->mental->scoutBeforeAttack) && !toGoIn->exists())
			{	
			this->mental->scoutBeforeAttack = false;
			toGoIn = NULL;
			}*/

			bool needScout = (attackers.inRadius(32*10,gatherPoint).size() >= 0.2 * attackers.size()) && !this->mental->hasScoutedBeforeAttack;
			int minD = 99999999;
			for each (Unit* u in attackers)
			{	
				if (toGoIn && u == toGoIn)
				{
					u->attack(p);
					continue;
				}

				if (needScout &&
					  !u->getType().isFlyer() &&
						u->getType() != UnitTypes::Terran_Siege_Tank_Siege_Mode &&
						u->getType() != UnitTypes::Terran_Siege_Tank_Tank_Mode &&
						u->getPosition().getApproxDistance(p) < minD)
				{
					minD = u->getPosition().getApproxDistance(p);
					toGoIn = u;
				}

				if (needTank && (u->getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode || u->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode))
				{
					tankAttackMode(u,gatherPoint);
				}
				else if (u->getPosition().getApproxDistance(this->gatherPoint) > 32*3)
				{
					if (!u->isAttackFrame())
						u->attack(this->gatherPoint);
				}
				//else
				//	u->holdPosition();
			}
			if (toGoIn && !this->mental->hasScoutedBeforeAttack)
			{
				this->mental->hasScoutedBeforeAttack = true;
				toGoIn->attack(p);
			}
			return;
		}
		else
		{ // let units attack enemy
			if (this->mental->hasScoutedBeforeAttack)
			{
				this->mental->shouldWaitBeforeAttack = false;
			}
		}
	}

	for each (Unit* u in this->attackers)
	{
		if (needTank && (u->getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode || u->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode))
		{
			tankAttackMode(u,p);
		}
		else
		{
			if (u->isAttackFrame())
			{
				continue;
			}

			if (!u->isIdle() &&
				  Broodwar->getFrameCount() - u->getLastCommandFrame() < 24*5 && 
					u->getLastCommand().getType() == UnitCommandTypes::Attack_Move &&
					u->getLastCommand().getTargetPosition().getApproxDistance(p) < 32*2)
			{
				continue;
			}

			u->attack(p);
		}
	}

	////check enemy type, if enemy is flyer
	//if (eu && (eu->getType().isFlyer() || eu->isLifted()))
	//{
	//	for each(Unit* u in attackers)
	//	{
	//		if (u->getType().airWeapon() == WeaponTypes::None)
	//			continue;

	//		if (u->getLastCommand().getTargetPosition()==u->getTargetPosition() && !u->isIdle())
	//			continue;

	//		else if(u->getGroundWeaponCooldown()!=0 || u->getAirWeaponCooldown()!=0)
	//			continue;	
	//		else if (Unit* best = getBestAttackTartget(u))
	//		{
	//			if (u->isAttacking() && u->getTarget() && u->getTarget()==best)
	//				continue;
	//			else
	//				u->attack(best);
	//		}						
	//		else if (u->isAttacking()||u->isAttackFrame())
	//			continue;		
	//		else
	//			u->attack(p);								
	//	}
	//}
	//else
	//{
	//	for each (Unit* u in this->attackers)
	//	{
	//		if (this->groupCenter && u->getType() != UnitTypes::Terran_Marine && Broodwar->enemy()->getRace() != Races::Terran)
	//		{
	//			if (u->isUnderAttack() && (u->getHitPoints()/u->getType().maxHitPoints()<=1/3))
	//			{
	//				u->move(*groupCenter);
	//				continue;
	//			}				
	//		}

	//		//fight mode
	//		if (u->getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode || u->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode)
	//			tankAttackMode(u,p);
	//		else
	//		{
	//			if (Broodwar->getFrameCount()%24 == 0)
	//			{
	//				if (u->getLastCommand().getTargetPosition()==u->getTargetPosition() && !u->isIdle())
	//					continue;
	//				else if(u->getGroundWeaponCooldown()!=0 || u->getAirWeaponCooldown()!=0)
	//					continue;	
	//				else if (Unit* best = getBestAttackTartget(u))
	//				{
	//					if (u->isAttacking() && u->getTarget() && u->getTarget()==best)
	//						continue;
	//					else
	//						u->attack(best);
	//				}						
	//				else if (u->isAttacking()||u->isAttackFrame())
	//					continue;		
	//				else
	//					u->attack(p);						
	//			}		
	//		}
	//	}
	//}
}

void MacroManager::tankAttackMode(Unit* tk, Position p, int reachRange)
{
	if (!tk || !tk->exists() || p == Positions::None)
	{
		return;
	}

	Broodwar->drawLineMap(tk->getPosition().x(),tk->getPosition().y(),p.x(),p.y(),Colors::White);
	int tankAttackRange = UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().maxRange();
	UnitGroup enemyInRange = SelectAllEnemy().not(isFlyer,isLifted).inRadius(tankAttackRange,tk->getPosition());
	reachRange *= 32;

	// if attack position is near siegePoint then just stay there
	if (this->siegePoint != Positions::None)
	{
		if (!this->mental->goAttack &&
			  p.getApproxDistance(this->siegePoint) < tankAttackRange * 1.5 &&
			  BWTA::getRegion(p) != BWTA::getRegion(Broodwar->self()->getStartLocation()))
		{
			p = this->siegePoint;
		}
	}

	if (p == this->siegePoint)
	{
		reachRange = 32*3;

		if (SelectAll()(Siege_Tank).inRadius(reachRange,p).size() > 4)
		{
			reachRange = 32*5;
		}

		if (SelectAll()(Siege_Tank).inRadius(reachRange,p).size() > 8)
		{
			reachRange = 32*7;
		}
	}

	/************************************************************************/
	/* Tank Mode                                                            */
	/************************************************************************/

	if (tk->getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode)
	{
		if (enemyInRange.empty())
		{
			if (tk->getPosition().getApproxDistance(p) > reachRange)
			{
				// move
				if (Broodwar->self()->getRace() != Races::Terran)
				{
					tk->attack(p);
					return;
				}

				// there may be invisible tanks. be careful!
				set<EnemyUnit*> invisibleEnemy;
				for each (EnemyUnit* e in this->eInfo->allEnemyUnits)
				{
					if (Broodwar->getFrameCount() - e->getLastUpdatedFrame() > 24*60*3)
					{
						continue;
					}

					if (e->getPosition() == Positions::Unknown)
					{
						continue;
					}

					if (e->getType() != UnitTypes::Terran_Siege_Tank_Siege_Mode && e->getType() != UnitTypes::Terran_Siege_Tank_Tank_Mode)
					{
						continue;
					}

					if (e->getPosition().getApproxDistance(tk->getPosition()) < tankAttackRange + 32*2)
					{
						Vector2 v1 = Vector2(e->getPosition()) - Vector2(tk->getPosition());
						Vector2 v2 = Vector2(p) - Vector2(tk->getPosition());
						if (v1.angle(v2) < 90) // this tank will get closer to enemy if move towards p
						{
							invisibleEnemy.insert(e);
						}
					}
				}
				if (!invisibleEnemy.empty())
				{
					//Broodwar->printf("be careful !!!");
					Broodwar->drawBoxMap(tk->getPosition().x()-15,tk->getPosition().y()-15,tk->getPosition().x()+15,tk->getPosition().y()+15,Colors::Orange);
					Broodwar->drawBoxMap(tk->getPosition().x()-20,tk->getPosition().y()-20,tk->getPosition().x()+20,tk->getPosition().y()+20,Colors::Orange);
					tk->siege();
				}
				else
				{
					tk->attack(p);
					return;
				}
			}
			else
			{
				// siege
				BWTA::Chokepoint* nearestCp = BWTA::getNearestChokepoint(tk->getPosition());
				if (nearestCp && nearestCp->getWidth() < 120 && tk->getPosition().getApproxDistance(nearestCp->getCenter()) < 32*4)
				{
					if (tk->getLastCommand().getType() != UnitCommandTypes::Attack_Move)
					{
						tk->attack(p);
						return;
					}
				}
				else
				{
					tk->siege();
				}
			}
		}
		else // enemy in range
		{
			double meleeUnitCount = 0;
			for each (Unit* e in enemyInRange)
			{
				if (e->getType().canAttack() && e->getType().groundWeapon() != WeaponTypes::None && e->getType().groundWeapon().maxRange() < 64)
				{
					if (e->getType() == UnitTypes::Zerg_Zergling)
					{
						meleeUnitCount += 0.5;
					}
					else
						meleeUnitCount += 1;
				}
			}

			// siege if enemy doesn't have many melee units
			if (p == this->siegePoint || meleeUnitCount < 0.7 * enemyInRange.size())
			{
				BWTA::Chokepoint* nearestCp = BWTA::getNearestChokepoint(tk->getPosition());
				if (nearestCp && nearestCp->getWidth() < 120 && tk->getPosition().getApproxDistance(nearestCp->getCenter()) < 32*4)
				{
					if (tk->getLastCommand().getType() != UnitCommandTypes::Attack_Move)
					{
						tk->attack(enemyInRange.getCenter());
						return;
					}
				}
				else
				{
					tk->siege();
					return;
				}
			}
			// attack in Tank mode
			// select target to attack
			Unit* target = NULL;
			double attackValue = 0;
			double maxValue = 0;

			if (!enemyInRange(canAttack).empty())
			{
				enemyInRange -= enemyInRange.not(canAttack);
			}

			for each (Unit* e in enemyInRange)
			{
				if (e->getPosition().getApproxDistance(tk->getPosition()) > UnitTypes::Terran_Siege_Tank_Tank_Mode.groundWeapon().maxRange())
				{
					continue;
				}

				if (e->getType() == UnitTypes::Protoss_High_Templar ||
					  e->getType() == UnitTypes::Zerg_Defiler	||
				  	e->getType() == UnitTypes::Zerg_Lurker ||
				  	e->getType() == UnitTypes::Protoss_Reaver || 
					  e->getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode || 
					  e->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode)
				{
					target = e;
					break;
				}

				double hp = e->getHitPoints() + e->getShields();
				double price = e->getType().mineralPrice() + e->getType().gasPrice();
				if (e->getType().canAttack() && e->getType().groundWeapon() != WeaponTypes::None)
				{
					double dpf = e->getType().groundWeapon().damageAmount() * 1.0 / e->getType().groundWeapon().damageCooldown();
					attackValue = dpf * price / hp;
				}
				else
				{
					attackValue = price / hp;
				}

				if (attackValue > maxValue)
				{
					maxValue = attackValue;
					target = e;
				}
			}

			// attack target
			if (target)
			{
				if (tk->getLastCommand().getType() == UnitCommandTypes::Attack_Unit && tk->getLastCommand().getTarget() == target)
				{
					return;
				}
				tk->attack(target);
			}
		}
	}


	/************************************************************************/
	/* Siege Mode                                                           */
	/************************************************************************/

	if (tk->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode)
	{
		// tank should not use siege mode near narrow choke point
		BWTA::Chokepoint* nearestCp = BWTA::getNearestChokepoint(tk->getPosition());
		if (nearestCp && nearestCp->getWidth() < 120 && tk->getPosition().getApproxDistance(nearestCp->getCenter()) < 32*4)
		{
			tk->unsiege();
			return;
		}

		if (enemyInRange.empty() && tk->getPosition().getApproxDistance(p) > reachRange)
		{
			if (Broodwar->self()->getRace() != Races::Terran)
			{	
				tk->unsiege();
				return;
			}

			// there may be invisible tanks. be careful!
			set<EnemyUnit*> invisibleEnemy;
			for each (EnemyUnit* e in this->eInfo->allEnemyUnits)
			{
				if (Broodwar->getFrameCount() - e->getLastUpdatedFrame() > 24*60*3)
				{
					continue;
				}

				if (e->getPosition() == Positions::Unknown)
				{
					continue;
				}

				if (e->getType() != UnitTypes::Terran_Siege_Tank_Siege_Mode && e->getType() != UnitTypes::Terran_Siege_Tank_Tank_Mode)
				{
					continue;
				}

				if (e->getPosition().getApproxDistance(tk->getPosition()) < tankAttackRange + 32*2)
				{
					Vector2 v1 = Vector2(e->getPosition()) - Vector2(tk->getPosition());
					Vector2 v2 = Vector2(p) - Vector2(tk->getPosition());
					if (v1.angle(v2) < 90) // this tank will get closer to enemy if move towards p
					{
						invisibleEnemy.insert(e);
					}
				}
			}
			if (!invisibleEnemy.empty())
			{
				Broodwar->drawBoxMap(tk->getPosition().x()-15,tk->getPosition().y()-15,tk->getPosition().x()+15,tk->getPosition().y()+15,Colors::Orange);
				Broodwar->drawBoxMap(tk->getPosition().x()-20,tk->getPosition().y()-20,tk->getPosition().x()+20,tk->getPosition().y()+20,Colors::Orange);
			}
			else
			{
				tk->unsiege();
				return;
			}
		}

		if (!enemyInRange.empty())
		{
			// unsiege if enemy has lots of melee units
			double meleeUnitCount = 0;
			for each (Unit* e in enemyInRange)
			{
				if (e->getType().canAttack() && e->getType().groundWeapon() != WeaponTypes::None && e->getType().groundWeapon().maxRange() < 64)
				{
					if (e->getType() == UnitTypes::Zerg_Zergling)
					{
						meleeUnitCount += 0.5;
					}
					else
						meleeUnitCount += 1;
				}
			}
			if (p != this->siegePoint && meleeUnitCount > 0.7 * enemyInRange.size())
			{
				tk->unsiege();
				return;
			}

			// select target to attack
			Unit* target = NULL;
			double attackValue = 0;
			double maxValue = 0;

			if (!enemyInRange(canAttack).empty())
			{
				enemyInRange -= enemyInRange.not(canAttack);
			}

			for each (Unit* e in enemyInRange)
			{
				if (e->getType() == UnitTypes::Protoss_High_Templar ||
					  e->getType() == UnitTypes::Zerg_Defiler	||
						e->getType() == UnitTypes::Zerg_Lurker ||
						e->getType() == UnitTypes::Protoss_Reaver || 
						e->getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode || 
						e->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode)
				{
					target = e;
					break;
				}

				double hp = e->getHitPoints() + e->getShields();
				double price = e->getType().mineralPrice() + e->getType().gasPrice();
				if (e->getType().canAttack() && e->getType().groundWeapon() != WeaponTypes::None)
				{
					double dpf = e->getType().groundWeapon().damageAmount() * 1.0 / e->getType().groundWeapon().damageCooldown();
					attackValue = dpf * price / hp;
				}
				else
				{
					attackValue = price / hp;
				}

				if (attackValue > maxValue)
				{
					maxValue = attackValue;
					target = e;
				}
			}

			// attack target
			if (target)
			{
				if (tk->getLastCommand().getType() == UnitCommandTypes::Attack_Unit && tk->getLastCommand().getTarget() == target)
				{
					return;
				}
				tk->attack(target);
			}
		}
	}
}

void MacroManager::onUnitDestroy(Unit* u)
{
	if (u == NULL)
	{
		return;
	}

	if (Broodwar->self()->isEnemy(u->getPlayer()) && u->getType().isBuilding())
	{
		if(this->scm->enemyStartLocation &&  u->getTilePosition().getDistance(this->scm->enemyStartLocation->getTilePosition())<=18)
			this->desEnemyMainBase = true;
	}
	//for DefendDropArmy Turret detect
	if (u->getType()==UnitTypes::Terran_Missile_Turret)
	{
		TilePosition see = u->getTilePosition();
		for each(TilePosition tp in receivedBTPositions){
			if((int)u->getTilePosition().getDistance(tp)<10)
				this->btCount--;
		}
	}

	if (searchingUnits.find(u)!=searchingUnits.end())
		searchingUnits.erase(u);
	if (this->attackers.find(u)!=this->attackers.end())
		this->attackers.erase(u);

	////remove this unit from vulture group
	vultureGroup.erase(u);
	
	//remove this unit from tank def team
	TankDefBase.erase(u);
	//remove from base protecting team
	for each(BaseClass* bc in this->baseManager->getBaseSet())
	{
		bc->protectors.erase(u);
	}
 
	this->dropManager->onUnitDestroy(u);
}

void MacroManager::onFrame()
{	
	ScanInvisibleEnemy();
	if (Broodwar->enemy()->getRace() == Races::Terran)
	{
		TvTTankProtectBase();
	}
	this->dropManager->update();
	
	BattleCruiserController();
	ScienceVesselController();

	if (!this->mental->goAttack)
		this->unitGroupCenterLimitation = Broodwar->self()->supplyUsed()/10;
	else
		this->unitGroupCenterLimitation = 40;
	//update vulture mining group
	for(std::set<Unit*>::iterator i = vultureGroup.begin();i!= vultureGroup.end();)
	{
		if ((*i)->isCompleted() && ((*i)->getSpiderMineCount()==0 || !SelectAllEnemy(canAttack).not(isFlyer).inRadius((*i)->getType().sightRange(),(*i)->getPosition()).empty()))
		{
			i = vultureGroup.erase(i);
		}
		else
		{
			i++;
			continue;
		}
	}

	// lift up Barrack near choke point
	if (Broodwar->getFrameCount()%(24*15) == 0 && Broodwar->enemy()->getRace() == Races::Protoss && this->terrainManager->bbPos != TilePositions::None)
	{
		if (this->mental->goAttack ||
			 (Broodwar->getFrameCount() >= 24*60*7.5 && this->mental->enemyInSight(canAttack).size() < 2))
		{
			for each (Unit* bb in SelectAll(UnitTypes::Terran_Barracks))
			{
				if (!bb->isTraining() && !bb->isLifted() && bb->getTilePosition().getDistance(this->terrainManager->bbPos) <= 4)
				{
					bb->lift();
				}
			}
		}
	}

	// clear dangerous mines
	if (Broodwar->getFrameCount()%24==0 && this->groupCenter && !this->mental->enemyInSight.empty())
	{
		//if enemy is still far
		if (this->mental->enemyInSight(canAttack).not(isWorker).getCenter().getApproxDistance(*this->groupCenter)>=32*8)
		{
			UnitGroup dangerMine =SelectAll()(Vulture_Spider_Mine).inRadius(32*3.2,*this->groupCenter);
			for each(Unit* mine in dangerMine){
				if (!this->attackers(canAttack).empty())
				{
					for each(Unit* u in this->attackers.not(isSieged))
					{
						if (u->isAttacking()||u->isAttackFrame()||u->getGroundWeaponCooldown()!=0)
							continue;
						else{
							u->attack(mine);
							break;
						}
					}
				}
			}
		}
	}

	//add fighters into set
	for each(Unit* u  in Broodwar->self()->getUnits())
	{
		if (u == theMarine)
		{
			this->attackers.erase(u);
			continue;
		}
		if (u->isLoaded())
		{
			this->attackers.erase(u);
			continue;
		}
		if (!u->isCompleted() ||
			  u->getType().isBuilding() ||
			  u->getType().isWorker() ||
				!u->getType().canAttack() ||
			  u->getType() == UnitTypes::Terran_Vulture_Spider_Mine ||
				u->getType() == UnitTypes::Terran_Battlecruiser)
		{
			continue;
		}
		//_T_
		if (this->dropManager->allUnitsToLoad.find(u) != this->dropManager->allUnitsToLoad.end())
		{
			this->attackers.erase(u);
			this->vultureGroup.erase(u);
			continue;
		}//_T_
		if (this->scm->scoutGroup.find(u) != this->scm->scoutGroup.end())
		{
			this->attackers.erase(u);
			this->vultureGroup.erase(u);
			continue;
		}
		if (this->TankDefBase.find(u) != this->TankDefBase.end())
			continue;
		//_T_
		// add unit to vulture group
		if (Broodwar->self()->hasResearched(TechTypes::Spider_Mines) &&
			this->vultureGroup.size() < 3 &&
			u->getType() == UnitTypes::Terran_Vulture &&
			u->getSpiderMineCount() == 3 &&
			this->scm->scoutGroup.find(u) == this->scm->scoutGroup.end() &&
			this->vultureGroup.find(u) == this->vultureGroup.end())
		{
			this->vultureGroup.insert(u);
		}

		this->attackers.insert(u);
	}
	//for zergling rush timing attack
	if (this->mental->STflag == MentalClass::ZrushZergling && SelectAll()(Marine)(isCompleted)(isLoaded).size() >= 3)
	{
		this->attackers+= SelectAll()(Marine)(isCompleted).not(isLoaded);
	}

	Broodwar->drawTextScreen(433,300,"VultureGroup: %d",this->vultureGroup.size());
	for each (Unit* u in this->vultureGroup)
	{
		Broodwar->drawCircleMap(u->getPosition().x(),u->getPosition().y(),15,Colors::Blue,true);
	}

	if (!this->vultureGroup.empty() && Broodwar->self()->hasResearched(TechTypes::Spider_Mines))
	{
		this->attackers -= this->vultureGroup;
		//if we go attack
		if (this->mental->goAttack)
		{	
			if(Broodwar->getFrameCount()%(24*5)==0 && SelectAllEnemy()(canAttack).not(isWorker,isBuilding).inRadius(32*12,attackers.getCenter()).size()<=3)
				vultureGroup.initMining((TilePosition)this->attackers(Siege_Tank).getCenter(),(TilePosition)this->terrainManager->eSecondChokepoint->getCenter());
			minetest();
		}		
		else
		{
			if(Broodwar->getFrameCount()%(24*5)==0)
				MiningRouteInitialization();
			if (this->mental->enemyInSight(canAttack).not(isWorker,isBuilding).empty())
				minetest();
		}		
	}

	//locate group center point
	int x = 0;
	int y = 0;
	int count = 0;

	for each(Unit* u in this->attackers){
		if (TankDefBase.find(u)!=TankDefBase.end()){
			TankDefBase.erase(u);
			break;
		}
		else{
			if (u->getTilePosition().getDistance(Broodwar->self()->getStartLocation())<this->unitGroupCenterLimitation)
				continue;
			else{
				x+=u->getPosition().x();
				y+=u->getPosition().y();
				count++;
			}
		}		
	}

	if (count==0)
		this->groupCenter=NULL;
	if (count>0)
		this->groupCenter = new Position(x/count,y/count);

	
	updateSetPoint();
	updateSiegePoint();

	//TvT
	LiftedBuildingController();

	if (this->mental->goAttack)
	{
		if (this->mental->enemyInSight.not(isWorker).size() < 10 || Broodwar->self()->supplyUsed()/2 > 180)
		{
			ArmyAttack();
		}
		else
		{
			ArmyDefend();
		}
	}
	else
	{
		if (this->mental->enemyInSight.empty())
		{
			ArmyGuard();
		}
		else
		{
		  ArmyDefend();
		}
	}


	//-------------------------------- BATTLE CONTROLLER

	clock_t t = clock();
	BattleController();
	Broodwar->drawTextScreen(433,290,"BattleController: %.0f",(float)(1000*(clock()-t))/CLOCKS_PER_SEC);

	// avoid psionic storm
	for each (Unit* u in SelectAll())
	{
		if (!u->isCompleted() || u->isLoaded() || u->getType().isBuilding() || !u->getType().canMove())
			continue;

		Position pos = Positions::None;
		Vector2 velocity = Vector2(0,0);

		for each (Unit* e in SelectAllEnemy())
		{
			if (e->getType() == UnitTypes::Protoss_High_Templar && e->getOrder() == Orders::CastPsionicStorm)
			{
				Position tarPos = e->getOrderTargetPosition();
				if (tarPos != Positions::None && u->getPosition().getApproxDistance(tarPos) <= 32*4)
				{
					velocity += PFFunctions::getVelocitySource(tarPos,u->getPosition()) * 1000;
				}
			}
		}

		for each (Bullet* b in Broodwar->getBullets())
		{
			if (b->getType() == BulletTypes::Psionic_Storm)
			{
				if (u->getPosition().getApproxDistance(b->getPosition()) <= 32*5)
				{
					velocity += PFFunctions::getVelocitySource(b->getPosition(),u->getPosition()) * 1000;
				}
			}
		}

		if (velocity != Vector2(0,0))
		{
			velocity = velocity * (256.0 / velocity.approxLen());
			pos = velocity + u->getPosition();
			pos = pos.makeValid();
			u->move(pos);
		}
	}
}

void MacroManager::BattleController()
{
	// add units to myBattleUnits group
	myBattleUnits.clear();
	for each (Unit* u in attackers)
	{
		if (u->isCompleted() && !u->isLoaded() && !u->getType().isBuilding() &&
			  (u->isAttacking() || u->getLastCommand().getType() == UnitCommandTypes::Attack_Unit || u->getOrder() == Orders::AttackUnit || u->isUnderAttack()))
		{
			myBattleUnits.insert(u);
		}
	}

	if (myBattleUnits.empty())
	{
		return;
	}

	Position mCen = myBattleUnits.getCenter();
	UnitGroup rest = attackers - myBattleUnits;
	rest += this->vultureGroup;
	for each (Unit* u in rest)
	{
		if (u->isCompleted() && !u->isLoaded() && !u->getType().isBuilding() && u->getPosition().getApproxDistance(mCen) <= 32*10)
		{
			myBattleUnits.insert(u);
			if (vultureGroup.find(u) != vultureGroup.end())
			{
				vultureGroup.erase(u);
			}
		}
	}

	if (myBattleUnits.empty())
	{
		return;
	}

	// add units to enemyBattleUnits group
	enemyBattleUnits.clear();
	for each (Unit* e in SelectAllEnemy())
	{
		UnitType et = e->getType();
		if (!e->isCompleted() || (et.isBuilding() && !et.canAttack()) || et.isWorker() || et.isFlyer() || et == UnitTypes::Terran_Vulture_Spider_Mine || et == UnitTypes::Zerg_Overlord
			|| et == UnitTypes::Zerg_Larva || et == UnitTypes::Protoss_Interceptor || e->isLoaded() || !e->isVisible() || !e->isDetected())
		{
			continue;
		}
		else if (e->getPosition().getApproxDistance(mCen) <= 32*20)
		{
			enemyBattleUnits.insert(e);
		}
	}

	// control our units
	if (myBattleUnits.size() < 12)
	{
		NormalBattleController();
	}
	else if (this->atkTar.first == "MainBase" || this->atkTar.first == "Only_MainBase" || this->atkTar.first == "Expansion" || this->atkTar.first == "Enemy_Army")
	{
		BWTA::Region* targetReg = BWTA::getRegion(this->atkTar.second);
		if (targetReg)
		{
			BWTA::Region* mReg = BWTA::getStartLocation(Broodwar->self())->getRegion();
			if (this->atkTar.first == "Enemy_Army" &&
				  (Broodwar->getFrameCount() <= 24*60*7 || ICEStarCraft::Helper::isDirectlyConnected(targetReg,mReg) || targetReg == mReg))
			{
				NormalBattleController();
			}
			else
			{
				BWTA::Chokepoint* theChokePoint = NULL;
				double minDis = 999999999;
				for each (BWTA::Chokepoint* cp in targetReg->getChokepoints())
				{
					double d = cp->getCenter().getApproxDistance(myBattleUnits.getCenter());
					if (d < minDis)
					{
						minDis = d;
						theChokePoint = cp;
					}
				}
				
				if (theChokePoint && theChokePoint->getWidth() < 120)
				{
					//int x = theChokePoint->getCenter().x();
					//int y = theChokePoint->getCenter().y();
					//Broodwar->drawTriangleMap(x,y-20,x-17,y+10,x+17,y+10,Colors::Brown);
					//Broodwar->drawTriangleMap(x-17,y-10,x+17,y-10,x,y+20,Colors::Brown);
					ChokepointBattleController(this->atkTar.second,theChokePoint);
				}
				else
				{
					NormalBattleController();
				}
			}
		}
	}
	else if (this->atkTar.first != "No_AtkTar")
	{
		NormalBattleController();
	}
}

void MacroManager::ChokepointBattleController(Position atkTar, BWTA::Chokepoint* theChokePoint)
{
	ChokepointBattleController(myBattleUnits,atkTar,theChokePoint);
}

void MacroManager::ChokepointBattleController(UnitGroup& units, Position atkTar, BWTA::Chokepoint* theChokePoint)
{
	Broodwar->drawTextScreen(180,335,"ChokepointBattleController");
	UnitGroup normalUnits;
	for each (Unit* u in units)
	{
		if (u->getType().isFlyer())
		{
			normalUnits.insert(u);
			continue;
		}

		Position des = MicroUnitControl::getDestinationNearChokepoint(u,units,atkTar,theChokePoint);

		if (des != Positions::None)
		{
			if (!(u->isMoving() && !u->isIdle() && u->getLastCommand().getType() == UnitCommandTypes::Move && u->getLastCommand().getTargetPosition().getApproxDistance(des) < 32))
			{
				u->move(des);
				//Broodwar->drawLineMap(u->getPosition().x(),u->getPosition().y(),des.x(),des.y(),Colors::Green);
				//Broodwar->drawCircleMap(des.x(),des.y(),2,Colors::Green);
			}
		}
		else
		{
			normalUnits.insert(u);
		}
	}

	NormalBattleController(normalUnits);
}

void MacroManager::NormalBattleController()
{
	NormalBattleController(myBattleUnits);
}

void MacroManager::NormalBattleController(UnitGroup& units)
{
	Broodwar->drawTextScreen(180,335," NormalBattleController");
	for each (Unit* u in units)
	{
		if (u->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode || u->getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode)
		{
			continue;
		}
		
		if (Broodwar->enemy()->getRace() == Races::Terran)
		{
			// there may be invisible tanks. be careful!
			Position targetPos = u->getOrderTarget() ? u->getOrderTarget()->getPosition() : u->getOrderTargetPosition();
			if (targetPos == Positions::None || targetPos == Positions::Invalid || targetPos == Positions::Unknown)
			{
				continue;
			}

			int tankAttackRange = UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().maxRange();
			set<EnemyUnit*> invisibleEnemy;

			for each (EnemyUnit* e in this->eInfo->allEnemyUnits)
			{
				if (Broodwar->getFrameCount() - e->getLastUpdatedFrame() > 24*60*3)
				{
					continue;
				}

				if (e->getPosition() == Positions::Unknown)
				{
					continue;
				}

				if (e->getType() != UnitTypes::Terran_Siege_Tank_Siege_Mode && e->getType() != UnitTypes::Terran_Siege_Tank_Tank_Mode)
				{
					continue;
				}

				if (e->getPosition().getApproxDistance(u->getPosition()) < tankAttackRange + 32*2)
				{
					Vector2 v1 = Vector2(e->getPosition()) - Vector2(u->getPosition());
					Vector2 v2 = Vector2(targetPos) - Vector2(u->getPosition());
					if (v1.angle(v2) < 90) // this unit will get closer to enemy tanks if move towards p
					{
						invisibleEnemy.insert(e);
					}
				}
			}

			if (!invisibleEnemy.empty())
			{
				//Broodwar->printf("%s stops",u->getType().getName().c_str());
				Broodwar->drawBoxMap(u->getPosition().x()-15,u->getPosition().y()-15,u->getPosition().x()+15,u->getPosition().y()+15,Colors::Orange);
				Broodwar->drawBoxMap(u->getPosition().x()-20,u->getPosition().y()-20,u->getPosition().x()+20,u->getPosition().y()+20,Colors::Orange);
				if (!units(Siege_Tank).empty())
				{
					Position targetPos = units(Siege_Tank).getCenter();
					if (u->isAttackFrame() ||
						  u->getLastCommand().getType() == UnitCommandTypes::Attack_Move && u->getLastCommand().getTargetPosition().getApproxDistance(targetPos) < 32*2)
					{
						// nothing
					}
					else
					{
						u->attack(targetPos);
					}			
				}
				else if (!u->isHoldingPosition())
				{
					u->holdPosition();
				}
				continue;
			}
		}

		Unit* target = NULL;
		Position targetPos;
		UnitGroup enemyInSight = enemyBattleUnits.inRadius(u->getType().sightRange(),u->getPosition());

		if (!u->getType().isFlyer() && !u->isAttackFrame() && u->getGroundWeaponCooldown() > 0 && u->getAirWeaponCooldown() > 0)
		{
			// move away from enemy
			UnitGroup closeEnemy = (enemyBattleUnits + SelectAllEnemy()(isWorker)).inRadius(32*4,u->getPosition());
			if (!closeEnemy.empty())
			{
				Vector2 v = Vector2(0,0);
				int minD = 99999999;
				Unit* closestEnemy = NULL;
				for each (Unit* e in closeEnemy)
				{
					if (e->getType().groundWeapon() != WeaponTypes::None &&
						  e->getType().groundWeapon().maxRange() < Broodwar->self()->groundWeaponMaxRange(u->getType()))
					{
						v += PFFunctions::getVelocitySource(e->getPosition(),u->getPosition()) * 1000;
					}
				}

				if (v != Vector2(0,0))
				{
					v = v * (128.0 / v.approxLen());
					Position des = v + u->getPosition();
					des = des.makeValid();

					if(!(u->getLastCommand().getType() == UnitCommandTypes::Move && u->getLastCommand().getTargetPosition().getApproxDistance(des) < 32))
					{
						//Broodwar->drawLineMap(u->getPosition().x(),u->getPosition().y(),des.x(),des.y(),Colors::Yellow);
						u->move(des);
					}
					continue;
				}
			}
		}

		if (u->getGroundWeaponCooldown() <= 1 || u->getAirWeaponCooldown() <= 1)
		{
			target = getBestAttackTartget(u);
			if (target)
			{
				//Broodwar->drawLineMap(u->getPosition().x(),u->getPosition().y(),target->getPosition().x(),target->getPosition().y(),Colors::Red);
				if(!u->isAttackFrame() && !(u->getLastCommand().getType() == UnitCommandTypes::Attack_Unit && u->getLastCommand().getTarget() == target))
				{
					u->attack(target);
				}
				continue;
			}
		}
		
		if (!enemyInSight.empty())
		{
			targetPos = enemyInSight.getCenter();
		}
		else if (this->atkTar.second != Positions::None)
		{
			targetPos = this->atkTar.second;
		}
		else
		{
			targetPos = myBattleUnits.getCenter();
		}

		int	attackRange = u->getType().groundWeapon().maxRange() > u->getType().airWeapon().maxRange() ? u->getType().groundWeapon().maxRange() : u->getType().airWeapon().maxRange();
		if (u->getPosition().getApproxDistance(targetPos) > attackRange &&
			  !(u->getLastCommand().getType() == UnitCommandTypes::Attack_Move && u->getLastCommand().getTargetPosition().getApproxDistance(targetPos) < 32))
		{
			u->attack(targetPos);
		}
	}
}

void MacroManager::onUnitDiscover(Unit* unit)
{
	if (unit->getPlayer()==Broodwar->self() && unit->getType()== UnitTypes::Terran_Vulture && Broodwar->self()->hasResearched(TechTypes::Spider_Mines) && this->vultureGroup.size() < 3)
	{
		this->vultureGroup.insert(unit);
	}

	//for tvt tank defend base team
	if (unit->getPlayer()==Broodwar->self() && unit->getType()== UnitTypes::Terran_Siege_Tank_Tank_Mode && Broodwar->enemy()->getRace()==Races::Terran)
	{
		UnitGroup tankGroup = SelectAll()(isCompleted)(Siege_Tank);
		unsigned int teamSize = baseManager->getBaseSet().size();
		if (tankGroup.size()>=10 && TankDefBase.size() < Broodwar->self()->allUnitCount(UnitTypes::Terran_Command_Center))
		{
			TankDefBase[unit]=Positions::None;
		}
	}
}

void MacroManager::unitAvoidMine(UnitGroup ug,Position p)
{	
	for each(Unit* u in ug){
		bool danger =false;
		std::set<Unit*> temp = u->getUnitsInRadius(32*3);
		for each(Unit* su in temp){			
			if (su->getType()==UnitTypes::Terran_Vulture_Spider_Mine && SelectAllEnemy()(isCompleted).not(isBuilding,isFlyer).inRadius(32*3,u->getPosition()).size()>0)
				u->move(this->terrainManager->mFirstChokepoint->getCenter());	
		}
	}		
}

void MacroManager::allBuildingSetPoint(Position setposition)
{
	if (setposition == Positions::None)
	{
		return;
	}

	UnitGroup allArmyFactory = SelectAll()(Barracks, Factory,Starport);
	allArmyFactory.setRallyPoint(setposition.makeValid());
}

void MacroManager::ScanInvisibleEnemy()
{
	std::set<Unit*> allUnits = Broodwar->getAllUnits();
	std::set<Unit*> myScanner = SelectAll()(Comsat_Station)(Energy,">=",50);
	if (myScanner.empty())
	{
		for each (Unit* eu in allUnits)
		{
			if (eu->getPlayer() != Broodwar->enemy())
				continue;
			
			if (!eu->isDetected() && eu->getType().canAttack())
			{
				//Broodwar->printf("invisible enemy! Build scanner as soon as possible!");
				if (this->bom->getPlannedCount(UnitTypes::Terran_Comsat_Station,120) < 2)
				{
					this->bom->build(2,UnitTypes::Terran_Comsat_Station,120);
				}
				return;
			}
		}
	}
	else if (Broodwar->getFrameCount()%(24*5) == 0)
	{
		std::set<Unit*> myArmy = SelectAll()(isCompleted)(canAttack).not(isWorker,isBuilding) + SelectAll()(isCompleted)(Bunker)(LoadedUnitsCount,">=",1);
		std::set<Unit*>::iterator scanner = myScanner.begin();
		for each (Unit* eu in allUnits)
		{
			if (eu->getPlayer() != Broodwar->enemy())
				continue;
			
			if (!eu->isDetected() && eu->getType().canAttack())
			{	
				// avoid wasting scanner energy to scan near position in short time
				if (eu->getPosition().getApproxDistance(this->lastScanPosition) <= 32*6 && Broodwar->getFrameCount() - this->lastScanTime <= 24*10)
					return;
				
				for each (Unit* army in myArmy)
				{
					if (army->getPosition().getApproxDistance(eu->getPosition()) <= army->getType().groundWeapon().maxRange()*1.1)
					{
						(*scanner)->useTech(TechTypes::Scanner_Sweep,eu->getPosition());
						this->lastScanTime = Broodwar->getFrameCount();
						this->lastScanPosition = eu->getPosition();
						break;
					}
				}
			}
		}
	}
}

void MacroManager::findRestEnemy()
{
	if (Broodwar->getFrameCount()%24==0)
	{
		/**********for scv to find rest enemy*******/
		//this->scm->setScoutNum(2);
		//this->scm->SCVScout(ScoutManager::EnemyExpansion);
		/**********for scanner to find*******/
		//check if all the positions have already been scanned
		int scanCounter = 0;
		for (std::map<Position,std::pair<bool,int>>::iterator i = this->positionTOattack.begin(); i != this->positionTOattack.end(); i++){
			if (i->second.first)
				scanCounter++;
		}
		//if so ,then reset	
		if(scanCounter ==(int)this->positionTOattack.size() && Broodwar->getFrameCount()%24*10==0)
		{
			for (std::map<Position,std::pair<bool,int>>::iterator i = this->positionTOattack.begin(); i != this->positionTOattack.end(); i++){
				i->second.first = false;
			}
		}
		//Processing scan task
		for (std::map<Position,std::pair<bool,int>>::iterator i = this->positionTOattack.begin(); i != this->positionTOattack.end(); i++)
		{
			std::set<Unit*> myScanner;
			myScanner = SelectAll()(Comsat_Station)(Energy,">=",50);
			//if the position is currently visible, then don't need to scan
			if (Broodwar->isVisible((TilePosition)i->first))
			{
				//make sure we never scan this position
				if(i->second.first)
				{
					i->second.second = Broodwar->getFrameCount();
					continue;
				}
				else
				{
					i->second.first = true;
					i->second.second = Broodwar->getFrameCount();
					continue;
				}
			}
			//if this position has just been scanned
			else if (i->second.first)
				continue;
			//position need to scan
			else
			{
				for each(Unit* sc in myScanner)
				{
					if ((int)myScanner.size()<=2 && sc->getEnergy()<100)
						return;
					else if (sc->getEnergy()<50)
						continue;
					else
					{
						int energy=sc->getEnergy();
						//scan this base location
						if(sc->useTech(TechTypes::Scanner_Sweep,i->first))
						{
							i->second.first = true;
							i->second.second = Broodwar->getFrameCount();
							return;
						}
					}
				}
			}
		}
	}
}


void MacroManager::ScienceVesselController()
{
	if (Broodwar->getFrameCount()%8 != 7)
	{
		return;
	}

	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Science_Vessel) < 1)
	{
		return;
	}

	for each (Unit* u in Broodwar->self()->getUnits())
	{
		if (u->getType() != UnitTypes::Terran_Science_Vessel || !u->isCompleted())
		{
			continue;
		}

		Vector2 v = Vector2(0,0);
		for each (Unit* e in Broodwar->getUnitsInRadius(u->getPosition(),u->getType().sightRange()))
		{
			if (e->getPlayer() == Broodwar->enemy() && e->getOrder() == Orders::AttackUnit && e->getOrderTarget() == u)
			{
				v += PFFunctions::getVelocitySource(e->getPosition(),u->getPosition()) * 1000;
			}
		}

		if (v != Vector2(0,0))
		{
			v = v * (96.0 / v.approxLen());
			u->move((v + u->getPosition()).makeValid());
			Broodwar->printf("Vessel is under attack");
			continue;
		}

		UnitGroup units = this->myBattleUnits + SelectAll()(Battlecruiser)(isCompleted);
		if (units.empty())
		{
			units = this->attackers.inRadius(32*20,u->getPosition());
			if (units.empty())
			{
				units = this->attackers;
			}
		}

		UnitGroup unDetectedEnemy = SelectAllEnemy()(isCompleted)(Lurker,Dark_Templar,Ghost,Wraith,Arbiter).inRadius(32*20,units.getCenter());
		unDetectedEnemy += SelectAllEnemy()(isCompleted).not(isDetected).inRadius(32*20,units.getCenter());

		Position targetPos = Positions::None;
		if (unDetectedEnemy.getNearest(u->getPosition()))
		{
			targetPos = unDetectedEnemy.getNearest(u->getPosition())->getPosition();
		}
		else if (!units.empty())
		{
			targetPos = units.getCenter();
		}
		else
		{
			targetPos = this->setPoint;
		}

		// use techs
		if (Broodwar->enemy()->getRace() == Races::Zerg &&
			  Broodwar->self()->hasResearched(TechTypes::Irradiate) &&
			  u->getEnergy() >= TechTypes::Irradiate.energyUsed())
		{
			MicroUnitControl::useIrradiate(u,SelectAllEnemy()(isCompleted).inRadius(u->getType().sightRange()*1.5,u->getPosition()));
			//Broodwar->printf("order use Irradiate");
		}
		
		if (Broodwar->enemy()->getRace() == Races::Protoss &&
			  Broodwar->self()->hasResearched(TechTypes::EMP_Shockwave) &&
			  u->getEnergy() >= TechTypes::EMP_Shockwave.energyUsed())
		{
			MicroUnitControl::useEMPShockwave(u,SelectAllEnemy()(isCompleted).inRadius(u->getType().sightRange()*1.5,u->getPosition()));
			//Broodwar->printf("order use EMPShockwave");
		}
		
		if (u->getEnergy() >= TechTypes::Defensive_Matrix.energyUsed())
		{
			MicroUnitControl::useDefensiveMatrix(u,this->attackers.inRadius(u->getType().sightRange()*1.5,u->getPosition()));
			//Broodwar->printf("order use DefensiveMatrix");
		}

		if ((u->getLastCommand().getType() != UnitCommandTypes::Use_Tech_Unit && u->getLastCommand().getType() != UnitCommandTypes::Use_Tech_Position)
			  ||
			  Broodwar->getFrameCount() - u->getLastCommandFrame() > 24*3)
		{
			u->move(targetPos);
		}
	}
}

void MacroManager::showScanPositionTable()
{
	if (this->scanTableFlag)
	{	
		Broodwar->drawTextScreen(400,20,"\x0E| Position - Scanned - LastScannedTime |");
		int uline = 1;
		for(std::map<Position,std::pair<bool,int>>::const_iterator i = this->positionTOattack.begin();i != this->positionTOattack.end();i++){
			Broodwar->drawTextScreen(400,20+16*uline,"\x0F|   ( %d ,%d )  -  %d  -  %d   |",i->first.x(),i->first.y(),i->second.first,i->second.second);
			uline++;
		}
	}
}

bool MacroManager::setScanTableFlag(bool b)
{
	this->scanTableFlag = b;
	return this->scanTableFlag;
}
void MacroManager::minetest()
{
	Broodwar->drawTextScreen(433,310,"minetest");
	MiningRouteInitialization();
	if (startTP != TilePositions::None && endTP != TilePositions::None && Broodwar->self()->hasResearched(TechTypes::Spider_Mines))
	{
		if (this->lastMineStart != startTP || this->lastMineEnd != endTP)
		{
			//Broodwar->printf("change mine route");
			vultureGroup.initMining(startTP,endTP);
			this->lastMineStart = startTP;
			this->lastMineEnd = endTP;
		}
		//Broodwar->drawLineMap(startTP.x()*32,startTP.y()*32,endTP.x()*32,endTP.y()*32,Colors::Cyan);
		vultureGroup.doMining(1.5);
	}
}
void MacroManager::MiningRouteInitialization()
{
	startTP = TilePosition(this->terrainManager->mSecondChokepoint->getCenter());

	TilePosition mapCenter = TilePosition(Broodwar->mapWidth()/2,Broodwar->mapHeight()/2).makeValid();
	if (this->scm->enemyStartLocation)
	{
		if (this->terrainManager->eSecondChokepoint && this->terrainManager->eThirdChokepoint)
		{
			TilePosition secondCp = TilePosition(this->terrainManager->eSecondChokepoint->getCenter());
			TilePosition thirdCp  = TilePosition(this->terrainManager->eThirdChokepoint->getCenter());

			if (secondCp.getDistance(thirdCp) <= 8)
			{
				endTP = thirdCp;
			}
			else
			{
				endTP = TilePosition(secondCp.x() * 2/3 + thirdCp.x() * 1/3,secondCp.y() * 2/3 + thirdCp.y() * 1/3).makeValid();
			}
		}
		else
		{
			endTP = mapCenter;
		}
	}
	else
	{
		endTP = mapCenter;
	}

	//_T_
	startTP = this->terrainManager->getConnectedTilePositionNear(startTP);
	endTP   = this->terrainManager->getConnectedTilePositionNear(endTP);
}

void MacroManager::TvTTankProtectBase()
{
	if (Broodwar->getFrameCount()%8 != 5)
	{
		return;
	}

	UnitGroup tankGroup = SelectAll()(isCompleted)(Siege_Tank);
	//add to tank defend group

	for(std::map<Unit*,Position>::iterator i = TankDefBase.begin();i != TankDefBase.end();i++)
	{
		if(this->attackers.find(i->first)!=this->attackers.end())
			this->attackers.erase(i->first);
	}


	//assign each tank a position to defend
	for each(BaseClass* bc in baseManager->getBaseSet())
	{
		//if this base already protected
		if(!bc->protectors.empty())
			continue;
		else
		{
			for(std::map<Unit*,Position>::iterator i = TankDefBase.begin();i!=TankDefBase.end();i++)
			{
				if (i->second!=Positions::None)
					continue;
				else
				{
					i->second = bc->getBaseLocation()->getPosition();
					bc->protectors.insert(i->first);
					break;
				}
			}
		}
	}

	if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode))
	{
		return;
	}

	for(map<Unit*,Position>::iterator i = TankDefBase.begin(); i != TankDefBase.end(); i++)
	{
		tankAttackMode(i->first,i->second,3);
	}
}

void MacroManager::keepFormationAttack(Unit* u)
{
	Position gatherCenter = Positions::None;
	Position siegedTankCenter = this->attackers(Siege_Tank)(isSieged).getCenter();
	Position allTankCenter = this->attackers(Siege_Tank).getCenter();

	if (siegedTankCenter != Positions::None)
		gatherCenter = siegedTankCenter;
	else if(allTankCenter != Positions::None)
		gatherCenter = allTankCenter;
	else if(groupCenter)
		gatherCenter = *this->groupCenter;
	else
		gatherCenter = this->attackers.getCenter();

	if (u->getPosition().getApproxDistance(gatherCenter)>=32*8)
		u->attack(gatherCenter);
	else
	{
		Unit* bestT = getBestAttackTartget(u);
		if (bestT && (u->getGroundWeaponCooldown()==0))
		{
			u->attack(bestT);
			//Broodwar->printf("bestTar: %s",bestT->getType().c_str());
		}
		else
		{
			UnitGroup eG = SelectAllEnemy()(canAttack)(isDetected).inRadius(32*13,attackers.getCenter());
			if (!u->isAttacking())
				u->attack(eG.getCenter());
		} 
	}
	//}	
}


/************************************************************************/
/* NEW                                                                  */
/************************************************************************/
void MacroManager::BattleCruiserController()
{
	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Battlecruiser) == 0)
	{
		Broodwar->drawTextScreen(433,15," BattleCruiser 0");
		return;
	}

	Position attackPosition;
	if (this->mental->goAttack)
	{
		if (this->atkTar.second == Positions::Invalid || this->atkTar.second == Positions::None || this->atkTar.second == Positions::Unknown)
		{
			attackPosition = this->setPoint;
		}
		else
		{
			if (this->atkTar.first == "Enemy_Army" &&
				  SelectAll(UnitTypes::Terran_Battlecruiser)(isCompleted)(HitPoints,">=",400).size() >= 6 &&
					this->scm->enemyStartLocation)
			{
				attackPosition = this->scm->enemyStartLocation->getPosition();
			}
			else
			{
				attackPosition = this->atkTar.second;
			}
		}
	}
	else if (SelectAll(UnitTypes::Terran_Battlecruiser)(isCompleted)(HitPoints,">=",400).size() >= 6 && this->scm->enemyStartLocation)
	{
		attackPosition = this->scm->enemyStartLocation->getPosition();
	}
	else if (!this->mental->enemyInSight.not(isWorker).empty())
	{
		attackPosition = this->mental->enemyInSight.not(isWorker).getNearest(this->setPoint)->getPosition();
	}
	else
	{
		attackPosition = this->setPoint;
	}

	Broodwar->drawTextScreen(433,15,"\x07 BattleCruiser %d | Attack (%d,%d)",Broodwar->self()->completedUnitCount(UnitTypes::Terran_Battlecruiser),attackPosition.x()/32,attackPosition.y()/32);

	for each (Unit* u in SelectAll(UnitTypes::Terran_Battlecruiser)(isCompleted))
	{
		if (workerManager->isInRepairList(u) || u->getHitPoints() <= 200 || u->isBeingHealed())
		{
			if (u->getPosition().getApproxDistance(this->terrainManager->mSecondChokepoint->getCenter()) > 32*3)
			{
				u->move(this->terrainManager->mSecondChokepoint->getCenter());
			}
			else
			{
				u->holdPosition();
			}
			continue;
		}

		if (u->getEnergy() >= TechTypes::Yamato_Gun.energyUsed())
		{
			UnitGroup targets = SelectAllEnemy()(isCompleted)(canAttack)(isDetected)(HitPoints,">=",100);
			targets =	targets.inRadius(u->getType().sightRange(),u->getPosition());
			MicroUnitControl::useYamatoGun(u,targets,attackPosition);
		}
		else
		{
			UnitGroup targets = SelectAllEnemy()(isCompleted)(isDetected)(canAttack,Bunker,Reaver,Carrier).not(Interceptor,Scarab,isWorker).inRadius(u->getType().sightRange(),u->getPosition());
			UnitGroup tmp = targets(maxAirHits,">",0) + targets(Carrier,Bunker);
			if (!tmp.empty())
			{
				targets = tmp;
			}
				
			Unit* target = targets.getNearest(u->getPosition());
			if (target)
			{
				if (u->isAttackFrame() || (u->getLastCommand().getType() == UnitCommandTypes::Attack_Unit && u->getLastCommand().getTarget() == target))
				{
					continue;
				}
				u->attack(target);
			}
			else
			{
				if (u->isAttackFrame())
				{
					continue;
				}
				if (!u->isIdle() &&
					  u->getLastCommand().getType() == UnitCommandTypes::Attack_Move &&
					  u->getLastCommand().getTargetPosition().getApproxDistance(attackPosition) < 32*2 &&
						Broodwar->getFrameCount() - u->getLastCommandFrame() < 24*10)
				{
					continue;
				}
				u->attack(attackPosition);
			}
		}
	}
}

Position MacroManager::getSetPoint() const
{
	return this->setPoint;
}

Position MacroManager::getSiegePoint() const
{
	return this->siegePoint;
}

void MacroManager::updateSetPoint()
{
	if (Broodwar->getFrameCount()%(24*5) != 0)
	{
		return;
	}

	/************************************************************************/
	/* VS Terran                                                            */
	/************************************************************************/

	if (Broodwar->enemy()->getRace() == Races::Terran)
	{
		Position mapCenter  = Position(Broodwar->mapWidth()*32/2, Broodwar->mapHeight()*32/2);
		Position mFirstChoke  = this->terrainManager->mFirstChokepoint->getCenter();
		Position mSecondChoke = this->terrainManager->mSecondChokepoint->getCenter();

		if (this->scm->enemyStartLocation)
		{
			Position eSecondChoke = this->terrainManager->eSecondChokepoint->getCenter();
			int deadArmy = Broodwar->self()->deadUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + 
										 Broodwar->self()->deadUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode) + 
										 Broodwar->self()->deadUnitCount(UnitTypes::Terran_Goliath) +
										 Broodwar->self()->deadUnitCount(UnitTypes::Terran_Vulture) +
				             Broodwar->self()->deadUnitCount(UnitTypes::Terran_Marine);

			int enemyTank = this->eInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode) + this->eInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Siege_Mode);

			if ((Broodwar->self()->supplyUsed()/2 >= 60 || this->eInfo->killedEnemyNum > deadArmy) &&
				  SelectAll()(isCompleted)(Siege_Tank).size() >= enemyTank + 3)
			{
				this->setPoint = Position(eSecondChoke.x()*3/4 + mapCenter.x()/4, eSecondChoke.y()*3/4 + mapCenter.y()/4);
			}
			else if (SelectAll()(isCompleted)(Siege_Tank).size() >= 3)
			{
				this->setPoint = Position(eSecondChoke.x()/2 + mapCenter.x()/2, eSecondChoke.y()/2 + mapCenter.y()/2);
			}
			else
			{
				if (this->gf->bunkerPosition && BWTA::getRegion(*gf->bunkerPosition) != BWTA::getRegion(Broodwar->self()->getStartLocation()))
				{
					this->setPoint = Position(*(this->gf->bunkerPosition));
				}
				else
				{
					if (Broodwar->getFrameCount() <= 24*60*5 && !this->mental->marineRushOver)
						this->setPoint = mapCenter;
					else
						this->setPoint = mSecondChoke;
				}
			}
		} 
		//if we do not know where the enemy is
		else
		{			
			if (Broodwar->getFrameCount() <= 24*60*5 && !this->mental->marineRushOver)
			{
				this->setPoint = mapCenter;
			}
			else
				this->setPoint = mSecondChoke;
		}
	}

	/************************************************************************/
	/* VS Other Races                                                       */
	/************************************************************************/

	else
	{
		if(Broodwar->self()->supplyUsed()/2 <= 100)
		{
			if (this->gf->bunkerPosition)
			{
				this->setPoint = Position(*(this->gf->bunkerPosition));
			}
			else
			{
				//if (Broodwar->self()->allUnitCount(UnitTypes::Terran_Command_Center) == 1)
				if (SelectAll()(isBuilding)(isCompleted).inRegion(this->terrainManager->mNearestBase->getRegion()).size() < 1)
				{
					// gather near first choke point inside main base
					Vector2 v = Vector2(this->terrainManager->mFirstChokepoint->getSides().first) - Vector2(this->terrainManager->mFirstChokepoint->getSides().second);
					v = Vector2(-v.y(),v.x());
					v = v * (32 * 6.0 / v.approxLen());
					Position p = v + this->terrainManager->mFirstChokepoint->getCenter();
					if (BWTA::getRegion(p) == this->terrainManager->mNearestBase->getRegion())
					{
						p = (-v) + this->terrainManager->mFirstChokepoint->getCenter();
					}

					this->setPoint = p.makeValid();
				}
				else
				{
					this->setPoint = this->terrainManager->mSecondChokepoint->getCenter();
				}
			}
			allBuildingSetPoint(this->setPoint);
		}
		// supplyUsed > 100
		else
		{
			Position mSecondChoke  = this->terrainManager->mSecondChokepoint->getCenter();
			Position mThirdChoke   = this->terrainManager->mThirdChokepoint->getCenter();
			if (this->mInfo->myFightingValue().first > this->eInfo->enemyFightingValue().first * 3)
			{
				if (mSecondChoke.getApproxDistance(mThirdChoke) < 15 * 32)
				{
					this->setPoint = mThirdChoke;
				}
				else
				{
					Vector2 v = Vector2(mThirdChoke) - Vector2(mSecondChoke);
					v = v * (32 * 15.0 / v.approxLen());
					this->setPoint = (v + mSecondChoke).makeValid();
				}
				allBuildingSetPoint(this->setPoint);
			}
			else if (this->mInfo->myFightingValue().first > this->eInfo->enemyFightingValue().first * 2)
			{
				this->setPoint = Position((mSecondChoke.x() + mThirdChoke.x())/2, (mSecondChoke.y() + mThirdChoke.y())/2);
				allBuildingSetPoint(this->setPoint);
			}
			else if (this->mInfo->myFightingValue().first > this->eInfo->enemyFightingValue().first * 1.2)
			{
				this->setPoint = mSecondChoke;
				allBuildingSetPoint(this->setPoint);
			}
			else
			{
				if (this->gf->bunkerPosition && BWTA::getRegion(*gf->bunkerPosition) != BWTA::getRegion(Broodwar->self()->getStartLocation()))
				{
					this->setPoint = (Position)(*(this->gf->bunkerPosition));
					allBuildingSetPoint(this->setPoint);
				}
				else
				{
					this->setPoint = mSecondChoke;
					allBuildingSetPoint(this->setPoint);
				}
			}		
		}
	}

	if (this->terrainManager->getGroundDistance(TilePosition(this->setPoint),Broodwar->self()->getStartLocation()) < 0)
	{
		this->setPoint = Position(this->terrainManager->getConnectedTilePositionNear(TilePosition(this->setPoint)));
	}
	Broodwar->drawCircleMap(this->setPoint.x(),this->setPoint.y(),12,Colors::Red,true);
	
	// set best set point for other units (not tanks)
	//this->setPoint2 = this->setPoint;
	/*if (this->setPoint != Positions::None)
	{
		if (!this->scm->enemyStartLocation)
		{
			this->setPoint2 = this->setPoint;
		}
		else
		{
			TilePosition mapCen = TilePosition(Broodwar->mapWidth()/2,Broodwar->mapHeight()/2).makeValid();
			bool mapCenReachable = BWTA::isConnected(Broodwar->self()->getStartLocation(),mapCen) || BWTA::getShortestPath(Broodwar->self()->getStartLocation(),mapCen).size() > 0;
			BWTA::Region* reg   = BWTA::getRegion(this->setPoint);
			BWTA::Region* start = BWTA::getRegion(Broodwar->self()->getStartLocation());
			if (!reg || !start)
			{
				this->setPoint2 = this->setPoint;
			}
			else if (reg != start)
			{
				double minD = 999999999;
				double d = 0;
				Position mBestSetPoint2 = Positions::None;
				for each (BWTA::Chokepoint* cp in reg->getChokepoints())
				{
					d = mapCenReachable ? BWTA::getGroundDistance(TilePosition(cp->getCenter()),mapCen) : TilePosition(cp->getCenter()).getDistance(mapCen);
					if (d < minD)
					{
						minD = d;
						mBestSetPoint2 = cp->getCenter();
					}
				}
				if (mBestSetPoint2 == Positions::None)
				{
					this->setPoint2 = this->setPoint;
				}
				else
				{
					if (mBestSetPoint2.getApproxDistance(this->setPoint) > 32*6)
					{
						Vector2 v = mBestSetPoint2 - this->setPoint;
						v	= v * (32.0*6/v.approxLen());
						mBestSetPoint2 = 	v + this->setPoint;
						mBestSetPoint2 = mBestSetPoint2.makeValid();
					}
					this->setPoint2 = mBestSetPoint2;
				}
			}
			else
			{
				this->setPoint2 = this->setPoint;
			}

			if (this->terrainManager->getGroundDistance(TilePosition(this->setPoint2),Broodwar->self()->getStartLocation()) < 0)
			{
				this->setPoint2 = Position(this->terrainManager->getConnectedTilePositionNear(TilePosition(this->setPoint2)));
			}

			Broodwar->drawCircleMap(this->setPoint.x(),this->setPoint.y(),12,Colors::Red,true);
			Broodwar->drawCircleMap(this->setPoint2.x(),this->setPoint2.y(),8,Colors::Orange,true);
		}
	}*/
}

void MacroManager::updateSiegePoint()
{
	if (Broodwar->getFrameCount()%(24*5) != 60)
	{
		return;
	}

	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->completedUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode) < 1)
	{
		return;
	}

	if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode) && !Broodwar->self()->isResearching(TechTypes::Tank_Siege_Mode))
	{
		this->siegePoint = Positions::None;
		return;
	}

	if (this->terrainManager->siegePoint == Positions::None)
	{
		this->siegePoint = Positions::None;
		return;
	}

	if (this->setPoint && 
		  (BWTA::getNearestChokepoint(this->setPoint) == this->terrainManager->mFirstChokepoint ||
		   BWTA::getNearestChokepoint(this->setPoint) == this->terrainManager->mSecondChokepoint ||
		   BWTA::getRegion(this->setPoint) == this->terrainManager->mNearestBase->getRegion()))
	{
		this->siegePoint = this->terrainManager->siegePoint;
		//Broodwar->printf("Siege point updated");
	}
	else
	{
		this->siegePoint = Positions::None;
	}
}

void MacroManager::LiftedBuildingController()
{
	if (Broodwar->enemy()->getRace() != Races::Terran)
	{
		return;
	}

	if (Broodwar->getFrameCount()%8 != 4)
	{
		return;
	}

	if (!this->mental->marineRushOver || (this->mInfo->countUnitNum(UnitTypes::Terran_Marine,1) < 4 && Broodwar->getFrameCount() < 24*60*5))
	{
		return;
	}

	UnitGroup liftedBuildings = SelectAll()(isCompleted)(Barracks,Engineering_Bay)(isLifted);

	for each (Unit* u in liftedBuildings)
	{
		Vector2 v = Vector2(0,0);
		for each (EnemyUnit* e in this->eInfo->allEnemyUnits)
		{
			if (e->getType() != UnitTypes::Terran_Bunker && e->getType().airWeapon() == WeaponTypes::None)
			{
				continue;
			}
			int range = e->getType() == UnitTypes::Terran_Bunker ? UnitTypes::Terran_Marine.seekRange() : e->getType().seekRange();
			if (e->getPosition().getApproxDistance(u->getPosition()) <= range + 32*4)
			{
				v += PFFunctions::getVelocitySource(e->getPosition(),u->getPosition())*1000;
			}
		}
		if (v != Vector2(0,0))
		{
			v = v * (128.0 / v.approxLen());
			u->move((v + u->getPosition()).makeValid());
			continue;
		}

		UnitGroup enemy = SelectAllEnemy()(isCompleted)(canAttack,Bunker).not(isFlyer,isWorker).inRadius(u->getType().sightRange() + 32*6, u->getPosition());
		if (enemy.empty())
		{
			if (this->terrainManager->eSecondChokepoint &&
				  u->getPosition().getApproxDistance(this->terrainManager->eSecondChokepoint->getCenter()) > 32)
			{
				u->move(this->terrainManager->eSecondChokepoint->getCenter());
			}							
		}
		//if we find enemy, follow the enemy center
		else
		{
			UnitGroup eTank = enemy(Siege_Tank);
			Position pos = eTank.empty() ? enemy.getCenter() : eTank.getCenter();
			u->move(pos);
		}
	}

	if (Broodwar->getFrameCount() > 24*60*5)
	{
		SelectAll()(isCompleted)(Barracks,Engineering_Bay).not(isLifted).lift();
	}
}

void MacroManager::ArmyGuard()
{
	// when goAttack is false and enemyInSight is empty

	Broodwar->drawTextScreen(200,0,"\x07 ArmyGuard");
	if (Broodwar->getFrameCount()%(24*3) != 9)
	{
		return;
	}

	this->atkTar.first = "No_AtkTar";
	this->atkTar.second = Position(0,0);

	for each (Unit* u in this->attackers)
	{
		if (!u->isCompleted() || !u->exists() || u->isLoaded())
		{
			continue;
		}

		if (u->getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode || u->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode)
		{
			tankAttackMode(u,this->setPoint);
		}
		else if (u->getType() == UnitTypes::Terran_Marine)
		{
			// go into bunker
			Unit* nearestBunker = NULL;
			int minD = 999999999;
			for each (Unit* bunker in SelectAll(UnitTypes::Terran_Bunker))
			{
				if (bunker->isCompleted() && bunker->getLoadedUnits().size() < 4 && u->getPosition().getApproxDistance(bunker->getPosition()) < minD)
				{
					minD = u->getPosition().getApproxDistance(bunker->getPosition());
					nearestBunker = bunker;
				}
			}

			if (nearestBunker)
			{
				u->rightClick(nearestBunker);
			}
			else if (u->getPosition().getApproxDistance(this->setPoint) > 32*4)
			{
				u->attack(this->setPoint);
			}
			else
			{
				u->holdPosition();
			}
		}
		else
		{
			if (u->getPosition().getApproxDistance(this->setPoint) > 32*4)
			{
				if (u->getType().canAttack())
				{
					u->attack(this->setPoint);
				}
				else
				{
					u->move(this->setPoint);
				}
			}
			else
			{
				u->holdPosition();
			}
		}
	}
}

void MacroManager::ArmyDefend()
{
	Broodwar->drawTextScreen(200,0,"\x07 ArmyDefend");

	if (Broodwar->getFrameCount()%8 != 5)
	{
		return;
	}

	for each (Unit* bunker in SelectAll(UnitTypes::Terran_Bunker))
	{
		if (!bunker->isCompleted() || bunker->getLoadedUnits().empty())
		{
			continue;
		}

		if (this->mental->enemyInSight.inRadius(32*10,bunker->getPosition()).empty())
		{
			bunker->unloadAll();
		}
	}

	Unit* target = this->mental->enemyInSight.getNearest(this->setPoint);
	Position targetPos = (target && target->exists()) ? target->getPosition() : this->setPoint;

	if (targetPos.getApproxDistance(this->setPoint) < 32*10 && BWTA::getRegion(targetPos) != BWTA::getRegion(Broodwar->self()->getStartLocation()))
	{
		targetPos = this->setPoint;
	}

	this->atkTar.first = "Enemy_Army";
	this->atkTar.second = targetPos;
	bool needTank = this->mental->enemyInSight.not(isWorker).size() < 6 ? false : true;
	allUnitAttack(target,targetPos,needTank);

	for each (Unit* u in this->attackers)
	{
		if (u->getType() != UnitTypes::Terran_Marine || u->isLoaded())
		{
			continue;
		}

		Unit* nearestBunker = NULL;
		int minD = 999999999;
		for each (Unit* bunker in SelectAll(UnitTypes::Terran_Bunker))
		{
			if (bunker->isCompleted() &&
				  bunker->getLoadedUnits().size() < 4 &&
				  !this->mental->enemyInSight.inRadius(32*10,bunker->getPosition()).empty() && 
				  u->getPosition().getApproxDistance(bunker->getPosition()) < minD)
			{
				minD = u->getPosition().getApproxDistance(bunker->getPosition());
				nearestBunker = bunker;
			}
		}

		if (nearestBunker)
		{
			u->rightClick(nearestBunker);
		}
	}
}

void MacroManager::ArmyAttack()
{
	Broodwar->drawTextScreen(200,0,"\x07 ArmyAttack");


	/************************************************************************/
	/* Bunker rush                                                          */
	/************************************************************************/

	if (Broodwar->enemy()->getRace() == Races::Terran && this->attackers.not(Marine).empty())
	{
		if (Broodwar->getFrameCount()%24 != 0)
		{
			return;
		}

		if (!this->scm->enemyStartLocation)
		{
			return;
		}

		this->atkTar.first = "Main_Base";
		this->atkTar.second = this->scm->enemyStartLocation->getPosition();

		// find the bunker we built in enemy base
		Unit* bunker = NULL;
		for each(Unit* b in SelectAll(UnitTypes::Terran_Bunker))
		{
			if (b->isCompleted())
			{
				bunker = b;
				break;
			}
		}

		if (bunker)
		{
			int range = Broodwar->self()->groundWeaponMaxRange(UnitTypes::Terran_Marine);
			Broodwar->drawCircleMap(bunker->getPosition().x(),bunker->getPosition().y(),range,Colors::Red);
			Broodwar->drawTextMap(bunker->getPosition().x()+range,bunker->getPosition().y(),"%d",range);
			UnitGroup targets = SelectAllEnemy().inRadius(range, bunker->getPosition());
			
			if (!targets.empty() && bunker->getLoadedUnits().size() < 4)
			{
				attackers(Marine).rightClick(bunker);
			}
			else
			{
				if(targets.empty() && bunker->getLoadedUnits().size() > 0)
				{
					bunker->unloadAll();
				}

				// send one marine to attack enemy scv
				if (!theMarine || !theMarine->exists())
				{
					for each (Unit* u in attackers(Marine))
					{
						if (u->getHitPoints() > 0.75 * u->getType().maxHitPoints())
						{
							theMarine = u;
							break;
						}
					}
				}
				
				if (theMarine && theMarine->exists())
				{
					Broodwar->drawCircleMap(theMarine->getPosition().x(),theMarine->getPosition().y(),15,Colors::Yellow);
					Broodwar->drawCircleMap(theMarine->getPosition().x(),theMarine->getPosition().y(),20,Colors::Yellow);
					attackers.erase(theMarine);
					UnitGroup eSCV = SelectAllEnemy()(isWorker).inRadius(UnitTypes::Terran_Marine.sightRange(),theMarine->getPosition());

					if (!eSCV.empty())
					{
						bool isInDanger = false;
						Unit* target = NULL;
						for each (Unit* e in eSCV)
						{
							if (e->getOrderTarget() == theMarine)
							{
								isInDanger = true;
								break;
							}
							target = e;
							if (e->isConstructing())
							{
								break;
							}
						}
						if (isInDanger || theMarine->isUnderAttack())
						{
							theMarine->rightClick(bunker);
						}
						else if (!(theMarine->getLastCommand().getType() == UnitCommandTypes::Attack_Unit && theMarine->getLastCommand().getTarget() == target))
						{
							Broodwar->drawLineMap(theMarine->getPosition().x(),theMarine->getPosition().y(),target->getPosition().x(),target->getPosition().y(),Colors::Red);
							theMarine->attack(target);
						}
					}
					else
					{
						theMarine->attack(this->atkTar.second);
					}
				}

				attackers(Marine).attackMove(this->atkTar.second);
			}
		}
		// the bunker is not completed, so just attack enemy
		else
		{
			attackers(Marine).attackMove(this->atkTar.second);
		}

		return;
	}


	/************************************************************************/
	/* VS other races or not bunker rush                                    */
	/************************************************************************/

	if (Broodwar->getFrameCount()%8 != 0)
	{
		return;
	}

	for each (Unit* u in SelectAll()(Bunker))
	{
		if (!u->getLoadedUnits().empty())
		{
			u->unloadAll();
		}
	}

	// if we know where enemy start location or expansions are, then start from enemy main base then expansions
	if (this->scm->enemyStartLocation && this->eInfo->enemyBaseMap.size() > 0)
	{
		double maxD = 0;
		Unit* mainbaseTar     = NULL;
		Position mainbasePos  = Positions::None;
		Unit* expansionTar    = NULL;
		Position expansionPos = Positions::None;

		for (map<Unit*,EnemyInfoManager::eBaseData>::iterator i = this->eInfo->enemyBaseMap.begin(); i != this->eInfo->enemyBaseMap.end(); i++)
		{
			if (i->second.isMainBase)
			{
				mainbaseTar = i->first;
				mainbasePos = i->second.position;
			}
			else
			{
				double d = this->terrainManager->getGroundDistance(i->second.tPosition, this->scm->enemyStartLocation->getTilePosition());
				if (d > maxD)
				{
					maxD = d;
					expansionTar = i->first;
					expansionPos = i->second.position;
				}
			}
		}

		// if our army is large enough then attack enemy main base directly
		if (Broodwar->self()->supplyUsed()/2 >= 190 ||
			  (Broodwar->self()->supplyUsed()/2 >= 150 && (this->mInfo->myFightingValue().first >= 1.5 * this->eInfo->enemyFightingValue().first)))
		{
			if (mainbaseTar && mainbasePos != Positions::None)
			{
				this->atkTar.first = "MainBase";
				this->atkTar.second = mainbasePos;
				allUnitAttack(mainbaseTar, mainbasePos);
			}
			else if (expansionTar && expansionPos != Positions::None)
			{
				this->atkTar.first = "Expansion";
				this->atkTar.second = expansionPos;
				allUnitAttack(expansionTar, expansionPos);
			}
			else
			{
				this->atkTar.first = "MainBase";
				this->atkTar.second = this->scm->enemyStartLocation->getPosition();
				allUnitAttack(NULL, this->atkTar.second);
			}
		}
		// our army is not large enough, so start from enemy expansion
		else
		{
			if (expansionTar && expansionPos != Positions::None)
			{
				this->atkTar.first = "Expansion";
				this->atkTar.second = expansionPos;
				allUnitAttack(expansionTar, expansionPos);
			}
			else if (mainbaseTar && mainbasePos != Positions::None)
			{
				this->atkTar.first = "Only_MainBase";
				this->atkTar.second = mainbasePos;
				allUnitAttack(mainbaseTar, mainbasePos);
			}
			else
			{
				this->atkTar.first = "Only_MainBase";
				this->atkTar.second = this->scm->enemyStartLocation->getPosition();
				allUnitAttack(NULL, this->atkTar.second);
			}
		}
	}
	// if we don't know where enemy start location or expansions are, then attack enemy buildings on map
	else
	{
		for (std::map<Unit*,std::pair<UnitType,Position>>::const_iterator i = this->eInfo->eBuildingPositionMap.begin(); i != this->eInfo->eBuildingPositionMap.end(); i++)
		{
			if (!i->first)
				continue;
			else if (i->second.second == Positions::Invalid || i->second.second == Positions::None || i->second.second == Positions::Unknown)
				continue;
			else if (Broodwar->isVisible(i->first->getTilePosition()) && SelectAllEnemy()(isBuilding).inRadius(32*8,i->second.second).size() < 1)
				continue;	
			else
			{
				allUnitAttack(i->first,i->second.second);
				this->atkTar.first = i->second.first.c_str();
				this->atkTar.second = i->second.second;
				break;
			}				
		}

		//_T_
		// don't know anything about enemy
		if (this->atkTar.first == "No_AtkTar")
		{
			allUnitAttack(NULL,this->terrainManager->mSecondChokepoint->getCenter());
			this->atkTar.second = this->terrainManager->mSecondChokepoint->getCenter();
		}
	}

	if (this->desEnemyMainBase)
	{
		if (!Broodwar->enemy()->isDefeated())
			findRestEnemy();
	}
}