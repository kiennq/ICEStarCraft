#include "InformationManager.h"
#include <math.h>
#include <time.h>

using namespace BWAPI;
using namespace BWTA;
using namespace std;

MyInfoManager* myInforManager=NULL;

MyInfoManager* MyInfoManager::create()
{
	if (myInforManager) return myInforManager;
	else return myInforManager = new MyInfoManager();
}


MyInfoManager::MyInfoManager()
{
	this->allmyUnits.clear();
	this->bmc= NULL;
	this->needScanNow = false;
	this->allMyFighter.clear();
	this->myDeadArmy = 0;
	this->myDeadWorker = 0;
}

void MyInfoManager::setUsefulManagers(BaseManager* baseManager)
{
	this->bmc = baseManager;
}


int MyInfoManager::getMyPopulation()
{
	return Broodwar->self()->supplyUsed()/2;
}

int MyInfoManager::getMyBaseNum()
{
	return	(int)this->bmc->getBaseSet().size();
}

int MyInfoManager::countUnitNum(UnitType uType,int complete)
{
	if (complete == 0)
	{
		return Broodwar->self()->incompleteUnitCount(uType);
	}

	if (complete == 1)
	{
		return Broodwar->self()->completedUnitCount(uType);
	}

	return Broodwar->self()->allUnitCount(uType);
}

void MyInfoManager::onFrame()
{
	if (Broodwar->getFrameCount()%24==0)
	{
		//calculate dead army
		int deadMarine = Broodwar->self()->deadUnitCount(UnitTypes::Terran_Marine);
		int deadTank = Broodwar->self()->deadUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode);
		int deadGoliath = Broodwar->self()->deadUnitCount(UnitTypes::Terran_Goliath);
		int deadVulture = Broodwar->self()->deadUnitCount(UnitTypes::Terran_Vulture);
		int deadBattlecruiser = Broodwar->self()->deadUnitCount(UnitTypes::Terran_Battlecruiser);
			
		if (Broodwar->self()->supplyUsed()/2 < 65)
		{
			this->myDeadArmy = deadMarine+deadTank+deadVulture+deadGoliath;
		}
		else
		{
			this->myDeadArmy = deadTank+deadVulture+deadGoliath+deadBattlecruiser*2;
		}

		this->myDeadWorker = Broodwar->self()->deadUnitCount(UnitTypes::Terran_SCV);
	}

#ifdef _BATTLE_DEBUG
	Broodwar->drawTextScreen(0,40,"My_Dead_Army: %d",this->myDeadArmy);
#endif
}

int MyInfoManager::getMyArmyNum()
{
	int fighterNum = 0;
	std::set<Unit*> mUnits = Broodwar->self()->getUnits();
	for (std::set<Unit*>::iterator i = mUnits.begin(); i != mUnits.end(); i++){
		Unit* u = *i;
		if(u->getType().isBuilding()|| u->getType().isWorker() || !u->isCompleted()||!u->getType().canAttack()||u->isLoaded())
			continue;
		else
			fighterNum++;
	}
	//fighterNum = (int)SelectAll()(isCompleted)(Vulture,Siege_Tank,Goliath).size();
	return fighterNum;
}

void MyInfoManager::destroy()
{
	if (myInforManager) delete myInforManager;
}

/****************************************************/
/**************FOR ENEMY INFORMATION*****************/
/****************************************************/

EnemyInfoManager* enemyInforManager=NULL;

EnemyInfoManager* EnemyInfoManager::create()
{
	if (enemyInforManager) return enemyInforManager;
	else return enemyInforManager = new EnemyInfoManager();
}

EnemyInfoManager::EnemyInfoManager()
{
	this->enemyDeadWorker = 0;
	this->killedEnemyNum = 0;
	this->eUnitTypeToTimeMap.clear();
	this->eUnitToUintTypeMap.clear();
	this->allenemyFighter.clear();
	this->eBuildingPositionMap.clear();
	this->enemyBaseMap.clear();
	this->needScanNow =false;
	this->showTypeToTime=false;
	this->showUnitToType=false;
	this->showBuildingToPosition=false;
	this->showBaseToData=false;
	this->drRangeUpgradeFlag = false;
	this->mInfor = MyInfoManager::create();
	this->scm = ScoutManager::create();
	this->eMainBaseCheck = false;
	//_T_
	allEnemyUnits.clear();
	deadUnitCount = 0;
	deadWorkerCount = 0;
}

