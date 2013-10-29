#include "DropManager.h"

using namespace std;
using namespace BWAPI;
using namespace BWTA;
using namespace ICEStarCraft;

/************************************************************************/
/* Dropper                                                              */
/************************************************************************/

Dropper::Dropper(Unit* unit): dropship(unit)
{
	debug = true;

	unitsToLoad.clear();
	purpose = DropNone;
	state = Idle;
	dropPos = Positions::None;
	target = NULL;
	current = 0;
	path.clear();
}

string Dropper::getStateString()
{
	switch(state)
	{
	case Idle:
		return "Idle";
	case Waiting:
		return "Waiting";
	case Loading:
		return "Loading";
	case Moving:
		return "Moving";
	case Dropping:
		return "Dropping";
	case Returning:
		return "Returning";
	default:
		return "";
	}
}

void Dropper::update()
{
	for (set<Unit*>::iterator i = unitsToLoad.begin(); i != unitsToLoad.end();)
	{
		Unit* u = *i;
		if (!u || !u->exists())
		{
			unitsToLoad.erase(i++);
		}
		else
		{
			++i;
		}
	}

	switch(state)
	{
	case Idle:
		{
			purpose = DropNone;
			target = NULL;
			path.clear();
			current = 0;
			dropPos = TerrainManager::create()->mSecondChokepoint->getCenter();
			unitsToLoad.clear();
			if (!dropship->getLoadedUnits().empty())
			{
				if (dropship->getLastCommand().getType() != UnitCommandTypes::Unload_All_Position)
				{
					dropship->unloadAll(dropPos);
				}
			}
			else
			{
				state = Waiting;
				if (dropship->getTilePosition().getDistance(Broodwar->self()->getStartLocation()) > 2)
				{
					dropship->move(Position(Broodwar->self()->getStartLocation()));
				}
			}
		}
		break;
	case Waiting:
		{
			if (target == NULL || !EnemyInfoManager::create()->isEnemyBase(target))
			{
				state = Idle;
				if (dropship->getTilePosition().getDistance(Broodwar->self()->getStartLocation()) > 2)
				{
					dropship->move(Position(Broodwar->self()->getStartLocation()));
				}
			}
			else if (purpose == Harrass && unitsToLoad.size() == 4)
			{
				state = Loading;
			}
			else if (purpose == HighGround && unitsToLoad.size() == 2)
			{
				state = Loading;
			}
		}
		break;
	case Loading:
		{
			if (target == NULL || !EnemyInfoManager::create()->isEnemyBase(target))
			{
				state = Idle;
				break;
			}

			if (dropship->getLoadedUnits().size() < unitsToLoad.size())
			{
				for each (Unit* u in unitsToLoad)
				{
					if (u->isSieged())
					{
						u->unsiege();
						dropship->load(u,true);
					}
					else if (!u->isLoaded())
					{
						dropship->load(u,true);
						u->rightClick(dropship);
					}
				}
			}
			else
			{
				state = Moving;
				current = 0;
			}
		}
		break;
	case Moving:
		{
			if (target == NULL || !EnemyInfoManager::create()->isEnemyBase(target))
			{
				state = Returning;
				break;
			}

			if (dropship->getLoadedUnits().empty())
			{
				state = Returning;
				break;
			}

			//emergency
			if (dropship->isUnderAttack() && dropship->getHitPoints() < 60)
			{
				if (BWTA::getRegion(dropship->getPosition()) == target->getRegion() || dropship->getHitPoints() < 40)
				{
					if (debug) Broodwar->printf("Emergency! Unload all!");
					if (dropship->getLastCommand().getType() != UnitCommandTypes::Unload_All
						  ||
							(dropship->getLastCommand().getType() == UnitCommandTypes::Unload_All && Broodwar->getFrameCount() - dropship->getLastCommandFrame() > 24*3))
					{
						dropship->unloadAll();
					}
				}
				else
				{
					state = Returning;
				}
				break;
			}
			
			if (path.empty())
			{
				state = Returning;
				if (debug) Broodwar->printf("path empty");
				break;
			}

			// draw path
			if (debug)
			{
				for (int i = 0; i < path.size(); i++)
				{
					if (i < path.size() - 1)
					{
						Broodwar->drawLineMap(path[i].x(),path[i].y(),path[i+1].x(),path[i+1].y(),Colors::Green);
					}
					Broodwar->drawCircleMap(path[i].x(),path[i].y(),5,Colors::Green,true);
				}
			}
			
			if ((purpose == HighGround && current == path.size() - 1)
					||
				  (purpose == Harrass && dropship->getPosition().getApproxDistance(target->getPosition()) <= dropship->getType().sightRange() + 32))
			{
				state = Dropping;
				if (purpose == Harrass)
				{
					dropPos = Positions::None;
				}
			}
			else if (dropship->getPosition().getApproxDistance(path[current]) > 32 * 2)
			{
				dropship->move(path[current]);
			}
			else
			{
				current++;
			}
		}
		break;
	case Dropping:
		{
			if (target == NULL || !EnemyInfoManager::create()->isEnemyBase(target))
			{
				state = Returning;
				break;
			}

			// find position to drop
			if (dropPos == Positions::None)
			{
				if (purpose == Harrass)
				{
					Vector2 v = Vector2(dropship->getPosition()) - Vector2(path[path.size()-1]);
					v = v * (32 * 4.0 / v.approxLen());
					dropPos = v + path[path.size()-1];
					dropPos = dropPos.makeValid();
				}
				else if (purpose == HighGround)
				{
					dropPos = path[path.size()-1];
				}
			}

			if (debug) Broodwar->drawCircleMap(dropPos.x(),dropPos.y(),10,Colors::Green);
			if (debug) Broodwar->drawCircleMap(dropPos.x(),dropPos.y(),15,Colors::Green);
				
			if (!dropship->getLoadedUnits().empty())
			{
				if (dropship->getOrder() != Orders::Unload
					  ||
					  dropship->getLastCommand().getType() != UnitCommandTypes::Unload_All_Position
					  ||
					  (dropship->getLastCommand().getType() == UnitCommandTypes::Unload_All_Position && Broodwar->getFrameCount() - dropship->getLastCommandFrame() > 24*3))
				{
					dropship->unloadAll(dropPos);
				}
			}
			else if (purpose == Harrass)
			{
				state = Returning;
			}
			else if (purpose == HighGround)
			{
				if (unitsToLoad.empty())
				{
					state = Returning;
					break;
				}

				// control drop ship
				bool inDanger = false;
				if (dropship->isUnderAttack())
				{
					inDanger = true;
				}
				else
				{
					for each (Unit* e in SelectAllEnemy()(canAttack).inRadius(32*6,dropship->getPosition()))
					{
						if (e->getOrderTarget() == dropship || e->getTarget() == dropship)
						{
							inDanger = true;
							break;
						}
					}
				}
				
				if (inDanger)
				{
					dropship->move(dropPos);
				}
				else
				{
					Position p = target->getPosition() + dropPos;
					p = Position(p.x()/2,p.y()/2);
					dropship->move(p);
				}
			}
		}
		break;
	case Returning:
		{
			dropPos = TerrainManager::create()->mSecondChokepoint->getCenter();
			target = NULL;
			unitsToLoad.clear();

			if (path.empty()) // switched from Moving and not yet had a path
			{
				if (dropship->getPosition().getApproxDistance(dropPos) > 32 * 3)
				{
					dropship->move(dropPos);
				}
				else
				{
					state = Idle;
				}
			}
			else
			{
				if (current <= 0)
				{
					state = Idle;
				}
				else if (dropship->getPosition().getApproxDistance(path[current-1]) > 32 * 2)
				{
					dropship->move(path[current-1]);
				}
				else
				{
					current--;
				}
			}
		}
		break;
	}
}

