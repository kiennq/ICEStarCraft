#include <algorithm>
#include <math.h>
#include "MineManager.h"

using namespace std;
using namespace BWAPI;
using namespace BWTA;
using namespace ICEStarCraft;

/************************************************************************/
/* MinePlacer                                                           */
/************************************************************************/

string MinePlacer::getMinePurposeString()
{
	switch (purpose)
	{
	case MineNone:
		{
			return "MineNone";
		}
		break;
	case MineBase:
		{
			return "MineBase";
		}
		break;
	case MineMap:
		{
			return "MineMap";
		}
		break;
	case MinePath:
		{
			return "MinePath";
		}
		break;
	default:
		{
			return "";
		}
	}
}

void MinePlacer::placeMine()
{
	if (!vulture || !vulture->exists())
	{
		return;
	}

	UnitGroup enemies = SelectAllEnemy()(isCompleted).not(isBuilding).inRadius(vulture->getType().sightRange(),vulture->getPosition());
	if (vulture->isUnderAttack()
		  ||
		  !enemies(isFlyer)(maxGroundHits,">",0).empty()
		  ||
			!enemies(Carrier).empty()
			||
			enemies(canAttack,Reaver).not(Vulture_Spider_Mine,Scarab).size() > 2)
	{
		if (debug) Broodwar->printf("Vulture in danger");
		vulture->move(ArmyManager::create()->getSetPoint());
		return;
	}

	Unit* target = enemies(isDetected)(isWorker).getNearest(vulture->getPosition());
	if (target)
	{
		if (debug)
		{
			Broodwar->drawLineMap(vulture->getPosition().x(),vulture->getPosition().y(),target->getPosition().x(),target->getPosition().y(),Colors::Red);
			Broodwar->printf("Vulture attacks %s",target->getType().getName().c_str());
		}

		if (!vulture->isIdle() &&
			  vulture->getLastCommand().getType() == UnitCommandTypes::Attack_Unit &&
			  vulture->getLastCommand().getTarget() == target &&
			  vulture->getLastCommandFrame() > Broodwar->getFrameCount() - 24*3)
		{
			return;
		}
		vulture->attack(target);
		return;
	}

	if (vulture->getSpiderMineCount() < 1)
	{
		baseTarget = NULL;
		tileTarget = TilePositions::None;
		purpose = MineNone;
		return;
	}

	switch (purpose)
	{
	case MineNone:
		{
			if (vulture->getPosition().getApproxDistance(ArmyManager::create()->getSetPoint()) > 32*4)
			{
				vulture->move(ArmyManager::create()->getSetPoint());
			}
		}
		break;
	case MineBase:
		{
			mineBase();
		}
		break;
	case MinePath:
		{
			minePath();
		}
		break;
	case MineMap:
		{
			mineMap();
		}
		break;
	default:
		{

		}
	}
}