EnemyInfoManager::eBaseData::eBaseData()
{
	this->checkFinish=false;
	this->startBuildTime = 0;
	this->currentTime=0;
	this->finishTime=0;
	this->ProgressRate=0;
	this->tPosition = TilePositions::None;
	this->position = Positions::None;
	this->isStartBase = false;
	this->isMainBase = false;
	uType = UnitTypes::Unknown;
}
bool EnemyInfoManager::EnemyhasBuilt(UnitType uType,int n)
{
	for (std::map<UnitType,std::pair<int,bool>>::iterator i=this->eUnitTypeToTimeMap.begin(); i!= this->eUnitTypeToTimeMap.end();i++){
		if (uType==i->first){
			//0 for not completed unit
			if (n==0){
				if (i->second.second)
					return false;
				else
					return true;
			}
			//1 for completed unit
			if (n==1){
				if (i->second.second)
					return true;
				else
					return false;
			}
			//other number for completed and uncompleted
			if (n>1)
				return true;
		}
	}
//if can not find 
	return false;
}
void EnemyInfoManager::onUnitDiscover(Unit* u)
{
	if (!u || (u && !u->exists()))
		return;

	//_T_
	if(Broodwar->self()->isEnemy(u->getPlayer()))
	{
		EnemyUnit* e = NULL;
		for each (EnemyUnit* eu in this->allEnemyUnits)
		{
			if (eu->getUnit() == u)
			{
				e = eu;
				break;
			}
		}

		if (e != NULL)
		{
			e->update();
		}
		else
		{
			this->allEnemyUnits.insert(new EnemyUnit(u));
		}
	}

	if(Broodwar->self()->isEnemy(u->getPlayer()))
	{
		UnitType thisType=u->getType();
		if (thisType == UnitTypes::Protoss_Cybernetics_Core && u->isResearching())
			this->drRangeUpgradeFlag = true;
		//collect enemy unit type and first seen time
		if(this->eUnitTypeToTimeMap.find(thisType)==this->eUnitTypeToTimeMap.end())
		{
			if (u->isCompleted())
				this->eUnitTypeToTimeMap[thisType]=std::make_pair(Broodwar->getFrameCount(),true);
			else
				this->eUnitTypeToTimeMap[thisType]=std::make_pair(Broodwar->getFrameCount(),false);
		}
			
		if(this->eUnitTypeToTimeMap.find(thisType)!=this->eUnitTypeToTimeMap.end() && this->eUnitTypeToTimeMap[u->getType()].first==0)
			this->eUnitTypeToTimeMap[thisType]=std::make_pair(Broodwar->getFrameCount(),true);

		unitTypeInfer(u);

		//collect all enemy units and their types 
		if(this->eUnitToUintTypeMap.find(u)==this->eUnitToUintTypeMap.end())
			this->eUnitToUintTypeMap[u]=u->getType();

		//collect all enemy units and their positions
		if(this->eBuildingPositionMap.find(u)==this->eBuildingPositionMap.end()&&u->getType().isBuilding())
			this->eBuildingPositionMap[u]=make_pair(u->getType(),u->getPosition());


		//collect all enemy resource depot
		if (u->getType().isResourceDepot()){
			if(this->enemyBaseMap.find(u)==this->enemyBaseMap.end()){
				//if resource depot already finished when find it
				if(u->isCompleted())
				{
					this->enemyBaseMap[u].position = u->getPosition();
					this->enemyBaseMap[u].tPosition = u->getTilePosition();
					this->enemyBaseMap[u].base = BWTA::getNearestBaseLocation(u->getPosition());
					this->enemyBaseMap[u].checkFinish=true;
					this->enemyBaseMap[u].uType=u->getType();
					this->enemyBaseMap[u].currentTime=Broodwar->getFrameCount();
					this->enemyBaseMap[u].ProgressRate = 100;

					std::set<TilePosition> startTileLocations;
					for each(BWTA::BaseLocation* bl in BWTA::getStartLocations()){
						startTileLocations.insert(bl->getTilePosition());
					}

					if(startTileLocations.find(u->getTilePosition())!=startTileLocations.end())
						this->enemyBaseMap[u].isStartBase = true;
					else
						this->enemyBaseMap[u].isStartBase = false;

					//if game time still early
					if(Broodwar->getFrameCount() <= 24*60*4)
					{
						for each(BaseLocation* bl in BWTA::getStartLocations())
						{
							if (bl->getTilePosition()==u->getTilePosition()){
								this->enemyBaseMap[u].isMainBase = true;
								break;
							}
						}
						if(this->enemyBaseMap[u].isMainBase){
							this->enemyBaseMap[u].finishTime = -1;
							this->enemyBaseMap[u].startBuildTime = -1;
						}
						else
						{
							this->enemyBaseMap[u].finishTime = Broodwar->getFrameCount()/24/60;
							this->enemyBaseMap[u].startBuildTime = Broodwar->getFrameCount()-u->getType().buildTime()/24/60;
						}					

					}
					//else we don't know when this depot being built
					else{
						this->enemyBaseMap[u].finishTime = -1;
						this->enemyBaseMap[u].startBuildTime = -1;

					}
				}
				//if resource hasn't finish yet
				else if(!u->isCompleted()){
					this->enemyBaseMap[u].checkFinish = false;
					this->enemyBaseMap[u].uType = u->getType();
					this->enemyBaseMap[u].currentTime=Broodwar->getFrameCount();
					this->enemyBaseMap[u].base = BWTA::getNearestBaseLocation(u->getPosition());
					this->enemyBaseMap[u].tPosition=u->getTilePosition();
					this->enemyBaseMap[u].position=u->getPosition();
					this->enemyBaseMap[u].ProgressRate = ((double)u->getHitPoints()/(double)u->getType().maxHitPoints())*100;
					int see1 = u->getHitPoints();
					int see2 = u->getType().maxHitPoints();
					double see3 = ((double)u->getHitPoints()/(double)u->getType().maxHitPoints())*100;
					//Broodwar->printf("hitpoint: %d,Max: %d, result:%f",see1,see2,see3);
					this->enemyBaseMap[u].finishTime = u->getRemainingBuildTime()+Broodwar->getFrameCount();
					this->enemyBaseMap[u].startBuildTime = Broodwar->getFrameCount()-(u->getType().buildTime()-u->getRemainingBuildTime());
				}
			}				
		}
		//add to allenemyFight
		//!u->getType().isFlyer() &&
		if (u->getType()!= UnitTypes::Zerg_Scourge && u->getType()!=UnitTypes::Protoss_Interceptor && u->getType()!=UnitTypes::Terran_Vulture_Spider_Mine && !u->getType().isWorker() && u->getType().canAttack() && !u->getType().isBuilding() && u->exists() && u->isCompleted())
			allenemyFighter[u]=u->getType();
		else if (u->getType()==UnitTypes::Zerg_Sunken_Colony || u->getType()==UnitTypes::Protoss_Photon_Cannon){
			allenemyFighter[u]=u->getType();
		}
	}
}

