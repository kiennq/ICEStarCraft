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
	this->attackTimingMap.clear();
	//three timing 
	//first is 3tanks,8 vultures + marine
	this->attackTimingMap[1]=false;
	//second is 6 factories, supply used>100
	this->attackTimingMap[2]=false;
	//last is supply > 170
	this->attackTimingMap[3]=false;
	this->mInfo = MyInfoManager::create();
	this->eInfo = EnemyInfoManager::create();
}

void TPTiming::CheckTiming()
{
	if (Broodwar->getFrameCount()%24 == 0)
	{
		int preAtkTime = (this->eInfo->killedEnemyNum - this->mInfo->myDeadArmy)*2*24;
		preAtkTime = preAtkTime > 0? preAtkTime : 0;
		int population = Broodwar->self()->supplyUsed()/2;
		int tank = Broodwar->self()->completedUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) + Broodwar->self()->completedUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode);
		int vulture = Broodwar->self()->completedUnitCount(UnitTypes::Terran_Vulture);
		int factory = Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory);
		double mFV = this->mInfo->myFightingValue().first;
		double mDV = this->mInfo->myFightingValue().second;
		double eFV = this->eInfo->enemyFightingValue().first;
		double eDV = this->eInfo->enemyFightingValue().second;
		bool eHasDK = (this->eInfo->EnemyhasBuilt(UnitTypes::Protoss_Dark_Templar,2)||this->eInfo->EnemyhasBuilt(UnitTypes::Protoss_Templar_Archives,2));
		bool mHasDetector = Broodwar->self()->completedUnitCount(UnitTypes::Terran_Comsat_Station) + Broodwar->self()->completedUnitCount(UnitTypes::Terran_Science_Vessel) > 0;
		//first timing slot
		if ((Broodwar->getFrameCount()>24*60*7-preAtkTime) && Broodwar->getFrameCount()<24*60*11+preAtkTime)
		{
			if (tank >= 3 && vulture >= 7 && mFV > eDV * 1.2)
			{
				if (eHasDK && mHasDetector)
				{
					this->attackTimingMap[1] = true;
				}
				else if (!eHasDK)
				{
					this->attackTimingMap[1] = true;
				}
			}
		}

		//times out
		if (this->attackTimingMap[1] == true && (Broodwar->getFrameCount() > 24*60*11 || mFV < eFV))
		{
			this->attackTimingMap[1] = false;
		}

		//_T_
		this->attackTimingMap[1] = false;

		//second timing slot
		if (Broodwar->getFrameCount() > 24*60*10 && Broodwar->getFrameCount()<24*60*14)
		{
			if (population >= 120 && factory >= 4 && mFV > 1.3 * eFV)
			{
				if (eHasDK && mHasDetector)
				{
					this->attackTimingMap[2] = true;
				}
				else if (!eHasDK)
				{
					this->attackTimingMap[2] = true;
				}
			}
		}

		//times out
		if (this->attackTimingMap[2] == true && mFV < eFV)
		{
			this->attackTimingMap[2] = false;
		}

		//third timing slot
		if (population >= 180 || Broodwar->getFrameCount() > 24*60*45)
		{
			this->attackTimingMap[3] = true;
		}
		
		//times out
		if (this->attackTimingMap[3] && mFV < eFV && population <= 150)
		{
			this->attackTimingMap[3] = false;
		}
	}
}