void MinePlacer::mineBase()
{
	if (Broodwar->getFrameCount()%8 != 1)
	{
		return;
	}

	map<BWTA::BaseLocation*,Unit*> baseTargets = MineManager::create()->getBaseTargets();
	
	if (baseTargets.empty())
	{
		purpose = MineNone;
		baseTarget = NULL;
		tileTarget = TilePositions::None;
		return;
	}

	if (baseTarget == NULL)
	{
		// find a base target for this vulture
		double minD = -1;
		BWTA::BaseLocation* target = NULL;

		for (map<BWTA::BaseLocation*,Unit*>::iterator i = baseTargets.begin(); i != baseTargets.end(); i++)
		{
			if (!i->first || (i->second && i->second->exists()))
			{
				continue;
			}

			if (minD == -1 || vulture->getTilePosition().getDistance(i->first->getTilePosition()) < minD)
			{
				minD = vulture->getTilePosition().getDistance(i->first->getTilePosition());
				target = i->first;
			}
		}

		baseTarget = target;
		
		if (baseTarget == NULL)
		{
			purpose = MineNone;
			return;
		}

		startFrame = Broodwar->getFrameCount();
		MineManager::create()->updateBaseTarget(baseTarget,vulture);
	}

	if (Broodwar->getFrameCount() - startFrame > 24*30)
	{
		if (debug) Broodwar->printf("Skip base (%d,%d)",baseTarget->getTilePosition().x(),baseTarget->getTilePosition().y());
		MineManager::create()->removeBaseTarget(baseTarget);
		baseTarget = NULL;
		return;
	}

	if (!SelectAll()(isBuilding,isWorker,Vulture_Spider_Mine).inRadius(32*4,baseTarget->getPosition()).empty()
		  ||
			!SelectAllEnemy()(isCompleted)(canAttack,Carrier,Reaver,Bunker).not(Vulture_Spider_Mine).inRadius(32*4,baseTarget->getPosition()).empty()
			||
			!SelectAllEnemy()(isResourceDepot).inRadius(32*4,baseTarget->getPosition()).empty())
	{
		MineManager::create()->removeBaseTarget(baseTarget);
		baseTarget = NULL;
		return;
	}

	if (vulture->getPosition().getApproxDistance(baseTarget->getPosition()) > 32*2)
	{
		vulture->move(baseTarget->getPosition());
		return;
	}

	if (!vulture->isIdle() &&
		  vulture->getLastCommand().getType() == UnitCommandTypes::Use_Tech_Position &&
		  vulture->getLastCommand().getTargetPosition() == baseTarget->getPosition() &&
			vulture->getLastCommandFrame() > Broodwar->getFrameCount() - 24*5)
	{
		return;
	}
	
	vulture->useTech(TechTypes::Spider_Mines,baseTarget->getPosition());
	MineManager::create()->removeBaseTarget(baseTarget);
	if (debug) Broodwar->printf("Lay mine at base (%d,%d)",baseTarget->getTilePosition().x(),baseTarget->getTilePosition().y());
}

void MinePlacer::minePath()
{
	if (Broodwar->getFrameCount()%8 != 3)
	{
		return;
	}

	vector<TilePosition> path = MineManager::create()->getMinePath();
	int step = MineManager::create()->getMineStep();

	if (path.empty())
	{
		purpose = MineNone;
		baseTarget = NULL;
		tileTarget = TilePositions::None;
		return;
	}

	for (int i = 0; i < path.size() - 1; i++)
	{
		if (debug) Broodwar->drawLineMap(path[i].x()*32,path[i].y()*32,path[i+1].x()*32,path[i+1].y()*32,Colors::Yellow);
	}

	if (tileTarget == TilePositions::None)
	{
		for (int i = 0; i < path.size(); i += step)
		{
			if (!SelectAll()(Vulture_Spider_Mine).inRadius(32,Position(path[i])).empty())
			{
				continue;
			}

			if (!SelectAll()(isBuilding).inRadius(32,Position(path[i])).empty())
			{
				continue;
			}

			if (!SelectAll().not(canMove).inRadius(32,Position(path[i])).empty())
			{
				continue;
			}

			tileTarget = path[i];
			break;
		}

		if (tileTarget == TilePositions::None)
		{
			MineManager::create()->clearMinePath();
			if (debug) Broodwar->printf("Clear mine path");
			return;
		}

		startFrame = Broodwar->getFrameCount();
	}
	
	if (debug) Broodwar->drawCircleMap(tileTarget.x()*32,tileTarget.y()*32,3,Colors::Yellow);

	if (Broodwar->getFrameCount() - startFrame > 24*30)
	{
		if (debug) Broodwar->printf("Skip tile (%d,%d) on path",tileTarget.x(),tileTarget.y());
		tileTarget = TilePositions::None;
		return;
	}

	if (vulture->getTilePosition().getDistance(tileTarget) > 4)
	{
		vulture->move(Position(tileTarget));
		return;
	}

	if (!vulture->isIdle() &&
		  vulture->getLastCommand().getType() == UnitCommandTypes::Use_Tech_Position &&
		  vulture->getLastCommand().getTargetPosition() == Position(tileTarget) &&
		  vulture->getLastCommandFrame() > Broodwar->getFrameCount() - 24*5)
	{
		return;
	}
	
	vulture->useTech(TechTypes::Spider_Mines,Position(tileTarget));
	if (debug) Broodwar->printf("Lay mine on path (%d,%d)",tileTarget.x(),tileTarget.y());
	tileTarget = TilePositions::None;
}