void EnemyInfoManager::onUnitDestroy(Unit* u)
{
	//_T_
	if(Broodwar->self()->isEnemy(u->getPlayer()))
	{
		EnemyUnit* e = NULL;
		for each (EnemyUnit* eu in this->allEnemyUnits)
		{
			if (eu->getUnit() == u)
			{
				e = eu;
				break;
			}
		}

		if (e != NULL)
		{
			this->allEnemyUnits.erase(e);
		}
	}

	//count for total population of killed enemy 
	if (Broodwar->self()->isEnemy(u->getPlayer()))
	{
		//if(!u->getType().isBuilding())
		//_T_
		if (!u->getType().isBuilding() && u->getType() != UnitTypes::Terran_Vulture_Spider_Mine)
			this->killedEnemyNum++;

		if(u->getType().isWorker())
			this->enemyDeadWorker ++;
		//delete unit from Maps
		this->eUnitToUintTypeMap.erase(u);

		this->eBuildingPositionMap.erase(u);

		if(CountEunitNum(u->getType())==0)
			this->eUnitTypeToTimeMap.erase(u->getType());
		this->enemyBaseMap.erase(u);
		this->allenemyFighter.erase(u);
	}

	//_T_
	if (u->getPlayer() == Broodwar->enemy())
	{
		if (u->getType().isBuilding()
			  ||
			  u->getType() == UnitTypes::Terran_Vulture_Spider_Mine
			  ||
			  u->getType() == UnitTypes::Terran_Nuclear_Missile
			  ||
			  u->getType() == UnitTypes::Protoss_Interceptor
			  ||
			  u->getType() == UnitTypes::Protoss_Scarab
				||
				(!u->getType().canAttack() && u->getType() != UnitTypes::Protoss_Reaver && u->getType() != UnitTypes::Protoss_Carrier))
		{
			// don't count this unit
		}
		else
		{
			if (u->getType().isWorker())
			{
				deadWorkerCount++;
			}
			//deadUnitCount++;
			deadUnitCount += u->getType().supplyRequired();
		}
	}
}

void EnemyInfoManager::onUnitEvade(Unit* u)
{
	if(Broodwar->self()->isEnemy(u->getPlayer()))
	{
		EnemyUnit* e = NULL;
		for each (EnemyUnit* eu in this->allEnemyUnits)
		{
			if (eu->getUnit() == u)
			{
				e = eu;
				break;
			}
		}

		if (e != NULL)
		{
			e->update();
		}
		else
		{
			this->allEnemyUnits.insert(new EnemyUnit(u));
		}
	}
}

void EnemyInfoManager::unitTypeInfer(Unit* u)
{
	map<UnitType,int> tempMap;
	tempMap.clear();
	tempMap = u->getType().requiredUnits();
	for(map<UnitType,int>::const_iterator i=tempMap.begin();i!=tempMap.end();i++)
	{
		if(this->eUnitTypeToTimeMap.find(i->first)==this->eUnitTypeToTimeMap.end())
			this->eUnitTypeToTimeMap[i->first] = std::make_pair(Broodwar->getFrameCount(),true);
	}
}

int EnemyInfoManager::CountEunitNum(UnitType uType)
{
	int num = 0;
	for (map<Unit*,UnitType>::const_iterator i = this->eUnitToUintTypeMap.begin(); i!= this->eUnitToUintTypeMap.end();i++)
	{
		if(i->second==uType)
			num++;
	}
	return num;
}

void EnemyInfoManager::onUnitMorph(BWAPI::Unit* u)
{
	if (Broodwar->self()->isEnemy(u->getPlayer())){
		if (this->eBuildingPositionMap.find(u)!=this->eBuildingPositionMap.end()){
			this->eBuildingPositionMap.erase(u);
		}
		//this->enemyBaseMap.erase(u);
	}

}

map<Unit*,EnemyInfoManager::eBaseData> EnemyInfoManager::getEnemyBaseMap()
{
	return this->enemyBaseMap;
}

pair<int,Position> EnemyInfoManager::getLastSeenEnemyArmy(UnitGroup ug)
{
	if (ug.size()<8)
		return make_pair(-1,Positions::None);	

	else		
		return make_pair(Broodwar->getFrameCount(),ug.getCenter());	
}

int  EnemyInfoManager::getEnemyBaseNum()
{
	return (int)this->enemyBaseMap.size();
}

int  EnemyInfoManager::getKilledEnemyNum()
{
	return this->killedEnemyNum;
}

int EnemyInfoManager::timeToDefendMutalisk(Unit* u)
{
	if (Broodwar->self()->isEnemy(u->getPlayer())){
		if (u->getType()==UnitTypes::Zerg_Mutalisk){
			return Broodwar->getFrameCount();
		}
		else if (u->getType()==UnitTypes::Zerg_Spire){
			if (u->isCompleted()){
				return Broodwar->getFrameCount();
			}
			else{
				int executeTime = 0;
				if(hasUnitType(UnitTypes::Terran_Engineering_Bay,Broodwar->self())){
					executeTime=Broodwar->getFrameCount()+u->getRemainingBuildTime()+UnitTypes::Zerg_Mutalisk.buildTime()-24*15;
					return executeTime;
				}
				else{
					executeTime=Broodwar->getFrameCount()+u->getRemainingBuildTime()+UnitTypes::Zerg_Mutalisk.buildTime()-UnitTypes::Terran_Engineering_Bay.buildTime()-24*15;
					return executeTime;
				}
			}
		}
	}
	return Broodwar->getFrameCount();
}


void EnemyInfoManager::showTypeToTimeMap()
{
	if(this->showTypeToTime){	
		Broodwar->drawTextScreen(460,20,"\x0F| UnitType - DiscoverTime |");
		int uline = 1;
		for(std::map<UnitType,std::pair<int,bool>>::iterator i = this->eUnitTypeToTimeMap.begin();i != this->eUnitTypeToTimeMap.end();i++){
			Broodwar->drawTextScreen(460,20+16*uline,"\x0F| %s - %d |",i->first.c_str(),i->second.first);
			uline++;
		}
	}	
}

