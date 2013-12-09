#include "GameFlow.h"
#include "Config.h"
#define _14CC_ false

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
	buildOrder = NULL;
	mInfo = NULL;
	eInfo = NULL;
	worker = NULL;
	lastFrameCheck = 0;
	stopGasTime =0 ;
	resumeGasTime = 0;
	stopGasFlag=false;
	scout = NULL;
	vulPri = 63;
	tankPri = 63;
	goliathPri = 62;
	terrainManager = NULL;
	bunkerPosition = NULL;
	upgradeManager = NULL;
	secondBaseTile = TilePositions::None;
	mental = NULL;
	mineral = 0;
	gas = 0;
	debug = Config::i().DEBUG_GAME_FLOW(); //false
	TurretTilePositions.clear();
}

void GameFlow::setManagers(BuildOrderManager* bom,UpgradeManager* um)
{
	bmc = BaseManager::create();
	terrainManager = TerrainManager::create();
	buildOrder = bom;
	mInfo = MyInfoManager::create();
	eInfo = EnemyInfoManager::create();
	worker = WorkerManager::create();
	scout = ScoutManager::create();
	upgradeManager = um;
	mental = MentalClass::create();
}

void GameFlow::stopGasTimeSlotSet(int time1,int time2,int stoplevel,int resumelevel)
{
	if (Broodwar->getFrameCount() - time2>24*5)
		return;
	if (Broodwar->getFrameCount()>=time1 && Broodwar->getFrameCount()<time2){
		worker->setNeedGasLevel(stoplevel);
		stopGasFlag =true;
	}
	if (Broodwar->getFrameCount()>=time2){
		worker->setNeedGasLevel(resumelevel);
		stopGasFlag =false;
	}
}

void GameFlow::onUnitDiscover(Unit* u)
{
}

void GameFlow::factoryAutoTrain(int limitV,int limitT,int limitG,int priV,int priT,int priG)
{
	if (Broodwar->getFrameCount()%(24*3)==0 && limitV!=0){
		if (buildOrder->getPlannedCount(UnitTypes::Terran_Vulture,30)<limitV){
			buildOrder->buildAdditional(1,UnitTypes::Terran_Vulture,priV);
		}
	}
	if (Broodwar->getFrameCount()%(24*3)==0 && limitT!=0){
		if (buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,30)<limitT){
			buildOrder->buildAdditional(1,UnitTypes::Terran_Siege_Tank_Tank_Mode,priT);
		}
	}
	if (Broodwar->getFrameCount()%(24*3)==0 && limitG!=0){
		if (buildOrder->getPlannedCount(UnitTypes::Terran_Goliath,30)<limitG){
			buildOrder->buildAdditional(1,UnitTypes::Terran_Goliath,priG);
		}
	}

	else
		return;
}

