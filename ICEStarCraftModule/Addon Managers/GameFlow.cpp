#include "GameFlow.h"
#include "Config.h"

using namespace BWAPI;
using namespace BWTA;
using namespace std;

GameFlow *theGameFlow = NULL;

GameFlow* GameFlow::create()
{
	if (theGameFlow) return theGameFlow;
	else return theGameFlow = new GameFlow();
}

GameFlow::GameFlow()
{
	this->buildOrder = NULL;
	this->mInfo = NULL;
	this->eInfo = NULL;
	this->worker = NULL;
	this->lastFrameCheck = 0;
	this->stopGasTime =0 ;
	this->resumeGasTime = 0;
	this->stopGasFlag=false;
	this->scout = NULL;
	this->vulPri = 63;
	this->tankPri = 63;
	this->goliathPri = 62;
	this->terrainManager = NULL;
	this->bunkerPosition = NULL;
	this->upgradeManager = NULL;
	this->secondBaseTile = TilePositions::None;
	this->mental = NULL;
	this->mineral = 0;
	this->gas = 0;
	this->TurretTilePositions.clear();
  this->_debugMode = Config::i().DEBUG_GAME_FLOW();
}

void GameFlow::setManagers(BuildOrderManager* bom,UpgradeManager* um)
{
	this->bmc = BaseManager::create();
	this->terrainManager = TerrainManager::create();
	this->buildOrder = bom;
	this->mInfo = MyInfoManager::create();
	this->eInfo = EnemyInfoManager::create();
	this->worker = WorkerManager::create();
	this->scout = ScoutManager::create();
	this->upgradeManager = um;
	this->mental = MentalClass::create();
}

void GameFlow::stopGasTimeSlotSet(int time1,int time2,int stoplevel,int resumelevel)
{
	if (Broodwar->getFrameCount() - time2>24*5)
		return;
	if (Broodwar->getFrameCount()>=time1 && Broodwar->getFrameCount()<time2){
		this->worker->setNeedGasLevel(stoplevel);
		this->stopGasFlag =true;
	}
	if (Broodwar->getFrameCount()>=time2){
		this->worker->setNeedGasLevel(resumelevel);
		this->stopGasFlag =false;
	}
}

void GameFlow::onUnitDiscover(Unit* u)
{
}

void GameFlow::factoryAutoTrain(int limitV,int limitT,int limitG,int priV,int priT,int priG)
{
	if (Broodwar->getFrameCount()%(24*3)==0 && limitV!=0){
		if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Vulture,30)<limitV){
			this->buildOrder->buildAdditional(1,UnitTypes::Terran_Vulture,priV);
		}
	}
	if (Broodwar->getFrameCount()%(24*3)==0 && limitT!=0){
		if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,30)<limitT){
			this->buildOrder->buildAdditional(1,UnitTypes::Terran_Siege_Tank_Tank_Mode,priT);
		}
	}
	if (Broodwar->getFrameCount()%(24*3)==0 && limitG!=0){
		if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Goliath,30)<limitG){
			this->buildOrder->buildAdditional(1,UnitTypes::Terran_Goliath,priG);
		}
	}

	else
		return;
}

void GameFlow::factoryTrainSet(int numV,int numT,int numG,int priV,int priT,int priG)
{
	if (Broodwar->getFrameCount()%24*5==0){
		if (numV!=0 && this->buildOrder->getPlannedCount(UnitTypes::Terran_Vulture,30)<numV)
			this->buildOrder->build(numV,UnitTypes::Terran_Vulture,priV);
		if (numT!=0 && this->buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,30)<numT)
			this->buildOrder->build(numT,UnitTypes::Terran_Siege_Tank_Tank_Mode,priT);
		if (numG!=0 && this->buildOrder->getPlannedCount(UnitTypes::Terran_Goliath,30)<numG)
			this->buildOrder->build(numG,UnitTypes::Terran_Goliath,priG);

	}
	else
		return;
}

void GameFlow::onFrame()
{
	if (Broodwar->enemy()->getRace() == Races::Zerg)
	{
		onFrameTZ();
	}
	else if (Broodwar->enemy()->getRace() == Races::Terran)
	{
		onFrameTT();
	}
	else
	{
		onFrameTP();
	}

	//build Turret
	if (Broodwar->getFrameCount() >= Config::i().GF_TURRET_BUILD_TIME() && 
      Broodwar->getFrameCount()%(24*20) == 0 &&	
      Broodwar->enemy()->getRace() != Races::Zerg)
	{
		if (Broodwar->self()->allUnitCount(UnitTypes::Terran_Engineering_Bay) < 1 && this->buildOrder->getPlannedCount(UnitTypes::Terran_Engineering_Bay,65) < 1)
		{
			this->buildOrder->build(1,UnitTypes::Terran_Engineering_Bay,80);
		}
		else if (Broodwar->self()->allUnitCount(UnitTypes::Terran_Engineering_Bay) > 0 && Broodwar->getFrameCount()%(24*60) == 0)
		{
			Position cp = terrainManager->mSecondChokepoint->getCenter();
			if (SelectAll(UnitTypes::Terran_Missile_Turret).inRadius(32*6,cp).empty())
			{
				if (TurretTilePositions.empty() || TurretTilePositions.find(TilePosition(cp)) == TurretTilePositions.end())
				{
					if (SelectAllEnemy()(canAttack).not(isWorker).inRadius(32*6,cp).size() < 2)
					{
						this->buildOrder->buildAdditional(1,UnitTypes::Terran_Missile_Turret,80,TilePosition(cp));
						TurretTilePositions.insert(TilePosition(cp));
					}
				}
			}
			else
			{
				TurretTilePositions.erase(TilePosition(cp));
			}

			int need = 1;
			if (Broodwar->getFrameCount() > 24*60*12 && (eInfo->EnemyhasBuilt(UnitTypes::Terran_Dropship,2) || eInfo->EnemyhasBuilt(UnitTypes::Protoss_Shuttle,2)))
			{
				need = 2;
			}

			for each (Unit* u in Broodwar->self()->getUnits())
			{
				if (!u->isCompleted() || u->getType() != UnitTypes::Terran_Command_Center)
				{
					continue;
				}

				int num = SelectAll(UnitTypes::Terran_Missile_Turret).inRadius(32*10,u->getPosition()).size();
				if (u->getTilePosition() == Broodwar->self()->getStartLocation())
				{
					need *= 2;
				}
				
				if (num < need)
				{
					if (TurretTilePositions.empty() || TurretTilePositions.find(u->getTilePosition()) == TurretTilePositions.end())
					{
						if (SelectAllEnemy()(canAttack).not(isWorker).inRadius(32*12,u->getPosition()).size() < 2)
						{
							this->buildOrder->buildAdditional(need-num,UnitTypes::Terran_Missile_Turret,80,u->getTilePosition());
							TurretTilePositions.insert(u->getTilePosition());
						}
					}
				}
				else
				{
					TurretTilePositions.erase(u->getTilePosition());
				}
			}
		}
	}
}

void GameFlow::balanceArmyCombination()
{
	//if (Broodwar->getFrameCount()%24*3==0)
	//	Broodwar->printf("tank:%d,vulture:%d,goliath:%d",tankPri,vulPri,goliathPri);
	if (Broodwar->getFrameCount()%24*5==0)
	{
		int vultureNum = SelectAll()(isCompleted)(Vulture).size();
		int tankNum = SelectAll()(isCompleted)(Siege_Tank).size();
		if (vultureNum>=13 || tankNum >=6){
			if (vultureNum >= (int)(1.3*tankNum) ){
				if(vulPri>=tankPri){
					vulPri = tankPri - 1;
					this->buildOrder->adjustPriority(Vulture,vulPri - tankPri);
					this->buildOrder->build(3,UnitTypes::Terran_Machine_Shop,65);
				}		
				else
					return;
			}
			if (vultureNum < (int)(1.3*tankNum)){
				if (vulPri<tankPri && vulPri<63){				
					this->buildOrder->adjustPriority(Vulture,63-vulPri);
					vulPri = 63;
				}
				else if (vulPri<tankPri && vulPri>=63){
					vulPri = tankPri + 1 ;
					this->buildOrder->adjustPriority(Vulture,vulPri - tankPri);
				}
				else
					return;
			}
		}	
	}

}

void GameFlow::destroy()
{
	if (theGameFlow) delete theGameFlow;
}

