#include <time.h>
#include "TerrainManager.h"

using namespace BWAPI;
using namespace BWTA;
using namespace ICEStarCraft;
using namespace std;

TerrainManager* theTerrainManager = NULL;

TerrainManager* TerrainManager::create()
{
	if (theTerrainManager) return theTerrainManager;
	else return theTerrainManager = new TerrainManager();
}

TerrainManager::TerrainManager()
{
	mapInfo = MapInfo();
	gameMap = mapInfo.getMapInfo();

	scm = NULL;
	BWTA::getGroundDistanceMap(Broodwar->self()->getStartLocation(),distanceFromMyStartLocation);

	mFirstChokepoint  = NULL;
	mSecondChokepoint = NULL;
	mThirdChokepoint  = NULL;

	eFirstChokepoint  = NULL;
	eSecondChokepoint = NULL;
	eThirdChokepoint  = NULL;

	mChokepointsAnalyzed = false;
	eChokepointsAnalyzed = false;

	mNearestBase = getNearestBase(BWTA::getStartLocation(Broodwar->self()));
	eNearestBase = NULL;
	eBaseCenter = BWAPI::Positions::None;

	siegePoint = Positions::None;

	regionVertices.clear();
	analyzeRegionVertices();

	buildTiles.clear();
	bbPos = TilePositions::None;
	bsPos = TilePositions::None;
	buPos = TilePositions::None;

	analyzeMyChokepoints();
	analyzeSiegePoint();
	
	if (Broodwar->enemy()->getRace() != Races::Zerg)
	{
		analyzeWallinPositions();
	}

	analyzeTankDropPositions();

	analyzeTurretPositions();
}

void TerrainManager::setScoutManager(ScoutManager* scout)
{
	scm = scout;
}

void TerrainManager::destroy()
{
	if (theTerrainManager) delete theTerrainManager;
	theTerrainManager = NULL;
}

void TerrainManager::onFrame()
{
	analyzeEnemyChokepoints();
}

void TerrainManager::analyzeMyChokepoints()
{
	if (gameMap != Maps::Unknown)
	{
		vector<Position> chokepoints = gameMap.getChokepoints(Broodwar->self()->getStartLocation());

		if (chokepoints.size() == 3)
		{
			mFirstChokepoint  = BWTA::getNearestChokepoint(chokepoints[0]);
			mSecondChokepoint = BWTA::getNearestChokepoint(chokepoints[1]);
			mThirdChokepoint  = BWTA::getNearestChokepoint(chokepoints[2]);
			
			mChokepointsAnalyzed = true;
			//Broodwar->printf("Chokepoints analyzed | Known map | %s",gameMap.getName().c_str());
			return;
		}
	}

	vector<BWTA::Chokepoint*> chokepoints = getUsefulChokepoints(BWTA::getStartLocation(Broodwar->self()));

	if (chokepoints.size() == 3)
	{
		mFirstChokepoint  = chokepoints[0];
		mSecondChokepoint = chokepoints[1];
		mThirdChokepoint  = chokepoints[2];
	}
	else
	{
		// should never happen
	}

	mChokepointsAnalyzed = true;
	//Broodwar->printf("Chokepoints analyzed | Unknown map");
}

void TerrainManager::analyzeEnemyChokepoints()
{	
	if (eChokepointsAnalyzed || scm->enemyStartLocation == NULL)
	{
		return;
	}

	eChokepointsAnalyzed = true;
	BWTA::getGroundDistanceMap(scm->enemyStartLocation->getTilePosition(),distanceFromEnemyStartLocation);
	eNearestBase = getNearestBase(scm->enemyStartLocation);
	
	if (gameMap != Maps::Unknown)
	{
		vector<Position> chokepoints = gameMap.getChokepoints(scm->enemyStartLocation->getTilePosition());
		
		eFirstChokepoint  = BWTA::getNearestChokepoint(chokepoints[0]);
		eSecondChokepoint = BWTA::getNearestChokepoint(chokepoints[1]);
		eThirdChokepoint  = BWTA::getNearestChokepoint(chokepoints[2]);

		//Broodwar->printf("Enemy chokepoints analyzed | Known map | %s",gameMap.getName().c_str());
		return;
	}

	vector<BWTA::Chokepoint*> chokepoints = getUsefulChokepoints(scm->enemyStartLocation);

	if (chokepoints.size() == 3)
	{
		eFirstChokepoint  = chokepoints[0];
		eSecondChokepoint = chokepoints[1];
		eThirdChokepoint  = chokepoints[2];
	}
	else
	{
		// should never happen
	}

	//Broodwar->printf("Enemy chokepoints analyzed | Unknown map");
}