/************************************************************************/
/* DropManager                                                          */
/************************************************************************/

DropManager* theDropManager = NULL;

DropManager::DropManager()
{
	debug = true;

	terrainManager = TerrainManager::create();
	mental         = MentalClass::create();
	scoutManager   = ScoutManager::create();
	eInfo          = EnemyInfoManager::create();
	mInfo          = MyInfoManager::create();

	requestedUnits.clear();
	allUnitsToLoad.clear();
	unitsToControl.clear();
	droppers.clear();
	dropperNumMax = 0;
	dropTargets.clear();
}

DropManager::~DropManager()
{

}

DropManager* DropManager::create()
{
	if (theDropManager) return theDropManager;
	else return theDropManager = new DropManager();
}

void DropManager::destroy()
{
	if (theDropManager)
	{
		delete theDropManager;
		theDropManager = NULL;
	}
}

void DropManager::onOffer(std::set<BWAPI::Unit*> units)
{
	for each (Unit* u in units)
	{
		if (unitsToControl.find(u) != unitsToControl.end())
		{
			arbitrator->accept(this, u);
			continue;
		}

		map<Unit*, Dropper*>::iterator i = requestedUnits.find(u);
		if (i == requestedUnits.end())
		{
			arbitrator->decline(this, u, 0);
			continue;
		}

		Dropper* d = i->second;
		if (!d || !d->dropship || !d->dropship->exists())
		{
			arbitrator->decline(this, u, 0);
			continue;
		}
		
		int need = d->purpose == HighGround ? 2 : 4;
		if (d->unitsToLoad.size() >= need || d->state != Waiting || d->unitsToLoad.find(u) != d->unitsToLoad.end())
		{
			arbitrator->decline(this, u, 0);
			continue;
		}

		arbitrator->accept(this, u);
		d->unitsToLoad.insert(u);
		allUnitsToLoad.insert(u);
		if (u->isSieged())
		{
			u->unsiege();
		}
		else
		{
			u->rightClick(d->dropship);
		}
	}
}

