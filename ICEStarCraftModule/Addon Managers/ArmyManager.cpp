#include <time.h>
#include "ArmyManager.h"

using namespace std;
using namespace BWAPI;
using namespace BWTA;
using namespace ICEStarCraft;

ArmyManager* theArmyManager = NULL;

ArmyManager* ArmyManager::create()
{
	if (theArmyManager) return theArmyManager;
	else return theArmyManager = new ArmyManager();
}

void ArmyManager::destroy()
{
	if (theArmyManager)
	{
		delete theArmyManager;
		theArmyManager = NULL;
	}
}

ArmyManager::ArmyManager()
{
	gameFlow = GameFlow::create();
	mInfo = MyInfoManager::create();
	eInfo = EnemyInfoManager::create();
	mental = MentalClass::create();
	scoutManager = ScoutManager::create();
	terrainManager = TerrainManager::create();
	arbitrator = NULL;

	attackTarget = new AttackTarget(NULL,Positions::None,"NoAttackTarget");
	state = ICEStarCraft::ArmyGuard;
	attackers.clear();
	vessels.clear();
	setPoint = Positions::None;
	setPoint2 = Positions::None;
	siegePoint = Positions::None;
	gatherPoint = Positions::None;
	lastAttackFrame = 0;
	startGatheringFrame = 0;
	shouldGatherBeforeAttack = true;
	hasScoutedBeforeAttack = false;
	scoutUnit = NULL;
}

ArmyManager::~ArmyManager()
{

}

void ArmyManager::onOffer(set<Unit*> units)
{
	for each (Unit* u in units)
	{
		arbitrator->accept(this, u);
		if (u->getType() == UnitTypes::Terran_Science_Vessel)
		{
			if (vessels.find(u) == vessels.end())
			{
				vessels.insert(u);
			}
		}
		else
		{
			if (attackers.find(u) == attackers.end())
			{
				attackers.insert(u);
			}
		}
	}
}

void ArmyManager::onRevoke(BWAPI::Unit* unit, double bid)
{
	if (!unit)
	{
		return;
	}

	if (unit->getType() == UnitTypes::Terran_Science_Vessel)
	{
		vessels.erase(unit);
	}
	else
	{
		attackers.erase(unit);
	}
}

void ArmyManager::onUnitDestroy(BWAPI::Unit* unit)
{
	if (!unit)
	{
		return;
	}
	
	if (unit->getPlayer() == Broodwar->self())
	{
		if (unit->getType() == UnitTypes::Terran_Science_Vessel)
		{
			vessels.erase(unit);
		}
		else
		{
			attackers.erase(unit);
		}
	}
}

void ArmyManager::update()
{
	if (!mental->goAttack)
	{
		shouldGatherBeforeAttack = true;
		hasScoutedBeforeAttack = false;
		startGatheringFrame = Broodwar->getFrameCount();
	}
	if ((mental->STflag == MentalClass::PtechCarrier && Broodwar->getFrameCount() < 24*60*20)
		  ||
			(Broodwar->enemy()->getRace() == Races::Terran && Broodwar->getFrameCount() < 24*60*10))
	{
		shouldGatherBeforeAttack = false;
	}

	updateSetPoint();
	updateSiegePoint();
	updateArmyUnits();

	if (mental->goAttack)
	{
		if (mental->enemyInSight.not(isWorker).size() < 10 || Broodwar->self()->supplyUsed()/2 > 180)
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
		if (mental->enemyInSight.empty())
		{
			ArmyGuard();
		}
		else
		{
			ArmyDefend();
		}
	}

	controlScienceVessels();
	allUnitsAvoidPsionicStorm();
	allUnitsAvoidNuclearMissile();
}