void EnemyInfoManager::showUnitToTypeMap()
{
	if(this->showUnitToType){	
		Broodwar->drawTextScreen(450,20,"\x08| UnitType - Number |");
		int uline = 1;
		for(std::map<UnitType,std::pair<int,bool>>::const_iterator i = this->eUnitTypeToTimeMap.begin();i != this->eUnitTypeToTimeMap.end();i++){
			Broodwar->drawTextScreen(450,20+16*uline,"\x11| %s - %d |",i->first.c_str(),CountEunitNum(i->first));
			uline++;
		}
	}	
}

void EnemyInfoManager::showBuildingToPositionMap()
{
	if (this->showBuildingToPosition){
		Broodwar->drawTextScreen(430,20,"\x08| Building - Position |");
		int uline = 1;
		for(std::map<Unit*,std::pair<UnitType,Position>>::const_iterator i = this->eBuildingPositionMap.begin();i != this->eBuildingPositionMap.end();i++){
			Broodwar->drawTextScreen(430,20+16*uline,"\x3| %s - (%d,%d) |",i->second.first.c_str(),i->second.second.x(),i->second.second.y());
			uline++;
		}
	}
}


void EnemyInfoManager::showBaseToDataMap()
{
	if(this->showBaseToData){		
		Broodwar->drawTextScreen(100,10,"\x08| BaseType--isStartLocation--Position--Progress|");
		int uline = 1;
		for(std::map<Unit*,eBaseData>::const_iterator i = this->enemyBaseMap.begin();i != this->enemyBaseMap.end();i++){
			Broodwar->drawTextScreen(100,10+16*uline,"\x19| %s--%d--(%d,%d)--%2lf%% | ",i->second.uType.c_str(),i->second.isMainBase,i->second.tPosition.x(),i->second.tPosition.y(),i->second.ProgressRate);
			uline++;
		}		
	}

}
/////////public//////////
bool hasUnitType(UnitType uType,Player* player)
{
	set<Unit*> myUnits = Broodwar->self()->getUnits();
	UnitGroup enemyUnits = SelectAllEnemy()(isVisible);
	if (player == Broodwar->self()){
		for each(Unit* unit in myUnits){
			if(unit->getType()==uType){
				return true;
			}
		}
	}
	else if (player == Broodwar->enemy()){
		for each(Unit* unit in enemyUnits){
			if(unit->getType()==uType){
				return true;
			}
		}
	}

	return false;
}

void EnemyInfoManager::onFrame()
{
	//_T_
	for each (Unit* u in SelectAllEnemy())
	{
		if(Broodwar->self()->isEnemy(u->getPlayer()))
		{
			EnemyUnit* e = NULL;
			for each (EnemyUnit* eu in this->allEnemyUnits)
			{
				if (eu->getUnit() == u)
				{
					e = eu;
					break;
				}
			}

			if (e != NULL)
			{
				e->update();
			}
			else
			{
				this->allEnemyUnits.insert(new EnemyUnit(u));
			}
		}
	}
	
	// if enemy unit doesn't exist at last position anymore
	if (Broodwar->getFrameCount()%24 == 12)
	{
		for each (EnemyUnit* e in this->allEnemyUnits)
		{
			if (e->getLastUpdatedFrame() == Broodwar->getFrameCount() || !Broodwar->isVisible(TilePosition(e->getPosition())))
			{
				continue;
			}

			UnitGroup tmp = SelectAllEnemy().inRadius(32,e->getPosition());
			if (tmp.find(e->getUnit()) == tmp.end())
			{
				e->update(true);
			}		
		}
	}
	
	
	if (Broodwar->getFrameCount()%24 == 0)
	{
		// delete destroyed enemy buildings on frame
		/*for(map<Unit*,std::pair<UnitType,Position>>::iterator i = eBuildingPositionMap.begin(); i != eBuildingPositionMap.end();)
		{
			UnitGroup myUnit = SelectAll()(isCompleted).not(Vulture_Spider_Mine).inRadius(32*4,i->second.second);
			UnitGroup eBuild = SelectAllEnemy()(isBuilding,isAddon).inRadius(32*4,i->second.second);
			if (myUnit.size()>0 && (!i->first->exists() || eBuild.size()<1))
			{
				i = eBuildingPositionMap.erase(i);
			}
			else
			{
				i++;
			}
		}*/
		
		// for protoss dragoon rush confirm
		UnitGroup researchingCC = SelectAllEnemy()(isVisible,isBuilding,isCompleted,isResearching)(Cybernetics_Core);
		if (!researchingCC.empty())
			drRangeUpgradeFlag = true;
		
	
		enemyMainBaseConfirm();

		//update enemy uncompleted units info
		for (std::map<UnitType,std::pair<int,bool>>::iterator i = this->eUnitTypeToTimeMap.begin(); i!= this->eUnitTypeToTimeMap.end();i++)
		{
			if (i->second.second == false)
			{
				if (Broodwar->getFrameCount()>=i->second.first+24*30)
					i->second.second = true;
			}	
		}
	}
	
#ifdef _BATTLE_DEBUG
	Broodwar->drawTextScreen(0,20,"Killed_Enemy_Total: %d",this->killedEnemyNum);
	Broodwar->drawTextScreen(0,30,"Killed_Enemy_Worker: %d",this->enemyDeadWorker);
#endif
	
}

void EnemyInfoManager::destroy()
{
	if (enemyInforManager) delete enemyInforManager;
}

