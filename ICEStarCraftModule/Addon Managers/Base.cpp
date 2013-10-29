#include "Base.h"
#include "UnitGroup.h"
#include "UnitGroupManager.h"
using namespace BWAPI;
using namespace std;

std::set<Unit*> emptySet;

BaseClass::BaseClass(BWTA::BaseLocation* b)
{
	mBaseLocation	= b;
	mBaseSet.clear();
	//mMineralSet.clear();
	
	//my resource set
	//mMineralSet.clear();
	mGeyserSet.clear();
	mRefineries.clear();

	_allBaseTile.clear();
	allMyBaseLocations.clear();
	this->workerNearBase.clear();
	this->thisCommandCenter = NULL;
	this->mMinedOut = false;
	this->currentWorkerNum =0;
	this->lackWorkerNum =0;
	this->overWorkerNum =0;
	this->workerNumForBase =0;
	this->worker = WorkerManager::create();
	this->NeedMoreWorker = false;
	this->enemyToDefend.clear();
	this->scvDefendTeam.clear();
	protectors.clear();
}

int BaseClass::getCurrentWorkerNum()
{
	return this->currentWorkerNum;
}

int BaseClass::getOverWorkerNum()
{
	SetWorkerNumBalance();
	return this->overWorkerNum;
}

int BaseClass::getLackWorkerNum()
{
	SetWorkerNumBalance();
	return this->lackWorkerNum;
}

bool BaseClass::getNeedMoreWorker(){
return this->NeedMoreWorker;
}

void BaseClass::SetWorkerNumBalance()
{
	if (this->currentWorkerNum >= this->workerNumForBase)
	{
		this->overWorkerNum = this->currentWorkerNum - this->workerNumForBase;
	}
	else
	{
		this->lackWorkerNum = this->workerNumForBase - this->currentWorkerNum;
	}
}

int BaseClass::getNeedWorkerNum()
{
	if (getGeysers() == emptySet)
	{
		workerNumForBase = (int)(getMinerals().size()*2.5);
	}
	else
		workerNumForBase = (int)(getMinerals().size()*2.5+getGeysers().size()*3);
	return workerNumForBase;
}


bwtaBL BaseClass::getBaseLocation() 
{
	return mBaseLocation;
}

const std::set<BWAPI::Unit*>& BaseClass::getMinerals() const
{
	if (mBaseLocation == NULL)
		return std::set<BWAPI::Unit*>();
	else
		return mBaseLocation->getMinerals(); 
}
const std::set<BWAPI::Unit*>& BaseClass::getGeysers() const
{
	if (mBaseLocation == NULL)
		return std::set<BWAPI::Unit*>();
	else
		return mBaseLocation->getGeysers();

}


bool BaseClass::isMinedOut()
{
	if (getMinerals().empty())
		return true;
	else
		return false;

}


