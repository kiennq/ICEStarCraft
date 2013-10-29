#pragma once
#include <Arbitrator.h>
#include <BWAPI.h>
#include <BuildManager.h>
#include <BuildOrderManager.h>
#include <TerrainManager.h>

class TerrainManager;

class SupplyManager
{
public:
	static SupplyManager* create();
	static void destroy();

	void setBuildManager(BuildManager* buildManager);
	void setBuildOrderManager(BuildOrderManager* buildOrderManager);
	void update();
	std::string getName() const;
	int getPlannedSupply() const;
	int getSupplyTime(int supplyCount) const;
	void setSeedPosition(BWAPI::TilePosition p);
	BuildManager* buildManager;
	BuildOrderManager* buildOrderManager;
	int lastFrameCheck;
	BWAPI::TilePosition seedPosition;

protected:
	SupplyManager();
};