//void GameFlow::TTonFrame()
//{
//	if (Broodwar->getFrameCount()%24*2==0){
//		//gas control
//		if (Broodwar->getFrameCount()%24*3==0 && this->myInfor->CountUnitNum(UnitTypes::Terran_Refinery,1)>0){
//			if(Broodwar->self()->gas()-Broodwar->self()->minerals()>100 && Broodwar->self()->gas()>200 && Broodwar->self()->minerals()<500)
//				this->worker->setWorkerPerGas(0);
//			else{
//				this->buildOrder->build(this->myInfor->CountUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Refinery,80);
//				if(this->myInfor->CountUnitNum(UnitTypes::Terran_Command_Center,1)>=3)
//					this->worker->setNeedGasLevel(3);
//				else
//					this->worker->setWorkerPerGas(3);
//			}
//
//		}
//
//		//gas control end-------------------
//		if (Broodwar->self()->supplyUsed()/2>100 && Broodwar->self()->minerals()>=2500 && Broodwar->self()->gas()<=200){
//			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100)<5)
//				this->buildOrder->autoExpand(100,5);
//		}
//		//for exceptions
//		if((Broodwar->getFrameCount()>24*60*5 && Broodwar->self()->supplyUsed()/2<40&&Broodwar->self()->minerals()>2000)){
//			//expand
//			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100)<4)
//				this->buildOrder->autoExpand(100,4);
//			//factory
//			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Factory,73)<7){
//				this->buildOrder->build(7,UnitTypes::Terran_Factory,73);
//			}
//			//add on
//			if (this->myInfor->CountUnitNum(UnitTypes::Terran_Factory,1)>=5&&this->buildOrder->getPlannedCount(UnitTypes::Terran_Machine_Shop,72)<3){
//				this->buildOrder->build(3,UnitTypes::Terran_Machine_Shop,72);
//			}
//			//army
//			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Goliath,70)<35){
//				this->buildOrder->build(35,UnitTypes::Terran_Goliath,70);
//			}
//			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,70)<35){
//				this->buildOrder->build(35,UnitTypes::Terran_Siege_Tank_Tank_Mode,70);
//			}
//
//		}
//		//stage 1-------------------------------------------------fixed opening
//		if (Broodwar->self()->supplyUsed()/2 ==12){
//			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Barracks,100)<1)
//				this->buildOrder->build(1,UnitTypes::Terran_Barracks,100);
//			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Refinery,99)<1&&this->myInfor->CountUnitNum(UnitTypes::Terran_Barracks,2)>0)
//				this->buildOrder->build(1,UnitTypes::Terran_Refinery,99);
//			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Marine,63)<3)
//				this->buildOrder->build(3,UnitTypes::Terran_Marine,63);
//		}
//		//if we have 2 marine(under training or already trained)
//		if (this->myInfor->CountUnitNum(UnitTypes::Terran_Marine,2)>=2 && Broodwar->getFrameCount()%24*2==0){
//			//expand 2nd base
//			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Factory,70)<2)
//				this->buildOrder->build(2,UnitTypes::Terran_Factory,70);
//			if (this->myInfor->CountUnitNum(UnitTypes::Terran_Factory,1)>=1){
//				if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Machine_Shop,69)<2)
//					this->buildOrder->build(2,UnitTypes::Terran_Machine_Shop,69);
//			}
//
//			//build two more marine
//			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Marine,63)<4){
//				this->buildOrder->buildAdditional(1,UnitTypes::Terran_Marine,68);
//			}				
//		}
//		if (this->myInfor->CountUnitNum(UnitTypes::Terran_Machine_Shop,1)>0){
//			if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode)&&!Broodwar->self()->isResearching(TechTypes::Tank_Siege_Mode)){
//				this->buildOrder->research(TechTypes::Tank_Siege_Mode,69);
//			}
//			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,68)<5){
//				this->buildOrder->build(5,UnitTypes::Terran_Siege_Tank_Tank_Mode,68);
//				Broodwar->printf("%d",this->buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,68));		
//			}
//		}
//	}
//}

void GameFlow::autoTrainArmy()
{
	mineral = Broodwar->self()->minerals();
	gas = Broodwar->self()->gas();
	//int line = 0;

	for each (UnitType type in UnitTypes::allUnitTypes())
	{
		if (type == UnitTypes::Terran_Command_Center)
		{
			if (this->buildOrder->getPlannedCount(type) - Broodwar->self()->allUnitCount(type) - Broodwar->self()->deadUnitCount(type) > 0 && Broodwar->self()->allUnitCount(type) < 3)
			{
				mineral -= type.mineralPrice();
				//Broodwar->drawTextScreen(5,10*(++line),"minus Unit:%s",type.getName().c_str());
			}
		}
		else
		{
			int minPriority = (type == UnitTypes::Terran_Siege_Tank_Tank_Mode) ? 65 : 100;
			if (this->buildOrder->getPlannedCount(type,minPriority) - Broodwar->self()->allUnitCount(type) - Broodwar->self()->deadUnitCount(type) > 0)
			{
				mineral -= type.mineralPrice();
				gas -= type.gasPrice();
			  //Broodwar->drawTextScreen(5,10*(++line),"minus Unit:%s",type.getName().c_str());
			}
		}
	}

	for each (TechType type in TechTypes::allTechTypes())
	{
		if (this->buildOrder->plannedTech(type) && !Broodwar->self()->hasResearched(type) && !Broodwar->self()->isResearching(type))
		{
			mineral -= type.mineralPrice();
			gas -= type.gasPrice();
			//Broodwar->drawTextScreen(5,10*(++line),"minus Tech:%s",type.getName().c_str());
		}
	}

	if (mineral <= 0 && gas <= 0)
	{
		return;
	}

	for each (Unit* u in SelectAll())
	{
		if (!u->getType().isBuilding() || !u->isCompleted())
		{
			continue;
		}

		if (u->isConstructing() || u->isTraining() || u->isLifted() || u->getHitPoints() < u->getType().maxHitPoints() / 3)
		{
			continue;
		}

		// Starport
		if (Broodwar->enemy()->getRace() == Races::Terran && u->getType() == UnitTypes::Terran_Starport)
		{
			if (u->getAddon() == NULL)
			{
				UnitType controltower = UnitTypes::Terran_Control_Tower;
				if (Broodwar->canMake(u,controltower) && mineral >= controltower.mineralPrice() && gas >= controltower.gasPrice() && !u->isUnderAttack())
				{
					u->buildAddon(controltower);
					mineral -= controltower.mineralPrice();
					gas -= controltower.gasPrice();
					//Broodwar->printf("Auto build %s",controltower.getName().c_str());
				}
			}
			else
			{
				UnitType battlecruiser = UnitTypes::Terran_Battlecruiser;
				if (Broodwar->canMake(u,battlecruiser) && mineral >= battlecruiser.mineralPrice() && gas >= battlecruiser.gasPrice())
				{
					u->train(battlecruiser);
					mineral -= battlecruiser.mineralPrice();
					gas -= battlecruiser.gasPrice();
					//Broodwar->printf("Auto train %s",battlecruiser.getName().c_str());
				}
			}
		}

		// Factory
		if (u->getType() == UnitTypes::Terran_Factory)
		{
			UnitType tank    = UnitTypes::Terran_Siege_Tank_Tank_Mode;
			UnitType tank_   = UnitTypes::Terran_Siege_Tank_Siege_Mode;
			UnitType vulture = UnitTypes::Terran_Vulture;
			UnitType goliath = UnitTypes::Terran_Goliath;

			if (Broodwar->canMake(u,vulture) && mineral >= vulture.mineralPrice() &&
				  eInfo->countUnitNum(UnitTypes::Protoss_Zealot) > eInfo->countUnitNum(UnitTypes::Protoss_Dragoon) + 5 &&
					mInfo->countUnitNum(vulture,2) < 24)
			{
				u->train(vulture);
				mineral -= vulture.mineralPrice();
				gas -= vulture.gasPrice();
				//Broodwar->printf("Auto train %s",vulture.getName().c_str());
			}
			else if (Broodwar->canMake(u,tank) && mineral >= tank.mineralPrice() && gas >= tank.gasPrice() && mInfo->countUnitNum(tank,2) + mInfo->countUnitNum(tank_,2) < 24)
			{
				u->train(tank);
				mineral -= tank.mineralPrice();
				gas -= tank.gasPrice();
				//Broodwar->printf("Auto train %s",tank.getName().c_str());
			}
			else if (Broodwar->canMake(u,goliath) && mineral >= goliath.mineralPrice() && gas >= goliath.gasPrice() && mInfo->countUnitNum(goliath,2) < 36)
			{
				u->train(goliath);
				mineral -= goliath.mineralPrice();
				gas -= goliath.gasPrice();
				//Broodwar->printf("Auto train %s",goliath.getName().c_str());
			}
			else if (Broodwar->canMake(u,vulture) && mineral >= vulture.mineralPrice() && mInfo->countUnitNum(vulture,2) < 24)
			{
				if (rand()%5 == 3 ||
						Broodwar->enemy()->getRace() == Races::Terran ||
						(Broodwar->enemy()->getRace() == Races::Protoss && Broodwar->getFrameCount() < 24*60*10 && SelectAll()(Siege_Tank).size() >= 5))
				{
					u->train(vulture);
					mineral -= vulture.mineralPrice();
					gas -= vulture.gasPrice();
					//Broodwar->printf("Auto train %s",vulture.getName().c_str());
				}
			}
		}
	}
}