void DropManager::onRevoke(BWAPI::Unit* unit, double bid)
{
	requestedUnits.erase(unit);
	allUnitsToLoad.erase(unit);

	for each (Dropper* d in droppers)
	{
		if (d->unitsToLoad.find(unit) != d->unitsToLoad.end())
		{
			d->unitsToLoad.erase(unit);
		}
	}
}

void DropManager::onUnitDestroy(Unit* unit)
{
	if (!unit)
	{
		return;
	}
	
	allUnitsToLoad.erase(unit);

	for each (Dropper* d in droppers)
	{
		if (d->dropship == unit)
		{
			droppers.erase(d);
			break;
		}
		
		if (d->unitsToLoad.find(unit) != d->unitsToLoad.end())
		{
			d->unitsToLoad.erase(unit);
		}
	}
}

void DropManager::update()
{
	if (Broodwar->getFrameCount()%8 != 6)
	{
		return;
	}

	int minPopulation = Broodwar->enemy()->getRace() == Races::Protoss ? 120 : 100;
	/*if (mInfo->myFightingValue().first > eInfo->enemyFightingValue().first * 4)
	{
		minPopulation -= 20;
	}
	else if (mInfo->myFightingValue().first > eInfo->enemyFightingValue().first)
	{
		minPopulation -= 10;
	}*/

	if (Broodwar->self()->supplyUsed()/2 > minPopulation)
	{
		dropperNumMax = Broodwar->self()->supplyUsed()/2 > minPopulation + 20 ? 2 : 1;
	}
	else
	{
		dropperNumMax = 0;
	}

	// build drop ship
	if (dropperNumMax > 0 &&
		  Broodwar->self()->deadUnitCount(UnitTypes::Terran_Dropship) < 3 &&
			mental->enemyInSight.size() < 5)
	{
		if (mInfo->countUnitNum(UnitTypes::Terran_Starport,2) < 1)
		{
			buildOrderManager->build(1,UnitTypes::Terran_Starport,100);
			buildOrderManager->build(1,UnitTypes::Terran_Control_Tower,98);
		}
		else if (mInfo->countUnitNum(UnitTypes::Terran_Starport,1) >= 1)
		{
			if (mInfo->countUnitNum(UnitTypes::Terran_Dropship,2) < dropperNumMax)
			{
				if (buildOrderManager->getPlannedCount(UnitTypes::Terran_Valkyrie,85) > 0 && Broodwar->self()->allUnitCount(UnitTypes::Terran_Valkyrie) < 4)
				{
					buildOrderManager->build(dropperNumMax,UnitTypes::Terran_Dropship,84);
				}
				else
				{
					buildOrderManager->build(dropperNumMax,UnitTypes::Terran_Dropship,105);
				}
			}
		}
	}

	// control units that have been dropped
	controlDroppedUnits();

	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Dropship) < 1)
	{
		return;
	}

	// add dropship to dropper group
	for each (Unit* u in SelectAll(UnitTypes::Terran_Dropship)(isCompleted))
	{
		bool isDropper = false;
		for each (Dropper* d in droppers)
		{
			if (d->dropship == u)
			{
				isDropper = true;
				break;
			}
		}
		if (!isDropper)
		{
			droppers.insert(new Dropper(u));
		}
	}

	for each (Dropper* d in droppers)
	{
		d->update();
	}

	// update drop targets
	for (std::set<DropTarget*>::iterator i = dropTargets.begin(); i != dropTargets.end();)
	{
		DropTarget* target = *i;
		if (!eInfo->isEnemyBase(target->base)
				||
			  !allUnitsToLoad.not(isLoaded).inRadius(32*10,target->base->getPosition()).empty()
				||
				eInfo->countUnitNum(Broodwar->enemy()->getRace().getWorker(),target->base->getPosition()) < 2)
		{
			dropTargets.erase(i++);
		}
		else
		{
			bool dropperDestroyed = false;
			for each (Dropper* d in droppers)
			{
				if (d->target == target->base && (!d->dropship || !d->dropship->exists()))
				{
					dropperDestroyed = true;
					break;
				}
			}

			if (dropperDestroyed)
			{
				dropTargets.erase(i++);
			}
			else
			{
			  ++i;
			}
		}
	}

	// first enemy expansion that our tank can attack from high ground
	for (map<Unit*,EnemyInfoManager::eBaseData>::iterator i = eInfo->enemyBaseMap.begin(); i != eInfo->enemyBaseMap.end(); i++)
	{
		if (dropTargets.size() >= dropperNumMax)
		{
			break;
		}

		BWTA::BaseLocation* base = i->second.base;
		if (base == NULL)
		{
			continue;
		}

		if (Broodwar->enemy()->getRace() != Races::Terran &&
				terrainManager->getTankDropPosition(base) != TilePositions::None &&
				SelectAll()(Siege_Tank).size() > 2 &&
				!isDropTarget(base) &&
				eInfo->countUnitNum(Broodwar->enemy()->getRace().getWorker(),base->getPosition()) >= 3)
		{
			int dangers = 0;
			if (Broodwar->enemy()->getRace() == Races::Protoss)
			{
				dangers += eInfo->countUnitNum(UnitTypes::Protoss_Scout,base->getPosition());
				dangers += eInfo->countUnitNum(UnitTypes::Protoss_Carrier,base->getPosition());
				dangers += eInfo->countUnitNum(UnitTypes::Protoss_Arbiter,base->getPosition());
			}
			if (Broodwar->enemy()->getRace() == Races::Zerg)
			{
				dangers += eInfo->countUnitNum(UnitTypes::Zerg_Mutalisk,base->getPosition());
				dangers += eInfo->countUnitNum(UnitTypes::Zerg_Guardian,base->getPosition());
			}
			if (dangers > 2)
			{
				continue;
			}
			
			Position start = terrainManager->mSecondChokepoint->getCenter();
			Position end = Position(terrainManager->getTankDropPosition(base));
			vector<Position> path = getBestFlightPath(start,end,EnemyInfoManager::create()->getAllEnemyUnits());
			if (path.empty())
			{
				if (debug) Broodwar->printf("Dangerous path. Skip drop on high ground (%d,%d)",base->getTilePosition().x(),base->getTilePosition().y());
				continue;
			}
			dropTargets.insert(new DropTarget(base,Position(terrainManager->getTankDropPosition(base)),HighGround,path));
		}
	}

	// then add enemy start location to drop targets
	if (dropTargets.size() < dropperNumMax &&
		  scoutManager->enemyStartLocation &&
			eInfo->isEnemyBase(scoutManager->enemyStartLocation) &&
			!isDropTarget(scoutManager->enemyStartLocation) &&
			eInfo->countUnitNum(Broodwar->enemy()->getRace().getWorker(),scoutManager->enemyStartLocation->getPosition()) >= 5 &&
			eInfo->countDangerTotal(scoutManager->enemyStartLocation->getPosition()) < 4)
	{
		Position start = terrainManager->mSecondChokepoint->getCenter();
		Position end = scoutManager->enemyStartLocation->getPosition();
		vector<Position> path = getBestFlightPath(start,end,EnemyInfoManager::create()->getAllEnemyUnits());
		if (path.empty())
		{
			if (debug) Broodwar->printf("Dangerous path. Skip enemy start location");
		}
		else
		{
			dropTargets.insert(new DropTarget(scoutManager->enemyStartLocation,scoutManager->enemyStartLocation->getPosition(),Harrass,path));
		}
	}

	// add other expansions if still possible
	for (map<Unit*,EnemyInfoManager::eBaseData>::iterator i = eInfo->enemyBaseMap.begin(); i != eInfo->enemyBaseMap.end(); i++)
	{
		if (dropTargets.size() < dropperNumMax &&
			  !isDropTarget(i->second.base) &&
				eInfo->countUnitNum(Broodwar->enemy()->getRace().getWorker(),i->second.base->getPosition()) >= 4 &&
				eInfo->countDangerTotal(i->second.base->getPosition()) < 4)
		{
			Position start = terrainManager->mSecondChokepoint->getCenter();
			Position end = i->second.base->getPosition();
			vector<Position> path = getBestFlightPath(start,end,EnemyInfoManager::create()->getAllEnemyUnits());
			if (path.empty())
			{
				if (debug) Broodwar->printf("Dangerous path. Skip expansion (%d,%d)",i->second.base->getTilePosition().x(),i->second.base->getTilePosition().y());
				continue;
			}
			dropTargets.insert(new DropTarget(i->second.base,i->second.base->getPosition(),Harrass,path));
		}
	}

	if (dropTargets.empty())
	{
		return;
	}

	// assign task and units to dropper
	for each (DropTarget* target in dropTargets)
	{
		// select a dropship
		Dropper* d = NULL;
		// first find the dropship we selected for this target before
		for each (Dropper* i in droppers)
		{
			if (i->target == target->base)
			{
				d = i;
				break;
			}
		}

		// select new dropship
		if (d == NULL)
		{
			for each (Dropper* i in droppers)
			{
				if (i->target == NULL && i->state == Waiting)
				{
					d = i;
					break;
				}
			}

			if (d == NULL)
			{
				continue;
			}

			d->target  = target->base;
			d->purpose = target->purpose;
			d->dropPos = target->dropPos;
			d->path    = target->path;
		}

		selectUnitsForDropper(d);
	}
}