void ArmyManager::updateArmyUnits()
{
	for each (Unit* u in Broodwar->self()->getUnits())
	{
		if (!u->isCompleted() ||
			  u->getType().isBuilding() ||
				u->getType().isWorker() ||
				(!u->getType().canAttack() && u->getType() != UnitTypes::Terran_Science_Vessel) ||
			  u->getType() == UnitTypes::Terran_Vulture_Spider_Mine ||
				u->getType() == UnitTypes::Terran_Nuclear_Missile)
		{
			continue;
		}

		if (u->isLoaded())
		{
			attackers.erase(u);
			arbitrator->removeBid(this, u);
			continue;
		}

		if (vessels.find(u) != vessels.end())
		{
			continue;
		}

		if (attackers.find(u) != attackers.end())
		{
			if (u->getType() == UnitTypes::Terran_Vulture)
			{
				if (mental->enemyInSight.size() > 3 && !mental->goAttack)
				{
					arbitrator->setBid(this, u, 150);
				}
				else if (mental->goAttack)
				{
					arbitrator->setBid(this, u, 90);
				}
				else
				{
					arbitrator->setBid(this, u, 50);
				}
			}
			continue;
		}

		if (u->getType() == UnitTypes::Terran_Science_Vessel)
		{
			arbitrator->setBid(this, u, 100);
		}
		else if (u->getType() == UnitTypes::Terran_Vulture)
		{
			if (mental->enemyInSight.size() > 3 && !mental->goAttack)
			{
				arbitrator->setBid(this, u, 150);
			}
			else if (mental->goAttack)
			{
				arbitrator->setBid(this, u, 90);
			}
			else
			{
				arbitrator->setBid(this, u, 50);
			}
		}
		else
		{
			arbitrator->setBid(this, u, 50);
		}
	}
}