void GameFlow::onFrameTZ()
{
	/*if (Broodwar->self()->supplyUsed()/2 > 60 && Broodwar->getFrameCount()%(24*10) == 120)
	{
		this->buildOrder->build(1,UnitTypes::Terran_Starport,120);
		this->buildOrder->build(1,UnitTypes::Terran_Control_Tower,115);
		this->buildOrder->build(1,UnitTypes::Terran_Science_Facility,115);
		this->buildOrder->build(3,UnitTypes::Terran_Science_Vessel,120);
		this->buildOrder->build(6,UnitTypes::Terran_Science_Vessel,95);
		if (!Broodwar->self()->hasResearched(TechTypes::Irradiate) && !this->buildOrder->plannedTech(TechTypes::Irradiate))
		{
			this->buildOrder->research(TechTypes::Irradiate,100);
		}
		if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Titan_Reactor) < 1 && this->upgradeManager->getPlannedLevel(UpgradeTypes::Titan_Reactor) < 1)
		{
			this->buildOrder->upgrade(1,UpgradeTypes::Titan_Reactor,95);
		}
	}*/

	if (Broodwar->getFrameCount()%(24*2) == 0)
	{
		//gas control
		if (Broodwar->getFrameCount()%(24*3) == 0 && this->mInfo->countUnitNum(UnitTypes::Terran_Refinery,1) > 0)
		{
			if(Broodwar->self()->gas()-Broodwar->self()->minerals() > 100 && Broodwar->self()->minerals() < 500 && Broodwar->self()->gas() > 200)
			{
				this->worker->setWorkerPerGas(0);
			}
			else
			{
				this->buildOrder->build(this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Refinery,80);
				this->worker->setNeedGasLevel(3);
				this->worker->setWorkerPerGas(3);
			}
		}

		//_T_
		// exceptions
		if(Broodwar->getFrameCount() > 24*60*5 && Broodwar->self()->minerals() > 1500)
		{
			//Broodwar->printf("too much money");
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100) < 4)
			{
				this->buildOrder->autoExpand(100,4);
			}

			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Factory,73) < 7)
			{
				this->buildOrder->build(7,UnitTypes::Terran_Factory,73);
			}

			// if enemy has built mutalisk
			if (this->eInfo->EnemyhasBuilt(UnitTypes::Zerg_Mutalisk,1) || this->eInfo->EnemyhasBuilt(UnitTypes::Zerg_Spire,2))
			{
				if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Starport,70) < 3 || this->mInfo->countUnitNum(UnitTypes::Terran_Starport,2) < 3)
				{
					this->buildOrder->build(3,UnitTypes::Terran_Starport,70);
					this->buildOrder->build(3,UnitTypes::Terran_Control_Tower,69);
				}

				if (this->mInfo->countUnitNum(UnitTypes::Terran_Valkyrie,2) < 12 && Broodwar->self()->gas() > 600)
				{
					this->buildOrder->build(12,UnitTypes::Terran_Valkyrie,71);
				}

				if (this->mInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode,2) < 12 && Broodwar->self()->gas() > 600)
				{
					this->buildOrder->build(12,UnitTypes::Terran_Siege_Tank_Tank_Mode,69);
				}
			}

			// if enemy has build lurker
			if (this->eInfo->EnemyhasBuilt(UnitTypes::Zerg_Lurker,1))
			{
				if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Starport,70) < 2)
				{
					this->buildOrder->build(2,UnitTypes::Terran_Starport,70);
					this->buildOrder->build(2,UnitTypes::Terran_Control_Tower,69);
				}

				if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Science_Vessel,85) < 2)
				{
					if (this->mInfo->countUnitNum(UnitTypes::Terran_Science_Facility,2) < 1)
					{
						this->buildOrder->build(1,UnitTypes::Terran_Science_Facility,85);
					}
					this->buildOrder->build(2,UnitTypes::Terran_Science_Vessel,85);
					this->buildOrder->build(5,UnitTypes::Terran_Science_Vessel,70);
					if (!this->buildOrder->plannedTech(TechTypes::Irradiate) && !Broodwar->self()->hasResearched(TechTypes::Irradiate))
					{
						this->buildOrder->research(TechTypes::Irradiate,80);
					}
					if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Titan_Reactor) < 1 && this->upgradeManager->getPlannedLevel(UpgradeTypes::Titan_Reactor) < 1)
					{
						this->buildOrder->upgrade(1,UpgradeTypes::Titan_Reactor,75);
					}
				}

				if (this->mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,2) < 5)
				{
					this->buildOrder->build(5,UnitTypes::Terran_Machine_Shop,75);
				}

				if (this->mInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode,2) < 30 && Broodwar->self()->gas() > 500)
				{
					this->buildOrder->buildAdditional(this->mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,1),UnitTypes::Terran_Siege_Tank_Tank_Mode,70);
				}
			}

			if (this->mInfo->countUnitNum(UnitTypes::Terran_Goliath,2) < 30 && Broodwar->self()->gas() > 300)
			{
				this->buildOrder->build(30,UnitTypes::Terran_Goliath,70);
			}

			if (this->mInfo->countUnitNum(UnitTypes::Terran_Vulture,2) < 15)
			{
				if (Broodwar->self()->gas() < 100)
				{
					this->buildOrder->build(15,UnitTypes::Terran_Vulture,70);
				}
				else
				{
					this->buildOrder->build(15,UnitTypes::Terran_Vulture,68);
				}
				if (!Broodwar->self()->hasResearched(TechTypes::Spider_Mines))
				{
					this->buildOrder->research(TechTypes::Spider_Mines,68);
				}
			}
		}

		//delete bunker
		if (Broodwar->getFrameCount() > 24*60*12 || Broodwar->self()->supplyUsed()/2 > 65)
		{
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Bunker) > 0)
			{
				this->buildOrder->deleteItem(UnitTypes::Terran_Bunker);
			}
		}

		//stage 1-------------------------------------------------fixed opening
		if (Broodwar->self()->supplyUsed()/2 == 10)
		{
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Barracks,100) < 1)
				this->buildOrder->build(1,UnitTypes::Terran_Barracks,100);
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Marine,63) < 1)
				this->buildOrder->build(2,UnitTypes::Terran_Marine,63);
		}

		if (this->mInfo->countUnitNum(UnitTypes::Terran_Marine,2) >= 1 && Broodwar->getFrameCount()%24 == 0)
		{
			//Broodwar->printf("stage 1");
			//expand 2nd base
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100) < 2)
			{
				if (this->mental->STflag && this->mental->STflag == MentalClass::ZrushZergling)
				{
					//Broodwar->printf("Zergling rush!");
					if (this->mInfo->countUnitNum(UnitTypes::Terran_Marine,1) >= 12)
					{
						this->buildOrder->autoExpand(100,2);
						this->buildOrder->build(1,UnitTypes::Terran_Refinery,99);
					}	
				}						
				else
				{
					this->buildOrder->autoExpand(2000,2);
					this->buildOrder->build(1,UnitTypes::Terran_Refinery,99);
				}
			}

			//build a bunker
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Bunker,2) < 2 && this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2) >= 2)
			{
				if (!this->bunkerPosition)
				{
					if (terrainManager->mNearestBase && terrainManager->mSecondChokepoint)
					{
						int x = (terrainManager->mNearestBase->getTilePosition().x() + terrainManager->mSecondChokepoint->getCenter().x() / 32) / 2;
						int y = (terrainManager->mNearestBase->getTilePosition().y() + terrainManager->mSecondChokepoint->getCenter().y() / 32) / 2;
						this->bunkerPosition = new TilePosition(x,y);
					}
				}
				
				if (this->bunkerPosition)
				{
					if (this->mental->STflag == MentalClass::ZrushZergling && this->mInfo->countUnitNum(UnitTypes::Terran_Bunker,1) > 0)
						this->buildOrder->build(2,UnitTypes::Terran_Bunker,72,*this->bunkerPosition);
					else
						this->buildOrder->build(1,UnitTypes::Terran_Bunker,72,*this->bunkerPosition);
				}	
			}

			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Marine,63) < 4)
			{
				this->buildOrder->buildAdditional(1,UnitTypes::Terran_Marine,68);
			}				
		}

		//opening over, stage 2 begin--------------------------------------------------build some advanced buildings
		//first check if all necessary buildings for opening are constructed
		if (this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2)>=2 && this->mInfo->countUnitNum(UnitTypes::Terran_Bunker,1) > 0)
		{
			//Broodwar->printf("stage 2");
			//build two factories, one with machine shop
			this->buildOrder->build(1,UnitTypes::Terran_Factory,100);
			//this->buildOrder->build(1,UnitTypes::Terran_Academy,70);
			this->buildOrder->build(1,UnitTypes::Terran_Academy,95);
			this->buildOrder->build(4,UnitTypes::Terran_Factory,65);
			//once one factory is finished
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1)>0){
				this->buildOrder->build(1,UnitTypes::Terran_Armory,72);					
				this->buildOrder->build(1,UnitTypes::Terran_Machine_Shop,71);
			}

			//once we have armory, upgrade and build 6 goliath
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Armory,2)>0)
			{
				this->buildOrder->build(1,UnitTypes::Terran_Engineering_Bay,87);
				if (this->upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons)<1)
					this->buildOrder->upgrade(1,UpgradeTypes::Terran_Vehicle_Weapons,67);
				if (Broodwar->self()->minerals()>180 && this->buildOrder->getPlannedCount(UnitTypes::Terran_Factory,65)<3){
					this->buildOrder->build(3,UnitTypes::Terran_Factory,68);
				}
				//this->buildOrder->build(6,UnitTypes::Terran_Goliath,67);
			}
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Armory,1)>0 && this->upgradeManager->getPlannedLevel(UpgradeTypes::Charon_Boosters)<1
				&&this->mInfo->countUnitNum(UnitTypes::Terran_Goliath,1)>=3 )
				this->buildOrder->upgrade(1,UpgradeTypes::Charon_Boosters,69);

			if (this->mInfo->countUnitNum(UnitTypes::Terran_Academy,1)>0)
				this->buildOrder->build(this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Comsat_Station,200);

			if (Broodwar->self()->supplyUsed()/2 > 80 || Broodwar->getFrameCount() > 24*60*10)
			{
				if (Broodwar->self()->allUnitCount(UnitTypes::Terran_Command_Center) < 3)
				{
					this->buildOrder->autoExpand(200,3);
				}
			}
		}

		//stage 2 over, stage 3 begin------------------------build more factories,and train army
		//first check buildings for stage 2
		if (this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1)>0 && this->mInfo->countUnitNum(UnitTypes::Terran_Armory,1)>0)
		{
			if (Broodwar->self()->minerals()>100 && 
				(this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2)<3 && this->buildOrder->getPlannedCount(UnitTypes::Terran_Goliath,65)<45) 
				&& Broodwar->getFrameCount()%24*5==0)
			{
				this->buildOrder->buildAdditional(this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1),UnitTypes::Terran_Goliath,67);
			}
			//build more factories
			if (Broodwar->self()->minerals()>180 && SelectAll()(isTraining)(Factory).size()>0 
				&& this->buildOrder->getPlannedCount(UnitTypes::Terran_Factory,65)<4				
				&& SelectAll()(isTraining)(Factory).size()== SelectAll()(isCompleted)(Factory).size()){
					//if (Broodwar->self()->minerals()>=320)
					//	this->buildOrder->buildAdditional(1,UnitTypes::Terran_Factory,70);
					//else
					this->buildOrder->buildAdditional(1,UnitTypes::Terran_Factory,68);
			}

			if (this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1)>=4 && this->mInfo->countUnitNum(UnitTypes::Terran_Goliath,1)>=10 &&
				Broodwar->self()->minerals()>200){
					if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Factory,65)<6)
						this->buildOrder->buildAdditional(1,UnitTypes::Terran_Factory,70);
			}
			//if finish upgrading Terran_Vehicle_Weapons, then continue upgrade
			if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons)>0 
				&& this->upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Plating)<1){
					this->buildOrder->upgrade(1,UpgradeTypes::Terran_Vehicle_Plating,68);
			}
			//if goliath > 30,expand 3rd base
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Goliath,2)>=30 || ((Broodwar->self()->supplyUsed()/2)>=105 && Broodwar->self()->minerals()>=500)
				|| Broodwar->self()->minerals()>=1500)
				this->buildOrder->autoExpand(100,4);
		}

		//stage 3 over, stage 4 begin----------------- build more advanced building and military, upgrade
		if((this->mInfo->countUnitNum(UnitTypes::Terran_Goliath,1)>=42 || Broodwar->self()->supplyUsed()/2>=120) &&
			this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1)>=5 && 
			this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2)>=3 &&
			Broodwar->self()->supplyUsed()/2<=160)
		{
			//Broodwar->printf("stage 4");
			if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons)==1 
				&& Broodwar->self()->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Plating)==1){
					if(this->upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons)<3)
						this->buildOrder->upgrade(1,UpgradeTypes::Terran_Vehicle_Weapons,70);
			}
			//delete vulture and marine from build order list
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Marine,40)>0){
				this->buildOrder->deleteItem(UnitTypes::Terran_Marine,63);
				this->buildOrder->deleteItem(UnitTypes::Terran_Marine,68);
			}
			//build scanner
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1)>=3 &&
				this->mInfo->countUnitNum(UnitTypes::Terran_Comsat_Station,1)< this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1))
				this->buildOrder->build(this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Comsat_Station,80);
			//research siege mode
			if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode))
				this->buildOrder->research(TechTypes::Tank_Siege_Mode,70);
			// we need some tanks 
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,2)<3){
				this->buildOrder->build(3,UnitTypes::Terran_Machine_Shop,69);
			}	

			if (Broodwar->self()->minerals()>100 && this->buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,65)<24
				&& Broodwar->getFrameCount()%24*5==0){
					this->buildOrder->buildAdditional(1,UnitTypes::Terran_Siege_Tank_Tank_Mode,67);
					//Broodwar->printf("%d",this->buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,65));
					//this->myInfor->CountUnitNum(UnitTypes::Terran_Factory,1)
			}

			if (Broodwar->self()->minerals()>100 && Broodwar->getFrameCount()%24*5==0 && 
				(this->mInfo->countUnitNum(UnitTypes::Terran_Goliath,1)<36)){
					this->buildOrder->build(36,UnitTypes::Terran_Goliath,68);
					//Broodwar->printf("%d",this->buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,65));
			}
			if (Broodwar->self()->minerals()>1500)
				this->buildOrder->autoExpand(100,5);

		}
		//stage 4 over, final stage begin----------------- upgrade all technique that we need,more army, more expansion
		if (this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2)>=2 && Broodwar->self()->supplyUsed()/2>160)
		{
			//Broodwar->printf("stage 4+");
			//keep training army , the remains money is used for upgrade and research, expand
			//SelectAll(isTraining)(Factory).size()>0 &&SelectAll(isCompleted)(Factory).size()-SelectAll(isTraining)(Factory).size()<=1 &&
			if (Broodwar->self()->minerals()>150)
			{
				if(this->mInfo->countUnitNum(UnitTypes::Terran_Factory,2)<8)
					this->buildOrder->build(8,UnitTypes::Terran_Factory,70);
				if(this->mInfo->countUnitNum(UnitTypes::Terran_Armory,2)<2)
					this->buildOrder->build(2,UnitTypes::Terran_Armory,70);
				if (this->upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons)<3)
					this->buildOrder->upgrade(3,UpgradeTypes::Terran_Vehicle_Weapons,70);
				if (this->upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons)<3)
					this->buildOrder->upgrade(3,UpgradeTypes::Terran_Vehicle_Plating,70);
				if(this->buildOrder->getPlannedCount(UnitTypes::Terran_Science_Vessel,85)<2)
					this->buildOrder->build(2,UnitTypes::Terran_Science_Vessel,85);
			}
			//expand more
			if (Broodwar->self()->supplyUsed()/2>170 || (Broodwar->self()->minerals()>1500 && Broodwar->self()->supplyUsed()/2>140))
			{
				if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100)<5)
					this->buildOrder->autoExpand(100,5);

				if (Broodwar->self()->supplyUsed()/2>180 && Broodwar->self()->minerals()>2500)
				{
					if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100)<6)
						this->buildOrder->autoExpand(100,6);				
				}					
			}
		}
	}
}

