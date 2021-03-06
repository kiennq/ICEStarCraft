#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include "Common.h"
#include "UnitGroup.h"
#include "WorkerManager.h"
#include "InformationManager.h"
#include <set>
typedef BWTA::BaseLocation* bwtaBL;
typedef BWAPI::TilePosition* bwapiTP;

class WorkerManager;
class EnemyInfoManager;

class BaseClass
{
public:
	BaseClass(BWTA::BaseLocation* b);
	bwtaBL getBaseLocation();// get bwta::baselocation, that is ,bwtaBL mBaseLocation;
	const std::set<BWAPI::Unit*>& getMinerals() const;
	const std::set<BWAPI::Unit*>& getGeysers() const;
	//bool isNoMineral();
	//bool isNoGas();
	//bool isStartLocation();
	void onBaseCreate(BWAPI::Unit* unit);
	void update();
	void onBaseDestroy(BWAPI::Unit* unit);
	bool isMinedOut();
	int getNeedWorkerNum();//get workerNumForBase
	int getCurrentWorkerNum();
	int getOverWorkerNum();
	int getLackWorkerNum();
	void SetWorkerNumBalance();
	bool getNeedMoreWorker();
	std::map<BWAPI::Unit*,std::set<BWAPI::Unit*>> getWorkerNearBaseSet(); 
	void setWorkerconfig();
	int getGasWorkerNum();
	bool isProtected();
	UnitGroup protectors;
	void scvDefendBase();
	std::set<ICEStarCraft::EnemyUnit*> enemyToDefend;
	UnitGroup scvDefendTeam;

private:
	WorkerManager* worker;
	int overWorkerNum;
	int lackWorkerNum;
	bwtaBL mBaseLocation;
	bool NeedMoreWorker;
	int workerNumForBase;//how many workers are needed for this base
	int currentWorkerNum;//current worker near this base
	UnitSet mBaseSet;
	//my resource set
	//std::set<BWAPI::Unit*> mMineralSet;
	std::set<BWAPI::Unit*> mGeyserSet;
	std::set<BWAPI::Unit*> mRefineries;
	std::map<BWAPI::Unit*,std::set<BWAPI::Unit*>> workerNearBase; 
	TilePositionSet _allBaseTile;
	/*bool noMineral;
	bool noGas;
	bool mIsStartLocation;*/
	std::set<bwtaBL> allMyBaseLocations;
	bool mMinedOut;
	BWAPI::Unit* thisCommandCenter;
};