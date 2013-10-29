#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include <string>
#include <Arbitrator.h>
#include "Helper.h"
#include "InformationManager.h"
#include "MicroUnitControl.h"
#include "PFFunctions.h"
#include "TerrainManager.h"
#include "UnitGroupManager.h"

class AttackTarget;
class MyInfoManager;
class EnemyInfoManager;

class BattleManager : public Arbitrator::Controller<BWAPI::Unit*,double>
{
public:

	static BattleManager* create();
	static void destroy();

	void setArbitrator(Arbitrator::Arbitrator<BWAPI::Unit*,double>* arbitrator) {this->arbitrator = arbitrator;}

	std::string getName() const {return "BattleManager";}

	void onOffer(std::set<BWAPI::Unit*>);
	void onRevoke(BWAPI::Unit*, double);
	void update();
	void onUnitDestroy(BWAPI::Unit*);

	UnitGroup getMyUnits();
	UnitGroup getEnemyUnits();

	void showDebugInfo();

protected:

	BattleManager();
	~BattleManager();

private:

	EnemyInfoManager* eInfo;
	MyInfoManager* mInfo;
	TerrainManager* terrainManager;

	Arbitrator::Arbitrator<BWAPI::Unit*,double>* arbitrator;

	UnitGroup mUnits;
	UnitGroup eUnits;

	BWTA::Chokepoint* getNarrowChokepointInBattle(UnitGroup&,AttackTarget*);
};