void TerrainManager::analyzeRegionVertices()
{
	for each (BWTA::Region* reg in BWTA::getRegions())
	{
		BWTA::Polygon border = reg->getPolygon();
		set<Position> vertices;
		Vector2 v = Vector2(0,0);
		Position tmp;
		Polygon::iterator cur;
		Polygon::iterator pre = border.begin();
		for (Polygon::iterator cur = border.begin(); cur != border.end(); cur++)
		{
			v = (*cur) - (*pre);
			if (cur != border.begin() && v.approxLen() > 64)
			{
				for (int i = 1; i <= 1 + v.approxLen()/64; i++)
				{	
					tmp = v * (i * 64.0 / v.approxLen()) + *pre;
					if (tmp != *cur)
					{
						if (tmp.y() >= Broodwar->mapHeight()*32 - 64)
							tmp.y() = Broodwar->mapHeight()*32;
						vertices.insert(tmp);
					}
				}
			}
			pre = cur;
			vertices.insert(*cur);
		}
		regionVertices.insert(make_pair(reg,vertices));
	}
}

double TerrainManager::getGroundDistance(BWAPI::TilePosition tp1, BWAPI::TilePosition tp2)
{
	if (!eChokepointsAnalyzed)
	{
		return BWTA::getGroundDistance(tp1,tp2);
	}

	if (tp1 == Broodwar->self()->getStartLocation())
	{
		return distanceFromMyStartLocation.getItem(tp2.x(),tp2.y());
	}

	if (tp2 == Broodwar->self()->getStartLocation())
	{
		return distanceFromMyStartLocation.getItem(tp1.x(),tp1.y());
	}

	if (scm && scm->enemyStartLocation && tp1 == scm->enemyStartLocation->getTilePosition())
	{
		return distanceFromEnemyStartLocation.getItem(tp2.x(),tp2.y());
	}

	if (scm && scm->enemyStartLocation && tp2 == scm->enemyStartLocation->getTilePosition())
	{
		return distanceFromEnemyStartLocation.getItem(tp1.x(),tp1.y());
	}

	return BWTA::getGroundDistance(tp1,tp2);
}

BWTA::BaseLocation* TerrainManager::getNearestBase(BWTA::BaseLocation* base, bool isStartLocation, bool hasGas)
{
	// if isStartLocation is false then we don't consider bases that are start locations
	// if hasGas is true then we consider only bases that have gas

	if (base == NULL)
	{
		return NULL;
	}

	BWTA::BaseLocation* nearestBase = NULL;
	double minD = 99999999;
	int n = 0;
	
	for each (BWTA::BaseLocation* b in BWTA::getBaseLocations())
	{
		if (b->isStartLocation() && !isStartLocation)
		{
			continue;
		}

		if (b->isMineralOnly() && hasGas)
		{
			continue;
		}

		if (b->isIsland() || getGroundDistance(b->getTilePosition(),base->getTilePosition()) < 0)
		{
			continue;
		}
		
		double d = getGroundDistance(b->getTilePosition(),base->getTilePosition());
		
		if (d < minD && d > 0)
		{
			minD = d;
			nearestBase = b;
		}
	}
	
	return nearestBase;
}

