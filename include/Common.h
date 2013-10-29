#pragma once
#include <BWAPI.h>
#include <BWTA.h>
typedef std::set<BWAPI::Unit*> UnitSet;
typedef std::set<BWAPI::TilePosition> TilePositionSet;
typedef std::map<BWAPI::UnitType, int> UnitToPercent;


// Managers:
class InformationManager;
class WorkerManager;
class ProductionManager;
class BaseManager;
class SupplyManager;
class UnitGroupManager;

// Initialized in AIModule::onStart();
extern InformationManager* informationManager;
extern WorkerManager* workerManager;
extern ProductionManager* productionManager;
extern BaseManager* baseManager;
extern SupplyManager* supplyManager;
extern UnitGroupManager* unitGroupManager;