bool DropManager::isDropTarget(BWTA::BaseLocation* base)
{
	if (base == NULL)
	{
		return false;
	}

	for each (DropTarget* target in dropTargets)
	{
		if (target->base == base)
		{
			return true;
		}
	}

	if (!allUnitsToLoad.not(isLoaded).inRadius(32*10,base->getPosition()).empty())
	{
		return true;
	}

	return false;
}

void DropManager::selectUnitsForDropper(Dropper* d)
{
	if (d->purpose == HighGround)
	{
		// select tanks
		if (d->unitsToLoad.size() == 2)
		{
			return;
		}

		UnitGroup tanks;
		for each (Unit* u in Broodwar->self()->getUnits())
		{
			if (u->isCompleted() &&
				  (u->getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode || u->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode) &&
				  !u->isAttacking() &&
					u->getLastCommand().getType() != UnitCommandTypes::Attack_Unit &&
				  allUnitsToLoad.find(u) == allUnitsToLoad.end() &&
				  u->getHitPoints() > 0.8 * u->getType().maxHitPoints())
			{
				tanks.insert(u);
			}
		}

		while (!tanks.empty() && d->unitsToLoad.size() < 2)
		{
			Unit* t = tanks.getNearest(d->dropship->getPosition());
			if (t != NULL)
			{
				tanks.erase(t);
				requestedUnits.insert(make_pair(t,d));
				arbitrator->setBid(this, t, 100);
			}
		}
	}

	if (d->purpose == Harrass)
	{
		// select vultures, goliaths

		if (d->unitsToLoad.size() == 4)
		{
			return;
		}

		// select vultures first
		UnitGroup vultures;
		for each (Unit* u in Broodwar->self()->getUnits())
		{
			if (u->isCompleted() &&
				  u->getType() == UnitTypes::Terran_Vulture &&
				  !u->isAttacking() &&
				  u->getLastCommand().getType() != UnitCommandTypes::Attack_Unit &&
					allUnitsToLoad.find(u) == allUnitsToLoad.end() &&
				  u->getHitPoints() > 0.8 * u->getType().maxHitPoints())
			{
				vultures.insert(u);
			}
		}

		while (!vultures.empty() && d->unitsToLoad.size() < 4)
		{
			Unit* v = vultures.getNearest(d->dropship->getPosition());
			if (v != NULL)
			{
				vultures.erase(v);
				requestedUnits.insert(make_pair(v,d));
				arbitrator->setBid(this, v, 100);
			}
		}

		if (d->unitsToLoad.size() == 4)
		{
			return;
		}

		// then select goliaths
		UnitGroup goliaths;
		for each (Unit* u in Broodwar->self()->getUnits())
		{
			if (u->isCompleted() &&
				  u->getType() == UnitTypes::Terran_Goliath &&
				  !u->isAttacking() &&
				  u->getLastCommand().getType() != UnitCommandTypes::Attack_Unit &&
				  allUnitsToLoad.find(u) == allUnitsToLoad.end() &&
				  u->getHitPoints() > 0.8 * u->getType().maxHitPoints())
			{
				goliaths.insert(u);
			}
		}

		while (!goliaths.empty() && d->unitsToLoad.size() < 4)
		{
			Unit* g = goliaths.getNearest(d->dropship->getPosition());
			if (g != NULL)
			{
				goliaths.erase(g);
				requestedUnits.insert(make_pair(g,d));
				arbitrator->setBid(this, g, 100);
			}
		}
	}
}