void BaseClass::setWorkerconfig()
{	
	//find the command center, check is it really a command center on this TilePosition
	for each(Unit* u in Broodwar->getUnitsOnTile(this->mBaseLocation->getTilePosition().x(),this->mBaseLocation->getTilePosition().y())){
		//if this command center has already been added in the workerNerBase map, then don't need check, break
		if(this->workerNearBase.find(u)!=this->workerNearBase.end() || this->thisCommandCenter != NULL) 
			break;
		else if (u->getPlayer()==Broodwar->self() && u->getType()==UnitTypes::Terran_Command_Center && !u->isLifted() &&
			((u->isConstructing() && u->getRemainingBuildTime()<24*10) || u->isCompleted())){
				this->thisCommandCenter = u;
		}
		else continue;
	}
	if(this->thisCommandCenter!= NULL){
		std::set<Unit*> finalSCVSet;
		std::set<BWAPI::Unit*> thisMineralSet = getMinerals(); // get the mineral set of this base
		finalSCVSet.clear();
		for (std::set<Unit*>::const_iterator it = Broodwar->self()->getUnits().begin();it != Broodwar->self()->getUnits().end();it++){
			//calculate the number of scvs around this base (distance<15)
			if ((*it)->isCompleted() && (*it)->getType() == UnitTypes::Terran_SCV && ((*it)->getTilePosition().getDistance(this->mBaseLocation->getTilePosition())<15)){
				//if this scv around this base, check if he is really working (not repairing, not constructing)
				if((*it)->getTarget()!=NULL && !(*it)->isRepairing() &&
					(thisMineralSet.find((*it)->getTarget())!=thisMineralSet.end()|| (*it)->getTarget()== this->thisCommandCenter||(*it)->getTarget()->getType() == UnitTypes::Terran_Refinery))
					finalSCVSet.insert((*it));
				else continue;
			}
		}
/*		for each(BWAPI::Unit* u in tempSCVSet){
		if(u->getTarget()!=NULL &&
			(thisMineralSet.find(u->getTarget())!=thisMineralSet.end()|| u->getTarget()== this->thisCommandCenter||u->getTarget()->getType() == UnitTypes::Terran_Refinery))
			finalSCVSet.insert(u);
		else continue;
	}
*/

	// mapping this scv set with this commandCenter
	this->workerNearBase[this->thisCommandCenter] = finalSCVSet;
	// get the first element in this map, in fact, there's only element in this map, because one baseClass only have one command center
	if (!this->workerNearBase.empty()){
		std::map<BWAPI::Unit*,std::set<BWAPI::Unit*>>::const_iterator uniqueRecord = this->workerNearBase.begin();
		// the scv set size is current worker number
		this->currentWorkerNum = (int)uniqueRecord->second.size();
	}

	if (this->currentWorkerNum<=3)
		this->NeedMoreWorker = true;
	else
		this->NeedMoreWorker = false;
	// calculate how many scv over or lack
	SetWorkerNumBalance();
	}
}

std::map<BWAPI::Unit*,std::set<BWAPI::Unit*>> BaseClass::getWorkerNearBaseSet()
{	
	return this->workerNearBase;
}

void BaseClass::update()
{


}

int BaseClass::getGasWorkerNum()
{
	int GatheringGasNum = 0;
	for(std::map<BWAPI::Unit*,std::set<BWAPI::Unit*>>::const_iterator it= this->workerNearBase.begin(); it!= this->workerNearBase.end();it++){
		for each(Unit* u in (*it).second){
			if (u->isGatheringGas()||(u->getTarget()->getType() == UnitTypes::Terran_Refinery)) GatheringGasNum++;
			else continue;
		}
	}
	return GatheringGasNum;
}


void BaseClass::onBaseDestroy(BWAPI::Unit* unit)
{
	if (this->workerNearBase.empty()) return;
	//get the unique element in this map
	std::map<Unit*,std::set<Unit*>>::iterator ur = this->workerNearBase.begin();
	// check if this command center or any SCV destroyed

	if (ur->first == unit){// command center
		this->workerNearBase.erase(unit);
		this->thisCommandCenter = NULL;
	}

	else{
		set<Unit*>::iterator i = ur->second.find(unit);
		if (i !=ur->second.end())// scv
			ur->second.erase(i);
	}
}


