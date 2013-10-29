#include <BWAPI.h>
#include "AllUsefulPlans.h"
#pragma once;

class ProtossStandard;

class Simulator
{
public:
	enum possibleAction{
		noAction,
		noMoney,
		buildBG,
		trainWorker,
		trainArmy,
		updateTech,
		buildPylon,
		otherAction
	};
	enum probability{
		veryHigh,
		high,
		medium,
		low,
		verLow
	};
	ProtossStandard* pStandard;
	Simulator();
	void onFrame();
	void enemySimu(int frameCount,enemyRace eRace,allEnemyPlan ePlan, int simuNum);
	int ProtossSimu(int frameCount,allEnemyPlan pPlan,int simuNum);
	void TerranSimu(int frameCount,allEnemyPlan tPlan,int simuNum);
	void ZergSimu(int frameCount,allEnemyPlan zPlan,int simuNum);
	void evaluate();
	void bgFinishTime();
	void armyUnlockTime();
	void techUnlockTime();
	possibleAction doAction(allEnemyPlan ep);
private:
	int simuCountdown;
	//enemy info
	int eProductionAbility;
	int techPro;
	int armyPro;
	int bgPro;
	int pylonPro;
	int needPylonNum;
	int otherActionPro;
	int trainWorkerPro;
	int bgMax;
	int techMax;
	int finishedBG;
	bool techflag;
	bool armyflag;
	bool bgflag;
	//int eTech;
	int ePopulation;
	int ePopuGrow;
	int eEconomy;
	bool allBGTraining;

	//int ePAGrow;
	//int eTechGrow;
	//int eArmyGrow;
	int eEconomyGrow;
	std::set<int> bgFinishTimeSet;
	std::set<int> armyUnlockTimeSet;
	std::set<int> techUnlockTimeSet;
	//my info
	int mProductionAbility;
	int mTech;
	int mArmy;
	int mEconomy;
	int mPAGrow;
	int mTechGrow;
	int mArmyGrow;
	int mEconomyGrow;
	int lastWorkerTrainingTime;
	int trainWorkerTimes;
	bool workerFlag;
	int techlevel;
};