vector<BWTA::Chokepoint*> TerrainManager::getUsefulChokepoints(BWTA::BaseLocation* base)
{
	// return 3 useful choke points for this start location

	vector<BWTA::Chokepoint*> chokepoints;

	if (base == NULL || !base->isStartLocation())
	{
		return chokepoints;
	}
	
	BWTA::BaseLocation* nearestBase = getNearestBase(base);
	if (nearestBase == NULL)
	{
		return chokepoints;
	}
	
	BWTA::Chokepoint* firstCp  = NULL;
	BWTA::Chokepoint* secondCp = NULL;
	BWTA::Chokepoint* thirdCp  = NULL;
	
	// find first choke point
	double minD = 9999999;

	for each (BWTA::Chokepoint* cp in BWTA::getRegion(nearestBase->getTilePosition())->getChokepoints())
	{
		double d = getGroundDistance(TilePosition(cp->getCenter()),base->getTilePosition());
		if (d < minD)
		{
			minD = d;
			firstCp = cp;
		}
	}

	if (firstCp == NULL)
	{
		return chokepoints;
	}

	// find second choke point
	minD = 9999999;
	TilePosition mapCen = TilePosition(Broodwar->mapWidth()/2,Broodwar->mapHeight()/2).makeValid();
	bool mapCenReachable = getGroundDistance(base->getTilePosition(),mapCen) > 0;

	for each (BWTA::Chokepoint* cp in BWTA::getRegion(nearestBase->getTilePosition())->getChokepoints())
	{
		if (cp == firstCp)
		{
			continue;
		}

		double d = mapCenReachable ? getGroundDistance(TilePosition(cp->getCenter()),mapCen) : TilePosition(cp->getCenter()).getDistance(mapCen);
		if (d < minD)
		{
			minD = d;
			secondCp = cp;
		}
	}

	if (secondCp == NULL)
	{
		secondCp = firstCp;
	}

	// find third choke point
	minD = 9999999;
	BWTA::Region* thirdReg = NULL;

	if (secondCp->getRegions().first == BWTA::getRegion(nearestBase->getTilePosition()))
	{
		thirdReg = secondCp->getRegions().second;
	}
	else
	{
		thirdReg = secondCp->getRegions().first;
	}

	for each (BWTA::Chokepoint* cp in thirdReg->getChokepoints())
	{
		if (cp == firstCp || cp == secondCp)
		{
			continue;
		}

		double d = mapCenReachable ? getGroundDistance(TilePosition(cp->getCenter()),mapCen) : TilePosition(cp->getCenter()).getDistance(mapCen);
		if (d < minD)
		{
			minD = d;
			thirdCp = cp;
		}
	}

	if (thirdCp == NULL)
	{
		thirdCp = secondCp;
	}
	else if (secondCp->getCenter().getApproxDistance(thirdCp->getCenter()) > secondCp->getCenter().getApproxDistance(Position(mapCen)))
	{
		thirdCp = secondCp;
	}
	else
	{
		Vector2 v1 = Vector2(Position(mapCen)) - Vector2(secondCp->getCenter());
		Vector2 v2 = Vector2(thirdCp->getCenter()) - Vector2(secondCp->getCenter());
		if (v1.angle(v2) > 60)
		{
			thirdCp = secondCp;
		}
	}

	chokepoints.push_back(firstCp);
	chokepoints.push_back(secondCp);
	chokepoints.push_back(thirdCp);
	return chokepoints;
}

