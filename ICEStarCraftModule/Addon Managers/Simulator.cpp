#include "Simulator.h"
#include "EnemyStandard.h"
#include <time.h>
#include <iostream>
using namespace BWAPI;
using namespace std;
Simulator::Simulator()
{
	pStandard = new ProtossStandard();
	simuCountdown = 0;
	eProductionAbility = 0;
	techPro = 0;
	armyPro = 0;
	bgPro = 0;
	pylonPro = 0;
	otherActionPro = 0;
	trainWorkerPro = 0;
	bgMax = 0;
	techflag = true;
	armyflag = true;
	bgflag = true;
	ePopulation = 0;
	eEconomy = 0;
	allBGTraining = true;
	eEconomyGrow = 0;
	ePopuGrow = 0;
	lastWorkerTrainingTime  = 0;
	workerFlag = true;
	trainWorkerTimes = 0;
	bgFinishTimeSet.clear();
	armyUnlockTimeSet.clear();
	techUnlockTimeSet.clear();
}

void Simulator::enemySimu(int frameCount,enemyRace eRace,allEnemyPlan ePlan, int simuNum)
{
	switch(eRace){
		case eProtoss:	
			ProtossSimu(frameCount,ePlan,simuNum);
			break;
		case eTerran:
			TerranSimu(frameCount,ePlan,simuNum);
			break;
		case eZerg:
			ZergSimu(frameCount,ePlan,simuNum);
			break;
		default:
			Broodwar->printf("can not get enemy race!");
			break;
	}
}

int Simulator::ProtossSimu(int frameCount,allEnemyPlan pPlan,int simuNum)
{
	int _simuNum = simuNum;
	int totalPopu = 0;
	int totalBG = 0;
	int averagePopu = 0;
	int averageBG = 0;

for (simuNum;simuNum>0;simuNum--)
{
	if (pPlan == pBalance) {}
	else if (pPlan == pEconomic)  pStandard->P_Economic(frameCount);
	else if (pPlan == pAggressive) {}
	else if (pPlan == pBalance) {}
	else if (pPlan == pTech) {}
	
	
	simuCountdown = pStandard->simuframe/24;
	eEconomy = 300;
	eProductionAbility = pStandard->baseBG;
	finishedBG = pStandard->baseBG;
	ePopulation = pStandard->basepopulation;
	eEconomyGrow = pStandard->moneyGrow;
	bgMax = pStandard->bglimitation;
	possibleAction getActionType;
	ePopuGrow = 0;
	armyflag = true;
	techflag = true;
	bgflag = true;
	workerFlag = true;
	techMax = 0;
	bgFinishTimeSet.clear();
	armyUnlockTimeSet.clear();
	techUnlockTimeSet.clear();
	for (simuCountdown;simuCountdown>0;simuCountdown--){
		eEconomy+= eEconomyGrow;
		if (eEconomy < 100) continue;
		else{
			//check if the building Gate is finished
			if (bgFinishTimeSet.size()>0 && bgFinishTimeSet.find(simuCountdown)!=bgFinishTimeSet.end()){
				finishedBG++;
				bgFinishTimeSet.erase(simuCountdown);
			}
			//check if army is produced
			if (armyUnlockTimeSet.size()>0 && armyUnlockTimeSet.find(simuCountdown)!=armyUnlockTimeSet.end()){
				armyflag=true;
				armyUnlockTimeSet.erase(simuCountdown);
			}
			//check if tech is unlocked
			if (techUnlockTimeSet.size()>0 && techUnlockTimeSet.find(simuCountdown)!= techUnlockTimeSet.end()&&techMax <=4 ){
				techflag = true;
				techUnlockTimeSet.erase(simuCountdown);
			}
			time_t t;
			srand((unsigned)time(&t));
			getActionType = doAction(pPlan);
			srand((unsigned)time(&t));
			if (getActionType == noMoney) continue;
			// rules for build pylon
			if (getActionType == buildPylon && needPylonNum>0){
				if (eEconomy<100)	continue;
				int j = needPylonNum;
				for (j;j>=0;j--){		
					 if(needPylonNum==0){
						armyflag = true;
						break;
					}
					else{
						eEconomy = eEconomy - 100;
						ePopuGrow = ePopuGrow - 8;
						needPylonNum = needPylonNum -1;
					}
				}
			}
			//rules for build Gate
			if (eProductionAbility >= bgMax && bgflag == true){
				bgflag = false;
			}
			
			if (getActionType == buildBG && eProductionAbility < bgMax){
				eEconomy= eEconomy - 150;
				eProductionAbility++;
				bgFinishTime();
			}
			//rules for training army
			if (getActionType == trainArmy && armyflag == true){
				int bgNum = finishedBG;
				int trainingBG = 0;
				for (bgNum;bgNum>0;bgNum--){
					if(eEconomy<110) break;
					eEconomy= eEconomy-110;
					trainingBG++;
				}
				ePopulation = ePopulation + 2*trainingBG;
				ePopuGrow += 2*trainingBG;
				needPylonNum = ePopuGrow/8;
				if (trainingBG == eProductionAbility)
					allBGTraining = true;
				else
					allBGTraining = false;
				armyflag = false;
				armyUnlockTime();
			}

			//rules for upgrade tech
			if (getActionType == updateTech && techflag == true){
				if(eEconomy > 156){
					eEconomy = eEconomy - 156; 
					techMax++;
					techflag = false;
				}
			}	

			if (getActionType == otherAction){
				if (eEconomy > 50){
					eEconomy = eEconomy - 50;
				}
			}
		}		
	}
	totalPopu+=ePopulation;
	totalBG+= eProductionAbility;
}
	averagePopu = totalPopu/_simuNum;
	averageBG = totalBG/_simuNum;
	Broodwar->printf("Enemy current population is about %d, BG number is about %d",averagePopu,averageBG);
	return averagePopu;
}

