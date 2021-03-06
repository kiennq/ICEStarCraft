#include <time.h>
#include "BaseManager.h"
#include "Base.h"
#include "UnitGroup.h"
#include "UnitGroupManager.h"

using namespace BWAPI;

BaseManager* theBaseManager = NULL;

BaseManager* BaseManager::create()
{
	if (theBaseManager) return theBaseManager;
	return theBaseManager = new BaseManager();

}

void BaseManager::destroy()
{
	if (theBaseManager)
		delete theBaseManager;
}

void BaseManager::onUnitDestroy(BWAPI::Unit* u)
{ 
	if (u->getPlayer()==Broodwar->self() && (u->getType() == UnitTypes::Terran_SCV || u->getType() == UnitTypes::Terran_Command_Center)){
		for each(BaseClass* b in this->_mAllBaseSet){
			//	b->onBaseDestroy(u);
			if (b->getBaseLocation()->getTilePosition() == u->getTilePosition()){
				this->_mAllBaseSet.erase(b);
				for each(Unit* mi in b->getMinerals()){
					allMineralSet.erase(mi);
				}			
				for each(Unit* mi in b->getGeysers()){
					allGeyserSet.erase(mi);
				}
				break;
			}		
		}
	}	
	allMineralSet.erase(u);
	allGeyserSet.erase(u);
}


BaseManager::BaseManager()
{
	BaseClass* startBase = new BaseClass(BWTA::getStartLocation(Broodwar->self()));
	mineralSize = 0;
	geryserSize = 0;
	this->checkflag = true;
	allMineOut = false;
	this->allBaseLocations = BWTA::getBaseLocations();
	this->myStartBase = BWTA::getStartLocation(Broodwar->self());
	this->counter = (int)allBaseLocations.size();
	//BLtoBCMap[BWTA::getStartLocation(Broodwar->self())]=startBase;
	BLtoCCMap.clear();
	BLtoBCMap.clear();
	_mAllBaseSet.insert(startBase);
	allMineralSet.clear();
	planedExpansionSet.clear();
	EnemyOnBL=false;
	this->scm = ScoutManager::create();
	this->bom = NULL;

	this->enmeyStartLocation = NULL;
	this->locationHasEnemy.clear();

	this->ProtectGroupCount = 2;
	//this->moveflag = false;

}



std::set<BWAPI::Unit*>& BaseManager::getMyMineralSet()
{
	return this->allMineralSet;
}



void BaseManager::update()
{
	if (Broodwar->getFrameCount()%24==0)
	{
		//check command center on tile
		for each(BWTA::BaseLocation* bl in BWTA::getBaseLocations())
		{	
			if (bl->getMinerals().empty())
			{
				continue;
			}
			
			if (BLtoBCMap.find(bl) == BLtoBCMap.end())
			{
				TilePosition tp = bl->getTilePosition();
				for each(Unit* u in Broodwar->getUnitsOnTile(tp.x(),tp.y()))
				{
					if (u->getPlayer() == Broodwar->self() && u->getType().isResourceDepot() && !u->isLifted() &&
							((u->isConstructing() && u->getRemainingBuildTime() < 24*10) || u->isCompleted()))
					{
						if (BLtoBCMap.find(bl) == BLtoBCMap.end())
						{
							BaseClass* base = new BaseClass(bl);
							BLtoBCMap[bl] = base;
							BLtoCCMap[bl] = u;
							allMineralSet.insert(base->getMinerals().begin(), base->getMinerals().end());
							allGeyserSet.insert(base->getGeysers().begin(),base->getGeysers().end());
							if(base->getBaseLocation() != BWTA::getStartLocation(Broodwar->self()))
							{
								this->checkflag = false;
								this->_mAllBaseSet.insert(base);
								this->checkflag = true;
							}
						}				
					}	
				}
				
				for each(Unit* u in Broodwar->getUnitsInRadius(bl->getPosition(),32*5))
				{
					if (u->getPlayer() == Broodwar->self() &&	u->getType().isResourceDepot() && !u->isLifted() &&
						  ((u->isConstructing() && u->getRemainingBuildTime()<24*10) || u->isCompleted()))
					{
						if (BLtoBCMap.find(bl)==BLtoBCMap.end())
						{
							BaseClass* base = new BaseClass(bl);
							BLtoBCMap[bl] = base;
							BLtoCCMap[bl] = u;
							allMineralSet.insert(base->getMinerals().begin(), base->getMinerals().end());
							allGeyserSet.insert(base->getGeysers().begin(),base->getGeysers().end());
							if(base->getBaseLocation() != BWTA::getStartLocation(Broodwar->self()))
							{
								this->checkflag = false;
								this->_mAllBaseSet.insert(base);
								this->checkflag = true;
							}				
						}	
					}
				}
			}

			if (BLtoCCMap.find(bl) != BLtoCCMap.end()){
				if(BLtoCCMap[bl]->isLifted() || BLtoCCMap[bl]->getHitPoints() <= 0){
					this->checkflag = false;
					//this->mAllBaseSet.erase(BLtoBCMap[bl]);
					BLtoCCMap.erase(bl);
					BLtoBCMap.erase(bl);
					this->checkflag = true;
				}

			}
		}

		/*for each(BaseClass* bl2 in this->mAllBaseSet){
			if (bl2->getBaseLocation()!= this->myStartBase){
				if(this->needProtection(bl2) == true){
					this->ProtectArmy();
					this->sendArmyProtect(this->protectGroup,bl2);
				}
			}		
		}*/
		
		for each (BaseClass* bc in this->_mAllBaseSet)
		{
			bc->scvDefendBase();	
		}
				
		for each(BaseClass* bc in _mAllBaseSet)
		{
			if (bc->isMinedOut())
			{
				BLtoCCMap.erase(bc->getBaseLocation());
				BLtoBCMap.erase(bc->getBaseLocation());
			}
		}
	}
}