void MinePlacer::mineMap()
{
	if (Broodwar->getFrameCount()%8 != 5)
	{
		return;
	}

	map<TilePosition,Unit*> tileTargets = MineManager::create()->getTileTargets();
	
	if (tileTargets.empty())
	{
		purpose = MineNone;
		baseTarget = NULL;
		tileTarget = TilePositions::None;
		return;
	}

	if (tileTarget == TilePositions::None)
	{
		// find a tile target for this vulture
		double minD = -1;
		TilePosition target = TilePositions::None;

		for (map<TilePosition,Unit*>::iterator i = tileTargets.begin(); i != tileTargets.end(); i++)
		{
			if (i->first == TilePositions::None || (i->second && i->second->exists()))
			{
				continue;
			}

			double d = 0;
			if (TerrainManager::create()->mSecondChokepoint)
			{
				d = TilePosition(TerrainManager::create()->mSecondChokepoint->getCenter()).getDistance(i->first);
			}
			else
			{
				d = Broodwar->self()->getStartLocation().getDistance(i->first);
			}
			if (minD == -1 || d < minD)
			{
				minD = d;
				target = i->first;
			}
		}

		tileTarget = target;

		if (tileTarget == TilePositions::None)
		{
			purpose = MineNone;
			return;
		}

		startFrame = Broodwar->getFrameCount();
		MineManager::create()->updateTileTarget(tileTarget,vulture);
	}

	if (Broodwar->getFrameCount() - startFrame > 24*30)
	{
		if (debug) Broodwar->printf("Skip tile (%d,%d)",tileTarget.x(),tileTarget.y());
		MineManager::create()->removeTileTarget(tileTarget);
		tileTarget = TilePositions::None;
		return;
	}

	Position pos = Position(tileTarget);
	if (!SelectAll()(Vulture_Spider_Mine).inRadius(32,pos).empty()
		  ||
		  !SelectAll()(isBuilding).inRadius(32,pos).empty()
		  ||
		  !SelectAll().not(canMove).inRadius(32,pos).empty()
		  ||
		  !SelectAllEnemy().inRadius(32*5,pos).empty())
	{
		MineManager::create()->removeTileTarget(tileTarget);
		tileTarget = TilePositions::None;
		return;
	}

	if (vulture->getTilePosition().getDistance(tileTarget) > 2)
	{
		vulture->move(pos);
		return;
	}

	if (!vulture->isIdle() &&
		  vulture->getLastCommand().getType() == UnitCommandTypes::Use_Tech_Position &&
		  vulture->getLastCommand().getTargetPosition() == pos &&
		  vulture->getLastCommandFrame() > Broodwar->getFrameCount() - 24*5)
	{
		return;
	}
	
	vulture->useTech(TechTypes::Spider_Mines,pos);
	MineManager::create()->removeTileTarget(tileTarget);
	if (debug) Broodwar->printf("Lay mine at tile (%d,%d)",tileTarget.x(),tileTarget.y());
}


/************************************************************************/
/* MineManager                                                          */
/************************************************************************/

MineManager* theMineManager = NULL;

MineManager* MineManager::create() 
{
	if (theMineManager) return theMineManager;
	else return theMineManager = new MineManager();
}

void MineManager::destroy()
{
	if (theMineManager)
	{
		delete theMineManager;
		theMineManager = NULL;
	}
}

MineManager::MineManager()
{
	debug = false;

	mInfo = MyInfoManager::create();
	eInfo = EnemyInfoManager::create();
	scoutManager = ScoutManager::create();
	terrainManager = TerrainManager::create();
	arbitrator = NULL;
	minePlacers.clear();
	baseTargets.clear();
	tileTargets.clear();
	path.clear();
	startTP = TilePositions::None;
	endTP   = TilePositions::None;
	lastStartTP = TilePositions::None;
	lastEndTP   = TilePositions::None;
	pathStep = 3;
	tileStep = 4;
	lastUpdateBaseFrame = 0;
	lastUpdateTileFrame = 0;
	lastUpdatePathFrame = 0;
}