std::pair<double,double> EnemyInfoManager::enemyFightingValue()
{
	//enemy movable army
	double eFV = 0;
	//enemy defend value
	double eDV = 0;
	double towerWeight=0;
	double flyerWeight=0;
	//for defend tower weight
	if (this->mInfor->countUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode,1) > 0 && Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode))
		towerWeight = 0.3;
	else
		towerWeight = 0.6;

	//for defend tower weight
	if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Charon_Boosters) > 0)
	{
		flyerWeight = pow(0.9,this->mInfor->countUnitNum(UnitTypes::Terran_Goliath,1));	
	}
	else
		flyerWeight = 1;

	//calculate enemy defend value
	for(std::map<Unit*,UnitType>::iterator i=this->allenemyFighter.begin(); i!= this->allenemyFighter.end();i++)
	{
		//if the unit type is defend tower, then fighting value * weight
		if (i->second.groundWeapon()==WeaponTypes::None || i->second.groundWeapon().damageCooldown()==0){
			if (i->second==UnitTypes::Protoss_Carrier){
				eDV+=i->first->getInterceptorCount()*UnitTypes::Protoss_Interceptor.groundWeapon().damageAmount()/UnitTypes::Protoss_Interceptor.groundWeapon().damageCooldown()*(i->second.maxHitPoints()+i->second.maxShields()-1)*flyerWeight;
				//eDV+=UnitTypes::Protoss_Interceptor.groundWeapon().damageAmount()/UnitTypes::Protoss_Interceptor.groundWeapon().damageCooldown()*(i->second.maxHitPoints()+i->second.maxShields()-1)*flyerWeight;
			}
			else if (i->second == UnitTypes::Zerg_Mutalisk)
			{
				eDV+= i->second.groundWeapon().damageAmount()/i->second.groundWeapon().damageCooldown()*i->second.maxHitPoints()*flyerWeight;
			}
			
			else
				continue;
		}
		else if (i->second==UnitTypes::Zerg_Sunken_Colony || i->second==UnitTypes::Protoss_Photon_Cannon)
			eDV+=(i->second.groundWeapon().damageAmount()*1.0/i->second.groundWeapon().damageCooldown()*(i->second.maxHitPoints()+i->second.maxShields()-1))*towerWeight;
		else if (i->second.groundWeapon().damageAmount()<2||!i->second.canAttack())
			continue;
		else if (i->second.isFlyer()&&i->second.groundWeapon()!=WeaponTypes::None)
			eDV+=i->second.groundWeapon().damageAmount()*1.0/i->second.groundWeapon().damageCooldown()*(i->second.maxHitPoints()+i->second.maxShields()-1)*flyerWeight;
			//eFV+=UnitTypes::Zerg_Sunken_Colony.groundWeapon().damageAmount()*1.0/UnitTypes::Zerg_Sunken_Colony.groundWeapon().damageCooldown()*(UnitTypes::Zerg_Sunken_Colony.maxHitPoints()-1)*towerWeight;
		else
			eDV+=i->second.groundWeapon().damageAmount()*1.0/i->second.groundWeapon().damageCooldown()*(i->second.maxHitPoints()+i->second.maxShields()-1);
	}

	//calculate enemy fighting value
	for(std::map<Unit*,UnitType>::iterator i=this->allenemyFighter.begin(); i!= this->allenemyFighter.end();i++){
		//if the unit type is defend tower, then fighting value * weight
		if (i->second==UnitTypes::Zerg_Sunken_Colony || i->second==UnitTypes::Protoss_Photon_Cannon)
			continue;
		else if (i->second.groundWeapon() == WeaponTypes::None || i->second.groundWeapon().damageCooldown() == 0)
		{
			if (i->second == UnitTypes::Protoss_Carrier)
			{
				eFV+=i->first->getInterceptorCount()*UnitTypes::Protoss_Interceptor.groundWeapon().damageAmount()/UnitTypes::Protoss_Interceptor.groundWeapon().damageCooldown()*(i->second.maxHitPoints()+i->second.maxShields()-1)*flyerWeight;
				//eDV+=UnitTypes::Protoss_Interceptor.groundWeapon().damageAmount()/UnitTypes::Protoss_Interceptor.groundWeapon().damageCooldown()*(i->second.maxHitPoints()+i->second.maxShields()-1)*flyerWeight;
			}
			else if (i->second == UnitTypes::Zerg_Mutalisk)
			{
				eFV+= i->second.groundWeapon().damageAmount()/i->second.groundWeapon().damageCooldown()*i->second.maxHitPoints()*flyerWeight;
			}
			else
				continue;
		}
		else if (i->second.isFlyer()&&i->second.groundWeapon()!=WeaponTypes::None)
			eFV+=i->second.groundWeapon().damageAmount()*1.0/i->second.groundWeapon().damageCooldown()*(i->second.maxHitPoints()+i->second.maxShields()-1)*flyerWeight;
		else
			eFV+=i->second.groundWeapon().damageAmount()*1.0/i->second.groundWeapon().damageCooldown()*(i->second.maxHitPoints()+i->second.maxShields()-1);
	}
	if (this->killedEnemyNum>0)
		return std::make_pair(eFV/killedEnemyNum,eDV/killedEnemyNum);
	else
		return std::make_pair(eFV,eDV);	
}