void ArmyManager::updateSetPoint()
{
	if (Broodwar->getFrameCount()%(24*5) != 0 && Broodwar->getFrameCount() != 0)
	{
		return;
	}

	Position lastSetPoint  = setPoint;
	Position lastSetPoint2 = setPoint2;

	/************************************************************************/
	/* VS Terran                                                            */
	/************************************************************************/

	if (Broodwar->enemy()->getRace() == Races::Terran)
	{
		Position mapCenter    = Position(Broodwar->mapWidth()*32/2, Broodwar->mapHeight()*32/2);
		Position mFirstChoke  = terrainManager->mFirstChokepoint->getCenter();
		Position mSecondChoke = terrainManager->mSecondChokepoint->getCenter();

		if (scoutManager->enemyStartLocation)
		{
			Position eSecondChoke = terrainManager->eSecondChokepoint->getCenter();
			int deadArmy = Broodwar->self()->deadUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + 
				             Broodwar->self()->deadUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode) + 
				             Broodwar->self()->deadUnitCount(UnitTypes::Terran_Goliath) +
				             Broodwar->self()->deadUnitCount(UnitTypes::Terran_Vulture) +
				             Broodwar->self()->deadUnitCount(UnitTypes::Terran_Marine);

			int enemyTank = eInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode) + eInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Siege_Mode);

			if ((Broodwar->self()->supplyUsed()/2 >= 60 || eInfo->killedEnemyNum > deadArmy) && SelectAll()(isCompleted)(Siege_Tank).size() >= enemyTank + 3)
			{
				setPoint = Position(eSecondChoke.x()*3/4 + mapCenter.x()/4, eSecondChoke.y()*3/4 + mapCenter.y()/4);
			}
			else if (SelectAll()(isCompleted)(Siege_Tank).size() >= 3)
			{
				setPoint = Position(eSecondChoke.x()/2 + mapCenter.x()/2, eSecondChoke.y()/2 + mapCenter.y()/2);
			}
			else
			{
				if (gameFlow->bunkerPosition && BWTA::getRegion(*(gameFlow->bunkerPosition)) != BWTA::getRegion(Broodwar->self()->getStartLocation()))
				{
					setPoint = Position(*(gameFlow->bunkerPosition));
				}
				else
				{
					if (Broodwar->getFrameCount() <= 24*60*5 && !mental->marineRushOver)
					{
						setPoint = mapCenter;
					}
					else
					{
						setPoint = mSecondChoke;
					}
				}
			}
		} 
		//if we do not know where the enemy is
		else
		{			
			if (Broodwar->getFrameCount() <= 24*60*5 && !mental->marineRushOver)
			{
				setPoint = mapCenter;
			}
			else
			{
				setPoint = mSecondChoke;
			}
		}
	}

	/************************************************************************/
	/* VS Other Races                                                       */
	/************************************************************************/

	else
	{
		if (Broodwar->self()->supplyUsed()/2 <= 60)
		{
			if (SelectAll()(isBuilding).inRegion(terrainManager->mNearestBase->getRegion()).empty())
			{
				// gather near first choke point inside main base
				BWTA::Chokepoint* cp = terrainManager->mFirstChokepoint;
				Vector2 v = Vector2(cp->getSides().first) - Vector2(cp->getSides().second);
				v = Vector2(-v.y(),v.x());
				v = v * (32 * 4.0 / v.approxLen());
				Position p = v + cp->getCenter();
				if (BWTA::getRegion(p) == terrainManager->mNearestBase->getRegion())
				{
					p = -v + cp->getCenter();
				}

				setPoint = p.makeValid();
			}
			else if (gameFlow->bunkerPosition)
			{
				this->setPoint = Position(*(gameFlow->bunkerPosition));
			}
			else
			{
				setPoint = terrainManager->mSecondChokepoint->getCenter();
			}
		}
		else if (Broodwar->self()->supplyUsed()/2 <= 90)
		{
			setPoint = terrainManager->mSecondChokepoint->getCenter();
		}
		else /*if (Broodwar->self()->supplyUsed()/2 > 90*/
		{
			Position mSecondChoke = terrainManager->mSecondChokepoint->getCenter();
			Position mThirdChoke  = terrainManager->mThirdChokepoint->getCenter();
			if (mInfo->myFightingValue().first > eInfo->enemyFightingValue().first * 3)
			{
				if (mSecondChoke.getApproxDistance(mThirdChoke) < 15 * 32)
				{
					setPoint = mThirdChoke;
				}
				else
				{
					Vector2 v = Vector2(mThirdChoke) - Vector2(mSecondChoke);
					v = v * (32.0 * 15 / v.approxLen());
					setPoint = (v + mSecondChoke).makeValid();
				}
			}
			else if (mInfo->myFightingValue().first > eInfo->enemyFightingValue().first * 2)
			{
				setPoint = Position(mSecondChoke.x()/2 + mThirdChoke.x()/2, mSecondChoke.y()/2 + mThirdChoke.y()/2);
			}
			else
			{
				setPoint = mSecondChoke;
			}
		}
	}

	if (terrainManager->getGroundDistance(TilePosition(setPoint),Broodwar->self()->getStartLocation()) < 0)
	{
		setPoint = Position(terrainManager->getConnectedTilePositionNear(TilePosition(setPoint)));
	}
	//Broodwar->drawCircleMap(setPoint.x(),setPoint.y(),12,Colors::Red,true);

	if (setPoint == lastSetPoint)
	{
		setPoint2 = lastSetPoint2;
	}
	else if (Broodwar->enemy()->getRace() == Races::Terran ||
			     terrainManager->getGroundDistance(TilePosition(setPoint),Broodwar->self()->getStartLocation()) < 0 ||
			     !terrainManager->eSecondChokepoint)
	{
		setPoint2 = setPoint;
	}
	else
	{
		//clock_t t = clock();
		vector<TilePosition> path = BWTA::getShortestPath(TilePosition(setPoint),TilePosition(terrainManager->eSecondChokepoint->getCenter()));
		//Broodwar->printf("getShortestPath %.0f",(float)(1000*(clock()-t)/CLOCKS_PER_SEC));
		if (path.empty() || path.size() < 5)
		{
			setPoint2 = setPoint;
		}
		else
		{
			setPoint2 = Position(path[4]);
		}
	}
	//Broodwar->drawCircleMap(setPoint2.x(),setPoint2.y(),8,Colors::Green,true);
}

