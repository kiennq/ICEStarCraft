#include <time.h>
#include "MacroUnitControl.h"

using namespace std;
using namespace BWAPI;
using namespace BWTA;
using namespace ICEStarCraft;

MacroManager* theMacroManager = NULL;

MacroManager* MacroManager::create() 
{
	if (theMacroManager) return theMacroManager;
	else return theMacroManager = new MacroManager();
}

void MacroManager::destroy()
{
	if (theMacroManager)
	{
		delete theMacroManager;
		theMacroManager = NULL;
	}

	ArmyManager::destroy();
	BaseDefenseManager::destroy();
	BattleManager::destroy();
	DropManager::destroy();
	MineManager::destroy();
}

MacroManager::MacroManager()
{
	arbitrator = NULL;
	buildOrder = NULL;
	eInfo = EnemyInfoManager::create();
	mInfo = MyInfoManager::create();
	mental = MentalClass::create();
	terrainManager = TerrainManager::create();
	lastScanFrame = 0;
	mineralRemoved = false;
	mineralPosition = terrainManager->getBlockingMineral();
	workers.clear();
}

MacroManager::~MacroManager()
{

}

void MacroManager::onOffer(set<Unit*> units)
{
	for each (Unit* u in units)
	{
		if (u->getType().isWorker())
		{
			arbitrator->accept(this, u);
			workers.insert(u);
		}
		else
		{
			arbitrator->decline(this, u, 0);
		}
	}
}

void MacroManager::onRevoke(Unit* unit, double bid)
{
	if (!unit || !unit->exists())
	{
		return;
	}

	workers.erase(unit);
}

void MacroManager::setManagers(BuildOrderManager* buildOrder)
{
	this->buildOrder = buildOrder;
	DropManager::create()->setManagers(buildOrder);
}

void MacroManager::setArbitrator(Arbitrator::Arbitrator<BWAPI::Unit*,double>* arbitrator)
{
	this->arbitrator = arbitrator;
	ArmyManager::create()->setArbitrator(arbitrator);
	BaseDefenseManager::create()->setArbitrator(arbitrator);
	BattleManager::create()->setArbitrator(arbitrator);
	DropManager::create()->setArbitrator(arbitrator);
	MineManager::create()->setArbitrator(arbitrator);
}

void MacroManager::onUnitDiscover(BWAPI::Unit* unit)
{

}

void MacroManager::onUnitDestroy(BWAPI::Unit* unit)
{
	if (!unit)
	{
		return;
	}

	workers.erase(unit);

	ArmyManager::create()->onUnitDestroy(unit);
	BaseDefenseManager::create()->onUnitDestroy(unit);
	BattleManager::create()->onUnitDestroy(unit);
	DropManager::create()->onUnitDestroy(unit);
	MineManager::create()->onUnitDestroy(unit);
}

void MacroManager::update()
{

}

void MacroManager::onFrame()
{
	clock_t t = clock();
	ArmyManager::create()->update();
	float time1 = (float)(1000*(clock()-t)/CLOCKS_PER_SEC);

	t = clock();
	BaseDefenseManager::create()->update();
	float time2 = (float)(1000*(clock()-t)/CLOCKS_PER_SEC);

	t = clock();
	BattleManager::create()->update();
	float time3 = (float)(1000*(clock()-t)/CLOCKS_PER_SEC);

	t = clock();
	DropManager::create()->update();
	float time4 = (float)(1000*(clock()-t)/CLOCKS_PER_SEC);

	t = clock();
	MineManager::create()->update();
	float time5 = (float)(1000*(clock()-t)/CLOCKS_PER_SEC);

	t = clock();
	scanInvisibleEnemies();
	controlLiftedBuildings();
	setRallyPoint(getSetPoint());
	liftBuildingsNearChokepoints();
	removeBlockingMinerals();
	float time6 = (float)(1000*(clock()-t)/CLOCKS_PER_SEC);
	
	Broodwar->drawTextScreen(433,290,"MacroManager: %.0f %.0f %.0f %.0f %.0f %.0f",time1,time2,time3,time4,time5,time6);

	for each (Unit* u in SelectAll())
	{
		if (u->isSelected() && !arbitrator->getAllBidders(u).empty() && arbitrator->getHighestBidder(u).first)
		{
			Broodwar->drawTextMap(u->getPosition().x(),u->getPosition().y()-10,"%s %d",arbitrator->getHighestBidder(u).first->getName().c_str(),(int)arbitrator->getHighestBidder(u).second);
		}
	}
}