std::set<BaseClass*>& BaseManager::getBaseSet()
{
	return this->_mAllBaseSet;

}

std::set<BaseClass*> BaseManager::getEffectiveBaseSet()
{
  std::set<BaseClass*> effectiveSet;
	for each (BaseClass* b in _mAllBaseSet)
  {
    if (!b->isMinedOut())
    {
      effectiveSet.insert(b);
    }
  }
  return effectiveSet;
}


std::set<Unit*>& BaseManager::getAllMineralSet()
{
	/*for (std::set<BaseClass*>::const_iterator b = mAllBaseSet.begin(); b != mAllBaseSet.end(); b++){
		for each(Unit* mineral in (*b)->getMinerals()){
			this->allMineralSet.insert(mineral);
		}
	}*/
	return this->allMineralSet;
}

std::set<Unit*>& BaseManager::getAllGeyserSet()
{
	/*this->mGeyserSet.clear();
	for (std::set<BaseClass*>::const_iterator b = mAllBaseSet.begin(); b != mAllBaseSet.end(); b++){
		for each(Unit* geyser in (*b)->getGeysers()){
			this->mGeyserSet.insert(geyser);
		}
	}*/

	return this->allGeyserSet;
}

std::map<BWTA::BaseLocation*,BWAPI::Unit*>& BaseManager::getBLtoCCMap()
{
	return this->BLtoCCMap;
}

std::map<BWTA::BaseLocation*,BaseClass*>& BaseManager::getBLtoBCMap()
{
	return this->BLtoBCMap;
}

bool BaseManager::getAllMineOutFlag()
{
	int remain = 0;
	for (std::set<Unit*>::const_iterator i = allMineralSet.begin(); i!= allMineralSet.end(); i++){
		remain += (*i)->getResources();	
	}
	if (remain == 0) allMineOut = true;

	return allMineOut;
}


void BaseManager::expandPlan()
{
	// counter is all baselocations' number
	for(this->counter;this->counter > 0;this->counter--){
		//set a null location as a candidate place for expansion 
		BWTA::BaseLocation* location=NULL;
		// sorting/ordering all baselocation based on the distance to my startLocation, as candidate 1,2,3,4... to expand
		// find nearest place to expand a base
		double minDist=-1;	
		for each (BWTA::BaseLocation* bl in this->allBaseLocations){
			if(bl == this->myStartBase)
				continue;
			//else if (bl->getGeysers().size()<1){
			//	this->allBaseLocations.erase(bl);
			//	break;
			//}

			else if (bl->isIsland()){
				this->allBaseLocations.erase(bl);
				break;
			}
			double dist=this->myStartBase->getTilePosition().getDistance(bl->getTilePosition());
			if (dist>0){
				if (minDist == -1 || dist<minDist){
					minDist = dist;
					location = bl;
				}
			}
		}
		// if this baselocation can be found, add to PlanExpansionSet, and delete from allBaseLocations,
		//continue to find 2nd nearest one 
		if (location!=NULL){
			this->planedExpansionSet.insert(location);
			this->allBaseLocations.erase(location);
		}
	}
}

std::set<BWTA::BaseLocation*>& BaseManager::getPlanExpansionSet()
{
	return this->planedExpansionSet;
}

void BaseManager::setManagers(BuildOrderManager* b)
{
	this->bom = b;
}