void GameFlow::onFrameTT()
{
	//_T_
	if (Broodwar->getFrameCount() > 24*60*8 && Broodwar->getFrameCount()%12 == 5)
	{
		autoTrainArmy();
	}

	if (Broodwar->getFrameCount()%(24*2) == 0)
	{
		//gas control
		if (Broodwar->getFrameCount()%(24*3)==0 && this->mInfo->countUnitNum(UnitTypes::Terran_Refinery,1)>0)
		{
			if(Broodwar->self()->gas()-Broodwar->self()->minerals()>100 && Broodwar->self()->gas()>200 && Broodwar->self()->minerals()<500)
				this->worker->setWorkerPerGas(0);
			else{
				this->buildOrder->build(this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Refinery,80);
				if(this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1)>=3)
					this->worker->setNeedGasLevel(3);
				else
					this->worker->setWorkerPerGas(3);
			}
		}
		
		if (Broodwar->self()->supplyUsed()/2>100 && Broodwar->self()->minerals()>=2500 && Broodwar->self()->gas()<=200){
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100)<5)
				this->buildOrder->autoExpand(100,5);
		}
		//for exceptions
		if((Broodwar->getFrameCount()>24*60*5 && Broodwar->self()->supplyUsed()/2<40&&Broodwar->self()->minerals()>2000)){
			//expand
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100)<4)
				this->buildOrder->autoExpand(100,4);
			//factory
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Factory,73)<7){
				this->buildOrder->build(7,UnitTypes::Terran_Factory,73);
			}
			//add on
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1)>=5&&this->buildOrder->getPlannedCount(UnitTypes::Terran_Machine_Shop,72)<3){
				this->buildOrder->build(3,UnitTypes::Terran_Machine_Shop,72);
			}
			//army
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Goliath,70)<35){
				this->buildOrder->build(35,UnitTypes::Terran_Goliath,70);
			}
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,70)<35){
				this->buildOrder->build(35,UnitTypes::Terran_Siege_Tank_Tank_Mode,70);
			}

		}
		//stage 1-------------------------------------------------fixed opening
		if (Broodwar->self()->supplyUsed()/2 >=7 && Broodwar->getFrameCount()<=24*60*5)
		{
			// Bunker rush
			//TilePosition mapCenter = TilePosition(Broodwar->mapWidth()/2,Broodwar->mapHeight()/2);
			////TilePosition(this->choke->mSecondChokepoint->getCenter())
			//if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Barracks,2000)<2)
			//	this->buildOrder->build(2,UnitTypes::Terran_Barracks,2000,mapCenter);
			//if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Marine,90)<8)
			//	this->buildOrder->build(8,UnitTypes::Terran_Marine,90);

			//if (this->scout->enemyStartLocation && SelectAll()(isCompleted)(Marine).size()>=2 &&
			//	  !this->eInfo->EnemyhasBuilt(UnitTypes::Terran_Vulture,1)	&&
			//		this->myInfor->myFightingValue().first > this->eInfo->enemyFightingValue().second &&
			//	  this->myInfor->CountUnitNum(UnitTypes::Terran_Command_Center,2)<2	&&
			//	  SelectAllEnemy()(isCompleted,canAttack).not(isWorker,isBuilding).size()<7)
			//{
			//	TilePosition eRegionCenter = TilePosition(BWTA::getRegion(this->scout->enemyStartLocation->getTilePosition())->getCenter());
			//	TilePosition eFirstChoke = TilePosition(this->choke->eFirstChokepoint->getCenter());
			//	TilePosition rushBK = TilePosition((eRegionCenter.x()+eFirstChoke.x())/2,(eRegionCenter.y()+eFirstChoke.y())/2);

			//	int see = this->buildOrder->getPlannedCount(UnitTypes::Terran_Bunker);
			//	if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Bunker,95)<1)					
			//		this->buildOrder->build(1,UnitTypes::Terran_Bunker,95,rushBK);
			//}
			
			//_T_
			// normal opening
			this->mental->marineRushOver = true;

			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Barracks,100) < 1 && Broodwar->self()->supplyUsed()/2 >= 10)
				this->buildOrder->build(1,UnitTypes::Terran_Barracks,130);

			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Refinery,99) < 1 && Broodwar->self()->supplyUsed()/2 >= 12)
				this->buildOrder->build(1,UnitTypes::Terran_Refinery,125);

			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Marine,70) < 4)
				this->buildOrder->build(4,UnitTypes::Terran_Marine,80);//_T_
		}

		//if we have 2 marine(under training or already trained)
		if (this->mInfo->countUnitNum(UnitTypes::Terran_Marine,2) >= 2 &&
			  this->mInfo->countUnitNum(UnitTypes::Terran_SCV,1) >= 15 &&
				Broodwar->getFrameCount()%(24*2) == 0)
		{
			//expand 2nd base
			if ((Broodwar->enemy()->deadUnitCount(UnitTypes::Terran_SCV) + Broodwar->enemy()->deadUnitCount(UnitTypes::Terran_Marine)) >= 12 && Broodwar->getFrameCount() <= 24*60*7.5)
				this->buildOrder->autoExpand(100,3);
			else
				this->buildOrder->autoExpand(100,2);
			
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Refinery,99) < 1 && Broodwar->self()->supplyUsed()/2 >= 12)
				this->buildOrder->build(1,UnitTypes::Terran_Refinery,99);

			//build a bunker			
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Bunker,2) == 0 && this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2) >= 2)
			{
				if (!this->bunkerPosition)
				{
					if (terrainManager->mNearestBase && terrainManager->mSecondChokepoint)
					{
						int x = (terrainManager->mNearestBase->getTilePosition().x() + terrainManager->mSecondChokepoint->getCenter().x() / 32) / 2;
						int y = (terrainManager->mNearestBase->getTilePosition().y() + terrainManager->mSecondChokepoint->getCenter().y() / 32) / 2;
						this->bunkerPosition = new TilePosition(x,y);
					}
				}
				
				if (this->bunkerPosition)
				{
					this->buildOrder->build(1,UnitTypes::Terran_Bunker,98,*this->bunkerPosition);
				}
			}

			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Marine,63) < 4 && this->mInfo->countUnitNum(UnitTypes::Terran_Factory,2)>1)
			{
				this->buildOrder->buildAdditional(1,UnitTypes::Terran_Marine,68);
			}				
		}

		//opening over, stage 2 begin--------------------------------------------------build some advanced buildings
		//first check if all necessary buildings for opening are constructed

		if (this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2) >= 2)
		{
			//build two factories, one with machine shop
			this->buildOrder->build(2,UnitTypes::Terran_Factory,68);

			//once one factory is finished
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1) >= 1)
			{
				if (SelectAll()(Siege_Tank).size() >= 2)
					this->buildOrder->build(1,UnitTypes::Terran_Armory,71);
				if (SelectAll()(Siege_Tank).size() >= 4)
					this->buildOrder->build(1,UnitTypes::Terran_Academy,70);
				this->buildOrder->build(2,UnitTypes::Terran_Machine_Shop,68);
			}

			if (SelectAll()(isCompleted)(Siege_Tank).size()>=3 && Broodwar->self()->gas()>350 && this->buildOrder->getPlannedCount(UnitTypes::Terran_Factory,65)<4)
			{
				this->buildOrder->build(5,UnitTypes::Terran_Factory,70);
			}
			//once we have armory
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Armory,2)>0)
			{
				if (this->upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons)<1)
					this->buildOrder->upgrade(1,UpgradeTypes::Terran_Vehicle_Weapons,76);
			}

			if (this->mInfo->countUnitNum(UnitTypes::Terran_Armory,1)>0 && this->upgradeManager->getPlannedLevel(UpgradeTypes::Charon_Boosters)<1)
				this->buildOrder->upgrade(1,UpgradeTypes::Charon_Boosters,67);
			
			if (Broodwar->self()->supplyUsed()/2>=50 && this->mInfo->countUnitNum(UnitTypes::Terran_Academy,1)>0)
				this->buildOrder->build(this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Comsat_Station,70);
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode,2)>=2)
				this->buildOrder->deleteItem(UnitTypes::Terran_Marine);
			//factoryTrainSet(3,0,0,66,0,0);
		}

		//stage 2 over, stage 3 begin------------------------build more factories,and train army
		//first check buildings for stage 2
		if (this->mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,1) > 0)
		{
			if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode) && !Broodwar->self()->isResearching(TechTypes::Tank_Siege_Mode))
			{
				this->buildOrder->research(TechTypes::Tank_Siege_Mode,69);
			}

			//train goliath
			if (Broodwar->self()->minerals()>100 && 
				  this->mInfo->countUnitNum(UnitTypes::Terran_Goliath,1) < 9 && SelectAll()(isCompleted)(Siege_Tank).size() >= 9 &&
					Broodwar->getFrameCount()%(24*5) == 0 && this->mInfo->countUnitNum(UnitTypes::Terran_Armory,1)>0)
			{
				this->buildOrder->build(9,UnitTypes::Terran_Goliath,69);
			}
			//train siege tank
			if (Broodwar->self()->minerals()>100 && 
				(this->buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,65)<25) 
				&& Broodwar->getFrameCount()%24*4==0)
			{
					this->buildOrder->buildAdditional(this->mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,1),UnitTypes::Terran_Siege_Tank_Tank_Mode,69);
			}

			//train vulture
			if (Broodwar->self()->minerals()>60&&
				(this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2)<3 && this->buildOrder->getPlannedCount(UnitTypes::Terran_Vulture,65)<20) 
				&& Broodwar->getFrameCount()%24==0 && Broodwar->self()->supplyUsed()/2<=150)
			{
				this->buildOrder->buildAdditional(this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1),UnitTypes::Terran_Vulture,69);
				if (!Broodwar->self()->hasResearched(TechTypes::Spider_Mines))
					this->buildOrder->research(TechTypes::Spider_Mines,69);
			}

			//build more factories
			if (Broodwar->self()->minerals()>180 && this->buildOrder->getPlannedCount(UnitTypes::Terran_Factory,65)<4				
				&& SelectAll()(isCompleted)(Siege_Tank).size()>=5)
			{
					this->buildOrder->buildAdditional(1,UnitTypes::Terran_Factory,68);
			}

			if (this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1)>=4 && Broodwar->self()->minerals()>200)
			{
				if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Factory,65)<5)
					this->buildOrder->buildAdditional(1,UnitTypes::Terran_Factory,70);
				if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Machine_Shop,65)<3)
					this->buildOrder->buildAdditional(1,UnitTypes::Terran_Machine_Shop,70);
			}
			//if finish upgrading Terran_Vehicle_Weapons, then continue upgrade
			if (!SelectAll()(Armory)(isCompleted)(isUpgrading)(RemainingUpgradeTime,"<",24*60*2).empty())
			{
				if (this->upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons)<3)
					this->buildOrder->upgrade(3,UpgradeTypes::Terran_Vehicle_Weapons,75);
			}
			//if we have advantages or too much money, then expand again
			if((Broodwar->self()->supplyUsed()/2 >= 100 && this->mInfo->myDeadArmy < this->eInfo->killedEnemyNum) ||
				 (Broodwar->self()->minerals() >= 1500 && Broodwar->self()->gas()<=300))
				this->buildOrder->autoExpand(100,5);
			else if (Broodwar->self()->supplyUsed()/2 > 80 || Broodwar->getFrameCount() > 24*60*10)
			{
				this->buildOrder->autoExpand(200,3);
			}
		}

		//stage 3 over, stage 4 begin----------------- build more advanced building and military, upgrade
		if(Broodwar->self()->supplyUsed()/2 >= 120 &&
			this->mInfo->countUnitNum(UnitTypes::Terran_Factory,2) >= 5 && 
			this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2)>=3 &&
			Broodwar->self()->supplyUsed()/2 <= 160)
		{
			if (Broodwar->self()->supplyUsed()/2 > 120 || Broodwar->getFrameCount() > 24*60*14)
			{
				this->buildOrder->autoExpand(200,4);
			}

			if (Broodwar->self()->supplyUsed()/2 > 140 || Broodwar->getFrameCount() > 24*60*18)
			{
				this->buildOrder->autoExpand(200,5);
			}

			//delete vulture and marine from build order list
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Marine,40)>0)
			{
				this->buildOrder->deleteItem(UnitTypes::Terran_Marine,63);
				this->buildOrder->deleteItem(UnitTypes::Terran_Marine,68);
			}
			//build scanner
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1)>=3 &&
				  this->mInfo->countUnitNum(UnitTypes::Terran_Comsat_Station,1)< this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1))
			{
				this->buildOrder->build(this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Comsat_Station,80);
			}
			//research siege mode
			if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode))
				this->buildOrder->research(TechTypes::Tank_Siege_Mode,70);
			// we need some tanks 
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,2)<3)
			{
				this->buildOrder->build(3,UnitTypes::Terran_Machine_Shop,69);
			}	

			if (this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1)<9)
			{
				this->buildOrder->build(9,UnitTypes::Terran_Factory,69);
			}

			if (Broodwar->self()->minerals()>100 && this->buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,65)<24
				&& Broodwar->getFrameCount()%24*5==0)
			{
				this->buildOrder->buildAdditional(1,UnitTypes::Terran_Siege_Tank_Tank_Mode,69);
			}

			if (Broodwar->self()->minerals() > 100 && Broodwar->getFrameCount()%(24*5) == 0 && this->mInfo->countUnitNum(UnitTypes::Terran_Goliath,1) < 25)
			{
				this->buildOrder->build(25,UnitTypes::Terran_Goliath,68);
			}

			if (Broodwar->self()->minerals()>1500 && this->buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100) < 5)
				this->buildOrder->autoExpand(100,5);

			//for produce battle cruiser , build star port first
			if (Broodwar->self()->supplyUsed()/2 >= 130 && SelectAll()(isCompleted)(Siege_Tank).size() >= 8 &&
				  this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1) >= 2)
			{
				if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Starport) < 5)
				{
					this->buildOrder->build(3,UnitTypes::Terran_Starport,110);
					this->buildOrder->build(5,UnitTypes::Terran_Starport,90);
				}
				if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Control_Tower)<5)
				{
					this->buildOrder->build(3,UnitTypes::Terran_Control_Tower,108);
					this->buildOrder->build(2,UnitTypes::Terran_Control_Tower,88);
				}		
			}

			//produce battle cruiser
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Starport,1) >= 2)
			{
				if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Battlecruiser) < 15)
				{
					this->buildOrder->build(5,UnitTypes::Terran_Battlecruiser,96);
					this->buildOrder->build(10,UnitTypes::Terran_Battlecruiser,94);
				}
				if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Science_Vessel) < 2)
				{
					this->buildOrder->build(2,UnitTypes::Terran_Science_Vessel,95);
				}
			}
			
			if (!SelectAll()(Physics_Lab)(isCompleted).empty())
			{
				// yamato gun research
				if (!Broodwar->self()->hasResearched(TechTypes::Yamato_Gun))
					this->buildOrder->research(TechTypes::Yamato_Gun,106);
				if (this->upgradeManager->getPlannedLevel(UpgradeTypes::Colossus_Reactor) < 1 && Broodwar->self()->hasResearched(TechTypes::Yamato_Gun))
				{
					this->buildOrder->upgrade(1,UpgradeTypes::Colossus_Reactor,105);
				}
			}	

			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Armory)<3)
				this->buildOrder->build(3,UnitTypes::Terran_Armory,99);
			if (this->upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Ship_Weapons)<3){
				this->buildOrder->upgrade(3,UpgradeTypes::Terran_Ship_Weapons,87);
			}
			if (this->upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Ship_Plating)<3){
				this->buildOrder->upgrade(3,UpgradeTypes::Terran_Ship_Plating,87);
			}	

			// produce some tanks
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,80)<15)
			{
				this->buildOrder->build(15,UnitTypes::Terran_Siege_Tank_Tank_Mode,80);
			}
		}

		//stage 4 over, final stage begin----------------- upgrade all technique that we need,more army, more expansion
		if (this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2) >= 2 && Broodwar->self()->supplyUsed()/2 > 160)
		{
			//keep training army , the remains money is used for upgrade and research, expand
			//SelectAll(isTraining)(Factory).size()>0 &&SelectAll(isCompleted)(Factory).size()-SelectAll(isTraining)(Factory).size()<=1 &&
			if (Broodwar->self()->minerals() > 150)
			{
				if (this->mInfo->countUnitNum(UnitTypes::Terran_Factory,2) < 8)
					this->buildOrder->build(8,UnitTypes::Terran_Factory,70);
				if (this->mInfo->countUnitNum(UnitTypes::Terran_Armory,2) < 2)
					this->buildOrder->build(2,UnitTypes::Terran_Armory,70);
				if (this->upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons) < 3)
					this->buildOrder->upgrade(3,UpgradeTypes::Terran_Vehicle_Weapons,70);
				if (this->upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons) < 3)
					this->buildOrder->upgrade(3,UpgradeTypes::Terran_Vehicle_Plating,70);
				if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Battlecruiser) < 15)
				{
					this->buildOrder->build(5,UnitTypes::Terran_Battlecruiser,96);
					this->buildOrder->build(10,UnitTypes::Terran_Battlecruiser,94);
				}
				if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Science_Vessel) < 2)
				{
					this->buildOrder->build(2,UnitTypes::Terran_Science_Vessel,95);
				}
			}
			//expand more
			if (Broodwar->self()->supplyUsed()/2>170 || (Broodwar->self()->minerals()>1500 &&Broodwar->self()->supplyUsed()/2>140))
			{
				if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100)<5)
					this->buildOrder->autoExpand(100,5);

				if (Broodwar->self()->supplyUsed()/2>180 && Broodwar->self()->minerals()>2500)
				{
					if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100)<7)
						this->buildOrder->autoExpand(100,7);				
				}					
			}
		}
	}
}

