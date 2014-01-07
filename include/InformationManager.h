#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include "BaseManager.h"
#include "Base.h"
#include "UnitGroup.h"
#include "UnitGroupManager.h"
#include "ScoutManager.h"
#include "EnemyUnit.h"
#include "MicroUnitControl.h"

using namespace BWAPI;
using namespace BWTA;
using namespace std;
using namespace ICEStarCraft;

class BaseManager;
class ScoutManager;

class MyInfoManager
{
public:
	int myDeadArmy;
	int myDeadWorker;
	static MyInfoManager* create();
	static void destroy();
	std::pair<double,double> myFightingValue();
	void setUsefulManagers(BaseManager* baseManager);
	int getMyPopulation();
	int getMyBaseNum();
	int getMyArmyNum();
	int countUnitNum(UnitType uType,int complete);
	void onUnitDiscover(Unit* u);
	void onFrame();
	void onUnitDestroy(Unit* u);
	bool needScanNow;
	UnitGroup allMyFighter;

	//_T_
	int getDeadUnitCount();
	int getDeadWorkerCount();
	double attackValue();
	double defenseValue();
	void showDebugInfo();

protected:
	MyInfoManager();
private:
	set<Unit*> allmyUnits;
	BaseManager* bmc;
};

class EnemyInfoManager
{
public:
	int enemyDeadWorker;
	static EnemyInfoManager* create();
	static void destroy();
	void onFrame();
	void onUnitDiscover(Unit* u); 
	void onUnitMorph(BWAPI::Unit* u);
	void onUnitDestroy(Unit* u);
	void onUnitEvade(Unit* u);
	void showTypeToTimeMap();
	void showUnitToTypeMap();
	void showBuildingToPositionMap();
	void showBaseToDataMap();
	bool EnemyhasBuilt(UnitType uType,int n);
	pair<int,Position>  getLastSeenEnemyArmy(UnitGroup ug);
	int  getEnemyBaseNum();
	int  getKilledEnemyNum();
	void unitTypeInfer(Unit* u);
	int CountEunitNum(UnitType uType);
	int timeToDefendMutalisk(Unit* u);
	bool drRangeUpgradeFlag;
	std::pair<double,double> enemyFightingValue();
	void enemyMainBaseConfirm();
	bool eMainBaseCheck;

	
	class eBaseData
	{
	public:
		eBaseData();
		bool checkFinish;
		UnitType uType;
		double currentTime;
		double finishTime;
		int startBuildTime;
		double ProgressRate;
		BWTA::BaseLocation* base; //_T_
		TilePosition tPosition;
		Position position;
		bool isStartBase;
		bool isMainBase;
	};
	map<Unit*,eBaseData> getEnemyBaseMap();
	bool needScanNow;
	bool showTypeToTime;
	bool showUnitToType;
	bool showBuildingToPosition;
	bool showBaseToData;
	int killedEnemyNum;
	map<UnitType,pair<int,bool>> eUnitTypeToTimeMap;
	map<Unit*,UnitType> eUnitToUintTypeMap;
	map<Unit*,UnitType> allenemyFighter;
	map<Unit*,std::pair<UnitType,Position>> eBuildingPositionMap;
	map<Unit*,eBaseData> enemyBaseMap;
	MyInfoManager* mInfo;

	//_T_
	std::set<EnemyUnit*>& getAllEnemyUnits();
	EnemyUnit* getEnemyUnit(BWAPI::Unit*);
	bool isEnemyBase(BWTA::BaseLocation*);
	int countBaseNum();
	int countUnitNum(UnitType type, BWAPI::Position p = Positions::None, int radius = 12 * 32);
	int countDangerToAir(BWAPI::Position p, int radius = 12 * 32);
	int countDangerToGround(BWAPI::Position p, int radius = 12 * 32);
	int countDangerTotal(BWAPI::Position p, int radius = 12 * 32);

	int getDeadUnitCount();
	int getDeadWorkerCount();
	double attackValue();
	double defenseValue();
	void showDebugInfo();

protected:
	ScoutManager* scm;
	EnemyInfoManager();

private:

	std::set<EnemyUnit*> allEnemyUnits;
	int deadUnitCount;
	int deadWorkerCount;
};


bool hasUnitType(UnitType uType,Player* player);