std::pair<double,double> MyInfoManager::myFightingValue()
{
	if (!this->allMyFighter.empty())
	{
		UnitGroup bunkerGroup = SelectAll()(isCompleted)(Bunker);
		double mFV = 0;
		double mDV = 0;
		for each(Unit* u in this->allMyFighter)
		{
			int atkBonus = 0;
			if (u->getType().groundWeapon() != WeaponTypes::None)
			{
				atkBonus = u->getType().groundWeapon().damageFactor()*Broodwar->self()->getUpgradeLevel(u->getType().groundWeapon().upgradeType());
			}
			else if (u->getType().airWeapon() != WeaponTypes::None)
			{
				atkBonus = u->getType().airWeapon().damageFactor()*Broodwar->self()->getUpgradeLevel(u->getType().airWeapon().upgradeType());
			}
			
			//int atkBonus =u->getType().groundWeapon().damageFactor()*Broodwar->self()->getUpgradeLevel(u->getType().groundWeapon().upgradeType());
			int defBonus = Broodwar->self()->getUpgradeLevel(u->getType().armorUpgrade());
			if(!u->getType().canAttack()||!u->isCompleted())
				continue;
			else if (u->getType()==UnitTypes::Terran_Vulture_Spider_Mine || u->isLoaded())
				continue;
			else
			{
				if (u->getType().groundWeapon() != WeaponTypes::None)
				{
					mFV+=(u->getType().groundWeapon().damageAmount()+atkBonus)*1.0/u->getType().groundWeapon().damageCooldown()*(u->getHitPoints()+u->getShields()-1+defBonus);
					mDV+=(u->getType().groundWeapon().damageAmount()+atkBonus)*1.0/u->getType().groundWeapon().damageCooldown()*(u->getHitPoints()+u->getShields()-1+defBonus);
				}
				else if (u->getType().airWeapon() != WeaponTypes::None)
				{
					mFV+=(u->getType().airWeapon().damageAmount()+atkBonus)*1.0/u->getType().airWeapon().damageCooldown()*(u->getHitPoints()+u->getShields()-1+defBonus);
					mDV+=(u->getType().airWeapon().damageAmount()+atkBonus)*1.0/u->getType().airWeapon().damageCooldown()*(u->getHitPoints()+u->getShields()-1+defBonus);
				}
			}				
		}
		for each(Unit* u in bunkerGroup)
		{
			//we only produce marine into bunker
			mDV+=u->getLoadedUnits().size()*6.0/UnitTypes::Terran_Marine.groundWeapon().damageCooldown()*(u->getHitPoints()-1);		
		}
		if(this->myDeadArmy+this->myDeadWorker>0)
			return std::make_pair(mFV/((double)1.3*(this->myDeadArmy+this->myDeadWorker)),mDV/((double)1.3*(this->myDeadArmy+this->myDeadWorker)));
		else
			return std::make_pair(mFV,mDV);
	}
	else
		return std::make_pair(0,0);
}

void MyInfoManager::onUnitDestroy(Unit* u)
{
	allmyUnits.erase(u);
	this->allMyFighter.erase(u);
}

void MyInfoManager::onUnitDiscover(Unit* u)
{
	if (u->getPlayer()==Broodwar->self()){
		if (u->getType().canAttack()&& !u->getType().isBuilding() && !u->getType().isWorker() && u->getType()!=UnitTypes::Terran_Vulture_Spider_Mine)
			this->allMyFighter.insert(u);
	}
}

void EnemyInfoManager::enemyMainBaseConfirm()
{
	if (this->scm->enemyStartLocation){
		if(this->eMainBaseCheck) return;

		for(std::map<Unit*,eBaseData>::iterator i = this->enemyBaseMap.begin();i != this->enemyBaseMap.end();i++){
			if (i->second.tPosition==this->scm->enemyStartLocation->getTilePosition()){
				i->second.isMainBase = true;
				this->eMainBaseCheck =true;
			}				
			else{
				i->second.isMainBase = false;
				continue;
			}
		}
	}
}

/**********************************************************************************************/
//_T_

int MyInfoManager::getDeadUnitCount()
{
	/*
	int count = 0;

	count += Broodwar->self()->deadUnitCount(UnitTypes::Terran_SCV);
	count += Broodwar->self()->deadUnitCount(UnitTypes::Terran_Marine);
	count += Broodwar->self()->deadUnitCount(UnitTypes::Terran_Firebat);
	count += Broodwar->self()->deadUnitCount(UnitTypes::Terran_Ghost);
	count += Broodwar->self()->deadUnitCount(UnitTypes::Terran_Medic);
	count += Broodwar->self()->deadUnitCount(UnitTypes::Terran_Vulture);
	count += Broodwar->self()->deadUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode);
	count += Broodwar->self()->deadUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode);
	count += Broodwar->self()->deadUnitCount(UnitTypes::Terran_Goliath);
	count += Broodwar->self()->deadUnitCount(UnitTypes::Terran_Wraith);
	count += Broodwar->self()->deadUnitCount(UnitTypes::Terran_Dropship);
	count += Broodwar->self()->deadUnitCount(UnitTypes::Terran_Valkyrie);
	count += Broodwar->self()->deadUnitCount(UnitTypes::Terran_Battlecruiser);

	return count;
	*/

	int count = 0;

	set<UnitType> types;
	types.insert(UnitTypes::Terran_SCV);
	types.insert(UnitTypes::Terran_Marine);
	types.insert(UnitTypes::Terran_Firebat);
	types.insert(UnitTypes::Terran_Ghost);
	types.insert(UnitTypes::Terran_Medic);
	types.insert(UnitTypes::Terran_Vulture);
	types.insert(UnitTypes::Terran_Siege_Tank_Tank_Mode);
	types.insert(UnitTypes::Terran_Siege_Tank_Siege_Mode);
	types.insert(UnitTypes::Terran_Goliath);
	types.insert(UnitTypes::Terran_Wraith);
	types.insert(UnitTypes::Terran_Dropship);
	types.insert(UnitTypes::Terran_Valkyrie);
	types.insert(UnitTypes::Terran_Battlecruiser);

	for each (UnitType type in types)
	{
		count += Broodwar->self()->deadUnitCount(type) * type.supplyRequired();
	}

	return count/2;
}