void Simulator::TerranSimu(int frameCount,allEnemyPlan tPlan, int simuNum)
{
	if(tPlan == tBalance) {}
	else if(tPlan == tTech) {}
	else if(tPlan == tAggressive) {}
	else if(tPlan == tEconomic) {}

}


void Simulator::ZergSimu(int frameCount,allEnemyPlan zPlan,int simuNum)
{
	if(zPlan == zBalance) {}
	else if(zPlan == z5DRush) {}
	else if(zPlan == zTech) {}
	else if(zPlan == zAggressive) {}
	else if(zPlan == zEconomic) {}

}

Simulator::possibleAction Simulator::doAction(allEnemyPlan ep)
{
	std::map<int,possibleAction> tempMap;
	tempMap.clear();
	possibleAction actionType = noAction;
	time_t t;
	switch(ep){
		case pEconomic:
			{	
				srand((unsigned)time(&t)); // time seed	
				techPro = 60+rand()%(70-60+1);
				srand((unsigned)time(&t));
				bgPro = 60+rand()%(70-60+1);
				srand((unsigned)time(&t));
				armyPro = 70+rand()%(85-70+1); 
				srand((unsigned)time(&t));
				trainWorkerPro = 80+rand()%(100-80+1);
				srand((unsigned)time(&t));
				otherActionPro = 0+rand()%(50-0+1);
				srand((unsigned)time(&t));
			}
			break;
		case pAggressive:
			{
				srand((unsigned)time(&t)); // time seed	
				techPro = 15+rand()%(25-15+1);
				srand((unsigned)time(&t));
				bgPro = 40+rand()%(70-40+1);
				srand((unsigned)time(&t));
				armyPro = 80+rand()%(95-80+1);
				srand((unsigned)time(&t));
				trainWorkerPro = 10+rand()%(20-10+1);
				srand((unsigned)time(&t));
				otherActionPro = 0+rand()%(10-0+1);
				srand((unsigned)time(&t));
			}
			break;
		case pTech:
			{
				srand((unsigned)time(&t)); // time seed	
				techPro = 65+rand()%(85-65+1);
				srand((unsigned)time(&t));
				bgPro = 30+rand()%(50-30+1);
				srand((unsigned)time(&t));
				armyPro = 80+rand()%(95-80+1);
				srand((unsigned)time(&t));
				trainWorkerPro = 10+rand()%(20-10+1);
				srand((unsigned)time(&t));
				otherActionPro = 0+rand()%(10-0+1);
				srand((unsigned)time(&t));
			}
			break;

	}

	if (needPylonNum > 0){
		pylonPro = 100;
		actionType = buildPylon;
		armyflag = false;
		return actionType;
	}

	if(techflag == false){
		srand((unsigned)time(&t));
		techPro = 20+rand()%(40-20+1);
		srand((unsigned)time(&t));
	}

	if(armyflag == false){
		armyPro = 0;
		srand((unsigned)time(&t));
		otherActionPro = 60+rand()%(80-60+1);
		srand((unsigned)time(&t));
	}

	if (bgflag == false){
		srand((unsigned)time(&t));
		bgPro = 0+rand()%(6-0+1);
		srand((unsigned)time(&t));
	}
	if (workerFlag == false)
	{
		trainWorkerPro = 0;
	}

	tempMap[armyPro]=trainArmy;
	tempMap[bgPro]=buildBG;
	tempMap[techPro]=updateTech;
	tempMap[otherActionPro]=otherAction;
	tempMap[trainWorkerPro]=trainWorker;
	int maxPro = 0;
	for (std::map<int,possibleAction>::iterator i= tempMap.begin();i!=tempMap.end();i++){
		if (maxPro == 0 || i->first>maxPro){
			maxPro = i->first;
			actionType = i->second;
		}
	}
	//check the money limitation
	if (eEconomy < 100) actionType = noMoney;
	else if (eEconomy < 50 && actionType == trainWorker) actionType = noMoney;
	else if (eEconomy < 110 && actionType == trainArmy)	actionType = noMoney;
	else if (eEconomy < 120 && actionType == otherAction)	actionType = noMoney;
	else if (eEconomy < 150 && actionType == buildBG)	actionType = noMoney;
	else if (eEconomy < 156 && actionType == updateTech)	actionType = noMoney;
	
	return actionType;
}