void DropManager::controlDroppedUnits()
{
	unitsToControl = allUnitsToLoad.not(isLoaded);

	for each (Dropper* d in droppers)
	{
		if (d->dropship->exists() && d->state != Dropping)
		{
			for each (Unit* u in d->unitsToLoad)
			{
				unitsToControl.erase(u);
			}
		}
	}

	UnitGroup unitsToRemove;

	for each (Unit* u in unitsToControl)
	{
		arbitrator->setBid(this, u, 900);

		if (debug) Broodwar->drawCircleMap(u->getPosition().x(),u->getPosition().y(),5,Colors::Purple,true);

		if (u->getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode ||
			  u->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode ||
			  u->getType() == UnitTypes::Terran_Vulture ||
			  u->getType() == UnitTypes::Terran_Goliath)
		{
			if (u->getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode)
			{
				u->siege();
				continue;
			}
			
			// move away from enemy
			if ((u->getType() == UnitTypes::Terran_Vulture || u->getType() == UnitTypes::Terran_Goliath) &&
				   u->getGroundWeaponCooldown() > 0 && u->getAirWeaponCooldown() > 0)
			{
				Vector2 v = Vector2(0,0);
				for each (Unit* e in Broodwar->enemy()->getUnits())
				{
					if (!e->isCompleted() || e->getPosition().getApproxDistance(u->getPosition()) > u->getType().sightRange() + 32)
					{
						continue;
					}

					if (e->getType().groundWeapon() == WeaponTypes::None &&
						  e->getType() != UnitTypes::Protoss_Reaver &&
							e->getType() != UnitTypes::Terran_Bunker)
					{
						continue;
					}
				
					if (e->getOrderTarget() == u || e->getTarget() == u)
					{
						v += PFFunctions::getVelocitySource(e->getPosition(),u->getPosition()) * 1000;
					}
				}
				
				if (v != Vector2(0,0))
				{
					v = v * (128.0 / v.approxLen());
					Position des = v + u->getPosition();
					des = des.makeValid();

					MicroUnitControl::move(u,des);
					continue;
				}
			}

			Unit* target = NULL;
			UnitGroup workers = SelectAllEnemy()(isDetected)(isWorker).inRadius(u->getType().seekRange(),u->getPosition());
			UnitGroup enemies = SelectAllEnemy()(isDetected).not(Vulture_Spider_Mine,Scarab,Interceptor,Nuclear_Missile).inRadius(u->getType().sightRange(),u->getPosition());
			if (u->getType().airWeapon() == WeaponTypes::None)
			{
				enemies = enemies.not(isFlyer);
			}
			UnitGroup dangers = enemies(canAttack,Reaver,High_Templar,Defiler,Bunker);

			if (!dangers.empty() && u->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode)
			{
				target = dangers.getNearest(u->getPosition());
			}
			else if (!workers.empty())
			{
				target = workers.getNearest(u->getPosition());
			}
			else if (!dangers.empty())
			{
				target = dangers.getNearest(u->getPosition());
			}
			else if (!enemies.empty())
			{
				target = enemies.getNearest(u->getPosition());
			}

			if (target)
			{
				if (debug) Broodwar->drawLineMap(u->getPosition().x(),u->getPosition().y(),target->getPosition().x(),target->getPosition().y(),Colors::Red);
				if (!(u->getLastCommand().getType() == UnitCommandTypes::Attack_Unit && u->getLastCommand().getTarget() == target))
				{
					u->attack(target);
				}
			}
			else
			{
				unitsToRemove.insert(u);
			}
		}
	}

	// mission completed! return units
	for each (Unit* u in unitsToRemove)
	{
		unitsToControl.erase(u);
		allUnitsToLoad.erase(u);
		arbitrator->removeBid(this, u);
	}
}