//void BaseClass::scvDefendBase()
//{
//	double odur = UnitTypes::Terran_SCV.groundWeapon().damageAmount()*1.0/UnitTypes::Terran_SCV.groundWeapon().damageCooldown()*60;
//	BWTA::Region* baseReg = getRegion(getBaseLocation()->getTilePosition());
//	this->enemyToDefend = SelectAllEnemy()(isVisible).not(isFlyer,isLifted).inRegion(baseReg);
//	if (!this->enemyToDefend(Lurker,Reaver).empty()) 
//		return;
//	if (SelectAll()(isCompleted,canAttack).not(isWorker,isBuilding,isLoaded).inRadius(32*12,baseReg->getCenter()).size()>=10) 
//		return;
//	if (SelectAll()(isCompleted)(Bunker).inRegion(baseReg).size()>0 &&
//		enemyToDefend.inRadius(32*5,BWTA::getStartLocation(Broodwar->self())->getPosition()).size()<1) 
//		return;
//
//	for (set<Unit*>::iterator i = this->scvDefendTeam.begin(); i!=this->scvDefendTeam.end();)
//	{
//		if (getRegion((*i)->getPosition()) != baseReg)
//		{	
//			this->worker->_workerUnits.insert(*i);
//			if (this->worker->_workersTarget[*i]!=NULL)
//			{				
//				(*i)->rightClick(this->worker->_workersTarget[*i]);
//				i = this->scvDefendTeam.erase(i);
//			}
//			else
//				i++;				
//		}
//		else if((*i)->isConstructing())
//		{
//			this->worker->_workerUnits.insert(*i);
//			i = this->scvDefendTeam.erase(i);
//		}
//		else if((*i)->getHitPoints()<=15)
//		{
//			this->worker->_workerUnits.insert(*i);
//			if (this->worker->_workersTarget[*i]!=NULL)
//			{				
//				(*i)->rightClick(this->worker->_workersTarget[*i]);
//				i = this->scvDefendTeam.erase(i);
//			}
//			else
//			{
//				i++;
//				//if (_workersTarget[*i]!=NULL){
//				//	(*i)->rightClick(_workersTarget[*i]);
//				//	i = this->scvDefendTeam.erase(i);
//				//}
//			}		
//		}
//
//		else
//			i++; 
//	}
//	if(this->enemyToDefend.empty())
//	{
//		for (set<Unit*>::iterator i = this->scvDefendTeam.begin(); i!=this->scvDefendTeam.end();)
//		{
//			this->worker->_workerUnits.insert(*i);
//			if (this->worker->_workersTarget[*i]!=NULL)
//			{				
//				(*i)->rightClick(this->worker->_workersTarget[*i]);
//				i = this->scvDefendTeam.erase(i);
//			}
//			else
//				i++;			
//		}
//		return;
//	}
//
//	// Only 1 scout
//	double edur = 0;
//	int buildingNum =0;
//	for each (Unit* u in this->enemyToDefend)
//	{
//		UnitType t = u->getType();
//		if(t.isWorker())
//			edur += t.groundWeapon().damageAmount()*1.0/t.groundWeapon().damageCooldown()*(u->getHitPoints()+u->getShields()-1);
//		else if(t.isBuilding())
//			buildingNum++;
//		else if(t.groundWeapon().damageCooldown() != 0)
//			edur += t.groundWeapon().damageAmount()*2.0/t.groundWeapon().damageCooldown()*(u->getHitPoints()+u->getShields()-1);
//	}
//
//	//the number of scv we need 
//	int num = (int)(edur/odur) + 1 + buildingNum*2;
//	if ((int)this->scvDefendTeam.size() >= num)
//	{
//		if ((int)this->scvDefendTeam.size() > num)
//		{	
//			UnitGroup::iterator i = this->scvDefendTeam.begin();
//			this->worker->_workerUnits.insert(*i);
//			if (this->worker->_workersTarget[*i]!=NULL)
//			{
//				(*i)->rightClick(this->worker->_workersTarget[*i]);
//				this->scvDefendTeam.erase(i);
//			}
//
//		}
//		Position p = this->enemyToDefend.getNearest(this->scvDefendTeam.getCenter())->getPosition();
//		for each (Unit* u in this->scvDefendTeam)
//		{
//			if (!u->isAttacking()||u->getGroundWeaponCooldown()==0)
//				u->attack(p);
//		}
//		return;
//	}
//
//	UnitGroup avaiWor = this->worker->_workerUnits(HitPoints,">=",20)(isCompleted).not(isAttacking,isConstructing);
//	if (avaiWor.empty()) return;
//	for (set<Unit*>::iterator i = avaiWor.begin(); i!=avaiWor.end(); i++)
//	{
//		if (BWTA::getRegion((*i)->getPosition())!=baseReg)
//			continue;
//		this->scvDefendTeam.insert(*i);
//		this->worker->_workerUnits.erase(*i);
//		if ((int)this->scvDefendTeam.size() >= num)
//			break;
//	}
//	Position p = this->enemyToDefend.getNearest(this->scvDefendTeam.getCenter())->getPosition();
//	for each (Unit* u in this->scvDefendTeam)
//	{
//		if (!u->isAttacking() ||u->getGroundWeaponCooldown()==0) u->attack(p);
//	}
//}

