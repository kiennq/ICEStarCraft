#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include "UnitGroup.h"
#include "UnitGroupManager.h"
#include "WorkerManager.h"
#include "Base.h"
#include "BaseManager.h"
#include "TerrainManager.h"
#include "InformationManager.h"
#include "ScoutController.h"
using namespace BWAPI;
using namespace BWTA;

class MyInfoManager;
class EnemyInfoManager;
class WorkerManager;
class BaseManager;
class TerrainManager;
class MentalClass;

//TODO: Add an Arbitrator::Controller here, best scout should be close to target
class ScoutManager
{
public:
	static ScoutManager* create();
	static void destroy();

	enum ScoutPurpose
	{
		EnemyStartLocation = 0,
		EnemyOpening,
		EnemyExpansion,
		EnemyTech,
		EnemyArmyNum,
		PreWarning,
		Running,
		MyMainBase
	};
	void SCVScout(ScoutPurpose purpose);
	void VultureScout(ScoutPurpose purpose);
	void ScannerScout(ScoutPurpose purpose,int reserveTimes);
	void scoutEnemyLocation(Unit*);
	void scoutEnemyOpening(Unit*);
	void scoutEnemyExpansion(Unit*);
	void scoutEnemyTech(Unit*);
	void scoutEnemyArmyNum(Unit*);
	void scoutMyMainBase(Unit*);//_T_
	void AsPreWarning(Unit*);
	void unitFlee(Unit* u);
	void onUnitDiscover(Unit* unit);
	void onUnitDestroy(Unit* unit);
	void onFrame();
	void fixMovingStuck(Unit* u);
	std::set<Unit*> scoutGroup;
	std::set<BWTA::BaseLocation*> startLocationsToScout;
	std::set<BWTA::BaseLocation*> startLocationsExplored;
	std::set<BWTA::BaseLocation*> baseLocationsExplored;
	std::set<BWTA::BaseLocation*> LocationsHasEnemy;
	std::set<BaseLocation*> baseLocationNeedToScout;
	std::set<BWTA::BaseLocation*> expansionToScout;
	void setManagers();
	void explorEnemyBase(Unit* u);
	std::map<Unit*,ScoutPurpose> ScoutUnitPurposeMap;
	std::map<Unit*,ScoutPurpose> ScoutUnitLastPurposeMap;
	BWTA::BaseLocation *myStartLocation;
	BWTA::BaseLocation *enemyStartLocation;
	void setExpansionToScout();
	void setNeedScan(bool needscan);
	bool hasArmyKeep(Unit* u ,BaseLocation* bl,int Radius,int armyNum);
	bool hasArmyKeep(Unit* u);
	bool scoutFinish(Unit* u ,BaseLocation* bl);
	bool seeResourceDepot(Unit* u ,BaseLocation* bl);
	void setScoutNum(int num);
	bool unitInDanger(Unit* u);

	//important positions for scanner to scan, such as enemy startPostion, chokepoint etc.
	std::set<Position>essentialPostions;
	MentalClass* mental;

protected:
	ScoutManager();

private:
	int moveLevel;
	MyInfoManager* mInfo;
	EnemyInfoManager* eInfo;
	std::set<BWTA::BaseLocation*> mapBaseLocations;	
	bool expansionScouting;
	BaseLocation* currentLocationTarget;
	bool currentFinish;
	int currentStartFrame; //_T_
	int lastScanTime;
	int taskStartTime;
	std::set<Position> scannedPositions;
	WorkerManager* workerMG;
	BaseManager* bmc;
	//std::set<Unit*> scoutGroup;
	int needScoutNum;
	Position lastExplorPosition;
	//	std::set<Position> exploredPositionSet;
	std::set<Position> lockedPositionSet;
	BaseLocation* currentSearchTarget;
	int returnTimes;
	TerrainManager* terrainManager;
	bool needMoreScan;
	std::set<Position> previousPositionSet;
	std::set<Position> PreWarningPositionSet;
	Position bestPrewarningpo;//best position for prewarning
	Position ScoutUpPosition;
	Position ScoutDownPosition;
	Position ScoutLeftPosition;
	Position ScoutRightPosition;
	Position ScoutUpRightPosition;
	Position ScoutUpLeftPosition;
	Position ScoutDownRightPosition;
	Position ScoutDownLeftPosition;
	Position ScoutCenterPosition;

	bool _fSwitchRegion;
	BaseLocation* _nextTargetBase;
  int _lastSwitchRegionFrame;
  bool _fChangedRegion;

	//_T_
	int deadScoutUnitCount;

};