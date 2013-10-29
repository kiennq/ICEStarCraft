#pragma once
#include "Base.h"
#include "Common.h"
#include <BWAPI.h>
#include <BWTA.h>
#include "BuildOrderManager.h"
#include "WorkerManager.h"
#include "ScoutManager.h"
class BuildOrderManager;
class ScoutManager;
class BaseClass;

class BaseManager
{
public:
	static BaseManager* create();
	static void destroy();
	void onUnitDestroy(BWAPI::Unit* u);
	//BaseClass* createBase(BWAPI::Unit* unit);
	void update();
	std::set<BWAPI::Unit*> getAllMineralSet();
	std::set<BWAPI::Unit*> getAllGeyserSet();
	std::set<BWAPI::Unit*> getMyMineralSet();
	std::set<BaseClass*> getBaseSet();
	std::map<BWTA::BaseLocation*,BWAPI::Unit*> getBLtoCCMap();
	bool getAllMineOutFlag();
	void expandPlan(); 
	std::set<BWTA::BaseLocation*> getPlanExpansionSet();
	void setManagers(BuildOrderManager* b);
	bool needProtection(BaseClass* bc);
	void sendArmyProtect(UnitGroup ug, BaseClass* bc);
	//int getNeedTotalWorkerNum();
	bool checkflag;
	bool EnemyOnBL;
	void ProtectArmy();
	UnitGroup protectGroup;

	ScoutManager* scm;
	
	BWTA::BaseLocation* enmeyStartLocation;
	std::set<BWTA::BaseLocation*> locationHasEnemy;

	BWTA::BaseLocation* getEnemyStartLocation();
	std::set<BWTA::BaseLocation*> getLocationHasEnemySet();

	std::set<BWAPI::Unit*> allMineralSet;
	std::set<BWAPI::Unit*> allGeyserSet;
protected:
	BaseManager();
private:
	bool allMineOut;
	std::map<BWTA::BaseLocation*,BaseClass*> BLtoBCMap;//baselocation to baseclass map
	std::map<BWTA::BaseLocation*,BWAPI::Unit*> BLtoCCMap;//baselocation to command center map
	std::set<BaseClass*>mAllBaseSet; //my all bases set
	unsigned int mineralSize;
	unsigned int geryserSize;
	
	std::set<BWTA::BaseLocation*> planedExpansionSet;
	std::set<BWTA::BaseLocation*> allBaseLocations;
	BWTA::BaseLocation* myStartBase;
	int counter;
	BuildOrderManager* bom;
	// int needTotalWorkerNum;
	BWAPI::UnitCommand* uc;
	int gruopcount;
	int ProtectGroupCount;
	//bool moveflag;
};