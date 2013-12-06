#pragma once
#include <Arbitrator.h>
#include <BWAPI.h>
#include "UnitItem.h"
#include "TechItem.h"
#include "Base.h"
#include "BaseManager.h"
#include "MentalState.h"

class BaseManager;
class BuildManager;
class TechManager;
class UpgradeManager;
class WorkerManager;
class SupplyManager;
class MentalClass;
//_T_
class MyInfoManager;
class EnemyInfoManager;
class ScoutManager;
class TerrainManager;

class BuildOrderManager
{
public:
  // The data for this priority level
	class PriorityLevel
	{
	public:
		std::set<TechItem> techs;
    // Map of the builder (factory and worker) and the desired unit.
    // Second map of the desired unit and its other info (number, buildPosition)
		std::map<BWAPI::UnitType, std::map<BWAPI::UnitType, UnitItem>> units;
	};
	class Resources
	{
	public:
		Resources() : minerals(0),gas(0) {}
		Resources(int m, int g) : minerals(m),gas(g) {}
		int minerals;
		int gas;
	};
	class MetaUnit
	{
	public:
		MetaUnit(BWAPI::Unit* unit);
		MetaUnit(int larvaSpawnTime);
		int nextFreeTime() const;
		int nextFreeTime(BWAPI::UnitType t) const;
		int nextFreeTime(BWAPI::TechType t) const;
		int nextFreeTime(BWAPI::UpgradeType t) const;
		BWAPI::UnitType getType() const;
		int getRemainingBuildTime() const;
		int getRemainingTrainTime() const;
		int getRemainingResearchTime() const;
		int getRemainingUpgradeTime() const;
		BWAPI::UpgradeType getUpgrade() const;
		bool hasBuildUnit() const;
		bool hasAddon() const;
		bool isBeingConstructed() const;
		bool isCompleted() const;
		bool isMorphing() const;
		bool isTraining() const;
		bool isUpgrading() const;
		BWAPI::Unit* unit;
		int larvaSpawnTime;
	};
	class Type
	{
	public:
		Type(BWAPI::UnitType t, MetaUnit* unit, int priority, int time=0) : unitType(t), unit(unit), priority(priority), time(time) {}
		Type(BWAPI::TechType t, MetaUnit* unit, int priority, int time=0) : techType(t), unit(unit), priority(priority), time(time) {}
		Type(BWAPI::UpgradeType t, MetaUnit* unit, int priority, int time=0) : upgradeType(t), unit(unit), priority(priority), time(time) {}
		BWAPI::UnitType unitType;
		BWAPI::TechType techType;
		BWAPI::UpgradeType upgradeType;
		MetaUnit* unit;
		int priority;
		int time;
	};

	BuildOrderManager(BuildManager* buildManager, TechManager* techManager, UpgradeManager* upgradeManager, WorkerManager* workerManager, SupplyManager* supplyManager);
	BuildOrderManager(BuildManager* buildManager, TechManager* techManager, UpgradeManager* upgradeManager, SupplyManager* supplyManager);
	BuildOrderManager(BuildManager* buildManager, SupplyManager* supplyManager,WorkerManager* workerManager);
	void update();
	std::string getName() const;
	void build(int count, BWAPI::UnitType t, int priority, BWAPI::TilePosition seedPosition=BWAPI::TilePositions::None, bool buildWithSpace = true);
	void buildAdditional(int count, BWAPI::UnitType t, int priority, BWAPI::TilePosition seedPosition=BWAPI::TilePositions::None);
	void deleteItem(BWAPI::UnitType t);
	void research(BWAPI::TechType t, int priority);
	void upgrade(int level, BWAPI::UpgradeType t, int priority);
	bool hasResources(BWAPI::UnitType t);
	bool hasResources(BWAPI::TechType t);
	bool hasResources(BWAPI::UpgradeType t);
	void spendResources(BWAPI::UnitType t);
	void spendResources(BWAPI::TechType t);
	void spendResources(BWAPI::UpgradeType t);
	void setBaseManagerClass(BaseManager* bmc);
	int getPlannedCount(BWAPI::UnitType t);
	int getPlannedCount(BWAPI::UnitType t, int minPriority);
	
	//_T_
	bool plannedTech(BWAPI::TechType t);

	void enableDependencyResolver();
	void setDebugMode(bool debugMode);
	std::set<BWAPI::UnitType> unitsCanMake(MetaUnit* builder, int time);
	std::set<BWAPI::TechType> techsCanResearch(MetaUnit* techUnit, int time);
	std::set<BWAPI::UpgradeType> upgradesCanResearch(MetaUnit* techUnit, int time);

	int nextFreeTime(const MetaUnit* unit);
	int nextFreeTime(BWAPI::UnitType t);
	int nextFreeTime(const MetaUnit* unit, BWAPI::UnitType t);
	int nextFreeTime(const MetaUnit* unit, BWAPI::TechType t);
	int nextFreeTime(const MetaUnit* unit, BWAPI::UpgradeType t);
	std::set<MetaUnit*> MetaUnitPointers;


	void deleteItem(BWAPI::UnitType t, int priority);
	void deleteItem(BWAPI::UnitType t,int priority, BWTA::Region* r);
	void adjustPriority(BWAPI::UnitType t,int variation);
	void autoExpand(int priority, int maxlimit);
	void onUnitDestroy(BWAPI::Unit* u);
	
private:
	bool hasResources(BWAPI::UnitType t, int time);
	bool hasResources(BWAPI::TechType t, int time);
	bool hasResources(BWAPI::UpgradeType t, int time);
	bool hasResources(std::pair<int, BuildOrderManager::Resources> res);
	std::pair<int, Resources> reserveResources(MetaUnit* builder, BWAPI::UnitType unitType);
	std::pair<int, Resources> reserveResources(MetaUnit* techUnit, BWAPI::TechType techType);
	std::pair<int, Resources> reserveResources(MetaUnit* techUnit, BWAPI::UpgradeType upgradeType);
	void reserveResources(std::pair<int, BuildOrderManager::Resources> res);
	void unreserveResources(std::pair<int, BuildOrderManager::Resources> res);
	bool updateUnits();
	void updatePlan();
	bool isResourceLimited();
	void removeCompletedItems(PriorityLevel* p);
	void debug(const char* text, ...);
	BuildManager* buildManager;
	TechManager* techManager;
	UpgradeManager* upgradeManager;
	WorkerManager* workerManager;
	SupplyManager* supplyManager;
	MentalClass* mental;
	
	//_T_
	std::map<BWAPI::TilePosition,int> plannedCommandCenter;
	MyInfoManager* myInfo;
	EnemyInfoManager* enemyInfo;
	ScoutManager* scoutManager;
	TerrainManager* terrainManager;

  // Map between priority and unit's other information
	std::map<int, PriorityLevel> items;
	int usedMinerals;
	int usedGas;
	std::list<MetaUnit> MetaUnits;
	std::map<int, Resources> reservedResources;
	std::set<MetaUnit*> reservedUnits;
	std::map<BWAPI::UnitType,int> currentlyPlannedCount;
	std::list<Type> savedPlan;
	bool dependencyResolver;
	bool isMineralLimited;
	bool isGasLimited;
	bool debugMode;
	int nextUpdateFrame;
	BaseManager* bmc;
	std::set<BWAPI::TilePosition> existBaseOrder;
	BWTA::BaseLocation* lastExpandLocation;
	BWTA::BaseLocation* planExpandLocation;
};