MineManager::~MineManager()
{

}

void MineManager::onOffer(set<Unit*> units)
{
	for each (Unit* u in units)
	{
		if (u->getType() != UnitTypes::Terran_Vulture)
		{
			arbitrator->decline(this, u, 0);
			continue;
		}

		arbitrator->accept(this, u);
		if (getMinePlacer(u) == NULL)			
		{
			minePlacers.insert(new MinePlacer(u));
		}
	}
}

void MineManager::onRevoke(Unit* unit, double bid)
{
	removeMinePlacer(unit);
}

void MineManager::onUnitDestroy(Unit* unit)
{
	if (!unit)
	{
		return;
	}

	removeMinePlacer(unit);
}

set<MinePlacer*> MineManager::getMinePlacers()
{
	return minePlacers;
}

MinePlacer* MineManager::getMinePlacer(Unit* u)
{
	for each (MinePlacer* m in minePlacers)
	{
		if (m->vulture == u)
		{
			return m;
		}
	}
	return NULL;
}

void MineManager::removeMinePlacer(Unit* u)
{
	for each (MinePlacer* m in minePlacers)
	{
		if (m->vulture == u)
		{
			minePlacers.erase(m);
			return;
		}
	}
}

int MineManager::getMinePlacerCount(MinePurpose purpose)
{
	int n = 0;

	for each (MinePlacer* m in minePlacers)
	{
		if (m->purpose == purpose)
		{
			n++;
		}
	}

	return n;
}

map<BWTA::BaseLocation*,BWAPI::Unit*> MineManager::getBaseTargets()
{
	return baseTargets;
}

void MineManager::updateBaseTarget(BWTA::BaseLocation* base, Unit* unit)
{
	if (baseTargets.find(base) != baseTargets.end())
	{
		baseTargets[base] = unit;
	}
}

void MineManager::removeBaseTarget(BWTA::BaseLocation* base)
{
	if (!base)
	{
		return;
	}

	baseTargets.erase(base);
}

void MineManager::updateBaseTargets()
{
	if (!baseTargets.empty())
	{
		return;
	}

	if (Broodwar->getFrameCount() - lastUpdateBaseFrame < 24*10)
	{
		return;
	}

	lastUpdateBaseFrame = Broodwar->getFrameCount();

	for each (BWTA::BaseLocation* b in BWTA::getBaseLocations())
	{
		if (b->isIsland() || terrainManager->getGroundDistance(b->getTilePosition(),Broodwar->self()->getStartLocation()) < 0)
		{
			continue;
		}

		if (ICEStarCraft::Helper::isDirectlyConnected(b->getRegion(),BWTA::getRegion(Broodwar->self()->getStartLocation())))
		{
			continue;
		}

		if (!SelectAll()(isBuilding,isWorker,Vulture_Spider_Mine).inRadius(32*4,b->getPosition()).empty())
		{
			continue;
		}

		if (!SelectAllEnemy()(isCompleted)(canAttack,Carrier,Reaver,Bunker).not(Vulture_Spider_Mine).inRadius(32*4,b->getPosition()).empty())
		{
			continue;
		}

		if (!SelectAllEnemy()(isResourceDepot).inRadius(32*4,b->getPosition()).empty())
		{
			continue;
		}

		if (eInfo->isEnemyBase(b))
		{
			continue;		
		}

		if (scoutManager->enemyStartLocation && ICEStarCraft::Helper::isDirectlyConnected(b->getRegion(),scoutManager->enemyStartLocation->getRegion()))
		{
			if (terrainManager->gameMap == Maps::Andromeda || terrainManager->gameMap == Maps::Electric_Circuit)
			{
				continue;
			}
		}

		baseTargets.insert(make_pair(b,(Unit*)NULL));
	}
}

