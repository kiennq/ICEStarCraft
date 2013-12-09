#pragma once
#include <BWAPI.h>
#include <windows.h>
#include "Common.h"
#include "Base.h"
#include "BaseManager.h"
#include "BuildOrderManager.h"
#include "MentalState.h"
#include "GameFlow.h"
#include "InformationManager.h"

class BuildOrderManager;
class BaseClass;
class GameFlow;
class MentalClass;
class MyInfoManager;
class EnemyInfoManager;
typedef std::map<BWAPI::Unit*, std::pair<int,int>> ResourceToWorkerMap;//for each mineral, has score(times that being target) and distance
//typedef std::map<BWAPI::Unit*, int> ResourceToWorkerMap;
typedef std::map<BWAPI::Unit*, BWAPI::Unit*> WorkerToTargetMap;
typedef std::map<BWAPI::Unit*, std::pair<BWAPI::TilePosition, BWAPI::UnitType>> WorkerToBuildOrderMap;
typedef std::map<BWAPI::Unit*, BWAPI::Unit*> WorkerToTargetMap;
typedef std::map<BWAPI::Unit*, BWTA::BaseLocation*> WorkerToBaseMap;

class WorkerManager : public Arbitrator::Controller<BWAPI::Unit*,double>
{
public:

	class WorkerData
	{
	public:
		WorkerData() {resource = NULL; lastFrameSpam = 0;}
		BWAPI::Unit* resource;
		BWAPI::Unit* newResource;
		int lastFrameSpam;
	};
	enum State
	{
		Gathering_Mineral,
		Gathering_Gas,
		Building,
		Defending,
		Attacking,
		Scouting
	};

	static WorkerManager* create();
	static void destroy();
	
	void setArbitrator(Arbitrator::Arbitrator<BWAPI::Unit*,double>* arbitrator) {this->arbitrator = arbitrator;}

	std::string getName() const {return "WorkerManager";}

	void onOffer(std::set<BWAPI::Unit*>);
	void onRevoke(BWAPI::Unit*, double);
	void update();

	void setBaseManagerClass(BaseManager* bmc);
	void addUnit(BWAPI::Unit* unit);
	void onUnitDestroy(BWAPI::Unit* unit);
	void onFrame();
	bool needWorkers();
	void setNeedGasLevel(int level);
  int getNeedTotalWorkerNum();
	BWAPI::Unit* getWorkerForTask(BWAPI::Position toPosition);
	unsigned int getWorkersMining();
	void rebalanceGathering(); // TODO
	void microBalance();
	double getMineralRate() const;
	double getGasRate() const;
	
	UnitGroup _workerUnits;
	WorkerToTargetMap _workersTarget;
	ResourceToWorkerMap _mineralsExploitation;
	ResourceToWorkerMap _gasExploitation;
	std::map<BWAPI::Unit*, WorkerManager::State> _workerState;
	WorkerToBuildOrderMap _workerBuildOrder;
	WorkerToBaseMap _workersScout;
	
	int getOptimalWorkerCount() const;
	void enableAutoBuild();
	void disableAutoBuild();
	void setAutoBuildPriority(int priority);
	void setBuildOrderManager(BuildOrderManager* buildOrderManager);
	void setWorkerPerGas(int num);
	void autoBuildWorker();
	std::set<BWAPI::Unit*> selectSCV(int n);
	void workerRepair();
	void onUnitMorph(BWAPI::Unit* u);
	void onUnitHide(BWAPI::Unit* u);
	void onUnitDiscover(BWAPI::Unit* u);
	void autoTrainSCV();
	UnitGroup constructingSCV;

	UnitGroup getRepairList();
	bool isInRepairList(Unit*) const;

protected:
	//Single ton class
	WorkerManager();

private:

	Arbitrator::Arbitrator<BWAPI::Unit*,double>* arbitrator;

	int repairGroupSize;
	//ScoutManager* scoutManager;
	MyInfoManager* mInfo;
	EnemyInfoManager* eInfo;
	GameFlow* gf;
	MentalClass* mental;
	BWAPI::Unit* getBestLocalMineral(BaseClass* b);
	BWAPI::Unit* getBestGlobalMineral();
	BWAPI::Position getPositionToScout(BWAPI::Position seedPos, BWTA::Region* myRegion, BWAPI::Position basePos, bool checkVisible = false);
	void tryMiningTrick(BWAPI::Unit* worker);
	double mineralRate;
	double gasRate;
	int _lastFrameCount;
	int _accumluatedMinerals[61];	// We save the "accumulated minerals" for the last 60 seconds (initially, it's all 50s, since you start with 50 minerals)
	int _mineralPS, _mineralPM;
	double _averageMineralPS, _averageMineralPM;
	UnitSet _workerBuildingRefinery;
  std::map<BWAPI::Unit*,WorkerData> workers;
	BaseManager* bmc;
	bool autoBuild;
	int autoBuildPriority;
	int optimalWorkerCount;
	BuildOrderManager* buildOrderManager;
	int WorkersPerGas;
	int needTotalWorkerNum;
	int currentNum;
	int lastNum;
	bool rebalancing;
	std::set<BWAPI::Unit*> allMineral ;
	int lastRebalanceTime;
	UnitGroup repairGroup;
	UnitGroup repairList;
	UnitGroup scvDefendTeam;
	UnitGroup enemyToDefend;
};