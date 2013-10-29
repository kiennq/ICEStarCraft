#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include "BaseManager.h"
#include "Base.h"
#include "UnitGroup.h"
#include "UnitGroupManager.h"
#include "InformationManager.h"
#include "ScoutManager.h"
#include "MacroUnitControl.h"
#include "TerrainManager.h"
#include "WorkerManager.h"
#include "GameFlow.h"
#include "vsProtossRules.h"
#include "UpgradeManager.h"
#include "TechManager.h"
#include "Helper.h"

using namespace BWAPI;
using namespace BWTA;
using namespace std;

class MacroManager;
class ScoutManager;
class MyInfoManager;
class EnemyInfoManager;
class BuildOrderManager;
class TerrainManager;
class TPTiming;
class UpgradeManager;
class TechManager;
class GameFlow;

class MentalClass
{
public:
	static MentalClass* create();
	static void destroy();

	enum eRace
	{
		P,
		T,
		Z
	};

	enum eStrategyType
	{
		NotSure = 1,
		PrushZealot,
		PrushDragoon,
		PtechDK,
		PtechReaver,
		BeCareful,
		P2Base,
		PtechCarrier,
		ZrushZergling,
		Ztech,
		Zexpansion,
		TrushMarine,
		Ttech,
		Texpansion,
	};

	void setManagers(BuildOrderManager* b,UpgradeManager* upgradeMng,TechManager* tech);

	void onFrame();
	void onUnitDestroy(BWAPI::Unit* u);
	void onUnitEvade(BWAPI::Unit* u);
	void onUnitMorph(BWAPI::Unit* u);

	void setEnemyPlanFlag();
	void counterMeasure();
	void baseUnderAttack();
	bool getUnderAttackFlag();
	void upDateSightRange();
	UnitGroup enemyInSight;
	bool goAttack;
	void attackTimingCheck();
	void TvPAttackTiming();

	int baseSightRangeLimitation;
	eStrategyType STflag;
	bool marineRushOver;

	void showDebugInfo();

protected:

	MentalClass();

private:
		
	UpgradeManager* upgradeMng;
	MyInfoManager* myInfo;
	EnemyInfoManager* enemyInfo;
	BuildOrderManager* bom;
	ScoutManager* scm;
	bool openCheckFlag;
	MacroManager* macroManager;
	bool reactionFinish;
	TerrainManager* terrainManager;
	int	prushTimer1;
	bool mUnderAttack;
	bool emergencyFlag;
	WorkerManager* worker;
	GameFlow* gf;
	TPTiming* tpTiming;
	TechManager* techMng;

	//_T_
	int LastTimeEnemyAttacked;
};