vector<Position> DropManager::getBestFlightPath(Position start, Position end, set<EnemyUnit*> allEnemyUnits)
{
	// best = safest && shortest
	// safest = the damage to the dropship is lowest

	int mapWidth  = Broodwar->mapWidth();
	int mapHeight = Broodwar->mapHeight() - 1;

	vector<Position> path;

	Position TL = Position(0,0);                          // top left
	Position TR = Position(32 * mapWidth,0);              // top right
	Position BL = Position(0,32 * mapHeight);             // bottom left
	Position BR = Position(32 * mapWidth,32 * mapHeight); // bottom right

	Position startX  = Position(start.x(),0);
	Position startXX = Position(start.x(),32 * mapHeight);
	Position startY  = Position(0,start.y());
	Position startYY = Position(32 * mapWidth,start.y());

	Position endX  = Position(end.x(),0);
	Position endXX = Position(end.x(),32 * mapHeight);
	Position endY  = Position(0,end.y());
	Position endYY = Position(32 * mapWidth,end.y());

	Graph g = Graph();

	// set vertices
	g.addVertex(TL);
	g.addVertex(TR);
	g.addVertex(BL);
	g.addVertex(BR);
	g.addVertex(start);
	g.addVertex(startX);
	g.addVertex(startXX);
	g.addVertex(startY);
	g.addVertex(startYY);
	g.addVertex(end);
	g.addVertex(endX);
	g.addVertex(endXX);
	g.addVertex(endY);
	g.addVertex(endYY);

	// set edges
	g.addEdge(start,TL);
	g.addEdge(start,TR);
	g.addEdge(start,BL);
	g.addEdge(start,BR);
	g.addEdge(start,startX);
	g.addEdge(start,startXX);
	g.addEdge(start,startY);
	g.addEdge(start,startYY);
	g.addEdge(end,TL);
	g.addEdge(end,TR);
	g.addEdge(end,BL);
	g.addEdge(end,BR);
	g.addEdge(end,endX);
	g.addEdge(end,endXX);
	g.addEdge(end,endY);
	g.addEdge(end,endYY);

	if (startX.x() <= endX.x())
	{
		g.addEdge(TL,startX);
		g.addEdge(startX,endX);
		g.addEdge(endX,TR);
		g.addEdge(BL,startXX);
		g.addEdge(startXX,endXX);
		g.addEdge(endXX,BR);
	}
	else
	{
		g.addEdge(TL,endX);
		g.addEdge(endX,startX);
		g.addEdge(startX,TR);
		g.addEdge(BL,endXX);
		g.addEdge(endXX,startXX);
		g.addEdge(startXX,BR);
	}

	if (startY.y() <= endY.y())
	{
		g.addEdge(TL,startY);
		g.addEdge(startY,endY);
		g.addEdge(endY,BL);
		g.addEdge(TR,startYY);
		g.addEdge(startYY,endYY);
		g.addEdge(endYY,BR);
	}
	else
	{
		g.addEdge(TL,endY);
		g.addEdge(endY,startY);
		g.addEdge(startY,BL);
		g.addEdge(TR,endYY);
		g.addEdge(endYY,startYY);
		g.addEdge(startYY,BR);
	}

	// set weights for edges
	// weight is the damage that the dropship will take while flying along this edge
	for each (Edge* e in g.edges)
	{
		int damage = 0;
		Position A = e->first;
		Position B = e->second;
		int ab = (int)A.getDistance(B);

		for each (EnemyUnit* u in allEnemyUnits)
		{
			if (u->getPosition() == Positions::Unknown)
			{
				continue;
			}

			if (u->getType() == UnitTypes::Protoss_Interceptor)
			{
				continue;
			}

			if (u->getType() != UnitTypes::Protoss_Carrier && u->getType() != UnitTypes::Terran_Bunker && u->getType().airWeapon() == WeaponTypes::None)
			{
				continue;
			}

			WeaponType weapon = u->getType().airWeapon();
			if (u->getType() == UnitTypes::Protoss_Carrier)
			{
				weapon = UnitTypes::Protoss_Interceptor.airWeapon();
			}
			else if (u->getType() == UnitTypes::Terran_Bunker)
			{
				weapon = UnitTypes::Terran_Marine.airWeapon();
			}

			Position I = u->getPosition();
			int R = u->getType() == UnitTypes::Protoss_Carrier ? 32*12 : weapon.maxRange();
			if (u->getType().canMove())
			{
				R += 32*2;
			}

			Vector2 AI = Vector2(I) - Vector2(A);
			Vector2 AB = Vector2(B) - Vector2(A);

			// distance from enemy unit to this edge
			int D = (int)(std::abs(AI ^ AB) / ab);

			if (D > R || (AI * AB) / ab < 0 || (AI * AB) / ab > ab)
			{
				// enemy unit cannot attack this edge
				continue;
			}

			int length = 0;
			if (D < R)
			{
				int ia = (int)I.getDistance(A);
				int ib = (int)I.getDistance(B);

				if (ia <= R && ib <= R)
				{
					length = ab;
				}
				else if (ia <= R && ib > R)
				{
					length = ab - (int)(sqrt(1.0*ib*ib - D*D) - sqrt(1.0*R*R - D*D));
				}
				else if (ia > R && ib <= R)
				{
					length = ab - (int)(sqrt(1.0*ia*ia - D*D) - sqrt(1.0*R*R - D*D));
				}
				else if (ia > R && ib > R)
				{
					length = ab - (int)(sqrt(1.0*ia*ia - D*D) - sqrt(1.0*R*R - D*D)) - (int)(sqrt(1.0*ib*ib - D*D) - sqrt(1.0*R*R - D*D));
				}
			}

			int time = (int)(length / Broodwar->self()->topSpeed(UnitTypes::Terran_Dropship));
			int hits = time / weapon.damageCooldown() + 1;
			int eDamage = weapon.damageAmount() + Broodwar->enemy()->getUpgradeLevel(weapon.upgradeType()) * weapon.damageBonus() * weapon.damageFactor();
			if (u->getType() == UnitTypes::Protoss_Carrier)
			{
				eDamage *= u->getInterceptorCount();
				damage += eDamage * UnitTypes::Protoss_Interceptor.maxAirHits() * hits;
			}
			else if (u->getType() == UnitTypes::Terran_Bunker)
			{
				eDamage *= 4;
				damage += eDamage * UnitTypes::Terran_Marine.maxAirHits() * hits;
			}
			else
			{
				damage += eDamage * u->getType().maxAirHits() * hits;
			}
		}

		e->setWeight(damage);
	}

	if (debug) g.drawGraph(Colors::White);

	// first find all safest paths
	map<Position,set<Position>> previous;
	g.Dijkstra(previous,start,end);

	// then find the shortest path among those safest paths
	Graph g1 = Graph();
	for (map<Position,set<Position>>::iterator i = previous.begin(); i != previous.end(); i++)
	{
		for each (Position p in i->second)
		{
			g1.addEdge(i->first,p,(int)i->first.getDistance(p));
		}
	}

	path = g1.getShortestPath(start,end);
	
	int damage = 0;
	for (int i = 0; i < path.size() - 1; i++)
	{
		damage += g.getWeight(path[i],path[i+1]);
	}
	if (damage >= UnitTypes::Terran_Dropship.maxHitPoints())
	{
		path.clear();
	}

	return path;
}