int MyInfoManager::getDeadWorkerCount()
{
	return Broodwar->self()->deadUnitCount(UnitTypes::Terran_SCV);
}

double MyInfoManager::attackValue()
{
	double groundAV = 0;
	double airAV = 0;

	for each (Unit* u in SelectAll())
	{
		int damage = 0;
		int cooldown = 0;
		int hp = 0;

		if (!u->isCompleted()
			  ||
		  	u->getType().isBuilding()
			  ||
			  u->getType().isWorker()
			  ||
			  !u->getType().canAttack()
			  ||
			  u->getType() == UnitTypes::Terran_Vulture_Spider_Mine
			  ||
			  u->getType() == UnitTypes::Terran_Nuclear_Missile)
		{
			continue;
		}

		UnitType type = u->getType();
		hp = u->getHitPoints() + u->getShields();

		if (u->getType().groundWeapon() != WeaponTypes::None)
		{
			WeaponType weapon = type.groundWeapon();
			cooldown = Broodwar->self()->groundWeaponDamageCooldown(type);
			damage = type.maxGroundHits() * (weapon.damageAmount() + Broodwar->self()->getUpgradeLevel(weapon.upgradeType()) * weapon.damageFactor() * weapon.damageBonus());
			groundAV = groundAV + 1.0 * hp * damage / cooldown;
		}

		if (u->getType().airWeapon() != WeaponTypes::None)
		{
			WeaponType weapon = type.airWeapon();
			cooldown = weapon.damageCooldown();
			damage = type.maxAirHits() * (weapon.damageAmount() + Broodwar->self()->getUpgradeLevel(weapon.upgradeType()) * weapon.damageFactor() * weapon.damageBonus());
			airAV = airAV + 1.0 * hp * damage / cooldown;
		}
	}

	return groundAV > airAV ? groundAV : airAV;
}

double MyInfoManager::defenseValue()
{

}

void MyInfoManager::showDebugInfo()
{
	Broodwar->drawTextScreen(180,325,"\x07 mAV: %.2f | %.2f - deadUnits: %d - deadWorkers: %d",attackValue(),attackValue()/(1+getDeadUnitCount()),getDeadUnitCount(),getDeadWorkerCount());
}

set<EnemyUnit*> EnemyInfoManager::getAllEnemyUnits()
{
	return allEnemyUnits;
}

EnemyUnit* EnemyInfoManager::getEnemyUnit(BWAPI::Unit* unit)
{
	for each (EnemyUnit* e in allEnemyUnits)
	{
		if (e->getUnit() == unit)
		{
			return e;
		}
	}

	return (EnemyUnit*)NULL;
}

bool EnemyInfoManager::isEnemyBase(BWTA::BaseLocation* base)
{
	if (!base)
	{
		return false;
	}

	for (map<Unit*,eBaseData>::iterator i = enemyBaseMap.begin(); i != enemyBaseMap.end(); i++)
	{
		if (i->second.base == base)
		{
			return true;
		}
	}
	return false;
}

int EnemyInfoManager::countUnitNum(UnitType type, Position p, int radius)
{
	int n = 0;

	if (p == Positions::None)
	{
		for each (EnemyUnit* e in allEnemyUnits)
		{
			if (e->getType() == type)
			{
				n++;
			}
		}
	}
	else
	{
		for each (EnemyUnit* e in allEnemyUnits)
		{
			if (e->getType() == type && e->getPosition() != Positions::Unknown && e->getPosition().getApproxDistance(p) <= radius)
			{
				n++;
			}
		}
	}
	
	return n;
}

int EnemyInfoManager::countDangerToAir(BWAPI::Position p, int radius)
{
	int n = 0;

	for each (UnitType type in UnitTypes::allUnitTypes())
	{
		if (type.getRace() != Broodwar->enemy()->getRace())
		{
			continue;
		}

		if (type != UnitTypes::Terran_Bunker && (!type.canAttack() || type.airWeapon() == WeaponTypes::None))
		{
			continue;
		}
		
		for each (EnemyUnit* e in allEnemyUnits)
		{
			if (e->getType() == type && e->getPosition() != Positions::Unknown && e->getPosition().getApproxDistance(p) <= radius)
			{
				n++;
			}
		}
	}

	return n;
}

int EnemyInfoManager::countDangerToGround(BWAPI::Position p, int radius)
{
	int n = 0;

	for each (UnitType type in UnitTypes::allUnitTypes())
	{
		if (type.getRace() != Broodwar->enemy()->getRace())
		{
			continue;
		}

		if (type != UnitTypes::Terran_Bunker && (!type.canAttack() || type.isWorker() || type.groundWeapon() == WeaponTypes::None))
		{
			continue;
		}

		for each (EnemyUnit* e in allEnemyUnits)
		{
			if (e->getType() == type && e->getPosition() != Positions::Unknown && e->getPosition().getApproxDistance(p) <= radius)
			{
				n++;
			}
		}
	}

	return n;
}

int EnemyInfoManager::countDangerTotal(BWAPI::Position p, int radius)
{
	int n = 0;

	for each (UnitType type in UnitTypes::allUnitTypes())
	{
		if (type.getRace() != Broodwar->enemy()->getRace())
		{
			continue;
		}

		if (type != UnitTypes::Terran_Bunker && (!type.canAttack() || type.isWorker()))
		{
			continue;
		}

		for each (EnemyUnit* e in allEnemyUnits)
		{
			if (e->getType() == type && e->getPosition() != Positions::Unknown && e->getPosition().getApproxDistance(p) <= radius)
			{
				n++;
			}
		}
	}

	return n;
}

int EnemyInfoManager::getDeadUnitCount()
{
	//return deadUnitCount;
	return deadUnitCount/2; //supply
}

int EnemyInfoManager::getDeadWorkerCount()
{
	return deadWorkerCount;
}