void ArmyManager::updateSiegePoint()
{
	if (Broodwar->getFrameCount()%(24*5) != 60 && Broodwar->getFrameCount() != 0)
	{
		return;
	}

	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->completedUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode) < 1)
	{
		return;
	}

	if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode) && !Broodwar->self()->isResearching(TechTypes::Tank_Siege_Mode))
	{
		return;
	}

	if (terrainManager->siegePoint == Positions::None)
	{
		siegePoint = Positions::None;
		return;
	}

	if (setPoint != Positions::None && 
		  (BWTA::getNearestChokepoint(setPoint) == terrainManager->mFirstChokepoint ||
		  BWTA::getNearestChokepoint(setPoint) == terrainManager->mSecondChokepoint ||
		  BWTA::getRegion(setPoint) == terrainManager->mNearestBase->getRegion()))
	{
		siegePoint = terrainManager->siegePoint;
	}
	else
	{
		siegePoint = Positions::None;
	}
}

void ArmyManager::allUnitsAttack(BWAPI::Position p, bool needTank)
{
	if (p == Positions::None)
	{
		return;
	}

	if (lastAttackFrame == Broodwar->getFrameCount())
	{
		return;
	}

	lastAttackFrame = Broodwar->getFrameCount();
	
	if (allUnitsGather(p,needTank) == true)
	{
		// army gathering
		return;
	}
	
	// ATTACK
	for each (Unit* u in attackers)
	{
		if (needTank && (u->getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode || u->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode))
		{
			MicroUnitControl::tankAttack(u,p);
		}
		else if (u->getType() == UnitTypes::Terran_Battlecruiser)
		{
			MicroUnitControl::battlecruiserAttack(u,p);
		}
		else
		{
			MicroUnitControl::attack(u,p);
		}
	}
}

bool ArmyManager::allUnitsGather(Position p, bool needTank)
{
	gatherPoint = Positions::None;

	if (attackTarget->getType() == "NoAttackTarget")
	{
		return true;
	}

	if (state != ICEStarCraft::ArmyAttack || !shouldGatherBeforeAttack)
	{
		return false;
	}

	// gather units before the attack
	if (attackTarget->getType() == "MainBase" || attackTarget->getType() == "OnlyMainBase" || attackTarget->getType() == "Expansion" || attackTarget->getType() == "EnemyArmy")
	{
		BWTA::Region* atkReg = BWTA::getRegion(attackTarget->getPosition());
		if (attackTarget->getType() == "MainBase" || attackTarget->getType() == "OnlyMainBase")
		{
			if (terrainManager->eNearestBase)
			{
				atkReg = terrainManager->eNearestBase->getRegion();
			}
		}
		if (atkReg)
		{
			int px = (atkReg->getCenter().x()/32 + Broodwar->mapWidth()/2)/2;
			int py = (atkReg->getCenter().y()/32 + Broodwar->mapHeight()/2)/2;
			gatherPoint = Position(terrainManager->getConnectedTilePositionNear(TilePosition(px,py)));
			if (gatherPoint.getApproxDistance(atkReg->getCenter()) > setPoint.getApproxDistance(atkReg->getCenter()))
			{
				gatherPoint = setPoint;
			}

			//Broodwar->drawCircleMap(gatherPoint.x(),gatherPoint.y(),20,Colors::Purple,true);
		}
		else
		{
			gatherPoint = Positions::None;
		}	
	}
	else
	{
		gatherPoint = Positions::None;
	}

	if (gatherPoint == Positions::None || Broodwar->getFrameCount() - startGatheringFrame > 24*90)
	{
		shouldGatherBeforeAttack = false;
		return false;
	}

	UnitGroup army = attackers + BattleManager::create()->getMyUnits();
	if (army.inRadius(32*10,gatherPoint).size() < 0.8 * army.size())
	{
		// gather units and send 1 unit to scout
		bool needScout = (army.inRadius(32*10,gatherPoint).size() >= 0.3 * army.size()) && !hasScoutedBeforeAttack;
		int minD = -1;

		for each (Unit* u in attackers)
		{	
			if (u == scoutUnit)
			{
				u->attack(p);
				continue;
			}

			if (needScout && !u->getType().isFlyer() &&	u->getType().canMove() && u->getType() != UnitTypes::Terran_Siege_Tank_Tank_Mode)
			{
				if (minD == -1 || u->getPosition().getApproxDistance(p) < minD)
				{
					minD = u->getPosition().getApproxDistance(p);
					scoutUnit = u;
				}	
			}

			if (needTank && (u->getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode || u->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode))
			{
				MicroUnitControl::tankAttack(u,gatherPoint);
			}
			else if (u->getType() == UnitTypes::Terran_Battlecruiser)
			{
				MicroUnitControl::battlecruiserAttack(u,gatherPoint);
			}
			else if (u->getPosition().getApproxDistance(gatherPoint) > 32*3)
			{
				u->attack(gatherPoint);
			}
		}
		if (scoutUnit && !hasScoutedBeforeAttack)
		{
			hasScoutedBeforeAttack = true;
			scoutUnit->attack(p);
		}
		return true;
	}

	// let units attack enemy
	if (hasScoutedBeforeAttack)
	{
		shouldGatherBeforeAttack = false;
		gatherPoint = Positions::None;
		return false;
	}

	return true;
}

