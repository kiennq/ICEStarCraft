#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include "BuildOrderManager.h"
#include "InformationManager.h"
#include "WorkerManager.h"
#include "ScoutManager.h"
#include "Base.h"
#include "BaseManager.h"
#include "TerrainManager.h"
#include "UpgradeManager.h"
#include "MentalState.h"

using namespace BWAPI;
using namespace BWTA;
using namespace std;

class MentalClass;
class BuildOrderManager;
class UpgradeManager;
class TerrainManager;

class GameFlow
{
public:

	static GameFlow* create();
	static void destroy();

	void onFrame();
	
	void setManagers(BuildOrderManager* bom,UpgradeManager* um);
	void stopGasTimeSlotSet(int time1,int time2,int stoplevel,int resumelevel);
	void onUnitDiscover(Unit* u);
	
	TilePosition* bunkerPosition;
	TilePosition secondBaseTile;
	
	//_T_
	bool debug;
	int mineral;
	int gas;

	void showDebugInfo();

private:

	int stopGasTime;
	int resumeGasTime;
	int lastFrameCheck;
	int vulPri;
	int tankPri;
	int goliathPri;
	bool stopGasFlag;
	
	BuildOrderManager* buildOrder;
	MyInfoManager* mInfo;
	EnemyInfoManager* eInfo;
	WorkerManager* worker;
	ScoutManager* scout;
	BaseManager* bmc;
	TerrainManager* terrainManager;
	UpgradeManager* upgradeManager;
	MentalClass* mental;

	//_T_
  std::set<BWAPI::TilePosition> TurretTilePositions;
	void onFrameTP();
  void onFrameTZ();
  void onFrameTT();
  void autoTrainArmy();
  void buildTurretsAroundMainBase();
  void buildTurretsInsideBases();

protected:

	GameFlow();
};