map<TilePosition,Unit*> MineManager::getTileTargets()
{
	return tileTargets;
}

void MineManager::updateTileTarget(TilePosition tile, Unit* unit)
{
	if (tileTargets.find(tile) != tileTargets.end())
	{
		tileTargets[tile] = unit;
	}
}

void MineManager::removeTileTarget(TilePosition tile)
{
	if (tile == TilePositions::None)
	{
		return;
	}

	tileTargets.erase(tile);
}

void MineManager::updateTileTargets()
{
	if (!tileTargets.empty())
	{
		return;
	}

	if (Broodwar->getFrameCount() - lastUpdateTileFrame < 24*10)
	{
		return;
	}

	lastUpdateTileFrame = Broodwar->getFrameCount();

	for (int i = 0; i < Broodwar->mapWidth(); i += tileStep)
	{
		for (int j = 0; j < Broodwar->mapHeight(); j += tileStep)
		{
			TilePosition tile = TilePosition(i,j);
			Position pos = Position(32*i+16,32*j+16);

			if (!SelectAll()(Vulture_Spider_Mine).inRadius(32,pos).empty())
			{
				continue;
			}

			if (!SelectAll()(isBuilding).inRadius(32,pos).empty())
			{
				continue;
			}

			if (!SelectAll().not(canMove).inRadius(32,pos).empty())
			{
				continue;
			}

			if (terrainManager->getGroundDistance(tile,Broodwar->self()->getStartLocation()) < 0)
			{
				continue;
			}
			
			double max = sqrt(1.0 * Broodwar->mapWidth() * Broodwar->mapWidth() + Broodwar->mapHeight() * Broodwar->mapHeight());
			double maxDistance = 50 + Broodwar->self()->supplyUsed()/2 * (max - 50) / 200;
			if (tile.getDistance(Broodwar->self()->getStartLocation()) > maxDistance)
			{
				continue;
			}

			bool skip = false;
			for each (BWTA::BaseLocation* b in BWTA::getBaseLocations())
			{
				if (b->isStartLocation() && BWTA::getRegion(tile) == b->getRegion())
				{
					skip = true;
					break;
				}

				double minDistance = (eInfo->isEnemyBase(b) || b == terrainManager->eNearestBase) ? 25 : 15;
				if (tile.getDistance(b->getTilePosition()) < minDistance)
				{
					skip = true;
					break;
				}
			}

			if (skip)
			{
				continue;
			}

			if (terrainManager->eSecondChokepoint && tile.getDistance(TilePosition(terrainManager->eSecondChokepoint->getCenter())) < 20)
			{
				continue;
			}

			tileTargets.insert(make_pair(tile,(Unit*)NULL));
		}
	}
}

vector<TilePosition> MineManager::getMinePath()
{
	return path;
}

void MineManager::clearMinePath()
{
	path.clear();
}

int MineManager::getMineStep() 
{
	return pathStep;
}

void MineManager::updateMinePath()
{
	if (Broodwar->getFrameCount() - lastUpdatePathFrame < 24*10)
	{
		return;
	}

	lastUpdatePathFrame = Broodwar->getFrameCount();

	bool endNearEnemyBase = false;
	startTP = TilePosition(terrainManager->mSecondChokepoint->getCenter());

	if (terrainManager->eSecondChokepoint)
	{
		endTP = TilePosition(terrainManager->eSecondChokepoint->getCenter());
		endNearEnemyBase = true;
	}
	else
	{
		endTP = TilePosition(Broodwar->mapWidth()/2,Broodwar->mapHeight()/2);
	}

	startTP = terrainManager->getConnectedTilePositionNear(startTP);
	endTP   = terrainManager->getConnectedTilePositionNear(endTP);

	if (startTP != lastStartTP || endTP != lastEndTP)
	{
		if (debug) Broodwar->printf("Change mine path");
		path = BWTA::getShortestPath(endTP,startTP);
		std::reverse(path.begin(),path.end());
		lastStartTP = startTP;
		lastEndTP   = endTP;

		if (endNearEnemyBase)
		{
			// delete some tiles
			int num = Broodwar->enemy()->getRace() == Races::Terran ? 16 : 12;
			int i = -1;
			while (++i < num && !path.empty())
			{
				path.pop_back();
			}
		}
	}
}