void BaseClass::scvDefendBase()
{
	BWTA::BaseLocation* base = this->mBaseLocation;
	enemyToDefend.clear();

	for each (Unit* e in Broodwar->enemy()->getUnits())
	{
		if (!e->isDetected() || e->isLifted() ||
			  e->getType().isFlyer() || e->getType() == UnitTypes::Terran_Vulture_Spider_Mine || e->getType() == UnitTypes::Protoss_Scarab)
		{
			continue;
		}
		if (base == BWTA::getStartLocation(Broodwar->self()))
		{
			if (BWTA::getRegion(e->getPosition()) == base->getRegion())
			{
				enemyToDefend.insert(e);
			}
		}
		else
		{
			if (e->getTilePosition().getDistance(base->getTilePosition()) < 12)
			{
				enemyToDefend.insert(e);
			}
		}
	}

	UnitGroup workers;
	UnitGroup army;
	UnitGroup bunkers;

	for each (Unit* u in Broodwar->self()->getUnits())
	{
		if (u->isCompleted() && u->getType() == UnitTypes::Terran_SCV && !u->isConstructing() && !u->isRepairing())
		{
			if (base == BWTA::getStartLocation(Broodwar->self()))
			{
				if (BWTA::getRegion(u->getPosition()) == base->getRegion())
				{
					workers.insert(u);
				}
			}
			else
			{
				if (u->getTilePosition().getDistance(base->getTilePosition()) < 12)
				{
					workers.insert(u);
				}
			}
		}

		if (u->isCompleted() && !u->isLoaded() && u->getType().canAttack() && !u->getType().isWorker() && !u->getType().isBuilding() &&
				u->getType() != UnitTypes::Terran_Vulture_Spider_Mine)
		{
			if (base == BWTA::getStartLocation(Broodwar->self()))
			{
				if (BWTA::getRegion(u->getPosition()) == base->getRegion())
				{
					army.insert(u);
				}
			}
			else
			{
				if (u->getTilePosition().getDistance(base->getTilePosition()) < 12)
				{
					army.insert(u);
				}
			}
		}

		if (u->getType() == UnitTypes::Terran_Bunker && !u->getLoadedUnits().empty())
		{
			if (base == BWTA::getStartLocation(Broodwar->self()))
			{
				if (BWTA::getRegion(u->getPosition()) == base->getRegion())
				{
					bunkers.insert(u);
				}
			}
			else
			{
				if (u->getTilePosition().getDistance(base->getTilePosition()) < 12)
				{
					bunkers.insert(u);
				}
			}
		}
	}

	// check if we need to defend at this base
	if (enemyToDefend.empty()
		  ||
			enemyToDefend.size() > 8
			||
			workers.size() + scvDefendTeam.size() < 1
			||
			army.size() >= 3
			||
			(Broodwar->getFrameCount() < 24*60*10 && !bunkers.empty())
			||
			!enemyToDefend(Lurker,Reaver).empty())
	{
		// no need to defend at this base
		for (set<Unit*>::iterator i = this->scvDefendTeam.begin(); i != this->scvDefendTeam.end();)
		{
			Unit* scv = *i;
			this->worker->_workerUnits.insert(scv);
			if (this->worker->_workersTarget[scv] != NULL)
			{				
				scv->rightClick(this->worker->_workersTarget[scv]);
			}
			this->scvDefendTeam.erase(i++);
		}
		return;
	}

	UnitType SCV = UnitTypes::Terran_SCV;
	double scvPower = 1.0 * (SCV.maxHitPoints() + SCV.maxShields()) * SCV.groundWeapon().damageAmount() / SCV.groundWeapon().damageCooldown();

	double ePower = 0;
	int buildingNum = 0;

	for each (Unit* e in this->enemyToDefend)
	{
		UnitType type = e->getType();
		
		if (type.isWorker())
		{
			ePower += 1.0 * (e->getHitPoints() + e->getShields()) * type.maxGroundHits() * type.groundWeapon().damageAmount() / type.groundWeapon().damageCooldown();
		}
		else if (type.isBuilding() && !type.canAttack())
		{
			buildingNum++;
		}
		else if (type.groundWeapon() != WeaponTypes::None)
		{
			ePower += 2.0 * (e->getHitPoints() + e->getShields()) * type.maxGroundHits() * type.groundWeapon().damageAmount() / type.groundWeapon().damageCooldown();
		}
	}
	
	// the number of scv we need 
	int num = (int)(ePower / scvPower) + 1 + 2 * buildingNum;
	//Broodwar->printf("ePower: %.2f | scvPower: %.2f | need %d",ePower,scvPower,num);

	// exception: remove SCVs that are constructing or have too low HP
	for (set<Unit*>::iterator i = this->scvDefendTeam.begin(); i != this->scvDefendTeam.end();)
	{
		Unit* scv = *i;
		if (scv->isConstructing() || scv->getHitPoints() < 15)
		{
			this->worker->_workerUnits.insert(scv);
			this->scvDefendTeam.erase(i++);
		}
		else
		{
			++i;
		}
	}

	// add scv to defend team
	if (this->scvDefendTeam.size() < num)
	{
		for (set<Unit*>::iterator i = workers.begin(); i != workers.end(); i++)
		{
			if (this->scvDefendTeam.size() >= num)
			{
				break;
			}

			Unit* scv = *i;
			if (this->scvDefendTeam.find(scv) != this->scvDefendTeam.end() || scv->isConstructing())
			{
				continue;
			}

			this->scvDefendTeam.insert(scv);
			this->worker->_workerUnits.erase(scv);
		}
	}
	// remove scv from defend team
	else if (this->scvDefendTeam.size() > num)
	{
		for (set<Unit*>::iterator i = this->scvDefendTeam.begin(); i != this->scvDefendTeam.end();)
		{
			if (this->scvDefendTeam.size() <= num)
			{
				break;
			}

			Unit* scv = *i;
			this->worker->_workerUnits.insert(scv);
			if (this->worker->_workersTarget[scv] != NULL)
			{				
				scv->rightClick(this->worker->_workersTarget[scv]);
			}
			this->scvDefendTeam.erase(i++);
		}
	}

	// attack enemy
	Unit* target = this->enemyToDefend.size() == 1 ? *(this->enemyToDefend.begin()) : NULL;
	Position targetPos = this->enemyToDefend.getCenter();
	for each (Unit* scv in this->scvDefendTeam)
	{
		if (target)
		{
			if (scv->getLastCommand().getType() == UnitCommandTypes::Attack_Unit && scv->getLastCommand().getTarget() == target)
			{
				continue;
			}
			scv->attack(target);
		}
		else
		{
			if (scv->isAttacking())
			{
				continue;
			}

			if (scv->getLastCommand().getType() == UnitCommandTypes::Attack_Move && scv->getLastCommand().getTargetPosition().getApproxDistance(targetPos) < 64)
			{
				continue;
			}

			scv->attack(targetPos);
		}	
	}
}