double EnemyInfoManager::attackValue()
{
	double groundAV = 0;
	double airAV = 0;

	for each (EnemyUnit* e in allEnemyUnits)
	{
		int damage = 0;
		int cooldown = 0;
		int hp = 0;

		if (e->getUnit() && e->getUnit()->exists() && !e->getUnit()->isCompleted())
		{
			continue;
		}

		if (e->getType().isBuilding()
			  ||
			  e->getType().isWorker()
				||
			  e->getType() == UnitTypes::Terran_Vulture_Spider_Mine
				||
				e->getType() == UnitTypes::Terran_Nuclear_Missile
				||
			  e->getType() == UnitTypes::Protoss_Interceptor
				||
			  e->getType() == UnitTypes::Protoss_Scarab)
		{
			continue;
		}

		if (!e->getType().canAttack() && e->getType() != UnitTypes::Protoss_Reaver && e->getType() != UnitTypes::Protoss_Carrier)
		{
			continue;
		}

		if (e->getType() == UnitTypes::Protoss_Reaver)
		{
			hp = e->getHitPoints() + e->getShields();
			UnitType scarab = UnitTypes::Protoss_Scarab;

			WeaponType weapon = scarab.groundWeapon();	
			cooldown = 60; // reavers launch scarabs every 60 frames
			damage = scarab.maxGroundHits() * (weapon.damageAmount() + Broodwar->enemy()->getUpgradeLevel(weapon.upgradeType()) * weapon.damageFactor() * weapon.damageBonus());
			groundAV = groundAV + 1.0 * hp * damage / cooldown;
		}
		else if (e->getType() == UnitTypes::Protoss_Carrier)
		{
			hp = e->getHitPoints() + e->getShields();
			UnitType interceptor = UnitTypes::Protoss_Interceptor;

			WeaponType weapon = interceptor.groundWeapon();
			cooldown = Broodwar->enemy()->groundWeaponDamageCooldown(interceptor);
			damage = e->getInterceptorCount() * interceptor.maxGroundHits() * (weapon.damageAmount() + Broodwar->enemy()->getUpgradeLevel(weapon.upgradeType()) * weapon.damageFactor() * weapon.damageBonus());
			groundAV = groundAV + 1.0 * hp * damage / cooldown;

			weapon = interceptor.airWeapon();
			cooldown = weapon.damageCooldown();
			damage = e->getInterceptorCount() * interceptor.maxAirHits() * (weapon.damageAmount() + Broodwar->enemy()->getUpgradeLevel(weapon.upgradeType()) * weapon.damageFactor() * weapon.damageBonus());
			airAV = airAV + 1.0 * hp * damage / cooldown;
		}
		else
		{
			UnitType type = e->getType();
			hp = e->getHitPoints() + e->getShields();

			if (e->getType().groundWeapon() != WeaponTypes::None)
			{
				WeaponType weapon = type.groundWeapon();
				cooldown = Broodwar->enemy()->groundWeaponDamageCooldown(type);
				damage = type.maxGroundHits() * (weapon.damageAmount() + Broodwar->enemy()->getUpgradeLevel(weapon.upgradeType()) * weapon.damageFactor() * weapon.damageBonus());
				groundAV = groundAV + 1.0 * hp * damage / cooldown;
			}
			
			if (e->getType().airWeapon() != WeaponTypes::None)
			{
				WeaponType weapon = type.airWeapon();
				cooldown = weapon.damageCooldown();
				damage = type.maxAirHits() * (weapon.damageAmount() + Broodwar->enemy()->getUpgradeLevel(weapon.upgradeType()) * weapon.damageFactor() * weapon.damageBonus());
				airAV = airAV + 1.0 * hp * damage / cooldown;
			}
		}
	}

	return groundAV > airAV ? groundAV : airAV;
}

double EnemyInfoManager::defenseValue()
{

}

void EnemyInfoManager::showDebugInfo()
{
	Broodwar->drawTextScreen(180,335,"\x08 eAV: %.2f | %.2f - deadUnits: %d - deadWorkers: %d",attackValue(),attackValue()/(1+getDeadUnitCount()),getDeadUnitCount(),getDeadWorkerCount());


	// allEnemyUnits info
	map<UnitType,int> list;
	int x = 433;
	int line = 16;
	int num = 0;

	for each (EnemyUnit* e in allEnemyUnits)
	{
		Position p = e->getPosition();
		UnitType t = e->getType();

		if (p != Positions::Unknown)
		{
			Broodwar->drawBoxMap(p.x()-t.dimensionLeft(),p.y()-t.dimensionUp(),p.x()+t.dimensionRight(),p.y()+t.dimensionDown(),Colors::Red);
		}

		if (t.isBuilding() && !t.canAttack())
		{
			continue;
		}

		if (list.empty())
		{
			list.insert(make_pair(t,1));
		}
		else
		{
			map<UnitType,int>::iterator i = list.find(t);
			if (i != list.end())
			{
				num = (*i).second;
				list.erase(i);
				list.insert(make_pair(t,num+1));
			}
			else
			{
				list.insert(make_pair(t,1));
			}
		}
	}

	Broodwar->drawTextScreen(x,line*10,"\x08 AllEnemyUnits: %d",allEnemyUnits.size());
	line++;
	for (map<UnitType,int>::iterator i = list.begin(); i != list.end(); i++)
	{
		Broodwar->drawTextScreen(x,line*10," %d %s",(*i).second,(*i).first.getName().c_str());
		line++;
	}


	// enemy bases
	for each(BWTA::BaseLocation* b in BWTA::getBaseLocations())
	{
		if (isEnemyBase(b))
		{
			Broodwar->drawCircleMap(b->getPosition().x(),b->getPosition().y(),50,Colors::Blue);
		}
	}
}