void MacroManager::scanInvisibleEnemies()
{
	if (Broodwar->getFrameCount() - lastScanFrame < 24*5)
	{
		return;
	}

	UnitGroup scanners = SelectAll()(Comsat_Station)(Energy,">=",50);
	
	if (scanners.empty())
	{
		for each (Unit* e in Broodwar->getAllUnits())
		{
			if (e->getPlayer() != Broodwar->enemy() || !e->isCompleted() || e->isDetected())
			{
				continue;
			}

			if (e->getType().canAttack() || e->getType() == UnitTypes::Protoss_Carrier || e->getType() == UnitTypes::Protoss_Reaver)
			{
				if (buildOrder->getPlannedCount(UnitTypes::Terran_Comsat_Station,120) < 2)
				{
					buildOrder->build(2,UnitTypes::Terran_Comsat_Station,120);
				}
				break;
			}
		}
		return;
	}
	
	UnitGroup myArmy = getAttackers() + SelectAll()(Bunker)(LoadedUnitsCount,">=",1);
	Unit* scanner = *(scanners.begin());

	for each (Unit* e in Broodwar->getAllUnits())
	{
		if (e->getPlayer() != Broodwar->enemy() || !e->isCompleted() || e->isDetected())
		{
			continue;
		}

		if (e->getType().canAttack() || e->getType() == UnitTypes::Protoss_Carrier || e->getType() == UnitTypes::Protoss_Reaver)
		{
			for each (Unit* u in myArmy)
			{
				int range = 0;
				if (u->getType() == UnitTypes::Terran_Bunker)
				{
					range = Broodwar->self()->groundWeaponMaxRange(UnitTypes::Terran_Marine);
				}
				else
				{
					int groundRange = Broodwar->self()->groundWeaponMaxRange(u->getType());
					int airRange    = Broodwar->self()->airWeaponMaxRange(u->getType());
					range = groundRange > airRange ? groundRange : airRange;
				}
				if (u->getPosition().getApproxDistance(e->getPosition()) < range + 32)
				{
					scanner->useTech(TechTypes::Scanner_Sweep,e->getPosition());
					lastScanFrame = Broodwar->getFrameCount();
					break;
				}
			}
		}
	}
}

void MacroManager::controlLiftedBuildings()
{
	if (Broodwar->enemy()->getRace() != Races::Terran)
	{
		return;
	}

	if (Broodwar->getFrameCount()%8 != 4)
	{
		return;
	}

	if (!mental->marineRushOver || (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Marine) < 4 && Broodwar->getFrameCount() < 24*60*5))
	{
		return;
	}

	UnitGroup liftedBuildings = SelectAll()(isCompleted)(Barracks,Engineering_Bay)(isLifted);

	for each (Unit* u in liftedBuildings)
	{
		Vector2 v = Vector2(0,0);
		for each (EnemyUnit* e in eInfo->getAllEnemyUnits())
		{
			if (e->getType() != UnitTypes::Terran_Bunker && e->getType().airWeapon() == WeaponTypes::None)
			{
				continue;
			}
			int range = e->getType() == UnitTypes::Terran_Bunker ? UnitTypes::Terran_Marine.seekRange() : e->getType().seekRange();
			if (e->getPosition().getApproxDistance(u->getPosition()) <= range + 32*5)
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

    // Change to eInfo
		//UnitGroup enemy = SelectAllEnemy()(isCompleted)(canAttack,Bunker).not(isFlyer,isWorker).inRadius(u->getType().sightRange() + 32*6, u->getPosition());
    // Just follow tank
    set<EnemyUnit*> enemy;
    Position enTankPos;
    int td = 99999;
    for each (EnemyUnit* eU in eInfo->getAllEnemyUnits())
    {
      UnitType ut = eU->getType();
      Position up = eU->getPosition();
      if (ut == UnitTypes::Terran_Siege_Tank_Siege_Mode || ut == UnitTypes::Terran_Siege_Tank_Tank_Mode) 
      {
        if (td > up.getApproxDistance(terrainManager->mSecondChokepoint->getCenter()))
        {
          td = up.getApproxDistance(terrainManager->mSecondChokepoint->getCenter());
          enTankPos = up;
        }
        enemy.insert(eU);
        //enTankPos += eU->getPosition();
      }
    }

		if (enemy.empty())
		{
			if (terrainManager->eSecondChokepoint && u->getPosition().getApproxDistance(terrainManager->eSecondChokepoint->getCenter()) > 32)
			{
				u->move(terrainManager->eSecondChokepoint->getCenter());
        Position ckp2 = terrainManager->eSecondChokepoint->getCenter();
        Broodwar->drawCircleMap(ckp2.x(), ckp2.y(), 5, Colors::Green, true);
        Broodwar->drawLineMap(u->getPosition().x(), u->getPosition().y(), ckp2.x(), ckp2.y(), Colors::Green);
			}							
		}
		// follow the enemy center
		else
		{
			//UnitGroup eTank = enemy(Siege_Tank);
			//Position pos = eTank.empty() ? enemy.getCenter() : eTank.getCenter();
			//u->move(pos);
      u->move(Position(enTankPos.x(), enTankPos.y()));
      Broodwar->drawCircleMap(enTankPos.x(), enTankPos.y(), 5, Colors::Green, true);
      Broodwar->drawLineMap(u->getPosition().x(), u->getPosition().y(), enTankPos.x(), enTankPos.y(), Colors::Green);
		}
	}

	if (Broodwar->getFrameCount() > 24*60*5.5)
	{
		SelectAll()(isCompleted)(Barracks,Engineering_Bay).not(isLifted).lift();
	}
}