void DropManager::showDebugInfo()
{
	if (!debug) return;

	Broodwar->drawTextScreen(433,270,"DropTargets: %d",dropTargets.size());
	Broodwar->drawTextScreen(433,280,"Droppers: %d",droppers.size());
	for each (DropTarget* target in dropTargets)
	{
		Position p = target->base->getPosition();
		Broodwar->drawCircleMap(p.x(),p.y(),40,Colors::Red,true);
	}

	for each (Unit* u in allUnitsToLoad)
	{
		Position p = u->getPosition();
		Broodwar->drawCircleMap(p.x(),p.y(),15,Colors::Purple);
	}

	for each (Dropper* d in droppers)
	{
		Position p = d->dropship->getPosition();
		Broodwar->drawTextMap(p.x(),p.y(),d->getStateString().c_str());
		Broodwar->drawTextMap(p.x(),p.y()+10,"%d/%d",d->dropship->getLoadedUnits().size(),d->unitsToLoad.size());
		
		if (d->target)
		{
			Position p2 = d->target->getPosition();
			Broodwar->drawLineMap(p.x(),p.y(),p2.x(),p2.y(),Colors::White);
		}

		for each (Unit* u in d->unitsToLoad)
		{
			Broodwar->drawLineMap(p.x(),p.y(),u->getPosition().x(),u->getPosition().y(),Colors::Purple);
		}
	}
}