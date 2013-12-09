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
	void factoryAutoTrain(int limitV,int limitT,int limitG,int priV,int priT,int priG);
	void factoryTrainSet(int numV,int numT,int numG,int priV,int priT,int priG);
	void balanceArmyCombination();
	TilePosition* bunkerPosition;
	TilePosition secondBaseTile;
	void TTonFrame();
	
	//_T_
	bool debug;
	void onFrameTP();
	void onFrameTZ();
	void onFrameTT();

	int mineral;
	int gas;

	std::set<BWAPI::TilePosition> TurretTilePositions;

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
	void autoTrainArmy();

protected:

	GameFlow();
};