void ArmyManager::controlScienceVessels()
{
	if (Broodwar->getFrameCount()%8 != 7)
	{
		return;
	}

	for each (Unit* u in vessels)
	{
		MicroUnitControl::scienceVesselMicro(u);
	}
}

void ArmyManager::allUnitsAvoidPsionicStorm()
{
	if (Broodwar->enemy()->getRace() != Races::Protoss)
	{
		return;
	}

	for each (Unit* u in Broodwar->self()->getUnits())
	{
		if (!u->isCompleted() || u->isLoaded() || u->getType().isBuilding() || !u->getType().canMove())
		{
			continue;
		}

		Position pos = Positions::None;
		Vector2 v = Vector2(0,0);

		for each (Unit* e in Broodwar->enemy()->getUnits())
		{
			if (e->getType() == UnitTypes::Protoss_High_Templar && e->getOrder() == Orders::CastPsionicStorm)
			{
				Position tarPos = e->getOrderTargetPosition();
				if (tarPos != Positions::None && u->getPosition().getApproxDistance(tarPos) <= 32*4)
				{
					v += PFFunctions::getVelocitySource(tarPos,u->getPosition()) * 1000;
				}
			}
		}

		for each (Bullet* b in Broodwar->getBullets())
		{
			if (b->getType() == BulletTypes::Psionic_Storm)
			{
				if (u->getPosition().getApproxDistance(b->getPosition()) <= 32*5)
				{
					v += PFFunctions::getVelocitySource(b->getPosition(),u->getPosition()) * 1000;
				}
			}
		}

		if (v != Vector2(0,0))
		{
			v = v * (256.0 / v.approxLen());
			pos = (v + u->getPosition()).makeValid();
			u->move(pos);
		}
	}
}

void ArmyManager::allUnitsAvoidNuclearMissile()
{
	if (Broodwar->enemy()->getRace() != Races::Terran)
	{
		return;
	}

	for each (Unit* u in Broodwar->self()->getUnits())
	{
		if (!u->isCompleted() || u->isLoaded() || u->getType().isBuilding() || !u->getType().canMove())
		{
			continue;
		}

		Position pos = Positions::None;
		Vector2 v = Vector2(0,0);

		for each (Position p in Broodwar->getNukeDots())
		{
			if (u->getPosition().getApproxDistance(p) < 32*10)
			{
				v += PFFunctions::getVelocitySource(p,u->getPosition()) * 1000;
			}
		}

		for each (Unit* i in Broodwar->getAllUnits())
		{
			if (i->getType() == UnitTypes::Terran_Nuclear_Missile)
			{
				if (u->getPosition().getApproxDistance(i->getPosition()) < 32*10)
				{
					v += PFFunctions::getVelocitySource(i->getPosition(),u->getPosition()) * 1000;
				}
			}
		}

		if (v != Vector2(0,0))
		{
			v = v * (256.0 / v.approxLen());
			pos = (v + u->getPosition()).makeValid();
			u->move(pos);
		}
	}
}