void TerrainManager::analyzeSiegePoint()
{
	if (!mChokepointsAnalyzed)
	{
		analyzeMyChokepoints();
	}

	if (!mNearestBase)
	{
		mNearestBase = getNearestBase(BWTA::getStartLocation(Broodwar->self()));
	}

	TilePosition firstCp = TilePosition(mFirstChokepoint->getCenter());
	TilePosition secondCp = TilePosition(mSecondChokepoint->getCenter());
	
	BWTA::Region* startReg  = BWTA::getRegion(Broodwar->self()->getStartLocation());
	BWTA::Region* secondReg = BWTA::getRegion(mNearestBase->getTilePosition());
	

	// find siege point on high ground

	double minD = 9999999;
	bool secondChokepointValid = BWTA::getGroundDistance(firstCp,secondCp) > 0;

	for (int i = 0; i < Broodwar->mapWidth(); i++)
	{
		for (int j = 0; j < Broodwar->mapHeight(); j++)
		{
			TilePosition tp = TilePosition(i,j);
			BWTA::Region* reg = BWTA::getRegion(tp);
			
			if (!reg || reg == secondReg)
			{
				continue;
			}

			if (reg != startReg && !ICEStarCraft::Helper::isDirectlyConnected(startReg,reg))
			{
				continue;
			}

			if (tp.getDistance(firstCp) < 5 && mFirstChokepoint->getWidth() < 120)
			{
				continue;
			}

			if (secondCp.getDistance(tp) > 14)
			{
				continue;
			}

			if (secondChokepointValid && getGroundDistance(tp,secondCp) > 32*40)
			{
				continue;
			}

			if (Broodwar->isWalkable(4*i,4*j) && Broodwar->getGroundHeight(tp) > Broodwar->getGroundHeight(secondCp))
			{
				if (tp.getDistance(TilePosition(secondCp)) < minD)
				{
					minD = tp.getDistance(TilePosition(secondCp));
					siegePoint = Position(tp);
				}
			}
		}
	}

	if (siegePoint != Positions::None)
	{
		//Broodwar->printf("Set siege point on high ground");
		return;
	}

	// find siege point on low ground

	BWTA::Chokepoint* cp = mSecondChokepoint;
	Vector2 v = Vector2(cp->getSides().first) - Vector2(cp->getSides().second);
	v = Vector2(-v.y(),v.x());
	v = v * (32 * 6.0 / v.approxLen());
	Position outPoint = v + cp->getCenter();

	if (BWTA::getRegion(outPoint) == secondReg)
	{
		outPoint = -v + cp->getCenter();
	}

	outPoint = outPoint.makeValid();

	double maxD = 0;
	double d;

	for (int i = 0; i < Broodwar->mapWidth(); i++)
	{
		for (int j = 0; j < Broodwar->mapHeight(); j++)
		{
			TilePosition tp = TilePosition(i,j);
			BWTA::Region* reg = BWTA::getRegion(tp);

			if (!reg || (reg != startReg && reg != secondReg && !ICEStarCraft::Helper::isDirectlyConnected(startReg,reg)))
			{
				continue;
			}

			if (tp.getDistance(firstCp) < 4 && mFirstChokepoint->getWidth() < 120)
			{
				continue;
			}

			if (tp.getDistance(secondCp) > 12)
			{
				continue;
			}

			if (Broodwar->getGroundHeight(tp) > Broodwar->getGroundHeight(secondCp))
			{
				continue;
			}

			d = BWTA::getGroundDistance(tp,TilePosition(outPoint)) - Position(tp).getApproxDistance(outPoint);
			if (d > maxD)
			{
				maxD = d;
				siegePoint = Position(tp);
			}
		}	
	}

	if (siegePoint != Positions::None)
	{
		//Broodwar->printf("Set siege point on low ground");
	}
	else
	{
		//Broodwar->printf("No siege point");
	}
}

double TerrainManager::getBuildingDistance(UnitType type, TilePosition tp, Position p)
{
	Position buildingCenter = Position(tp.x() * 32 + type.tileWidth() * 16, tp.y() * 32 + type.tileHeight() * 16);
	return p.getDistance(buildingCenter);
}

