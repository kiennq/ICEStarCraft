#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include <vector>
#include "ScoutManager.h"
#include "Vector2.h"
#include "Helper.h"
#include "MapInfo.h"

class ScoutManager;
class TerrainManager
{
public:
	
	static TerrainManager* create();
	static void destroy();
	void onFrame();
	void setScoutManager(ScoutManager* scout);
	
	BWTA::Chokepoint* mFirstChokepoint;
	BWTA::Chokepoint* mSecondChokepoint;
	BWTA::Chokepoint* mThirdChokepoint;

	BWTA::Chokepoint* eFirstChokepoint;
	BWTA::Chokepoint* eSecondChokepoint;
	BWTA::Chokepoint* eThirdChokepoint;

	bool mChokepointsAnalyzed;
	bool eChokepointsAnalyzed;

	BWAPI::Position siegePoint;

	BWTA::BaseLocation* mNearestBase;
	BWTA::BaseLocation* eNearestBase;
	BWAPI::Position eBaseCenter;
		
	map<BWTA::Region*,set<BWAPI::Position>> regionVertices;
	
	// for wall-in
	BWAPI::TilePosition bbPos;
	BWAPI::TilePosition bsPos;
	BWAPI::TilePosition buPos;

	ICEStarCraft::MapInfo mapInfo;
	ICEStarCraft::Map gameMap;

	double getGroundDistance(BWAPI::TilePosition, BWAPI::TilePosition);
	BWTA::BaseLocation* getNearestBase(BWTA::BaseLocation*, bool isStartLocation = false, bool hasGas = true);
	std::vector<BWTA::Chokepoint*> getUsefulChokepoints(BWTA::BaseLocation*);
	BWAPI::TilePosition getTankDropPosition(BWTA::BaseLocation*);
	BWAPI::TilePosition getConnectedTilePositionNear(BWAPI::TilePosition, int radius = 5);
	BWAPI::TilePosition getBlockingMineral();

	void showDebugInfo();

protected:

	TerrainManager();

private:

	ScoutManager* scm;

	std::map<BWAPI::TilePosition,BWAPI::UnitType> buildTiles; // for wall-in
	BWTA::RectangleArray<double> distanceFromMyStartLocation;
	BWTA::RectangleArray<double> distanceFromEnemyStartLocation;

	std::map<BWAPI::TilePosition,BWAPI::TilePosition> TankDropPositions;

	double getBuildingDistance(BWAPI::UnitType, BWAPI::TilePosition, BWAPI::Position);
	bool canBuildHere(BWAPI::UnitType, BWAPI::TilePosition);
	BWAPI::TilePosition getWallinPosition(BWAPI::UnitType, BWAPI::Position);

	void analyzeRegionVertices();
	void analyzeMyChokepoints();
	void analyzeEnemyChokepoints();
	void analyzeSiegePoint();
	void analyzeWallinPositions();
	void analyzeTankDropPositions();
};