void Simulator::bgFinishTime()
{
	time_t t;
	srand((unsigned)time(&t)); // time seed	
	int randomNum = 10+rand()%(10-1+1);
	srand((unsigned)time(&t));
	int nextFinishTime=0;
	nextFinishTime = simuCountdown-40;
	if (nextFinishTime < 0) {
		while(randomNum){
			if (bgFinishTimeSet.find(randomNum)==bgFinishTimeSet.end()){
				nextFinishTime =randomNum;
				bgFinishTimeSet.insert(nextFinishTime);
				break;
		
			}
			else
				randomNum--;
		}	
	}// if simulation time is not enough, then randomly assign a finish time to it
	else
		bgFinishTimeSet.insert(nextFinishTime);
}

void Simulator::armyUnlockTime()
{
	time_t t;
	srand((unsigned)time(&t)); // time seed	
	int randomNum = 10+rand()%(10-1+1);
	srand((unsigned)time(&t));
	int nextUnlocktime=0;
	if(allBGTraining == true)
		nextUnlocktime = simuCountdown - 28;
	else
		nextUnlocktime = simuCountdown - 15;
	if(nextUnlocktime <0){
		while (randomNum)
		{
			if (armyUnlockTimeSet.find(randomNum)==armyUnlockTimeSet.end()){
				nextUnlocktime = randomNum;
				armyUnlockTimeSet.insert(nextUnlocktime);
				break;
			}
			else
				randomNum--;
		}
		
	} 	// if simulation time is not enough, then randomly assign a finish time to it
	else
		armyUnlockTimeSet.insert(nextUnlocktime);
}

void Simulator::techUnlockTime()
{
	int nextUnlocktime=0;
	nextUnlocktime = simuCountdown - 30;
	if(nextUnlocktime < 0)
		techflag = true;
	else
		techUnlockTimeSet.insert(nextUnlocktime);
}