void GameFlow::onFrameTP()
{
	//_T_
	if (Broodwar->getFrameCount() > 24*60*5 && Broodwar->getFrameCount()%12 == 5)
	{
		autoTrainArmy();
	}

	//gas control
	if (Broodwar->getFrameCount()%(24*3) == 0 && this->mInfo->countUnitNum(UnitTypes::Terran_Refinery,1) > 0)
	{
		if(Broodwar->self()->gas()-Broodwar->self()->minerals() > 20 && Broodwar->self()->gas() > 170 && Broodwar->self()->minerals() < 500)
			this->worker->setWorkerPerGas(0);
		else if (this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2)<2 && Broodwar->self()->gas() >= 200)
			this->worker->setWorkerPerGas(0);
		else
		{
			this->worker->setNeedGasLevel(3);
			this->worker->setWorkerPerGas(3);
		}
	}

	//prevent mined out
	if (this->bmc->getBaseSet().size()>=2 && this->bmc->getAllMineralSet().size()<20 && Broodwar->self()->minerals()>300 && Broodwar->self()->supplyUsed()/2>120)
	{
		if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100)<this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1)+1)
		{
			this->buildOrder->autoExpand(100,this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1)+1);
		}
	}


	//_T_
	// 2 Factory

	//if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) == 1 && Broodwar->getFrameCount() < 24 * 60 * 12)
	//{
	//	int population = Broodwar->self()->supplyUsed()/2;

	//	if (population == 10)
	//	{
	//		this->buildOrder->build(1,UnitTypes::Terran_Barracks,500,this->choke->bbPos);
	//	}
	//	
	//	if (population == 12)
	//	{
	//		this->buildOrder->build(1,UnitTypes::Terran_Refinery,500);
	//	}
	//	
	//	if (population == 17)
	//	{
	//		this->buildOrder->build(1,UnitTypes::Terran_Factory,500);
	//	}

	//	if (population == 20)
	//	{
	//		this->buildOrder->build(2,UnitTypes::Terran_Factory,500);
	//	}

	//	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) == 1)
	//	{
	//		this->buildOrder->build(1,UnitTypes::Terran_Machine_Shop,500);
	//	}

	//	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Machine_Shop) == 1)
	//	{
	//		this->buildOrder->build(1,UnitTypes::Terran_Siege_Tank_Tank_Mode,500);
	//		this->buildOrder->build(1,UnitTypes::Terran_Bunker,480,this->choke->buPos);
	//		this->buildOrder->build(4,UnitTypes::Terran_Marine,450);
	//	}
	//	
	//	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) >= 2)
	//	{
	//		this->buildOrder->build(2,UnitTypes::Terran_Machine_Shop,500);
	//	}

	//	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Machine_Shop) >= 2)
	//	{
	//		if (SelectAll()(Siege_Tank).size() < 3)
	//		{
	//			this->buildOrder->build(3,UnitTypes::Terran_Siege_Tank_Tank_Mode,500);
	//			if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode) && !this->buildOrder->plannedTech(TechTypes::Tank_Siege_Mode))
	//			{
	//				this->buildOrder->research(TechTypes::Tank_Siege_Mode,490);
	//			}
	//		}
	//		else if (Broodwar->self()->allUnitCount(UnitTypes::Terran_Vulture) < 5)
	//		{					
	//			this->buildOrder->build(5,UnitTypes::Terran_Vulture,500);

	//			if (this->upgradeManager->getPlannedLevel(UpgradeTypes::Ion_Thrusters) < 1)
	//			{
	//				this->buildOrder->upgrade(1,UpgradeTypes::Ion_Thrusters,480);
	//			}

	//			if (!Broodwar->self()->hasResearched(TechTypes::Spider_Mines) && !this->buildOrder->plannedTech(TechTypes::Spider_Mines))
	//			{
	//				this->buildOrder->research(TechTypes::Spider_Mines,470);
	//			}
	//		}
	//	}

	//	if (SelectAll()(Siege_Tank,Vulture).size() >= 8)
	//	{
	//		if (SelectAll()(Siege_Tank,Vulture).size() >= 14)
	//		{
	//			this->buildOrder->autoExpand(200,2);
	//		}
	//		else
	//		{
	//			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) < 5)
	//			{
	//				//this->buildOrder->buildAdditional(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory),UnitTypes::Terran_Siege_Tank_Tank_Mode,500);
	//				this->buildOrder->buildAdditional(1,UnitTypes::Terran_Siege_Tank_Tank_Mode,500);
	//				//Broodwar->printf("build Tank %d",this->buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode));
	//			}

	//			this->buildOrder->build(9,UnitTypes::Terran_Vulture,500);
	//			
	//			if (Broodwar->self()->minerals() > 800 && Broodwar->self()->gas() < 100)
	//			{
	//				this->buildOrder->autoExpand(200,2);
	//			}
	//		}
	//	}
	//}//_T_
	
	//stage 1-------------------------------------------------fixed opening
	if (Broodwar->self()->supplyUsed()/2 >= 10 && Broodwar->getFrameCount() < 24*60*5)
	{
		if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Barracks,100) < 1)
			this->buildOrder->build(1,UnitTypes::Terran_Barracks,130,this->terrainManager->bbPos);

		if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Refinery,99) < 1 && Broodwar->self()->supplyUsed()/2 >= 12)
			this->buildOrder->build(1,UnitTypes::Terran_Refinery,125);

		if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Factory,80) < 1 && Broodwar->self()->supplyUsed()/2 >= 16)
			this->buildOrder->build(1,UnitTypes::Terran_Factory,120);

		if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Marine,70) < 4)
			this->buildOrder->build(4,UnitTypes::Terran_Marine,80);
	}
	
	if (this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1)>0)
	{
		if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Machine_Shop,100) < 1)//72
			this->buildOrder->build(1,UnitTypes::Terran_Machine_Shop,100);//72
		if (this->mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,1)>0)
		{
			if (!Broodwar->self()->hasResearched(TechTypes::Spider_Mines)&&this->mInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode,2)>0)
				this->buildOrder->research(TechTypes::Spider_Mines,74);
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode,2)<1)
				this->buildOrder->build(1,UnitTypes::Terran_Siege_Tank_Tank_Mode,72);
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Vulture,70)<6 &&this->mInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode,1)>0)
				this->buildOrder->build(4,UnitTypes::Terran_Vulture,70);
			if(SelectAll()(isCompleted)(Siege_Tank,Vulture).size()==5 &&this->mInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode,1)<3)
				this->buildOrder->build(3,UnitTypes::Terran_Siege_Tank_Tank_Mode,72);
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Marine,68)<6)
				this->buildOrder->buildAdditional(2,UnitTypes::Terran_Marine,68);
		}

		if ((this->mInfo->countUnitNum(UnitTypes::Terran_Vulture,2) >= 1 || this->eInfo->killedEnemyNum > 2) &&	this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1) < 2)
		{
			this->buildOrder->autoExpand(100,2);
		}
	}

	//stage2------------------------
	if (this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2) >= 2 && this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1) < 3)
	{		
		//build a bunker
		if (!this->bunkerPosition)
		{
			if (terrainManager->mNearestBase && terrainManager->mSecondChokepoint)
			{
				int x = (terrainManager->mNearestBase->getTilePosition().x() + terrainManager->mSecondChokepoint->getCenter().x() / 32) / 2;
				int y = (terrainManager->mNearestBase->getTilePosition().y() + terrainManager->mSecondChokepoint->getCenter().y() / 32) / 2;
				this->bunkerPosition = new TilePosition(x,y);
			}
		}

		if (Broodwar->self()->allUnitCount(UnitTypes::Terran_Bunker) < 1)
		{
			this->buildOrder->build(1,UnitTypes::Terran_Bunker,100,*this->bunkerPosition);
		}
		else if (Broodwar->getFrameCount() > 24*60*9 && Broodwar->getFrameCount() < 24*60*12 && mInfo->myFightingValue().first < eInfo->enemyFightingValue().first)
		{
			this->buildOrder->build(8,UnitTypes::Terran_Marine,80);
			this->buildOrder->build(2,UnitTypes::Terran_Bunker,80,*this->bunkerPosition);
		}

		//check if 2nd base already has bunker 
		/*std::set<Unit*> mBunker = SelectAll()(Bunker);
		for each (Unit* bk in mBunker)
		{
			if (bk->getTilePosition().getDistance(*this->bunkerPosition)<5)
				continue;
			else
			{
				if (this->mInfo->CountUnitNum(UnitTypes::Terran_Bunker,1) == 0)
					this->buildOrder->build(1,UnitTypes::Terran_Bunker,100,*this->bunkerPosition);
				else if(this->mInfo->CountUnitNum(UnitTypes::Terran_Bunker,1) == 1)
				{
					this->buildOrder->build(8,UnitTypes::Terran_Marine,80);
					this->buildOrder->build(2,UnitTypes::Terran_Bunker,100,*this->bunkerPosition);
				}
			}
		}*/

		//build more factories and military force
		if (Broodwar->self()->minerals()>100 && this->buildOrder->getPlannedCount(UnitTypes::Terran_Factory,65)<5)
		{
			//_T_
			if(this->mInfo->countUnitNum(UnitTypes::Terran_Factory,2)<3)
			{	
				this->buildOrder->build(2,UnitTypes::Terran_Factory,79);
				this->buildOrder->build(3,UnitTypes::Terran_Factory,68);
			}
			if (Broodwar->getFrameCount() >= 24*60*9)
			{
				if(this->mInfo->countUnitNum(UnitTypes::Terran_Factory,2)<4)
					this->buildOrder->build(4,UnitTypes::Terran_Factory,68);
			}	
			if (Broodwar->getFrameCount() >= 24*60*12)
			{
				if(this->mInfo->countUnitNum(UnitTypes::Terran_Factory,2)<5)
					this->buildOrder->build(5,UnitTypes::Terran_Factory,68);
			}			
		}
		if (this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1)>=1)//2
		{
			if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode) && this->mInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode,2) > 0)
			{
				if (Broodwar->getFrameCount() >= 24*60*7)
				{
					this->buildOrder->research(TechTypes::Tank_Siege_Mode,120);
				}
				else
				{
					this->buildOrder->research(TechTypes::Tank_Siege_Mode,80);//78
				}
			}
			if (Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode) && this->upgradeManager->getPlannedLevel(UpgradeTypes::Ion_Thrusters) < 1)
				this->buildOrder->upgrade(1,UpgradeTypes::Ion_Thrusters,68);
		}

		if (Broodwar->self()->minerals() > 40 && this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2)<3)
		{
			if(this->mInfo->countUnitNum(UnitTypes::Terran_Vulture,1) < 18 && this->buildOrder->getPlannedCount(UnitTypes::Terran_Vulture,65) < 18 && Broodwar->getFrameCount()%(24*5)==0)	
					this->buildOrder->buildAdditional(this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1),UnitTypes::Terran_Vulture,68);
			if(this->mInfo->countUnitNum(UnitTypes::Terran_Goliath,1) < 18 && this->buildOrder->getPlannedCount(UnitTypes::Terran_Goliath,65) < 18 && Broodwar->getFrameCount()%(24*5)==0)	
				this->buildOrder->buildAdditional(this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1),UnitTypes::Terran_Goliath,68);
			if(this->mInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode,1)<18&&this->buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,65)<18&& Broodwar->getFrameCount()%(24*7)==0)
				this->buildOrder->buildAdditional(this->mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,1),UnitTypes::Terran_Siege_Tank_Tank_Mode,68);
		}
		if (Broodwar->self()->supplyUsed()/2>60 && Broodwar->self()->minerals()>130 && this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1) >= 2)
		{
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Comsat_Station,68)<2)
			{
				this->buildOrder->build(1,UnitTypes::Terran_Academy,69);
				this->buildOrder->build(this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Comsat_Station,68);
			}
		}
		if (Broodwar->self()->supplyUsed()/2>50 && this->buildOrder->getPlannedCount(UnitTypes::Terran_Marine,60)>0)
			this->buildOrder->deleteItem(UnitTypes::Terran_Marine,70);

		if (this->mInfo->countUnitNum(UnitTypes::Terran_Factory,2)>=3 &&(Broodwar->self()->minerals()>180|| SelectAll()(isCompleted)(Siege_Tank,Vulture).size()>=12))
		{
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Factory,65)<6)
			{
				this->buildOrder->buildAdditional(1,UnitTypes::Terran_Factory,67);
			}			
		}

		if ((this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1)>=4))
		{
			if (Broodwar->self()->supplyUsed()/2>70 && this->mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,2)<3)
			{
				//Broodwar->printf("build more machine shops. state 2");
				this->buildOrder->build(3,UnitTypes::Terran_Machine_Shop,75);
			}
		}

		if (this->mInfo->countUnitNum(UnitTypes::Terran_Armory,2) < 1 && Broodwar->getFrameCount() > 24*60*8)
		{
			this->buildOrder->build(1,UnitTypes::Terran_Armory,71);
		}

		if (this->mInfo->countUnitNum(UnitTypes::Terran_Armory,1) > 0 && Broodwar->getFrameCount() > 24*60*9)
		{
			if (this->upgradeManager->getPlannedLevel(UpgradeTypes::Charon_Boosters) < 1 && this->mInfo->countUnitNum(UnitTypes::Terran_Goliath,1) > 0)
			{
				this->buildOrder->upgrade(1,UpgradeTypes::Charon_Boosters,68);
			}
			if (this->upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons) < 1)
			{
				this->buildOrder->upgrade(1,UpgradeTypes::Terran_Vehicle_Weapons,72);
			}
			if (this->upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Plating) < 1)
			{
				this->buildOrder->upgrade(1,UpgradeTypes::Terran_Vehicle_Plating,72);
			}
		}	

		if (this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1) >= 3 &&
			  Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode) &&
			  Broodwar->self()->supplyUsed()/2 > 75 &&
			  this->mInfo->myFightingValue().first > this->eInfo->enemyFightingValue().first	&&
			  this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2) < 3)
		{
			this->buildOrder->autoExpand(100,3);
		}
	}	

	//stage 3-----------------------------------------------------
	if (this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1) >= 3 && Broodwar->self()->supplyUsed()/2 <= 160)
	{
		//for scanner
		if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Comsat_Station,68)<3 && Broodwar->self()->supplyUsed()/2>100)
		{
			this->buildOrder->build(3,UnitTypes::Terran_Comsat_Station,68);
		}

		if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Factory,78) < 7)
		{
			this->buildOrder->build(7,UnitTypes::Terran_Factory,80);
		}

		//for machine shop ,produce more tanks
		if ((this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1)>=6))
		{
			if (Broodwar->self()->supplyUsed()/2>100 && this->mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,2)<5)
			{
				//Broodwar->printf("build more machine shops. state 3");
				this->buildOrder->build(5,UnitTypes::Terran_Machine_Shop,90);
			}
		}
		if (Broodwar->self()->minerals() > 70)
		{
			if(this->mInfo->countUnitNum(UnitTypes::Terran_Vulture,1)<12 && this->buildOrder->getPlannedCount(UnitTypes::Terran_Vulture,77)<12 && Broodwar->getFrameCount()%(24*5)==0)	
				this->buildOrder->buildAdditional(this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1),UnitTypes::Terran_Vulture,77);
			if(this->mInfo->countUnitNum(UnitTypes::Terran_Goliath,1)<30 && this->buildOrder->getPlannedCount(UnitTypes::Terran_Goliath,78)<30 && Broodwar->getFrameCount()%(24*6)==0)	
				this->buildOrder->buildAdditional(this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1),UnitTypes::Terran_Goliath,78);
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode,1)<30 && this->buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,65)<30 && Broodwar->getFrameCount()%(24*7)==0)	
				this->buildOrder->buildAdditional(this->mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,1),UnitTypes::Terran_Siege_Tank_Tank_Mode,78);
		}

		//_T_

		if (this->mInfo->countUnitNum(UnitTypes::Terran_Armory,2) < 1)
		{
			this->buildOrder->build(1,UnitTypes::Terran_Armory,71);
		}

		if (this->mInfo->countUnitNum(UnitTypes::Terran_Armory,1) > 0)
		{
			if (this->upgradeManager->getPlannedLevel(UpgradeTypes::Charon_Boosters) < 1 && this->mInfo->countUnitNum(UnitTypes::Terran_Goliath,1) > 0)
			{
				this->buildOrder->upgrade(1,UpgradeTypes::Charon_Boosters,68);
			}
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Science_Facility,82) < 1)
			{
				this->buildOrder->build(1,UnitTypes::Terran_Science_Facility,82);
			}
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Science_Facility,1) > 0)
			{
				if (this->upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons) < 2)
				{
					this->buildOrder->upgrade(2,UpgradeTypes::Terran_Vehicle_Weapons,82);
				}
				if (this->upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Plating) < 2)
				{
					this->buildOrder->upgrade(2,UpgradeTypes::Terran_Vehicle_Plating,82);
				}
			}
		}	

		if (this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1) >= 5 &&
			  this->mInfo->myFightingValue().first > this->eInfo->enemyFightingValue().first &&
			  this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2) < 4	&&
			  Broodwar->self()->supplyUsed()/2 > 100 &&
			  this->mental->enemyInSight.empty())
		{
			//Broodwar->printf("expand to 4th base");
			this->buildOrder->autoExpand(100,4);
		}
	}
	//stage 4-------------------------------------
	if (Broodwar->self()->supplyUsed()/2 > 160 && this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1) >= 4)
	{
		//for machine shop ,produce more tanks
		if (this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1) >= 6)
		{
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,2) < 5)
				this->buildOrder->build(5,UnitTypes::Terran_Machine_Shop,68);
		}

		if (this->mInfo->countUnitNum(UnitTypes::Terran_Armory,2) < 2)
		{
			this->buildOrder->build(2,UnitTypes::Terran_Armory,71);
		}

		if (this->mInfo->countUnitNum(UnitTypes::Terran_Armory,1) > 0)
		{
			if (this->upgradeManager->getPlannedLevel(UpgradeTypes::Charon_Boosters) < 1 && this->mInfo->countUnitNum(UnitTypes::Terran_Goliath,1) > 0)
			{
				this->buildOrder->upgrade(1,UpgradeTypes::Charon_Boosters,68);
			}
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Science_Facility,72) < 1)
			{
				this->buildOrder->build(1,UnitTypes::Terran_Science_Facility,72);
			}
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Science_Facility,1) > 0)
			{
				if (this->upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons) < 3)
				{
					this->buildOrder->upgrade(3,UpgradeTypes::Terran_Vehicle_Weapons,72);
				}
				if (this->upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Plating) < 3)
				{
					this->buildOrder->upgrade(3,UpgradeTypes::Terran_Vehicle_Plating,72);
				}
			}
		}
		
		//produce army
		if (Broodwar->self()->minerals()>100)
		{
			//vulture
			if(this->mInfo->countUnitNum(UnitTypes::Terran_Vulture,1)<24 && Broodwar->getFrameCount()%24*5==0)
			{
				if(this->buildOrder->getPlannedCount(UnitTypes::Terran_Vulture,65)<24)	
				{
					this->buildOrder->buildAdditional(this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1),UnitTypes::Terran_Vulture,70);
				}
			}

			//goliath
			if(this->mInfo->countUnitNum(UnitTypes::Terran_Armory,1)>0&&this->mInfo->countUnitNum(UnitTypes::Terran_Goliath,1)<24 && Broodwar->getFrameCount()%(24*8)==0)
			{
				if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Goliath,65)<24)
				{
					this->buildOrder->buildAdditional(this->mInfo->countUnitNum(UnitTypes::Terran_Factory,1),UnitTypes::Terran_Goliath,70);
				}
			}

			//tank
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode,2) + this->mInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Siege_Mode,2) < 24 && Broodwar->getFrameCount()%(24*9) ==0)
			{
				if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode) < 24)
				{
					this->buildOrder->buildAdditional(this->mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,1),UnitTypes::Terran_Siege_Tank_Tank_Mode,70);
				}
			}

			//Science_Vessel
			if (this->mInfo->countUnitNum(UnitTypes::Terran_Science_Vessel,2)<2 && Broodwar->self()->supplyUsed() >= 170)
			{
				if(this->buildOrder->getPlannedCount(UnitTypes::Terran_Science_Vessel,72)<2)
					this->buildOrder->build(2,UnitTypes::Terran_Science_Vessel,72);

				if (!this->buildOrder->plannedTech(TechTypes::EMP_Shockwave) && !Broodwar->self()->hasResearched(TechTypes::EMP_Shockwave))
				{
					this->buildOrder->research(TechTypes::EMP_Shockwave,72);
				}

				if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Titan_Reactor) < 1 && this->upgradeManager->getPlannedLevel(UpgradeTypes::Titan_Reactor) < 1)
				{
					this->buildOrder->upgrade(1,UpgradeTypes::Titan_Reactor,68);
				}
			}
		}
		//for scanner
		if (this->mInfo->countUnitNum(UnitTypes::Terran_Comsat_Station,2)<this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1)){
			this->buildOrder->build(this->mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Comsat_Station,73);
		}

		//if we have lots of money and don't expand
		if(Broodwar->getFrameCount()>=24*60*10 && Broodwar->self()->minerals()>3000)
		{
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100)<5)
				this->buildOrder->autoExpand(100,5);
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Factory,65)<4)
				this->buildOrder->build(4,UnitTypes::Terran_Factory,73);
		}
		//more factories and expansion if we are rich
		if (Broodwar->self()->supplyUsed()/2>170 || (Broodwar->self()->minerals()>1300 && Broodwar->self()->supplyUsed()/2>150)){
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100)<6)
				this->buildOrder->autoExpand(100,6);
			if (this->buildOrder->getPlannedCount(UnitTypes::Terran_Factory,65)<10){
				this->buildOrder->build(10,UnitTypes::Terran_Factory,73);
			}
		}
	}
}

void GameFlow::showDebugInfo()
{
	
  if (!_debugMode) return;
	// draw planned units info
	int x = 5;
	int y = 10;
	for each (UnitType type in UnitTypes::allUnitTypes())
	{
		if (type.getRace() == Broodwar->self()->getRace() && this->buildOrder->getPlannedCount(type) > 0)
		{
			Broodwar->drawTextScreen(x,y," %s : %d",type.getName().c_str(),this->buildOrder->getPlannedCount(type));
			y += 10;
		}
	}
	

	// draw resources info
	Broodwar->drawTextScreen(335,0,"\x07 Mineral:");
	Broodwar->drawTextScreen(375,0,"\x07 %d",mineral);
	Broodwar->drawTextScreen(335,10,"\x07 Gas:");
	Broodwar->drawTextScreen(375,10,"\x07 %d",gas);
}