bool TerrainManager::canBuildHere(UnitType type, TilePosition tp)
{
	if (!Broodwar->canBuildHere(NULL,tp,type,false))
	{
		return false;
	}

	// if this tile position is too close to a base location then false
	BWTA::BaseLocation* base = BWTA::getNearestBaseLocation(tp);
	if (base)
	{
		TilePosition baseTile = base->getTilePosition();
		for (int x = 0; x < UnitTypes::Terran_Command_Center.tileWidth(); x++)
		{
			for (int y = 0; y < UnitTypes::Terran_Command_Center.tileHeight(); y++)
			{
				if (tp == TilePosition(baseTile.x()+x,baseTile.y()+y))
				{
					return false;
				}
			}
		}
	}

	for (int x = 0; x < type.tileWidth(); x++)
	{
		for (int y = 0; y < type.tileHeight(); y++)
		{
			map<TilePosition,UnitType>::iterator i = buildTiles.find(TilePosition(tp.x()+x,tp.y()+y));
			if (i == buildTiles.end())
			{
				return false;
			}

			if (i->second != UnitTypes::None)
			{
				return false;
			}
		}
	}

	return true;
}

TilePosition TerrainManager::getWallinPosition(UnitType type, Position p)
{
	TilePosition buildPos = TilePositions::None;

	if (!type.isBuilding())
	{
		return buildPos;
	}

	double d = 0;
	double minD = 99999999;
	double min = type.isFlyingBuilding() ? -1 : 32;

	for (map<TilePosition,UnitType>::iterator i = buildTiles.begin(); i != buildTiles.end(); i++)
	{
		TilePosition tp = i->first;

		if (canBuildHere(type,tp))
		{
			d = getBuildingDistance(type,tp,p);
			if (d > min && d < minD)
			{
				minD = d;
				buildPos = tp;
			}
		}
	}

	if (buildPos != TilePositions::None)
	{
		for (int x = 0; x < type.tileWidth(); x++)
		{
			for (int y = 0; y < type.tileHeight(); y++)
			{
				map<TilePosition,UnitType>::iterator i = buildTiles.find(TilePosition(buildPos.x()+x,buildPos.y()+y));
				if (i != buildTiles.end())
				{
					i->second = type;
				}
			}
		}
	}

	return buildPos;
}

void TerrainManager::analyzeWallinPositions()
{
	if (gameMap == Maps::Destination)
	{
		if (Broodwar->self()->getStartLocation() == TilePosition(31,7))
		{
			bbPos = TilePosition(56,20);
			bsPos = TilePosition(60,24);
			buPos = TilePosition(60,22);
		}

		if (Broodwar->self()->getStartLocation() == TilePosition(64,118))
		{
			bbPos = TilePosition(36,105);
			bsPos = TilePosition(33,102);
			buPos = TilePosition(33,104);
		}
	}
	else
	{
		// other maps
		Position cp = mSecondChokepoint->getCenter();
		BWTA::Region* secondReg = mNearestBase->getRegion();

		BWTA::Region* reg = NULL;
		int radius = 10;

		for (int i = 0; i < Broodwar->mapWidth(); i++)
		{
			for (int j = 0; j < Broodwar->mapHeight(); j++)
			{
				if (TilePosition(cp).getDistance(TilePosition(i,j)) <= radius && Broodwar->isBuildable(TilePosition(i,j)))
				{
					reg = BWTA::getRegion(TilePosition(i,j));
					if (reg && reg == secondReg)
					{
						buildTiles.insert(make_pair(TilePosition(i,j),UnitTypes::None));
					}
				}
			}	
		}
		bbPos = getWallinPosition(UnitTypes::Terran_Barracks, cp);
		bsPos = getWallinPosition(UnitTypes::Terran_Supply_Depot, cp);
		buPos = getWallinPosition(UnitTypes::Terran_Bunker, cp);
	}
}