void MineManager::update()
{
	if (!Broodwar->self()->hasResearched(TechTypes::Spider_Mines))
	{
		return;
	}

	for each (Unit* u in Broodwar->self()->getUnits())
	{
		if (u->getType() == UnitTypes::Terran_Vulture &&
			  u->isCompleted() &&
				u->getSpiderMineCount() > 0 &&
				u->getHitPoints() >= u->getType().maxHitPoints() / 2 &&
				getMinePlacer(u) == NULL)
		{
			arbitrator->setBid(this, u, 80);
		}
	}

	// remove vultures that don't have mines
	for (set<MinePlacer*>::iterator i = minePlacers.begin(); i != minePlacers.end();)
	{
		MinePlacer* m = *i;
		if (m->vulture->getSpiderMineCount() < 1)
		{
			minePlacers.erase(i++);
			arbitrator->removeBid(this, m->vulture);
		}
		else
		{
			++i;
		}
	}
	
	if (minePlacers.empty())
	{
		baseTargets.clear();
		tileTargets.clear();
		return;
	}

	updateBaseTargets();
	updateTileTargets();
	updateMinePath();
	
	for each (MinePlacer* m in minePlacers)
	{
		if (m->purpose != MineNone)
		{
			if (m->purpose == MineBase)
			{
				// check again !!! DropManager::update() is called before MineManager::update()
				// take this vulture from ArmyManager (when goAttack is true) and DropManager
				arbitrator->setBid(this, m->vulture, 110);
			}
			else
			{
				arbitrator->setBid(this, m->vulture, 80);
			}
			m->placeMine();
			continue;
		}
		
		if (!baseTargets.empty() && getMinePlacerCount(MineBase) < min(2,(int)baseTargets.size()))
		{
			m->purpose = MineBase;
		}
		else if (!tileTargets.empty() && getMinePlacerCount(MineMap) < min(5,(int)tileTargets.size()))
		{
			m->purpose = MineMap;
		}
		else if (!path.empty() && getMinePlacerCount(MinePath) < 1)
		{
			m->purpose = MinePath;
		}
		/*else if (!tileTargets.empty() && getMinePlacerCount(MineMap) < (int)tileTargets.size())
		{
			m->purpose = MineMap;
		}*/
	}
}

void MineManager::showDebugInfo()
{
	if (!debug) return;

	for each (MinePlacer* m in minePlacers)
	{
		Position p = m->vulture->getPosition();
		Broodwar->drawCircleMap(p.x(),p.y(),15,Colors::Orange,true);
		Broodwar->drawTextMap(p.x(),p.y(),"%s",m->getMinePurposeString().c_str());
		
		if (m->baseTarget)
		{
			Position q = m->baseTarget->getPosition();
			Broodwar->drawLineMap(p.x(),p.y(),q.x(),q.y(),Colors::Yellow);
		}

		if (m->tileTarget != TilePositions::None)
		{
			Position q = Position(m->tileTarget);
			Broodwar->drawLineMap(p.x(),p.y(),q.x(),q.y(),Colors::Yellow);
		}
	}

	for (map<BWTA::BaseLocation*,Unit*>::iterator i = baseTargets.begin(); i != baseTargets.end(); i++)
	{
		Broodwar->drawCircleMap(i->first->getPosition().x(),i->first->getPosition().y(),40,Colors::Yellow);
	}

	int count = 0;
	for (map<TilePosition,Unit*>::iterator i = tileTargets.begin(); i != tileTargets.end(); i++)
	{
		Broodwar->drawCircleMap(i->first.x()*32,i->first.y()*32,3,Colors::White);
		if (!i->second)
		{
			count++;
		}
	}
	//Broodwar->drawTextScreen(433,250,"TileTargets: %d/%d",count,tileTargets.size());
	//Broodwar->drawTextScreen(433,260,"MineMap: %d",getMinePlacerCount(MineMap));
}