BWTA::BaseLocation* BaseManager::getEnemyStartLocation()
{
	return this->enmeyStartLocation  = scm->enemyStartLocation;
}
std::set<BWTA::BaseLocation*>& BaseManager::getLocationHasEnemySet()
{
	return this->scm->LocationsHasEnemy;

}


bool BaseManager::needProtection(BaseClass* bc)
{
	if (bc->getBaseLocation()== this->myStartBase){

		return false;
	}		
	/*if (this->moveflag== true)
	{
     //this->sendArmyProtect(this->protectGroup,bl2);
     return false;
	}*/
	BWTA::Region* myRegion = BWTA::getRegion(bc->getBaseLocation()->getPosition());

	if((int)SelectAll()(Siege_Tank)(isSieged).inRegion(myRegion).size() >= this->ProtectGroupCount)
	//if(!SelectAll(Broodwar->self(), UnitTypes::Terran_Siege_Tank_Siege_Mode).inRegion(myRegion).empty())
	{
	//	Broodwar->printf("not send");
		return false;
	}
	if((int)SelectAll()(Siege_Tank)(isSieged).inRegion(myRegion).size() < this->ProtectGroupCount)
	//if(SelectAll(Broodwar->self(), UnitTypes::Terran_Siege_Tank_Siege_Mode).inRegion(myRegion).empty())
	{
		//Broodwar->printf("send");
		return true;
	}
	if(!SelectAll(Broodwar->self(), UnitTypes::Terran_Missile_Turret).inRegion(myRegion).empty())
	{
		//Broodwar->printf("not creat");
		return true;

	}
	if(SelectAll(Broodwar->self(), UnitTypes::Terran_Missile_Turret).inRegion(myRegion).empty())
	{
		//Broodwar->printf("creat xyz");
		return true;

	}
	return false;

}

void BaseManager::ProtectArmy()
{
	std::set<BWAPI::Unit*> protectarmy=SelectAll(Broodwar->self(), UnitTypes::Terran_Siege_Tank_Tank_Mode)(isCompleted).not(isAttacking);//.not(isSieged);
	this->protectGroup.clear();
	int n=(int)protectarmy.size();
	n=2;
	//int t=(int)this->mAllBaseSet.size();
	//t=t-1;
	//n=n*t;
	for each(BaseClass* bl in this->_mAllBaseSet)
	{
		if(bl->getBaseLocation() == this->myStartBase) continue;
		if(n>=(int)protectGroup.size() &&0<(int)protectGroup.size())
		{
			for each(BWAPI::Unit* u in protectarmy)
			{
				if(bl->getBaseLocation() != this->myStartBase && u->getTilePosition()==bl->getBaseLocation()->getTilePosition())
				{	
					if (protectarmy.find(u)!=protectarmy.end()){
						protectarmy.erase(u);
					}
				}
			}
		}
	}
	this->gruopcount =0;
	for each(Unit* u in protectarmy){
		if (gruopcount>=n)
			break;
		else{
			if((int)protectGroup.size()<n)
				this->protectGroup.insert(u);
			gruopcount++;
		}
	}
	//if((int)protectGroup.size()<n)this->moveflag=true;


}

void BaseManager::sendArmyProtect(UnitGroup ug, BaseClass* bc)
{

	BWTA::Region* myRegion = BWTA::getRegion(bc->getBaseLocation()->getPosition());
	if(SelectAll(Broodwar->self(), UnitTypes::Terran_Missile_Turret).inRegion(myRegion).empty()){
		int n = SelectAll(Broodwar->self(),UnitTypes::Terran_Missile_Turret).size();
		bom->build(n+1,UnitTypes::Terran_Missile_Turret,120,bc->getBaseLocation()->getTilePosition());
	}

	this->gruopcount=0;
	if((int)SelectAll(Broodwar->self(), UnitTypes::Terran_Siege_Tank_Tank_Mode).inRegion(myRegion).size()<=2)
	{
		for each(BWAPI::Unit* u in ug)
		{
			if((int)SelectAll(Broodwar->self(), UnitTypes::Terran_Siege_Tank_Tank_Mode).inRegion(myRegion).size()<=2&&this->gruopcount<=2)
			{
				u->move(bc->getBaseLocation()->getPosition());
				this->gruopcount++;
			}
			double dis = u->getTilePosition().getDistance(bc->getBaseLocation()->getTilePosition());
			if (dis>=0 && dis<=4)
			{	
				if (u->getType()==UnitTypes::Terran_Siege_Tank_Tank_Mode)
					u->siege();
			}
		}
		//if((int)SelectAll(Broodwar->self(), UnitTypes::Terran_Siege_Tank_Tank_Mode).inRegion(myRegion).size()>=2)
		//	this->moveflag=false;
	}
}