void TerrainManager::analyzeTankDropPositions()
{
	if (gameMap != Maps::Unknown)
	{
		TankDropPositions = gameMap.getTankDropPositions();

		if (!TankDropPositions.empty())
		{
			//Broodwar->printf("Tank drop positions available | Known map | %s",gameMap.getName().c_str());
		}
		else
		{
			//Broodwar->printf("Tank drop positions NOT available | Known map | %s",gameMap.getName().c_str());
		}
		return;
	}
	
	// unknown maps
	for each (BWTA::BaseLocation* b in BWTA::getBaseLocations())
	{
		set<TilePosition> dropPos;
		for (int i = 0; i < Broodwar->mapWidth(); i++)
		{
			for (int j = 0; j < Broodwar->mapHeight(); j++)
			{
				TilePosition tp = TilePosition(i,j);
				if (!ICEStarCraft::Helper::isWalkable(tp))
					continue;

				if (Broodwar->getGroundHeight(tp) > Broodwar->getGroundHeight(b->getTilePosition())	&&
					  tp.getDistance(b->getTilePosition()) <= 16 &&
						!BWTA::isConnected(tp,b->getTilePosition()))
				{
					dropPos.insert(tp);
				}
			}
		}

		if (dropPos.empty())
		{
			continue;
		}

		int X = 0;
		int Y = 0;
		for each (TilePosition tp in dropPos)
		{
			X += tp.x();
			Y += tp.y();
		}

		X = X / dropPos.size();
		Y = Y / dropPos.size();

		TilePosition cen = TilePosition(X,Y).makeValid();
		TankDropPositions.insert(make_pair(b->getTilePosition(),cen));
	}

	if (!TankDropPositions.empty())
	{
		//Broodwar->printf("Tank drop positions available | Unknown map");
	}
	else
	{
		//Broodwar->printf("Tank drop positions NOT available | Unknown map");
	}
}

void TerrainManager::analyzeTurretPositions()
{
	TurretPositions.clear();

	for each (BWTA::BaseLocation* base in BWTA::getStartLocations())
	{
		BWTA::Region* reg = BWTA::getRegion(base->getTilePosition());
		if (!reg)
		{
			continue;
		}
		BWTA::Polygon vertices = reg->getPolygon();
		Position cen = reg->getCenter();
		vector<TilePosition> tiles;

		for (int i = 0; i < vertices.size(); i++)
		{
			if (i%5 == 0)
			{
				Vector2 v = Vector2(cen) - Vector2(vertices[i]);
				v = v * (3.0 * 32 / v.approxLen());
				Position p = v + vertices[i];
				if (BWTA::getRegion(p) != base->getRegion())
				{
					continue;
				}
				if (BWTA::getNearestChokepoint(p) && p.getApproxDistance(BWTA::getNearestChokepoint(p)->getCenter()) < 32*4)
				{
					continue;
				}
				tiles.push_back(TilePosition(p));
			}
		}
		TurretPositions.insert(make_pair(base->getTilePosition(),tiles));
	}
}

TilePosition TerrainManager::getTankDropPosition(BaseLocation* base)
{
	if (base == NULL)
	{
		return TilePositions::None;
	}

	if (TankDropPositions.find(base->getTilePosition()) == TankDropPositions.end())
	{
		return TilePositions::None;
	}

	return TankDropPositions[base->getTilePosition()];
}

vector<TilePosition> TerrainManager::getTurretPositions(BaseLocation* base)
{
	vector<TilePosition> tiles;
	if (base == NULL)
	{
		return tiles;
	}

	if (TurretPositions.find(base->getTilePosition()) == TurretPositions.end())
	{
		return tiles;
	}

	return TurretPositions[base->getTilePosition()];
}

TilePosition TerrainManager::getConnectedTilePositionNear(TilePosition tp, int radius /* = 5*/)
{
	// return the tile position near tp and connected to our start location
	// if such tile position doesn't exist then return tp

	int X = tp.x();
	int Y = tp.y();
	int x = 0;
	int y = 0;
	int dx = 0;
	int dy = -1;
	int r = radius;

	for (int i = 0; i < (2 * r + 1) * (2 * r + 1); i++)
	{
		if ((x <= r && x >= -r) && (y <= r && y >= -r) &&
			  (x + X >= 0 && x + X <= Broodwar->mapWidth()) && (y + Y >= 0 && y + Y <= Broodwar->mapHeight()))
		{
			if (getGroundDistance(TilePosition(x + X, y + Y),Broodwar->self()->getStartLocation()) > 0)
			{
				return TilePosition(x + X, y + Y);
			}
		}

		x += dx;
		y += dy;

		if (x == y || (x < 0 && x == - y) || (x >= 0 && x == - 1 - y))
		{
			// change direction
			// rotate 90 degrees counter clockwise
			int t = -dx;
			dx = dy;
			dy = t;
		}
	}

	return tp;
}

