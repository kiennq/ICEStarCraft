#pragma once
#include <Arbitrator.h>
#include <BWAPI.h>
#include <BWTA.h>
class BuildingPlacer;
class ConstructionManager;
class ProductionManager;
class BuildManager
{
public:
	BuildManager(Arbitrator::Arbitrator<BWAPI::Unit*,double>* arbitrator);
	~BuildManager();
	void update();
	std::string getName() const;
	BuildingPlacer* getBuildingPlacer() const;
	void onRemoveUnit(BWAPI::Unit* unit);
	bool build(BWAPI::UnitType type);
	bool build(BWAPI::UnitType type, bool forceNoAddon);
	bool build(BWAPI::UnitType type, BWAPI::TilePosition goalPosition);
	bool build(BWAPI::UnitType type, BWAPI::TilePosition goalPosition, bool forceNoAddon);
	int getPlannedCount(BWAPI::UnitType type) const;
	void deletePlannedItem(BWAPI::UnitType type, BWTA::Region* r);
	int getStartedCount(BWAPI::UnitType type) const;
	int getCompletedCount(BWAPI::UnitType type) const;
	void setBuildDistance(int distance);
	BWAPI::UnitType getBuildType(BWAPI::Unit* unit) const;
	void setDebugMode(bool debugMode);

private:
	Arbitrator::Arbitrator<BWAPI::Unit*,double>* arbitrator;
	BuildingPlacer* buildingPlacer;
	ConstructionManager* constructionManager;
	ProductionManager* productionManager;
	bool debugMode;
};