void GameFlow::factoryTrainSet(int numV,int numT,int numG,int priV,int priT,int priG)
{
	if (Broodwar->getFrameCount()%24*5==0){
		if (numV!=0 && buildOrder->getPlannedCount(UnitTypes::Terran_Vulture,30)<numV)
			buildOrder->build(numV,UnitTypes::Terran_Vulture,priV);
		if (numT!=0 && buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,30)<numT)
			buildOrder->build(numT,UnitTypes::Terran_Siege_Tank_Tank_Mode,priT);
		if (numG!=0 && buildOrder->getPlannedCount(UnitTypes::Terran_Goliath,30)<numG)
			buildOrder->build(numG,UnitTypes::Terran_Goliath,priG);

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
		if (Broodwar->self()->allUnitCount(UnitTypes::Terran_Engineering_Bay) < 1 && buildOrder->getPlannedCount(UnitTypes::Terran_Engineering_Bay,65) < 1)
		{
			buildOrder->build(1,UnitTypes::Terran_Engineering_Bay,80);
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
						buildOrder->buildAdditional(1,UnitTypes::Terran_Missile_Turret,80,TilePosition(cp));
						//Broodwar->printf("Build Turret at second choke point");
						TurretTilePositions.insert(TilePosition(cp));
					}
				}
			}
			else
			{
				TurretTilePositions.erase(TilePosition(cp));
			}

			for each (Unit* u in Broodwar->self()->getUnits())
			{
				if (!u->isCompleted() || u->getType() != UnitTypes::Terran_Command_Center)
				{
					continue;
				}
				int need = 1;
				if (Broodwar->getFrameCount() > 24*60*12 && (eInfo->EnemyhasBuilt(UnitTypes::Terran_Dropship,2) || eInfo->EnemyhasBuilt(UnitTypes::Protoss_Shuttle,2)))
				{
					need = 2;
				}
				if (u->getTilePosition() == Broodwar->self()->getStartLocation())
				{
					need *= 2;
				}
				int num = SelectAll(UnitTypes::Terran_Missile_Turret).inRadius(32*10,u->getPosition()).size();
				
				if (num < need)
				{
					if (TurretTilePositions.empty() || TurretTilePositions.find(u->getTilePosition()) == TurretTilePositions.end())
					{
						if (SelectAllEnemy()(canAttack).not(isWorker).inRadius(32*12,u->getPosition()).size() < 2)
						{
							buildOrder->buildAdditional(need-num,UnitTypes::Terran_Missile_Turret,80,u->getTilePosition());
							//Broodwar->printf("Build %d Turret at base (%d,%d)",need-num,u->getTilePosition().x(),u->getTilePosition().y());
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
					buildOrder->adjustPriority(Vulture,vulPri - tankPri);
					buildOrder->build(3,UnitTypes::Terran_Machine_Shop,65);
				}		
				else
					return;
			}
			if (vultureNum < (int)(1.3*tankNum)){
				if (vulPri<tankPri && vulPri<63){				
					buildOrder->adjustPriority(Vulture,63-vulPri);
					vulPri = 63;
				}
				else if (vulPri<tankPri && vulPri>=63){
					vulPri = tankPri + 1 ;
					buildOrder->adjustPriority(Vulture,vulPri - tankPri);
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

void GameFlow::autoTrainArmy()
{
	mineral = Broodwar->self()->minerals();
	gas = Broodwar->self()->gas();
	//int line = 0;

	for each (UnitType type in UnitTypes::allUnitTypes())
	{
		if (type == UnitTypes::Terran_Command_Center)
		{
			if (buildOrder->getPlannedCount(type) - Broodwar->self()->allUnitCount(type) - Broodwar->self()->deadUnitCount(type) > 0 && Broodwar->self()->allUnitCount(type) < 3)
			{
				mineral -= type.mineralPrice();
				//Broodwar->drawTextScreen(5,10*(++line),"minus Unit:%s",type.getName().c_str());
			}
		}
		else
		{
			int minPriority = (type == UnitTypes::Terran_Siege_Tank_Tank_Mode) ? 65 : 100;
			if (buildOrder->getPlannedCount(type,minPriority) - Broodwar->self()->allUnitCount(type) - Broodwar->self()->deadUnitCount(type) > 0)
			{
				mineral -= type.mineralPrice();
				gas -= type.gasPrice();
			  //Broodwar->drawTextScreen(5,10*(++line),"minus Unit:%s",type.getName().c_str());
			}
		}
	}

	for each (TechType type in TechTypes::allTechTypes())
	{
		if (buildOrder->plannedTech(type) && !Broodwar->self()->hasResearched(type) && !Broodwar->self()->isResearching(type))
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

	for each (Unit* u in Broodwar->self()->getUnits())
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
			else if (Broodwar->canMake(u,goliath) && mineral >= goliath.mineralPrice() && gas >= goliath.gasPrice() && mInfo->countUnitNum(goliath,2) < 24)
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
		buildOrder->build(1,UnitTypes::Terran_Starport,120);
		buildOrder->build(1,UnitTypes::Terran_Control_Tower,115);
		buildOrder->build(1,UnitTypes::Terran_Science_Facility,115);
		buildOrder->build(3,UnitTypes::Terran_Science_Vessel,120);
		buildOrder->build(6,UnitTypes::Terran_Science_Vessel,95);
		if (!Broodwar->self()->hasResearched(TechTypes::Irradiate) && !buildOrder->plannedTech(TechTypes::Irradiate))
		{
			buildOrder->research(TechTypes::Irradiate,100);
		}
		if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Titan_Reactor) < 1 && upgradeManager->getPlannedLevel(UpgradeTypes::Titan_Reactor) < 1)
		{
			buildOrder->upgrade(1,UpgradeTypes::Titan_Reactor,95);
		}
	}*/

	if (Broodwar->getFrameCount()%(24*2) == 0)
	{
		//gas control
		if (Broodwar->getFrameCount()%(24*3) == 0 && mInfo->countUnitNum(UnitTypes::Terran_Refinery,1) > 0)
		{
			if(Broodwar->self()->gas()-Broodwar->self()->minerals() > 100 && Broodwar->self()->minerals() < 500 && Broodwar->self()->gas() > 200)
			{
				worker->setWorkerPerGas(0);
			}
			else
			{
				buildOrder->build(mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Refinery,80);
				worker->setNeedGasLevel(3);
				worker->setWorkerPerGas(3);
			}
		}

		//_T_
		// exceptions
		if(Broodwar->getFrameCount() > 24*60*5 && Broodwar->self()->minerals() > 1500)
		{
			//Broodwar->printf("too much money");
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100) < 4)
			{
				buildOrder->autoExpand(100,4);
			}

			if (buildOrder->getPlannedCount(UnitTypes::Terran_Factory,73) < 7)
			{
				buildOrder->build(7,UnitTypes::Terran_Factory,73);
			}

			// if enemy has built mutalisk
			if (eInfo->EnemyhasBuilt(UnitTypes::Zerg_Mutalisk,1) || eInfo->EnemyhasBuilt(UnitTypes::Zerg_Spire,2))
			{
				if (buildOrder->getPlannedCount(UnitTypes::Terran_Starport,70) < 3 || mInfo->countUnitNum(UnitTypes::Terran_Starport,2) < 3)
				{
					buildOrder->build(3,UnitTypes::Terran_Starport,70);
					buildOrder->build(3,UnitTypes::Terran_Control_Tower,69);
				}

				if (mInfo->countUnitNum(UnitTypes::Terran_Valkyrie,2) < 12 && Broodwar->self()->gas() > 600)
				{
					buildOrder->build(12,UnitTypes::Terran_Valkyrie,71);
				}

				if (mInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode,2) < 12 && Broodwar->self()->gas() > 600)
				{
					buildOrder->build(12,UnitTypes::Terran_Siege_Tank_Tank_Mode,69);
				}
			}

			// if enemy has build lurker
			if (eInfo->EnemyhasBuilt(UnitTypes::Zerg_Lurker,1))
			{
				if (buildOrder->getPlannedCount(UnitTypes::Terran_Starport,70) < 2)
				{
					buildOrder->build(2,UnitTypes::Terran_Starport,70);
					buildOrder->build(2,UnitTypes::Terran_Control_Tower,69);
				}

				if (buildOrder->getPlannedCount(UnitTypes::Terran_Science_Vessel,85) < 2)
				{
					if (buildOrder->getPlannedCount(UnitTypes::Terran_Science_Facility,85) < 1)
					{
						buildOrder->build(1,UnitTypes::Terran_Science_Facility,85);
					}

					if (mInfo->countUnitNum(UnitTypes::Terran_Science_Facility,1) > 0)
					{
						buildOrder->build(2,UnitTypes::Terran_Science_Vessel,85);
						buildOrder->build(5,UnitTypes::Terran_Science_Vessel,70);
					
						if (!buildOrder->plannedTech(TechTypes::Irradiate) && !Broodwar->self()->hasResearched(TechTypes::Irradiate))
						{
							buildOrder->research(TechTypes::Irradiate,80);
						}
						if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Titan_Reactor) < 1 && upgradeManager->getPlannedLevel(UpgradeTypes::Titan_Reactor) < 1)
						{
							buildOrder->upgrade(1,UpgradeTypes::Titan_Reactor,75);
						}
					}
				}

				if (mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,2) < 5)
				{
					buildOrder->build(5,UnitTypes::Terran_Machine_Shop,75);
				}

				if (mInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode,2) < 30 && Broodwar->self()->gas() > 500)
				{
					buildOrder->buildAdditional(mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,1),UnitTypes::Terran_Siege_Tank_Tank_Mode,70);
				}
			}

			if (mInfo->countUnitNum(UnitTypes::Terran_Goliath,2) < 30 && Broodwar->self()->gas() > 300)
			{
				buildOrder->build(30,UnitTypes::Terran_Goliath,70);
			}

			if (mInfo->countUnitNum(UnitTypes::Terran_Vulture,2) < 15)
			{
				if (Broodwar->self()->gas() < 100)
				{
					buildOrder->build(15,UnitTypes::Terran_Vulture,70);
				}
				else
				{
					buildOrder->build(15,UnitTypes::Terran_Vulture,68);
				}
				if (!Broodwar->self()->hasResearched(TechTypes::Spider_Mines))
				{
					buildOrder->research(TechTypes::Spider_Mines,68);
				}
			}
		}

		//delete bunker
		if (Broodwar->getFrameCount() > 24*60*12 || Broodwar->self()->supplyUsed()/2 > 65)
		{
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Bunker) > 0)
			{
				buildOrder->deleteItem(UnitTypes::Terran_Bunker);
			}
		}

		//stage 1-------------------------------------------------fixed opening
		if (Broodwar->self()->supplyUsed()/2 == 10)
		{
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Barracks,100) < 1)
				buildOrder->build(1,UnitTypes::Terran_Barracks,100);
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Marine,63) < 1)
				buildOrder->build(2,UnitTypes::Terran_Marine,63);
		}

		if (mInfo->countUnitNum(UnitTypes::Terran_Marine,2) >= 1 && Broodwar->getFrameCount()%24 == 0)
		{
			//Broodwar->printf("stage 1");
			//expand 2nd base
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100) < 2)
			{
				if (mental->STflag && mental->STflag == MentalClass::ZrushZergling)
				{
					//Broodwar->printf("Zergling rush!");
					if (mInfo->countUnitNum(UnitTypes::Terran_Marine,1) >= 12)
					{
						buildOrder->autoExpand(100,2);
						buildOrder->build(1,UnitTypes::Terran_Refinery,99);
					}	
				}						
				else
				{
					buildOrder->autoExpand(2000,2);
					buildOrder->build(1,UnitTypes::Terran_Refinery,99);
				}
			}

			//build a bunker
			if (mInfo->countUnitNum(UnitTypes::Terran_Bunker,2) < 2 && mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2) >= 2)
			{
				if (!bunkerPosition)
				{
					if (terrainManager->mNearestBase && terrainManager->mSecondChokepoint)
					{
						int x = (terrainManager->mNearestBase->getTilePosition().x() + terrainManager->mSecondChokepoint->getCenter().x() / 32) / 2;
						int y = (terrainManager->mNearestBase->getTilePosition().y() + terrainManager->mSecondChokepoint->getCenter().y() / 32) / 2;
						bunkerPosition = new TilePosition(x,y);
					}
				}
				
				if (bunkerPosition)
				{
					if (mental->STflag == MentalClass::ZrushZergling && mInfo->countUnitNum(UnitTypes::Terran_Bunker,1) > 0)
						buildOrder->build(2,UnitTypes::Terran_Bunker,72,*bunkerPosition);
					else
						buildOrder->build(1,UnitTypes::Terran_Bunker,72,*bunkerPosition);
				}	
			}

			if (buildOrder->getPlannedCount(UnitTypes::Terran_Marine,63) < 4)
			{
				buildOrder->buildAdditional(1,UnitTypes::Terran_Marine,68);
			}				
		}

		//opening over, stage 2 begin--------------------------------------------------build some advanced buildings
		//first check if all necessary buildings for opening are constructed
		if (mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2)>=2 && mInfo->countUnitNum(UnitTypes::Terran_Bunker,1) > 0)
		{
			//Broodwar->printf("stage 2");
			//build two factories, one with machine shop
			buildOrder->build(1,UnitTypes::Terran_Factory,100);
			//buildOrder->build(1,UnitTypes::Terran_Academy,70);
			buildOrder->build(1,UnitTypes::Terran_Academy,95);
			buildOrder->build(4,UnitTypes::Terran_Factory,65);
			//once one factory is finished
			if (mInfo->countUnitNum(UnitTypes::Terran_Factory,1)>0){
				buildOrder->build(1,UnitTypes::Terran_Armory,72);					
				buildOrder->build(1,UnitTypes::Terran_Machine_Shop,71);
			}

			//once we have armory, upgrade and build 6 goliath
			if (mInfo->countUnitNum(UnitTypes::Terran_Armory,2)>0)
			{
				buildOrder->build(1,UnitTypes::Terran_Engineering_Bay,87);
				if (upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons)<1)
					buildOrder->upgrade(1,UpgradeTypes::Terran_Vehicle_Weapons,67);
				if (Broodwar->self()->minerals()>180 && buildOrder->getPlannedCount(UnitTypes::Terran_Factory,65)<3){
					buildOrder->build(3,UnitTypes::Terran_Factory,68);
				}
				//buildOrder->build(6,UnitTypes::Terran_Goliath,67);
			}
			if (mInfo->countUnitNum(UnitTypes::Terran_Armory,1)>0 && upgradeManager->getPlannedLevel(UpgradeTypes::Charon_Boosters)<1
				&&mInfo->countUnitNum(UnitTypes::Terran_Goliath,1)>=3 )
				buildOrder->upgrade(1,UpgradeTypes::Charon_Boosters,69);

			if (mInfo->countUnitNum(UnitTypes::Terran_Academy,1)>0)
				buildOrder->build(mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Comsat_Station,200);

			if (Broodwar->self()->supplyUsed()/2 > 80 || Broodwar->getFrameCount() > 24*60*10)
			{
				if (Broodwar->self()->allUnitCount(UnitTypes::Terran_Command_Center) < 3)
				{
					buildOrder->autoExpand(200,3);
				}
			}
		}

		//stage 2 over, stage 3 begin------------------------build more factories,and train army
		//first check buildings for stage 2
		if (mInfo->countUnitNum(UnitTypes::Terran_Factory,1)>0 && mInfo->countUnitNum(UnitTypes::Terran_Armory,1)>0)
		{
			if (Broodwar->self()->minerals()>100 &&
				  (mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2) < 3 && buildOrder->getPlannedCount(UnitTypes::Terran_Goliath,65) < 45) &&
					Broodwar->getFrameCount()%(24*5) == 0)
			{
				buildOrder->buildAdditional(mInfo->countUnitNum(UnitTypes::Terran_Factory,1),UnitTypes::Terran_Goliath,67);
			}

			//build more factories
			if (Broodwar->self()->minerals()>180 && SelectAll()(isTraining)(Factory).size()>0 &&
				  buildOrder->getPlannedCount(UnitTypes::Terran_Factory,65)<4 &&
				  SelectAll()(isTraining)(Factory).size()== SelectAll()(isCompleted)(Factory).size())
			{
				buildOrder->buildAdditional(1,UnitTypes::Terran_Factory,68);
			}

			if (mInfo->countUnitNum(UnitTypes::Terran_Factory,1)>=4 && mInfo->countUnitNum(UnitTypes::Terran_Goliath,1)>=10 &&
				Broodwar->self()->minerals()>200){
					if (buildOrder->getPlannedCount(UnitTypes::Terran_Factory,65)<6)
						buildOrder->buildAdditional(1,UnitTypes::Terran_Factory,70);
			}
			//if finish upgrading Terran_Vehicle_Weapons, then continue upgrade
			if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons)>0 
				&& upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Plating)<1){
					buildOrder->upgrade(1,UpgradeTypes::Terran_Vehicle_Plating,68);
			}
			//if goliath > 30,expand 3rd base
			if (mInfo->countUnitNum(UnitTypes::Terran_Goliath,2) >= 30 ||
				  ((Broodwar->self()->supplyUsed()/2) >= 105 && Broodwar->self()->minerals() >= 500) ||
					Broodwar->self()->minerals() >= 1500)
				buildOrder->autoExpand(100,4);
		}

		//stage 3 over, stage 4 begin----------------- build more advanced building and military, upgrade
		if((mInfo->countUnitNum(UnitTypes::Terran_Goliath,1)>=42 || Broodwar->self()->supplyUsed()/2>=120) &&
			mInfo->countUnitNum(UnitTypes::Terran_Factory,1)>=5 && 
			mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2)>=3 &&
			Broodwar->self()->supplyUsed()/2<=160)
		{
			//Broodwar->printf("stage 4");
			if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons)==1 && Broodwar->self()->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Plating)==1)
			{
				if (upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons)<3)
					buildOrder->upgrade(3,UpgradeTypes::Terran_Vehicle_Weapons,70);
			}
			//delete vulture and marine from build order list
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Marine,40)>0)
			{
				buildOrder->deleteItem(UnitTypes::Terran_Marine,63);
				buildOrder->deleteItem(UnitTypes::Terran_Marine,68);
			}
			//build scanner
			if (mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1)>=3 &&
				mInfo->countUnitNum(UnitTypes::Terran_Comsat_Station,1)< mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1))
				buildOrder->build(mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Comsat_Station,80);
			//research siege mode
			if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode))
				buildOrder->research(TechTypes::Tank_Siege_Mode,70);
			// we need some tanks 
			if (mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,2)<3){
				buildOrder->build(3,UnitTypes::Terran_Machine_Shop,69);
			}	

			if (Broodwar->self()->minerals()>100 && buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,65) < 24	&& Broodwar->getFrameCount()%(24*5) == 0)
			{
				buildOrder->buildAdditional(1,UnitTypes::Terran_Siege_Tank_Tank_Mode,67);
			}

			if (Broodwar->self()->minerals()>100 && Broodwar->getFrameCount()%24*5==0 && 
				(mInfo->countUnitNum(UnitTypes::Terran_Goliath,1)<36)){
					buildOrder->build(36,UnitTypes::Terran_Goliath,68);
					//Broodwar->printf("%d",buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,65));
			}
			if (Broodwar->self()->minerals()>1500)
				buildOrder->autoExpand(100,5);

		}
		//stage 4 over, final stage begin----------------- upgrade all technique that we need,more army, more expansion
		if (mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2)>=2 && Broodwar->self()->supplyUsed()/2>160)
		{
			//Broodwar->printf("stage 4+");
			if (Broodwar->self()->minerals()>150)
			{
				if(mInfo->countUnitNum(UnitTypes::Terran_Factory,2)<8)
					buildOrder->build(8,UnitTypes::Terran_Factory,70);
				if(mInfo->countUnitNum(UnitTypes::Terran_Armory,2)<2)
					buildOrder->build(2,UnitTypes::Terran_Armory,70);
				if (upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons)<3)
					buildOrder->upgrade(3,UpgradeTypes::Terran_Vehicle_Weapons,70);
				if (upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons)<3)
					buildOrder->upgrade(3,UpgradeTypes::Terran_Vehicle_Plating,70);
				if(buildOrder->getPlannedCount(UnitTypes::Terran_Science_Vessel,85)<2)
					buildOrder->build(2,UnitTypes::Terran_Science_Vessel,85);
			}
			//expand more
			if (Broodwar->self()->supplyUsed()/2>170 || (Broodwar->self()->minerals()>1500 && Broodwar->self()->supplyUsed()/2>140))
			{
				if (buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100)<5)
					buildOrder->autoExpand(100,5);

				if (Broodwar->self()->supplyUsed()/2>180 && Broodwar->self()->minerals()>2500)
				{
					if (buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100)<6)
						buildOrder->autoExpand(100,6);				
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
		if (Broodwar->getFrameCount()%(24*3)==0 && mInfo->countUnitNum(UnitTypes::Terran_Refinery,1)>0)
		{
			if(Broodwar->self()->gas()-Broodwar->self()->minerals()>100 && Broodwar->self()->gas()>200 && Broodwar->self()->minerals()<500)
			{
				worker->setWorkerPerGas(0);
			}
			else
			{
				buildOrder->build(mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Refinery,80);
				if(mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1)>=3)
					worker->setNeedGasLevel(3);
				else
					worker->setWorkerPerGas(3);
			}
		}
		
		if (Broodwar->self()->supplyUsed()/2>100 && Broodwar->self()->minerals()>=2500 && Broodwar->self()->gas()<=200)
		{
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100)<5)
				buildOrder->autoExpand(100,5);
		}
		//for exceptions
		if((Broodwar->getFrameCount()>24*60*5 && Broodwar->self()->supplyUsed()/2<40&&Broodwar->self()->minerals()>2000))
		{
			//expand
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100)<4)
				buildOrder->autoExpand(100,4);
			//factory
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Factory,73)<7){
				buildOrder->build(7,UnitTypes::Terran_Factory,73);
			}
			//add on
			if (mInfo->countUnitNum(UnitTypes::Terran_Factory,1)>=5&&buildOrder->getPlannedCount(UnitTypes::Terran_Machine_Shop,72)<3){
				buildOrder->build(3,UnitTypes::Terran_Machine_Shop,72);
			}
			//army
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Goliath,70)<35){
				buildOrder->build(35,UnitTypes::Terran_Goliath,70);
			}
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,70)<35){
				buildOrder->build(35,UnitTypes::Terran_Siege_Tank_Tank_Mode,70);
			}

		}
		//stage 1-------------------------------------------------fixed opening
		if (!_14CC_ && Broodwar->self()->supplyUsed()/2 >=7 && Broodwar->getFrameCount()<=24*60*5)
		{
			// Bunker rush
			//TilePosition mapCenter = TilePosition(Broodwar->mapWidth()/2,Broodwar->mapHeight()/2);
			////TilePosition(choke->mSecondChokepoint->getCenter())
			//if (buildOrder->getPlannedCount(UnitTypes::Terran_Barracks,2000)<2)
			//	buildOrder->build(2,UnitTypes::Terran_Barracks,2000,mapCenter);
			//if (buildOrder->getPlannedCount(UnitTypes::Terran_Marine,90)<8)
			//	buildOrder->build(8,UnitTypes::Terran_Marine,90);

			//if (scout->enemyStartLocation && SelectAll()(isCompleted)(Marine).size()>=2 &&
			//	  !eInfo->EnemyhasBuilt(UnitTypes::Terran_Vulture,1)	&&
			//		myInfor->myFightingValue().first > eInfo->enemyFightingValue().second &&
			//	  myInfor->CountUnitNum(UnitTypes::Terran_Command_Center,2)<2	&&
			//	  SelectAllEnemy()(isCompleted,canAttack).not(isWorker,isBuilding).size()<7)
			//{
			//	TilePosition eRegionCenter = TilePosition(BWTA::getRegion(scout->enemyStartLocation->getTilePosition())->getCenter());
			//	TilePosition eFirstChoke = TilePosition(choke->eFirstChokepoint->getCenter());
			//	TilePosition rushBK = TilePosition((eRegionCenter.x()+eFirstChoke.x())/2,(eRegionCenter.y()+eFirstChoke.y())/2);

			//	int see = buildOrder->getPlannedCount(UnitTypes::Terran_Bunker);
			//	if (buildOrder->getPlannedCount(UnitTypes::Terran_Bunker,95)<1)					
			//		buildOrder->build(1,UnitTypes::Terran_Bunker,95,rushBK);
			//}

			//_T_
			// normal opening
			mental->marineRushOver = true;

			if (buildOrder->getPlannedCount(UnitTypes::Terran_Barracks,100) < 1 && Broodwar->self()->supplyUsed()/2 >= 10)
				buildOrder->build(1,UnitTypes::Terran_Barracks,130);

			if (buildOrder->getPlannedCount(UnitTypes::Terran_Refinery,99) < 1 && Broodwar->self()->supplyUsed()/2 >= 12)
				buildOrder->build(1,UnitTypes::Terran_Refinery,125);

			if (buildOrder->getPlannedCount(UnitTypes::Terran_Marine,70) < 4)
				buildOrder->build(4,UnitTypes::Terran_Marine,80);//_T_
		}

		//if we have 2 marine(under training or already trained)
		if (!_14CC_ && mInfo->countUnitNum(UnitTypes::Terran_Marine,2) >= 2 && mInfo->countUnitNum(UnitTypes::Terran_SCV,1) >= 15 &&	Broodwar->getFrameCount()%(24*2) == 0)
		{
			//expand 2nd base
			if ((Broodwar->enemy()->deadUnitCount(UnitTypes::Terran_SCV) + Broodwar->enemy()->deadUnitCount(UnitTypes::Terran_Marine)) >= 12 && Broodwar->getFrameCount() <= 24*60*7.5)
				buildOrder->autoExpand(100,3);
			else
				buildOrder->autoExpand(100,2);

			if (buildOrder->getPlannedCount(UnitTypes::Terran_Refinery,99) < 1 && Broodwar->self()->supplyUsed()/2 >= 12)
				buildOrder->build(1,UnitTypes::Terran_Refinery,99);

			//build a bunker			
			if (mInfo->countUnitNum(UnitTypes::Terran_Bunker,2) == 0 && mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2) >= 2)
			{
				if (!bunkerPosition)
				{
					if (terrainManager->mNearestBase && terrainManager->mSecondChokepoint)
					{
						int x = (terrainManager->mNearestBase->getTilePosition().x() + terrainManager->mSecondChokepoint->getCenter().x() / 32) / 2;
						int y = (terrainManager->mNearestBase->getTilePosition().y() + terrainManager->mSecondChokepoint->getCenter().y() / 32) / 2;
						bunkerPosition = new TilePosition(x,y);
					}
				}

				if (bunkerPosition)
				{
					buildOrder->build(1,UnitTypes::Terran_Bunker,98,*bunkerPosition);
				}
			}

			if (buildOrder->getPlannedCount(UnitTypes::Terran_Marine,63) < 4 && mInfo->countUnitNum(UnitTypes::Terran_Factory,2)>1)
			{
				buildOrder->buildAdditional(1,UnitTypes::Terran_Marine,68);
			}				
		}

		if (_14CC_ && Broodwar->self()->supplyUsed()/2 >= 13 && Broodwar->getFrameCount() < 24*60*5)
		{
			mental->marineRushOver = true;
			buildOrder->autoExpand(200,2);
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Barracks,100) < 1 && Broodwar->self()->supplyUsed()/2 >= 14)
			{
				buildOrder->build(1,UnitTypes::Terran_Barracks,100);
			}
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Refinery,100) < 1 && Broodwar->self()->supplyUsed()/2 >= 15)
			{
				buildOrder->build(1,UnitTypes::Terran_Refinery,100);
			}
		}

		//opening over, stage 2 begin--------------------------------------------------build some advanced buildings
		//first check if all necessary buildings for opening are constructed

		if (mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2) >= 2)
		{
			if (_14CC_) buildOrder->build(1,UnitTypes::Terran_Factory,108);
			if (_14CC_) buildOrder->build(2,UnitTypes::Terran_Factory,78);

			//build two factories, one with machine shop
			if (!_14CC_) buildOrder->build(2,UnitTypes::Terran_Factory,68);

			//once one factory is finished
			if (mInfo->countUnitNum(UnitTypes::Terran_Factory,1) >= 1)
			{
				if (SelectAll()(Siege_Tank).size() >= 2)
					buildOrder->build(1,UnitTypes::Terran_Armory,71);
				if (SelectAll()(Siege_Tank).size() >= 4)
					buildOrder->build(1,UnitTypes::Terran_Academy,70);
				if (_14CC_) buildOrder->build(1,UnitTypes::Terran_Machine_Shop,80);
				buildOrder->build(2,UnitTypes::Terran_Machine_Shop,68);
			}

			if (SelectAll()(isCompleted)(Siege_Tank).size()>=3 && Broodwar->self()->gas()>350 && buildOrder->getPlannedCount(UnitTypes::Terran_Factory,65)<4)
			{
				buildOrder->build(5,UnitTypes::Terran_Factory,70);
			}
			//once we have armory
			if (mInfo->countUnitNum(UnitTypes::Terran_Armory,2)>0)
			{
				if (upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons)<1)
					buildOrder->upgrade(1,UpgradeTypes::Terran_Vehicle_Weapons,76);
			}

			if (mInfo->countUnitNum(UnitTypes::Terran_Armory,1)>0 && upgradeManager->getPlannedLevel(UpgradeTypes::Charon_Boosters)<1)
				buildOrder->upgrade(1,UpgradeTypes::Charon_Boosters,67);
			
			if (Broodwar->self()->supplyUsed()/2>=50 && mInfo->countUnitNum(UnitTypes::Terran_Academy,1)>0)
				buildOrder->build(mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Comsat_Station,70);
			if (mInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode,2)>=2)
				buildOrder->deleteItem(UnitTypes::Terran_Marine);
			//factoryTrainSet(3,0,0,66,0,0);
		}

		//stage 2 over, stage 3 begin------------------------build more factories,and train army
		//first check buildings for stage 2
		if (mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,1) > 0)
		{
			if (_14CC_ && buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,72) < 1)
			{
				buildOrder->build(1,UnitTypes::Terran_Siege_Tank_Tank_Mode,72);
			}

			if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode) && !Broodwar->self()->isResearching(TechTypes::Tank_Siege_Mode))
			{
				buildOrder->research(TechTypes::Tank_Siege_Mode,69);
			}

			//train goliath
			if (Broodwar->self()->minerals()>100 && 
				  mInfo->countUnitNum(UnitTypes::Terran_Goliath,1) < 9 && SelectAll()(isCompleted)(Siege_Tank).size() >= 9 &&
					Broodwar->getFrameCount()%(24*5) == 0 && mInfo->countUnitNum(UnitTypes::Terran_Armory,1)>0)
			{
				buildOrder->build(9,UnitTypes::Terran_Goliath,69);
			}
			//train siege tank
			if (Broodwar->self()->minerals()>100 && buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,65)<25 && Broodwar->getFrameCount()%(24*4)==0)
			{
				buildOrder->buildAdditional(mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,1),UnitTypes::Terran_Siege_Tank_Tank_Mode,69);
			}

			//train vulture
			if (Broodwar->self()->minerals() > 60 &&
					Broodwar->getFrameCount()%(24*5) == 0 &&
					Broodwar->self()->supplyUsed()/2 <= 150 &&
					buildOrder->getPlannedCount(UnitTypes::Terran_Vulture,65) < 20)
			{
				buildOrder->buildAdditional(mInfo->countUnitNum(UnitTypes::Terran_Factory,1),UnitTypes::Terran_Vulture,69);
				if (!Broodwar->self()->hasResearched(TechTypes::Spider_Mines))
					buildOrder->research(TechTypes::Spider_Mines,69);
			}

			//build more factories
			if (Broodwar->self()->minerals()>180 && buildOrder->getPlannedCount(UnitTypes::Terran_Factory,65) < 4	&& SelectAll()(isCompleted)(Siege_Tank).size() >= 5)
			{
				buildOrder->buildAdditional(1,UnitTypes::Terran_Factory,68);
			}

			if (mInfo->countUnitNum(UnitTypes::Terran_Factory,1)>=4 && Broodwar->self()->minerals()>200)
			{
				if (buildOrder->getPlannedCount(UnitTypes::Terran_Factory,65)<5)
					buildOrder->buildAdditional(1,UnitTypes::Terran_Factory,70);
				if (buildOrder->getPlannedCount(UnitTypes::Terran_Machine_Shop,65)<3)
					buildOrder->buildAdditional(1,UnitTypes::Terran_Machine_Shop,70);
			}
			//if finish upgrading Terran_Vehicle_Weapons, then continue upgrade
			if (!SelectAll()(Armory)(isCompleted)(isUpgrading)(RemainingUpgradeTime,"<",24*60*2).empty())
			{
				if (upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons)<3)
					buildOrder->upgrade(3,UpgradeTypes::Terran_Vehicle_Weapons,75);
			}
			//if we have advantages or too much money, then expand again
			if((Broodwar->self()->supplyUsed()/2 >= 100 && mInfo->myDeadArmy < eInfo->killedEnemyNum) ||
				 (Broodwar->self()->minerals() >= 1500 && Broodwar->self()->gas()<=300))
				buildOrder->autoExpand(100,5);
			else if (Broodwar->self()->supplyUsed()/2 > 80 || Broodwar->getFrameCount() > 24*60*10)
			{
				buildOrder->autoExpand(200,3);
			}
		}

		//stage 3 over, stage 4 begin----------------- build more advanced building and military, upgrade
		if(Broodwar->self()->supplyUsed()/2 >= 120 &&
			mInfo->countUnitNum(UnitTypes::Terran_Factory,2) >= 5 && 
			mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2)>=3 &&
			Broodwar->self()->supplyUsed()/2 <= 160)
		{
			if (Broodwar->self()->supplyUsed()/2 > 120 || Broodwar->getFrameCount() > 24*60*14)
			{
				buildOrder->autoExpand(200,4);
			}

			if (Broodwar->self()->supplyUsed()/2 > 140 || Broodwar->getFrameCount() > 24*60*18)
			{
				buildOrder->autoExpand(200,5);
			}

			//delete vulture and marine from build order list
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Marine,40)>0)
			{
				buildOrder->deleteItem(UnitTypes::Terran_Marine,63);
				buildOrder->deleteItem(UnitTypes::Terran_Marine,68);
			}
			//build scanner
			if (mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1)>=3 &&
				  mInfo->countUnitNum(UnitTypes::Terran_Comsat_Station,1)< mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1))
			{
				buildOrder->build(mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Comsat_Station,80);
			}
			//research siege mode
			if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode))
				buildOrder->research(TechTypes::Tank_Siege_Mode,70);
			// we need some tanks 
			if (mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,2)<3)
			{
				buildOrder->build(3,UnitTypes::Terran_Machine_Shop,69);
			}	

			if (mInfo->countUnitNum(UnitTypes::Terran_Factory,1)<9)
			{
				buildOrder->build(9,UnitTypes::Terran_Factory,69);
			}

			if (Broodwar->self()->minerals()>100 && buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,65)<24
				&& Broodwar->getFrameCount()%24*5==0)
			{
				buildOrder->buildAdditional(1,UnitTypes::Terran_Siege_Tank_Tank_Mode,69);
			}

			if (Broodwar->self()->minerals() > 100 && Broodwar->getFrameCount()%(24*5) == 0 && mInfo->countUnitNum(UnitTypes::Terran_Goliath,1) < 25)
			{
				buildOrder->build(25,UnitTypes::Terran_Goliath,68);
			}

			if (Broodwar->self()->minerals()>1500 && buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100) < 5)
				buildOrder->autoExpand(100,5);

			//for produce battle cruiser , build star port first
			if (Broodwar->self()->supplyUsed()/2 >= 130 && SelectAll()(isCompleted)(Siege_Tank).size() >= 8 &&
				  mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1) >= 2)
			{
				if (buildOrder->getPlannedCount(UnitTypes::Terran_Starport) < 5)
				{
					buildOrder->build(3,UnitTypes::Terran_Starport,110);
					buildOrder->build(5,UnitTypes::Terran_Starport,90);
				}
				if (buildOrder->getPlannedCount(UnitTypes::Terran_Control_Tower)<5)
				{
					buildOrder->build(3,UnitTypes::Terran_Control_Tower,108);
					buildOrder->build(2,UnitTypes::Terran_Control_Tower,88);
				}		
			}

			//produce battle cruiser
			if (mInfo->countUnitNum(UnitTypes::Terran_Starport,1) >= 2)
			{
				if (buildOrder->getPlannedCount(UnitTypes::Terran_Battlecruiser) < 15)
				{
					buildOrder->build(5,UnitTypes::Terran_Battlecruiser,96);
					buildOrder->build(10,UnitTypes::Terran_Battlecruiser,94);
				}
				if (buildOrder->getPlannedCount(UnitTypes::Terran_Science_Vessel) < 2)
				{
					buildOrder->build(2,UnitTypes::Terran_Science_Vessel,95);
				}
			}
			
			if (!SelectAll()(Physics_Lab)(isCompleted).empty())
			{
				// yamato gun research
				if (!Broodwar->self()->hasResearched(TechTypes::Yamato_Gun))
					buildOrder->research(TechTypes::Yamato_Gun,106);
				if (upgradeManager->getPlannedLevel(UpgradeTypes::Colossus_Reactor) < 1 && Broodwar->self()->hasResearched(TechTypes::Yamato_Gun))
				{
					buildOrder->upgrade(1,UpgradeTypes::Colossus_Reactor,105);
				}
			}	

			if (buildOrder->getPlannedCount(UnitTypes::Terran_Armory)<3)
				buildOrder->build(3,UnitTypes::Terran_Armory,99);
			if (upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Ship_Weapons)<3){
				buildOrder->upgrade(3,UpgradeTypes::Terran_Ship_Weapons,87);
			}
			if (upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Ship_Plating)<3){
				buildOrder->upgrade(3,UpgradeTypes::Terran_Ship_Plating,87);
			}	

			// produce some tanks
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,80)<15)
			{
				buildOrder->build(15,UnitTypes::Terran_Siege_Tank_Tank_Mode,80);
			}
		}

		//stage 4 over, final stage begin----------------- upgrade all technique that we need,more army, more expansion
		if (mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2) >= 2 && Broodwar->self()->supplyUsed()/2 > 160)
		{
			if (Broodwar->self()->minerals() > 150)
			{
				if (mInfo->countUnitNum(UnitTypes::Terran_Factory,2) < 8)
					buildOrder->build(8,UnitTypes::Terran_Factory,70);
				if (mInfo->countUnitNum(UnitTypes::Terran_Armory,2) < 2)
					buildOrder->build(2,UnitTypes::Terran_Armory,70);
				if (upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons) < 3)
					buildOrder->upgrade(3,UpgradeTypes::Terran_Vehicle_Weapons,70);
				if (upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons) < 3)
					buildOrder->upgrade(3,UpgradeTypes::Terran_Vehicle_Plating,70);
				if (buildOrder->getPlannedCount(UnitTypes::Terran_Battlecruiser) < 15)
				{
					buildOrder->build(5,UnitTypes::Terran_Battlecruiser,96);
					buildOrder->build(10,UnitTypes::Terran_Battlecruiser,94);
				}
				if (buildOrder->getPlannedCount(UnitTypes::Terran_Science_Vessel) < 2)
				{
					buildOrder->build(2,UnitTypes::Terran_Science_Vessel,95);
				}
			}
			//expand more
			if (Broodwar->self()->supplyUsed()/2>170 || (Broodwar->self()->minerals()>1500 &&Broodwar->self()->supplyUsed()/2>140))
			{
				if (buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100)<5)
					buildOrder->autoExpand(100,5);

				if (Broodwar->self()->supplyUsed()/2>180 && Broodwar->self()->minerals()>2500)
				{
					if (buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100)<7)
						buildOrder->autoExpand(100,7);				
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
	if (Broodwar->getFrameCount()%(24*3) == 0 && mInfo->countUnitNum(UnitTypes::Terran_Refinery,1) > 0)
	{
		if(Broodwar->self()->gas()-Broodwar->self()->minerals() > 20 && Broodwar->self()->gas() > 170 && Broodwar->self()->minerals() < 500)
			worker->setWorkerPerGas(0);
		else if (mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2)<2 && Broodwar->self()->gas() >= 200)
			worker->setWorkerPerGas(0);
		else
		{
			worker->setNeedGasLevel(3);
			worker->setWorkerPerGas(3);
		}
	}

	//prevent mined out
	if (bmc->getBaseSet().size()>=2 && bmc->getAllMineralSet().size()<20 && Broodwar->self()->minerals()>300 && Broodwar->self()->supplyUsed()/2>120)
	{
		if (buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100)<mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1)+1)
		{
			buildOrder->autoExpand(100,mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1)+1);
		}
	}

	if (Broodwar->self()->supplyUsed()/2 >= 10 && Broodwar->getFrameCount() < 24*60*5)
	{
		if (buildOrder->getPlannedCount(UnitTypes::Terran_Barracks,100) < 1)
			buildOrder->build(1,UnitTypes::Terran_Barracks,130,terrainManager->bbPos);

		if (buildOrder->getPlannedCount(UnitTypes::Terran_Refinery,99) < 1 && Broodwar->self()->supplyUsed()/2 >= 12)
			buildOrder->build(1,UnitTypes::Terran_Refinery,125);

		if (buildOrder->getPlannedCount(UnitTypes::Terran_Factory,80) < 1 && Broodwar->self()->supplyUsed()/2 >= 16)
			buildOrder->build(1,UnitTypes::Terran_Factory,120);

		if (buildOrder->getPlannedCount(UnitTypes::Terran_Marine,70) < 4)
			buildOrder->build(4,UnitTypes::Terran_Marine,80);
	}
	
	if (mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1) < 2 && mInfo->countUnitNum(UnitTypes::Terran_Factory,1) > 0)
	{
		if (buildOrder->getPlannedCount(UnitTypes::Terran_Machine_Shop,100) < 1)
			buildOrder->build(1,UnitTypes::Terran_Machine_Shop,100);
		if (mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,1) > 0)
		{
			if (!Broodwar->self()->hasResearched(TechTypes::Spider_Mines) && !buildOrder->plannedTech(TechTypes::Spider_Mines) && mInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode,2) > 0)
				buildOrder->research(TechTypes::Spider_Mines,74);
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,72) < 1)
				buildOrder->build(1,UnitTypes::Terran_Siege_Tank_Tank_Mode,72);
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Vulture,70) < 4 && mInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode,1) > 0)
				buildOrder->build(4,UnitTypes::Terran_Vulture,70);
			if(SelectAll()(isCompleted)(Siege_Tank,Vulture).size() == 5 && buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,72) < 3)
				buildOrder->build(3,UnitTypes::Terran_Siege_Tank_Tank_Mode,72);
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Marine,68) < 6)
				buildOrder->buildAdditional(2,UnitTypes::Terran_Marine,68);
		}

		if ((mInfo->countUnitNum(UnitTypes::Terran_Vulture,2) >= 1 || eInfo->killedEnemyNum > 2) &&
			  mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1) < 2 &&
				mInfo->countUnitNum(UnitTypes::Terran_SCV,2) > 14)
		{
			buildOrder->autoExpand(100,2);
		}
	}

	//stage2------------------------
	if (mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2) >= 2 && mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1) < 3)
	{		
		//build a bunker
		if (!bunkerPosition)
		{
			if (terrainManager->mNearestBase && terrainManager->mSecondChokepoint)
			{
				int x = (terrainManager->mNearestBase->getTilePosition().x() + terrainManager->mSecondChokepoint->getCenter().x() / 32) / 2;
				int y = (terrainManager->mNearestBase->getTilePosition().y() + terrainManager->mSecondChokepoint->getCenter().y() / 32) / 2;
				bunkerPosition = new TilePosition(x,y);
			}
		}

		if (Broodwar->self()->allUnitCount(UnitTypes::Terran_Bunker) < 1)
		{
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Bunker,100) < 1) buildOrder->build(1,UnitTypes::Terran_Bunker,100,*bunkerPosition);
		}
		else if (Broodwar->getFrameCount() > 24*60*9 && Broodwar->getFrameCount() < 24*60*12 && mInfo->myFightingValue().first < eInfo->enemyFightingValue().first)
		{
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Marine,80) < 8) buildOrder->build(8,UnitTypes::Terran_Marine,80);
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Bunker,80) < 2) buildOrder->build(2,UnitTypes::Terran_Bunker,80,*bunkerPosition);
		}

		if (Broodwar->self()->minerals()>100 && buildOrder->getPlannedCount(UnitTypes::Terran_Factory,65) < 5)
		{
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Factory,68) < 3)
			{	
				buildOrder->build(2,UnitTypes::Terran_Factory,79);
				buildOrder->build(3,UnitTypes::Terran_Factory,72);//68
			}
			if (Broodwar->getFrameCount() >= 24*60*9)
			{
				if (buildOrder->getPlannedCount(UnitTypes::Terran_Factory,68) < 4)
					buildOrder->build(4,UnitTypes::Terran_Factory,72);//68
			}	
			if (Broodwar->getFrameCount() >= 24*60*12)
			{
				if (buildOrder->getPlannedCount(UnitTypes::Terran_Factory,68) < 5)
					buildOrder->build(5,UnitTypes::Terran_Factory,72);//68
			}			
		}

		if (mInfo->countUnitNum(UnitTypes::Terran_Factory,1) > 0 && mInfo->countUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode,2) > 0)
		{
			if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode) && !buildOrder->plannedTech(TechTypes::Tank_Siege_Mode))
			{
				if (Broodwar->getFrameCount() >= 24*60*7)
				{
					buildOrder->research(TechTypes::Tank_Siege_Mode,120);
				}
				else
				{
					buildOrder->research(TechTypes::Tank_Siege_Mode,80);
				}
			}

			if (!Broodwar->self()->hasResearched(TechTypes::Spider_Mines) && !buildOrder->plannedTech(TechTypes::Spider_Mines))
				buildOrder->research(TechTypes::Spider_Mines,74);

			if (upgradeManager->getPlannedLevel(UpgradeTypes::Ion_Thrusters) < 1 && Broodwar->getFrameCount()%50 == 0)
				buildOrder->upgrade(1,UpgradeTypes::Ion_Thrusters,72);
		}

		// produce combat units
		if (Broodwar->getFrameCount()%(24*5) == 10 && buildOrder->getPlannedCount(UnitTypes::Terran_Vulture,68) < 18)
		{
			int num = min(mInfo->countUnitNum(UnitTypes::Terran_Factory,1), (Broodwar->self()->supplyTotal() - Broodwar->self()->supplyUsed()) / UnitTypes::Terran_Vulture.supplyRequired());
			if (num > 0) buildOrder->buildAdditional(num,UnitTypes::Terran_Vulture,68);
		}
		//if (Broodwar->getFrameCount()%(24*5) == 20 && buildOrder->getPlannedCount(UnitTypes::Terran_Goliath,68) < 18  && mInfo->countUnitNum(UnitTypes::Terran_Armory,1) > 0)
		//{
		//	int num = min(mInfo->countUnitNum(UnitTypes::Terran_Factory,1), (Broodwar->self()->supplyTotal() - Broodwar->self()->supplyUsed()) / UnitTypes::Terran_Goliath.supplyRequired());
		//	if (num > 0) buildOrder->buildAdditional(num,UnitTypes::Terran_Goliath,68);
		//}
		if (Broodwar->getFrameCount()%(24*5) == 30 && buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,68) + buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Siege_Mode,68) < 18)
		{
			int num = min(mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,1), (Broodwar->self()->supplyTotal() - Broodwar->self()->supplyUsed()) / UnitTypes::Terran_Siege_Tank_Tank_Mode.supplyRequired());
			if (num > 0) buildOrder->buildAdditional(num,UnitTypes::Terran_Siege_Tank_Tank_Mode,68);
		}

		// build combat station
		if (Broodwar->self()->supplyUsed()/2>60 && Broodwar->self()->minerals()>130 && mInfo->countUnitNum(UnitTypes::Terran_Factory,1) >= 2)
		{
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Comsat_Station,88) < 2)
			{
				buildOrder->build(1,UnitTypes::Terran_Academy,89);
				buildOrder->build(mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Comsat_Station,88);//68
			}
		}
		if (Broodwar->self()->supplyUsed()/2>50 && buildOrder->getPlannedCount(UnitTypes::Terran_Marine,60)>0)
			buildOrder->deleteItem(UnitTypes::Terran_Marine,70);

		// build machine shop
		if (mInfo->countUnitNum(UnitTypes::Terran_Factory,1) >= 4)
		{
			if (Broodwar->self()->supplyUsed()/2 > 70 && buildOrder->getPlannedCount(UnitTypes::Terran_Machine_Shop,75) < 3)
			{
				buildOrder->build(3,UnitTypes::Terran_Machine_Shop,75);
			}
		}
		else if (mInfo->countUnitNum(UnitTypes::Terran_Factory,1) >= 3)
		{
			if (Broodwar->self()->supplyUsed()/2 > 60 && buildOrder->getPlannedCount(UnitTypes::Terran_Machine_Shop,75) < 2)
			{
				buildOrder->build(2,UnitTypes::Terran_Machine_Shop,75);
			}
		}

		// upgrade
		if (buildOrder->getPlannedCount(UnitTypes::Terran_Armory,71) < 1 && Broodwar->getFrameCount() > 24*60*8)
		{
			buildOrder->build(1,UnitTypes::Terran_Armory,71);
		}

		if (mInfo->countUnitNum(UnitTypes::Terran_Armory,1) > 0 && Broodwar->getFrameCount() > 24*60*9)
		{
			if (upgradeManager->getPlannedLevel(UpgradeTypes::Charon_Boosters) < 1 && mInfo->countUnitNum(UnitTypes::Terran_Goliath,1) > 0)
			{
				buildOrder->upgrade(1,UpgradeTypes::Charon_Boosters,72);
			}
			if (Broodwar->getFrameCount()%50 == 0)
			{
				int num = mInfo->countUnitNum(UnitTypes::Terran_Armory,1) - SelectAll(UnitTypes::Terran_Armory)(isUpgrading).size();
				if (upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons) < 1 && num > 0)
				{
					buildOrder->upgrade(1,UpgradeTypes::Terran_Vehicle_Weapons,82);
				}
				if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) == 1 || num > 1)
				{
					if (upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Plating) < 1)
					{
						buildOrder->upgrade(1,UpgradeTypes::Terran_Vehicle_Plating,82);
					}
				}
			}
		}

		// expand
		if (mInfo->countUnitNum(UnitTypes::Terran_Factory,1) >= 3 &&
			  Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode) &&
			  Broodwar->self()->supplyUsed()/2 > 90 &&
			  (mInfo->myFightingValue().first > eInfo->enemyFightingValue().first || Broodwar->self()->supplyUsed()/2 > 120))
		{
			buildOrder->autoExpand(100,3);
		}
	}

	//stage 3-----------------------------------------------------
	if (mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1) >= 3 && Broodwar->self()->supplyUsed()/2 <= 160)
	{
		//for scanner
		if (buildOrder->getPlannedCount(UnitTypes::Terran_Comsat_Station,68)<3 && Broodwar->self()->supplyUsed()/2 > 100)
		{
			buildOrder->build(3,UnitTypes::Terran_Comsat_Station,68);
		}

		if (buildOrder->getPlannedCount(UnitTypes::Terran_Factory,80) < 7)
		{
			buildOrder->build(7,UnitTypes::Terran_Factory,80);
		}

		//for machine shop ,produce more tanks
		if (mInfo->countUnitNum(UnitTypes::Terran_Factory,1) >= 6)
		{
			if (Broodwar->self()->supplyUsed()/2 > 100 && buildOrder->getPlannedCount(UnitTypes::Terran_Machine_Shop,80) < 5)
			{
				buildOrder->build(5,UnitTypes::Terran_Machine_Shop,80);
			}
		}
		
		if (Broodwar->getFrameCount()%(24*5) == 10 && buildOrder->getPlannedCount(UnitTypes::Terran_Vulture,77) < 24)
		{
			int num = min(mInfo->countUnitNum(UnitTypes::Terran_Factory,1), (Broodwar->self()->supplyTotal() - Broodwar->self()->supplyUsed()) / UnitTypes::Terran_Vulture.supplyRequired());
			if (num > 0) buildOrder->buildAdditional(num,UnitTypes::Terran_Vulture,77);
		}
		if (Broodwar->getFrameCount()%(24*5) == 20 && buildOrder->getPlannedCount(UnitTypes::Terran_Goliath,78) < 12 && mInfo->countUnitNum(UnitTypes::Terran_Armory,1) > 0)
		{
			int num = min(mInfo->countUnitNum(UnitTypes::Terran_Factory,1), (Broodwar->self()->supplyTotal() - Broodwar->self()->supplyUsed()) / UnitTypes::Terran_Goliath.supplyRequired());
			if (num > 0) buildOrder->buildAdditional(num,UnitTypes::Terran_Goliath,78);
		}
		if (Broodwar->getFrameCount()%(24*5) == 30 && buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,78) + buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Siege_Mode,78) < 24)
		{
			int num = min(mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,1), (Broodwar->self()->supplyTotal() - Broodwar->self()->supplyUsed()) / UnitTypes::Terran_Siege_Tank_Tank_Mode.supplyRequired());
			if (num > 0) buildOrder->buildAdditional(num,UnitTypes::Terran_Siege_Tank_Tank_Mode,78);
		}

		if (buildOrder->getPlannedCount(UnitTypes::Terran_Armory,71) < 2)
		{
			buildOrder->build(2,UnitTypes::Terran_Armory,71);
		}

		if (mInfo->countUnitNum(UnitTypes::Terran_Armory,1) > 0)
		{
			if (upgradeManager->getPlannedLevel(UpgradeTypes::Charon_Boosters) < 1 && mInfo->countUnitNum(UnitTypes::Terran_Goliath,1) > 0)
			{
				buildOrder->upgrade(1,UpgradeTypes::Charon_Boosters,72);
			}
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Science_Facility,82) < 1)
			{
				buildOrder->build(1,UnitTypes::Terran_Science_Facility,82);
			}

			int max = mInfo->countUnitNum(UnitTypes::Terran_Science_Facility,1) > 0 ? 2 : 1;
			if (Broodwar->getFrameCount()%50 == 0)
			{
				int num = mInfo->countUnitNum(UnitTypes::Terran_Armory,1) - SelectAll(UnitTypes::Terran_Armory)(isUpgrading).size();

				int level = min(Broodwar->self()->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons)+1, max);
				if (upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons) < level && num > 0)
				{
					buildOrder->upgrade(level,UpgradeTypes::Terran_Vehicle_Weapons,82);
				}
				if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) == level || num > 1)
				{
					level = min(Broodwar->self()->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Plating)+1, max);
					if (upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Plating) < level)
					{
						buildOrder->upgrade(level,UpgradeTypes::Terran_Vehicle_Plating,82);
					}
				}
			}
		}

		// expand
		if (mInfo->countUnitNum(UnitTypes::Terran_Factory,1) >= 5 &&
			  mInfo->myFightingValue().first > eInfo->enemyFightingValue().first &&
			  mInfo->countUnitNum(UnitTypes::Terran_Command_Center,2) < 4	&&
			  Broodwar->self()->supplyUsed()/2 > 110 &&
			  mental->enemyInSight.empty())
		{
			buildOrder->autoExpand(100,4);
		}
	}

	//stage 4-------------------------------------
	if (Broodwar->self()->supplyUsed()/2 > 160 && mInfo->countUnitNum(UnitTypes::Terran_Factory,1) >= 4)
	{
		//for machine shop ,produce more tanks
		if (mInfo->countUnitNum(UnitTypes::Terran_Factory,1) >= 6)
		{
			if (Broodwar->self()->supplyUsed()/2 > 100 && buildOrder->getPlannedCount(UnitTypes::Terran_Machine_Shop,80) < 5)
			{
				buildOrder->build(5,UnitTypes::Terran_Machine_Shop,80);
			}
		}

		if (buildOrder->getPlannedCount(UnitTypes::Terran_Armory,71) < 2)
		{
			buildOrder->build(2,UnitTypes::Terran_Armory,71);
		}

		if (mInfo->countUnitNum(UnitTypes::Terran_Armory,1) > 0)
		{
			if (upgradeManager->getPlannedLevel(UpgradeTypes::Charon_Boosters) < 1 && mInfo->countUnitNum(UnitTypes::Terran_Goliath,1) > 0)
			{
				buildOrder->upgrade(1,UpgradeTypes::Charon_Boosters,72);
			}
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Science_Facility,72) < 1)
			{
				buildOrder->build(1,UnitTypes::Terran_Science_Facility,72);
			}

			int max = mInfo->countUnitNum(UnitTypes::Terran_Science_Facility,1) > 0 ? 3 : 1;
			if (Broodwar->getFrameCount()%50 == 0)
			{
				int num = mInfo->countUnitNum(UnitTypes::Terran_Armory,1) - SelectAll(UnitTypes::Terran_Armory)(isUpgrading).size();

				int level = min(Broodwar->self()->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons)+1, max);
				if (upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons) < level && num > 0)
				{
					buildOrder->upgrade(level,UpgradeTypes::Terran_Vehicle_Weapons,72);
				}
				if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) == level || num > 1)
				{
					level = min(Broodwar->self()->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Plating)+1, max);
					if (upgradeManager->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Plating) < level)
					{
						buildOrder->upgrade(level,UpgradeTypes::Terran_Vehicle_Plating,72);
					}
				}
			}
		}
		
		//Science_Vessel
		if (buildOrder->getPlannedCount(UnitTypes::Terran_Science_Vessel,72) < 2 && Broodwar->self()->supplyUsed() > 160)
		{
			buildOrder->build(2,UnitTypes::Terran_Science_Vessel,72);

			if (!buildOrder->plannedTech(TechTypes::EMP_Shockwave) && !Broodwar->self()->hasResearched(TechTypes::EMP_Shockwave))
			{
				buildOrder->research(TechTypes::EMP_Shockwave,72);
			}

			if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Titan_Reactor) < 1 && upgradeManager->getPlannedLevel(UpgradeTypes::Titan_Reactor) < 1)
			{
				buildOrder->upgrade(1,UpgradeTypes::Titan_Reactor,68);
			}
		}

		if (Broodwar->getFrameCount()%(24*5) == 10 && buildOrder->getPlannedCount(UnitTypes::Terran_Vulture,77) < 24)
		{
			int num = min(mInfo->countUnitNum(UnitTypes::Terran_Factory,1), (Broodwar->self()->supplyTotal() - Broodwar->self()->supplyUsed()) / UnitTypes::Terran_Vulture.supplyRequired());
			if (num > 0) buildOrder->buildAdditional(num,UnitTypes::Terran_Vulture,77);
		}
		if (Broodwar->getFrameCount()%(24*5) == 20 && buildOrder->getPlannedCount(UnitTypes::Terran_Goliath,78) < 24)
		{
			int num = min(mInfo->countUnitNum(UnitTypes::Terran_Factory,1), (Broodwar->self()->supplyTotal() - Broodwar->self()->supplyUsed()) / UnitTypes::Terran_Goliath.supplyRequired());
			if (num > 0) buildOrder->buildAdditional(num,UnitTypes::Terran_Goliath,78);
		}
		if (Broodwar->getFrameCount()%(24*5) == 30 && buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,78) + buildOrder->getPlannedCount(UnitTypes::Terran_Siege_Tank_Siege_Mode,78) < 24)
		{
			int num = min(mInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,1), (Broodwar->self()->supplyTotal() - Broodwar->self()->supplyUsed()) / UnitTypes::Terran_Siege_Tank_Tank_Mode.supplyRequired());
			if (num > 0) buildOrder->buildAdditional(num,UnitTypes::Terran_Siege_Tank_Tank_Mode,78);
		}

		//for scanner
		if (buildOrder->getPlannedCount(UnitTypes::Terran_Comsat_Station,73) < mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1))
		{
			buildOrder->build(mInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Comsat_Station,73);
		}

		//if we have lots of money and don't expand
		if(Broodwar->getFrameCount()>=24*60*10 && Broodwar->self()->minerals()>3000)
		{
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100)<5)
				buildOrder->autoExpand(100,5);
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Factory,65)<4)
				buildOrder->build(4,UnitTypes::Terran_Factory,73);
		}
		//more factories and expansion if we are rich
		if (Broodwar->self()->supplyUsed()/2>170 || (Broodwar->self()->minerals()>1300 && Broodwar->self()->supplyUsed()/2>150))
		{
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Command_Center,100)<6)
				buildOrder->autoExpand(100,6);
			if (buildOrder->getPlannedCount(UnitTypes::Terran_Factory,65)<10){
				buildOrder->build(10,UnitTypes::Terran_Factory,73);
			}
		}
	}
}

void GameFlow::showDebugInfo()
{
	// draw planned units info
	if (debug)
	{
		int x = 5;
		int y = 10;
		for each (UnitType type in UnitTypes::allUnitTypes())
		{
			if (type.getRace() == Broodwar->self()->getRace() && buildOrder->getPlannedCount(type) > 0)
			{
				Broodwar->drawTextScreen(x,y," %s : %d",type.getName().c_str(),buildOrder->getPlannedCount(type));
				y += 10;
			}
		}
	}
	
	// draw resources info
	Broodwar->drawTextScreen(335,0,"\x07 Mineral:");
	Broodwar->drawTextScreen(375,0,"\x07 %d",mineral);
	Broodwar->drawTextScreen(335,10,"\x07 Gas:");
	Broodwar->drawTextScreen(375,10,"\x07 %d",gas);
}