TilePosition TerrainManager::getBlockingMineral()
{
	if (gameMap == Maps::Destination)
	{
		if (Broodwar->self()->getStartLocation().y() < Broodwar->mapHeight()/2)
		{
			 return TilePosition(54,6);
		}
		else
		{
			 return TilePosition(40,120);
		}
	}

	if (gameMap == Maps::Heartbreak_Ridge)
	{
		if (Broodwar->self()->getStartLocation().x() < Broodwar->mapWidth()/2)
		{
			return TilePosition(6,91);
		}
		else
		{
			return TilePosition(119,3);
		}
	}

	return TilePositions::None;
}

void TerrainManager::showDebugInfo()
{
	// draw choke points
	for each (BWTA::Chokepoint* cp in BWTA::getChokepoints())
	{
		if (Broodwar->isExplored(TilePosition(cp->getCenter())))
		{
			Broodwar->drawCircleMap(cp->getCenter().x(),cp->getCenter().y(),5,Colors::Cyan,true);
		}
		else
		{
			Broodwar->drawCircleMap(cp->getCenter().x(),cp->getCenter().y(),5,Colors::Brown,true);
		}

		if (cp == mFirstChokepoint)
		{
			Broodwar->drawCircleMap(cp->getCenter().x(),cp->getCenter().y(),20,Colors::Blue);
		}

		if (cp == mSecondChokepoint)
		{
			Broodwar->drawCircleMap(cp->getCenter().x(),cp->getCenter().y(),15,Colors::Yellow);
		}

		if (cp == mThirdChokepoint)
		{
			Broodwar->drawCircleMap(cp->getCenter().x(),cp->getCenter().y(),10,Colors::Cyan);
		}

		Broodwar->drawTextMap(cp->getCenter().x()+5,cp->getCenter().y(),"%d",(int)cp->getWidth());
	}

	// draw wall-in positions
	if (bbPos != Positions::None)
	{
		Position tp = Position(bbPos);
		Broodwar->drawBoxMap(tp.x(), tp.y(), tp.x() + UnitTypes::Terran_Barracks.tileWidth() * 32, tp.y() + UnitTypes::Terran_Barracks.tileHeight() * 32, Colors::Green);
		Broodwar->drawTextMap(tp.x() - 10 + UnitTypes::Terran_Barracks.tileWidth() * 16, tp.y() - 6 + UnitTypes::Terran_Barracks.tileHeight() * 16, "\x07 BB");
	}

	if (bsPos != Positions::None)
	{
		Position tp = Position(bsPos);
		Broodwar->drawBoxMap(tp.x(), tp.y(), tp.x() + UnitTypes::Terran_Supply_Depot.tileWidth() * 32, tp.y() + UnitTypes::Terran_Supply_Depot.tileHeight() * 32, Colors::Blue);
		Broodwar->drawTextMap(tp.x() - 10 + UnitTypes::Terran_Supply_Depot.tileWidth() * 16, tp.y() - 6 + UnitTypes::Terran_Supply_Depot.tileHeight() * 16, "\x0E BS");
	}

	if (buPos != Positions::None)
	{
		Position tp = Position(buPos);
		Broodwar->drawBoxMap(tp.x(), tp.y(), tp.x() + UnitTypes::Terran_Bunker.tileWidth() * 32, tp.y() + UnitTypes::Terran_Bunker.tileHeight() * 32, Colors::Red);
		Broodwar->drawTextMap(tp.x() - 10 + UnitTypes::Terran_Bunker.tileWidth() * 16, tp.y() - 6 + UnitTypes::Terran_Bunker.tileHeight() * 16, "\x08 BU");
	}
}