#pragma once
#include <BWTA.h>
#include <BWAPI.h>
#include <string>
#include "ArmyManager.h"
#include "BaseDefenseManager.h"
#include "BattleManager.h"
#include "BuildOrderManager.h"
#include "DropManager.h"
#include "GameFlow.h"
#include "MineManager.h"
#include "MentalState.h"
#include "TerrainManager.h"
#include "UnitGroupManager.h"

class MacroManager : public Arbitrator::Controller<BWAPI::Unit*,double>
{
public:

	static MacroManager* create();
	static void destroy();

	void update();
	void onOffer(std::set<BWAPI::Unit*> units);
	void onRevoke(BWAPI::Unit* unit, double bid);

	void onFrame();
	void onUnitDiscover(BWAPI::Unit*);
	void onUnitDestroy(BWAPI::Unit*);

	void setManagers(BuildOrderManager*);
	void setArbitrator(Arbitrator::Arbitrator<BWAPI::Unit*,double>*);

	std::string getName() const {return "MacroManager";}

	UnitGroup getAttackers();
	UnitGroup getMyBattleUnits();
	UnitGroup getEnemyBattleUnits();
	AttackTarget*   getAttackTarget() const;
	BWAPI::Position getSetPoint()     const;
	BWAPI::Position getSiegePoint()   const;
	BWAPI::Position getGatherPoint()  const;

protected:

	MacroManager();
	~MacroManager();

private:

	Arbitrator::Arbitrator<BWAPI::Unit*,double>* arbitrator;
	BuildOrderManager* buildOrder;
	EnemyInfoManager* eInfo;
	MyInfoManager* mInfo;
	MentalClass* mental;
	TerrainManager* terrainManager;

	int lastScanFrame;
	bool mineralRemoved;
	BWAPI::TilePosition mineralPosition;
	UnitGroup workers;

	void scanInvisibleEnemies();
	void controlLiftedBuildings();
	void setRallyPoint(BWAPI::Position);
	void liftBuildingsNearChokepoints();
	void removeBlockingMinerals();
};