void ArmyManager::ArmyGuard()
{
	state = ICEStarCraft::ArmyGuard;
	attackTarget->update(NULL,setPoint,"NoAttackTarget");

	if (Broodwar->getFrameCount()%(10) != 9)
	{
		return;
	}
	
	for each (Unit* u in attackers)
	{
		if (u->getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode || u->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode)
		{
			MicroUnitControl::tankAttack(u,setPoint);
		}
		else if (u->getType() == UnitTypes::Terran_Battlecruiser)
		{
			MicroUnitControl::battlecruiserAttack(u,setPoint);
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
			else if (u->getPosition().getApproxDistance(setPoint) > 32*4)
			{
				u->attack(setPoint2);
			}
		}
		else
		{
			if (u->getPosition().getApproxDistance(setPoint) > 32*4)
			{
				u->attack(setPoint2);
			}
		}
	}
}

void ArmyManager::ArmyDefend()
{
	state = ICEStarCraft::ArmyDefend;

	if (attackers.empty())
	{
		return;
	}

	if (Broodwar->getFrameCount()%8 == 4)
	{
		for each (Unit* bunker in SelectAll(UnitTypes::Terran_Bunker))
		{
			if (!bunker->isCompleted() || bunker->getLoadedUnits().empty())
			{
				continue;
			}

			Position p = this->mental->enemyInSight.getTargetPosition();
			if (p != Positions::None)
			{
				if (p.getApproxDistance(bunker->getPosition()) > 32*6 && mental->enemyInSight.inRadius(32*10,bunker->getPosition()).empty())
				{
					bunker->unloadAll();
				}
			}
		}

		Unit* target = mental->enemyInSight.getNearest(setPoint);
		Position targetPos = (target && target->exists()) ? target->getPosition() : setPoint;

		if (targetPos.getApproxDistance(setPoint) < 32*10 &&
			  BWTA::getRegion(targetPos) != BWTA::getRegion(Broodwar->self()->getStartLocation()) &&
				BWTA::getRegion(targetPos) != TerrainManager::create()->mNearestBase->getRegion())
		{
			targetPos = setPoint;
		}

		attackTarget->update(target,targetPos,"EnemyArmy");
	}

	if (attackTarget->getPosition() != Positions::None)
	{
		Position tankPos = (siegePoint == Positions::None) ? setPoint : siegePoint;
		bool needTank = true;
		if (mental->enemyInSight.not(isWorker).size() < 6 && attackTarget->getPosition().getApproxDistance(tankPos) > 32*15)
		{
			needTank = false;
		}
		allUnitsAttack(attackTarget->getPosition(),needTank);
	}
	
	if (Broodwar->getFrameCount()%8 == 7)
	{
		for each (Unit* u in attackers)
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
					  !mental->enemyInSight.inRadius(32*10,bunker->getPosition()).empty() && 
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
}

void ArmyManager::ArmyAttack()
{
	state = ICEStarCraft::ArmyAttack;

	if (attackers.empty())
	{
		return;
	}

	if (Broodwar->getFrameCount()%24 == 12)
	{
		for each (Unit* u in SelectAll()(Bunker))
		{
			if (!u->getLoadedUnits().empty())
			{
				u->unloadAll();
			}
		}
	}

	if (Broodwar->getFrameCount()%(24*5) == 0)
	{
		// if enemy start location or expansions are known, then start from enemy main base then expansions
		if (scoutManager->enemyStartLocation && eInfo->enemyBaseMap.size() > 0)
		{
			double maxD = 0;
			Unit* mainbaseTar     = NULL;
			Position mainbasePos  = Positions::None;
			Unit* expansionTar    = NULL;
			Position expansionPos = Positions::None;

			for (map<Unit*,EnemyInfoManager::eBaseData>::iterator i = eInfo->enemyBaseMap.begin(); i != eInfo->enemyBaseMap.end(); i++)
			{
				if (i->second.isMainBase)
				{
					mainbaseTar = i->first;
					mainbasePos = i->second.position;
				}
				else
				{
					double d = terrainManager->getGroundDistance(i->second.tPosition, scoutManager->enemyStartLocation->getTilePosition());
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
				  (Broodwar->self()->supplyUsed()/2 >= 150 && (mInfo->myFightingValue().first >= 1.5 * eInfo->enemyFightingValue().first)))
			{
				if (mainbaseTar && mainbasePos != Positions::None)
				{
					attackTarget->update(mainbaseTar,mainbasePos,"MainBase");
				}
				else if (expansionTar && expansionPos != Positions::None)
				{
					attackTarget->update(expansionTar,expansionPos,"Expansion");
				}
				else
				{
					attackTarget->update(NULL,scoutManager->enemyStartLocation->getPosition(),"MainBase");
				}
			}
			// our army is not large enough, so start from enemy expansion
			else
			{
				if (expansionTar && expansionPos != Positions::None)
				{
					attackTarget->update(expansionTar,expansionPos,"Expansion");
				}
				else if (mainbaseTar && mainbasePos != Positions::None)
				{
					attackTarget->update(mainbaseTar,mainbasePos,"OnlyMainBase");
				}
				else
				{
					attackTarget->update(NULL,scoutManager->enemyStartLocation->getPosition(),"OnlyMainBase");
				}
			}
		}
		// if enemy start location or expansions are unknown, then attack enemy buildings on map
		else
		{
			int minD = -1;
			Position mCenter = attackers.getCenter();
			EnemyUnit* target = NULL;

			for each (EnemyUnit* e in eInfo->getAllEnemyUnits())
			{
				if (e->getPosition() == Positions::Unknown)
				{
					continue;
				}

				int d = e->getPosition().getApproxDistance(mCenter);
				if (!e->getType().isBuilding())
				{
					d *= 1000;
				}
				if (minD == -1 || d < minD)
				{
					minD = d;
					target = e;
				}
			}

			if (target)
			{
				attackTarget->update(target->getUnit(),target->getPosition(),target->getType().getName());
			}
			else
			{
				// we don't know anything about enemy
				attackTarget->update(NULL,terrainManager->mSecondChokepoint->getCenter(),"NoAttackTarget");
			}
		}
	}
	
	allUnitsAttack(attackTarget->getPosition());
}

bool ArmyManager::isAttackTarget(Unit* unit)
{
	if (!unit || !unit->exists() || unit->getPlayer() != Broodwar->enemy())
	{
		return false;
	}
	
	if (!unit->isCompleted() || unit->isLoaded() || !unit->isDetected())		 
	{
		return false;
	}

	UnitType type = unit->getType();

	if (type.canAttack())
	{
		if (type == UnitTypes::Protoss_Scarab ||
			  type == UnitTypes::Protoss_Interceptor ||
			  type == UnitTypes::Terran_Vulture_Spider_Mine ||
			  type == UnitTypes::Terran_Nuclear_Missile)
		{
			return false;
		}
	}

	if (!type.canAttack())
	{
		if (type != UnitTypes::Protoss_Carrier &&
			  type != UnitTypes::Protoss_Reaver &&
			  type != UnitTypes::Protoss_High_Templar &&
			  type != UnitTypes::Terran_Bunker &&
			  type != UnitTypes::Zerg_Defiler)
		{
			return false;
		}
	}

	return true;
}

UnitGroup ArmyManager::getAttackTargets()
{
	UnitGroup enemies = UnitGroup::getUnitGroup(Broodwar->enemy()->getUnits());
	return getAttackTargets(enemies, Positions::None, 0);
}

UnitGroup ArmyManager::getAttackTargets(UnitGroup& enemies)
{
	return getAttackTargets(enemies, Positions::None, 0);
}

UnitGroup ArmyManager::getAttackTargets(BWAPI::Position p, int range)
{
	UnitGroup enemies = UnitGroup::getUnitGroup(Broodwar->enemy()->getUnits());
	return getAttackTargets(enemies, p, range);
}

UnitGroup ArmyManager::getAttackTargets(UnitGroup& enemies, BWAPI::Position p, int range)
{
	UnitGroup targets;

	for each (Unit* e in enemies)
	{
		if (!isAttackTarget(e))
		{
			continue;
		}

		if (p != Positions::None && e->getPosition().getDistance(p) > range)
		{
			continue;
		}

		targets.insert(e);
	}

	return targets;
}

string ArmyManager::getArmyStateString() const
{
	switch (state)
	{
	case ICEStarCraft::ArmyGuard:
		return "ArmyGuard";
	case ICEStarCraft::ArmyDefend:
		return "ArmyDefend";
	case ICEStarCraft::ArmyAttack:
		return "ArmyAttack";
	default:
		return "None";
	}
}

void ArmyManager::showDebugInfo()
{
	// army state
	if (state == ICEStarCraft::ArmyGuard)
	{
		Broodwar->drawTextScreen(200,0,"\x07 ArmyGuard");
	}
	else if (state == ICEStarCraft::ArmyDefend)
	{
		Broodwar->drawTextScreen(200,0,"\x07 ArmyDefend");
	}
	else if (state == ICEStarCraft::ArmyAttack)
	{
		Broodwar->drawTextScreen(200,0,"\x07 ArmyAttack");
	}
	
	// siege point
	if (siegePoint != Positions::None)
	{
		Position siegePos = siegePoint;
		Broodwar->drawCircleMap(siegePos.x(),siegePos.y(),20,Colors::Green);
		Broodwar->drawCircleMap(siegePos.x(),siegePos.y(),10,Colors::Green);
	}

	// attackers info
	map<UnitType,int> list;
	int x = 433;
	int num;
	int line = 5;

	Broodwar->drawTextScreen(x,line*10," Workers: %d",SelectAll(UnitTypes::Terran_SCV)(isCompleted).size());
	line += 2;
	
	for each (Unit* u in attackers)
	{
		Broodwar->drawCircleMap(u->getPosition().x(),u->getPosition().y(),2,Colors::Green,true);
	}

	UnitGroup fighers = attackers + BattleManager::create()->getMyUnits();
	for each (Unit* u in fighers)
	{
		if (list.empty())
		{
			list.insert(make_pair(u->getType(),1));
		}
		else
		{
			map<UnitType,int>::iterator i = list.find(u->getType());
			if (i != list.end())
			{
				num = (*i).second;
				list.erase(i);
				list.insert(make_pair(u->getType(),num+1));
			}
			else
			{
				list.insert(make_pair(u->getType(),1));
			}
		}
	}

	Broodwar->drawTextScreen(x,line*10,"\x07 Attackers: %d/%d",BattleManager::create()->getMyUnits().size(),fighers.size());
	line++;
	for (map<UnitType,int>::iterator i = list.begin(); i != list.end(); i++)
	{
		Broodwar->drawTextScreen(x,line*10," %d %s",(*i).second,(*i).first.getName().c_str());
		line++;
	}

	// region we are attacking
	if (attackTarget->getType() != "NoAttackTarget")
	{
		BWTA::Region* r = BWTA::getRegion(attackTarget->getPosition());
		if (r)
		{
			BWTA::Polygon p = r->getPolygon();
			set<Chokepoint*> cp = BWTA::getChokepoints();
			bool isChokePoint = false;

			for (Polygon::iterator i = p.begin(); i != p.end(); i++)
			{
				isChokePoint = false;
				for (set<Chokepoint*>::iterator c = cp.begin(); c != cp.end(); c++)
				{
					if ((*i) == (*c)->getCenter())
					{
						isChokePoint = true;
						break;
					}
				}
				Broodwar->drawCircleMap((*i).x(),(*i).y(),2,Colors::Yellow,isChokePoint);
			}
		}
	}
}