void MacroManager::setRallyPoint(Position p)
{
	if (Broodwar->getFrameCount()%(24*10) != 10)
	{
		return;
	}

	if (p == Positions::None || p == Positions::Invalid || p == Positions::Unknown)
	{
		return;
	}

	SelectAll()(isCompleted)(Barracks,Factory,Starport).setRallyPoint(p);
}

void MacroManager::liftBuildingsNearChokepoints()
{
	if (Broodwar->getFrameCount()%(24*30) != 0)
	{
		return;
	}

	if (mental->goAttack || (Broodwar->getFrameCount() >= 24*60*7.5 && mental->enemyInSight.not(isWorker).size() < 2))
	{
		for each (Unit* u in Broodwar->self()->getUnits())
		{
			if (!u->getType().isFlyingBuilding() || u->getType().isResourceDepot() || !u->isCompleted() || u->isTraining() || u->isLifted() || u->isConstructing())
			{
				continue;
			}

			if (u->getPosition().getApproxDistance(terrainManager->mFirstChokepoint->getCenter())  <= 32*3
				  ||
				  u->getPosition().getApproxDistance(terrainManager->mSecondChokepoint->getCenter()) <= 32*3
					||
					u->getPosition().getApproxDistance(getSetPoint()) <= 32*3
					||
					(terrainManager->bbPos != TilePositions::None && u->getPosition().getApproxDistance(Position(terrainManager->bbPos)) <= 32*3))
			{
				u->lift();
			}
		}
	}
}

void MacroManager::removeBlockingMinerals()
{
	if (mineralPosition == TilePositions::None || mineralRemoved)
	{
		return;
	}

	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) < 2 || Broodwar->self()->completedUnitCount(UnitTypes::Terran_SCV) < 30)
	{
		return;
	}
	
	if (Broodwar->getFrameCount()%24 != 22)
	{
		return;
	}

	if (workers.empty())
	{
		for each (Unit* u in Broodwar->self()->getUnits())
		{
			if (!u->isCompleted() || !u->getType().isWorker() || u->isCarryingMinerals() || u->isCarryingGas() || u->isConstructing() || u->isRepairing() || u->isLoaded())
			{
				continue;
			}

			if (u->getTilePosition().getDistance(Broodwar->self()->getStartLocation()) < 50)
			{
				arbitrator->setBid(this, u, 300);
				break;
			}
		}
		return;
	}

	for each (Unit* u in workers)
	{
		if (u->getTilePosition().getDistance(mineralPosition) > 4 || !Broodwar->isVisible(mineralPosition))
		{
			u->move(Position(mineralPosition));
			continue;
		}

		set<Unit*> minerals = Broodwar->getUnitsOnTile(mineralPosition.x(),mineralPosition.y());
		for (set<Unit*>::iterator i = minerals.begin(); i != minerals.end();)
		{
			if (!(*i)->getType().isMineralField())
			{
				minerals.erase(i++);
			}
			else
			{
				++i;
			}
		}
		if (minerals.empty())
		{
			workers.clear();
			arbitrator->removeAllBids(this);
			mineralRemoved = true;
			return;
		}

		Unit* mineral = *(minerals.begin());
		if (!u->isGatheringMinerals())
		{
			u->gather(mineral);
		}
	}
}

UnitGroup MacroManager::getAttackers()
{
	return ArmyManager::create()->getAttackers();
}

AttackTarget* MacroManager::getAttackTarget() const
{
	return ArmyManager::create()->getAttackTarget();
}

Position MacroManager::getSetPoint() const
{
	return ArmyManager::create()->getSetPoint();
}

Position MacroManager::getSiegePoint() const
{
	return ArmyManager::create()->getSiegePoint();
}

Position MacroManager::getGatherPoint() const
{
	return ArmyManager::create()->getGatherPoint();
}