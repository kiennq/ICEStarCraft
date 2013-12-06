#include "vsProtossRules.h"
using namespace std;
using namespace BWAPI;
using namespace BWTA;

TPTiming* theTPTiming = NULL;

TPTiming* TPTiming::create()
{
	if (theTPTiming) return theTPTiming;
	return theTPTiming = new TPTiming();
}

void TPTiming::destroy(){
	if (theTPTiming) 
		delete theTPTiming;
}

TPTiming::TPTiming()
{
	attackTimingMap.clear();
	//three timing 
	//first is 3tanks,8 vultures + marine
	attackTimingMap[1]=false;
	//second is 6 factories, supply used>100
	attackTimingMap[2]=false;
	//last is supply > 170
	attackTimingMap[3]=false;
	mInfo = MyInfoManager::create();
	eInfo = EnemyInfoManager::create();
}

void TPTiming::CheckTiming()
{
	if (Broodwar->getFrameCount()%24 == 0)
	{
		int preAtkTime = (eInfo->killedEnemyNum - mInfo->myDeadArmy)*2*24;
		preAtkTime = preAtkTime > 0? preAtkTime : 0;
		int population = Broodwar->self()->supplyUsed()/2;
		int tank = Broodwar->self()->completedUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->completedUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode);
		int vulture = Broodwar->self()->completedUnitCount(UnitTypes::Terran_Vulture);
		int factory = Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory);
		double mFV = mInfo->myFightingValue().first;
		double mDV = mInfo->myFightingValue().second;
		double eFV = eInfo->enemyFightingValue().first;
		double eDV = eInfo->enemyFightingValue().second;
		bool eHasDK = (eInfo->EnemyhasBuilt(UnitTypes::Protoss_Dark_Templar,2) || eInfo->EnemyhasBuilt(UnitTypes::Protoss_Templar_Archives,2));
		bool mHasDetector = Broodwar->self()->completedUnitCount(UnitTypes::Terran_Comsat_Station) + Broodwar->self()->completedUnitCount(UnitTypes::Terran_Science_Vessel) > 0;
		
		// 1st timing slot
		if ((Broodwar->getFrameCount()>24*60*7-preAtkTime) && Broodwar->getFrameCount()<24*60*11+preAtkTime)
		{
			if (tank >= 3 && vulture >= 7 && mFV > eDV * 1.2)
			{
				if ((eHasDK && mHasDetector) || !eHasDK)
				{
					attackTimingMap[1] = true;
				}
			}
		}

		//times out
		if (attackTimingMap[1] == true && (Broodwar->getFrameCount() > 24*60*11 || mFV < eFV))
		{
			attackTimingMap[1] = false;
		}

		//_T_
		attackTimingMap[1] = false;

		// 2nd timing slot
		if (Broodwar->getFrameCount() > 24*60*10 && Broodwar->getFrameCount() < 24*60*14)
		{
			if (population >= 120 && factory >= 4 && mFV > 1.3 * eFV)
			{
				if ((eHasDK && mHasDetector) || !eHasDK)
				{
					attackTimingMap[2] = true;
				}
			}
		}

		//times out
		if (attackTimingMap[2] == true && mFV < eFV)
		{
			attackTimingMap[2] = false;
		}
		
		//_T_
		// 3rd timing slot
		if (Broodwar->getFrameCount() > 24*60*15 && Broodwar->getFrameCount() < 24*60*18)
		{
			if (population >= 160 && factory >= 5 && mFV > 1.3 * eFV)
			{
				attackTimingMap[3] = true;
			}
		}

		//times out
		if (attackTimingMap[3] == true && mFV < eFV)
		{
			attackTimingMap[3] = false;
		}

		// 4th timing slot
		if (population >= 180 || Broodwar->getFrameCount() > 24*60*45)
		{
			attackTimingMap[4] = true;
		}
		
		//times out
		if (attackTimingMap[4] && mFV < eFV && population <= 150)
		{
			attackTimingMap[4] = false;
		}
	}
}