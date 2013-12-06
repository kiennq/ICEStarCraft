#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include <Arbitrator.h>
#include "Base.h"
#include "BaseManager.h"
#include "Common.h"
#include "Helper.h"
#include "InformationManager.h"
#include "ScoutManager.h"
#include "UnitGroupManager.h"

class BaseClass;

namespace ICEStarCraft
{
	enum MinePurpose
	{
		MineBase,
		MineMap,
		MinePath,
		MineNone
	};
}

class MinePlacer
{
public:

	MinePlacer(BWAPI::Unit* vulture): vulture(vulture)
	{
		debug = false;
		
		purpose = ICEStarCraft::MineNone;
		baseTarget = NULL;
		tileTarget = BWAPI::TilePositions::None;
		startFrame = 0;
	}

	bool debug;

	BWAPI::Unit* vulture;
	ICEStarCraft::MinePurpose purpose;
	BWTA::BaseLocation* baseTarget;
	BWAPI::TilePosition tileTarget;
	int startFrame;

	void placeMine();
	void mineBase();
	void minePath();
	void mineMap();

	std::string getMinePurposeString();
};

class MineManager : public Arbitrator::Controller<BWAPI::Unit*,double>
{
public:

	static MineManager* create();
	static void destroy();

	void setArbitrator(Arbitrator::Arbitrator<BWAPI::Unit*,double>* arbitrator) {this->arbitrator = arbitrator;}

	std::string getName() const {return "MineManager";}

	void update();
	void onUnitDestroy(BWAPI::Unit*);
	void onOffer(std::set<BWAPI::Unit*> units);
	void onRevoke(BWAPI::Unit* unit, double bid);

	std::set<MinePlacer*> getMinePlacers();
	MinePlacer* getMinePlacer(BWAPI::Unit*);
	void removeMinePlacer(BWAPI::Unit*);
	int getMinePlacerCount(ICEStarCraft::MinePurpose);

	std::map<BWTA::BaseLocation*,BWAPI::Unit*> getBaseTargets();
	void updateBaseTarget(BWTA::BaseLocation*,BWAPI::Unit*);
	void removeBaseTarget(BWTA::BaseLocation*);

	std::map<BWAPI::TilePosition,BWAPI::Unit*> getTileTargets();
	void updateTileTarget(BWAPI::TilePosition,BWAPI::Unit*);
	void removeTileTarget(BWAPI::TilePosition);

	std::vector<BWAPI::TilePosition> getMinePath();
	void clearMinePath();
	int getMineStep();

	void showDebugInfo();

protected:

	MineManager();
	~MineManager();

private:

	bool debug;

	MyInfoManager* mInfo;
	EnemyInfoManager* eInfo;
	ScoutManager* scoutManager;
	TerrainManager* terrainManager;

	Arbitrator::Arbitrator<BWAPI::Unit*,double>* arbitrator;
	
	std::set<MinePlacer*> minePlacers;
	std::map<BWTA::BaseLocation*,BWAPI::Unit*> baseTargets;
	std::map<BWAPI::TilePosition,BWAPI::Unit*> tileTargets;
	std::vector<BWAPI::TilePosition> path;
	BWAPI::TilePosition startTP;
	BWAPI::TilePosition lastStartTP;
	BWAPI::TilePosition endTP;
	BWAPI::TilePosition lastEndTP;
	int pathStep;
	int tileStep;
	int lastUpdateBaseFrame;
	int lastUpdateTileFrame;
	int lastUpdatePathFrame;
	
	void updateBaseTargets();
	void updateTileTargets();
	void updateMinePath();
};