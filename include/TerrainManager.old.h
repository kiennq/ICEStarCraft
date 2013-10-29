#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include <vector>
#include "ScoutManager.h"
#include "Vector2.h"
#include "Helper.h"
#include "MapInfo.h"

using namespace BWAPI;
using namespace BWTA;
class ScoutManager;
class ChokepointManager{

public:
	
	static ChokepointManager* create();
	static void destroy();

	void setScoutManager(ScoutManager* scout);
	void setUsefulChokepoint();
	
	//_T_
	vector<BWTA::BaseLocation*> exploredBase;
	vector<BWTA::Chokepoint*> exploredChoke;
	vector<BWTA::Chokepoint*> forbiddenChoke;
	void updateChokepoint();
	bool chokepointUpdated;
	Position mBestSetPoint2;
	Position siegePoint;
	bool highGroundSiegePointAvailable;

	Chokepoint* getFirstChoke();
	Chokepoint* getSecondChoke();
	Chokepoint* getThirdChoke();
	std::set<TilePosition> firstCPBuilding();
	void bestSetPoint();
	Position earlyBestPoint;
	Position lateBestPoint;
	void setTankSiegePosition();
	TilePosition getTankSiegePosition();
	void getEnemyChokePoints();
	Position getMyBestSetPoint();
	Chokepoint* efirstChokepoint;
	Chokepoint* esecondChokepoint;
	Chokepoint* ethirdChokepoint;
	Position eBaseCenter;
	BaseLocation* enemyNearestExpansion;
	void onFrame();
	std::set<Chokepoint*> ckpSet;
	std::set<BWTA::Region*> rgSet;
	Position mBestSetPoint;
	
	map<BWTA::Region*,set<Position>> regionVertices;
	void getRegionVertices();
	set<TilePosition> dropReg;
	set<TilePosition> getDroppablePosition();

	//position near mfirstChoke to build Barracks, Supply Depot, Bunker (against Zealot rush)
	map<TilePosition,UnitType> buildTiles;
	TilePosition bbPos;
	TilePosition bsPos;
	TilePosition buPos;

	ICEStarCraft::MapInfo mapinfo;
	ICEStarCraft::Map bwmap;

protected:	
	ChokepointManager();
private:
	bool finishEnemyChoke;
	ScoutManager* scm;
	Chokepoint* mfirstChokepoint;
	Chokepoint* msecondChokepoint;
	Chokepoint* mthirdChokepoint;

	std::set<TilePosition> BTPostion ;
	Position CCPosition;
	TilePosition CCTilePosition;
	std::set<TilePosition> SiegePostionSet ;
	TilePosition bestSiegeTile;
};



