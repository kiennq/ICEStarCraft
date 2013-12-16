#include <algorithm>
#include "WorkerManager.h"
#include "BaseManager.h"
#include <BWTA.h>
#include "UnitGroup.h"
#include "UnitGroupManager.h"

using namespace BWAPI;
using namespace BWTA;
using namespace std;

WorkerManager* theWorkerManager = NULL;

WorkerManager* WorkerManager::create(){
	if (theWorkerManager) 
		return theWorkerManager;
	else
  {
		theWorkerManager = new WorkerManager();
		return theWorkerManager;
	}	
}

void WorkerManager::destroy()
{
	if (theWorkerManager) delete theWorkerManager;
}

WorkerManager::WorkerManager()
	:_mineralPS(0)
	,_mineralPM(0)
	,_averageMineralPS(0)
	,_averageMineralPM(0)
{
	arbitrator = NULL;

	// Initialize the counters for minerals per second, or per minute:
	for(int i = 0;i<61;i++) _accumluatedMinerals[i]=50;
	_lastFrameCount = GetTickCount();

//********************previous code
	this->_bmc = NULL;

	/*setBaseManagerClass(bmc);
	set<BWAPI::Unit*> allMyMineral = this->bmc->getMyMineralSet();
	for(UnitSet::const_iterator i=allMyMineral.begin();i!=allMyMineral.end();i++) {
			_mineralsExploitation[*i].first = 0;
			_mineralsExploitation[*i].second = (int)Broodwar->self()->getStartLocation().getDistance((*i)->getTilePosition()) ;
	
		
	}*/
	this->_mineralRate       = 0;
	this->_gasRate           = 0;
	
	_workerState.clear();
	_workerBuildOrder.clear();
	_workerBuildingRefinery.clear();
	this->_rebalancing = false;
	this->_autoBuild         = false;
	this->_autoBuildPriority = 65;
	this->_optimalWorkerCount = 0;
	this->_WorkersPerGas = -1;
	this->_needTotalWorkerNum = 0;
	this->_currentNum =0;
	this->_lastNum = 0;
	this->_allMineral = Broodwar->getMinerals();
	this->_lastRebalanceTime = 0;
	this->_repairGroup.clear();
	this->_repairList.clear();
	this->_scvDefendTeam.clear();
	this->_enemyToDefend.clear();
	this->constructingSCV.clear();
	this->_mental=MentalClass::create();
	this->_gf=NULL;
	this->_mInfo = NULL;
	this->_eInfo = NULL;
	_repairGroupSize = 1;
};

void WorkerManager::onOffer(std::set<BWAPI::Unit*> units)
{
	for each (Unit* u in units)
	{
		arbitrator->accept(this, u);
    addUnit(u);
		_workerUnits.insert(u);
	}
}

void WorkerManager::onRevoke(BWAPI::Unit* u, double bid)
{
  _workerUnits.erase(u);
	_repairGroup.erase(u);
}

void WorkerManager::update()
{
	// onFrame
}

void WorkerManager::setBaseManagerClass(BaseManager* bmc)
{
	this->_bmc = bmc;
	this->_gf = GameFlow::create();
	this->_mInfo = MyInfoManager::create();
	this->_eInfo = EnemyInfoManager::create();
}

void WorkerManager::addUnit(Unit* newWorker) 
{
	_workerUnits.insert(newWorker);	  
	WorkerData temp;
	_workers.insert(make_pair(newWorker,temp));
	for each(BaseClass* bc in this->_bmc->getBaseSet())
	{
		if (bc->getCurrentWorkerNum() >= bc->getNeedWorkerNum())
		{
			Unit* bestMineral = getBestGlobalMineral();
			if (!bestMineral) continue;
			_workersTarget[newWorker] = bestMineral;			
			_workerState[newWorker] = Gathering_Mineral;
			// increase mineral exploitation
			_mineralsExploitation[bestMineral].first = _mineralsExploitation[bestMineral].first++;//every time this mineral being target,then score+1,in order to find next best mineral
			_mineralsExploitation[bestMineral].second = _mineralsExploitation[bestMineral].second + 5;// every time this mineral's distance being measured, then distance+5,in order to find next best mineral 
		}
		else if (bc->getCurrentWorkerNum() < bc->getNeedWorkerNum())
		{
			Unit* bestMineral = getBestLocalMineral(bc);
			if (!bestMineral) continue;
			if (newWorker->getTilePosition().getDistance(bestMineral->getTilePosition()) > 15)
				continue;
			else
			{
			_workersTarget[newWorker] = bestMineral;
			_workerState[newWorker] = Gathering_Mineral;
			// increase mineral exploitation
			_mineralsExploitation[bestMineral].first = _mineralsExploitation[bestMineral].first++;//every time this mineral being target,then score+1,in order to find next best mineral
			_mineralsExploitation[bestMineral].second = _mineralsExploitation[bestMineral].second + 5;// every time this mineral's distance being measured, then distance+5,in order to find next best mineral
			}	
		}
	}
	//for exception
	if (_workersTarget[newWorker]==NULL||!_workersTarget[newWorker]->exists())
	{
		if (!this->_bmc->getAllMineralSet().empty())
		{
			Unit* mMineralPitch = (*this->_bmc->getAllMineralSet().begin());
			_workersTarget[newWorker] = mMineralPitch;
		}
		//if we are mined out,find a visible mineral to him
		else
		{
			for (std::set<BWAPI::Unit*>::const_iterator it = _allMineral.begin(); it != _allMineral.end(); it++)
			{
				if ((*it)->isVisible() && (*it)->exists())
				{
					_workersTarget[newWorker] = (*it);
					return;
				}
				else continue;
			}
		}
	}

	//// assign best mineral
	//Unit* bestMineral = getBestMineral();
	//_workersTarget[newWorker] = bestMineral;
	//_workerState[newWorker] = Gathering_Mineral;
	//// increase mineral exploitation
	//_mineralsExploitation[bestMineral].first = _mineralsExploitation[bestMineral].first++;//every time this mineral being target,then score+1,in order to find next best mineral
	//_mineralsExploitation[bestMineral].second = _mineralsExploitation[bestMineral].second + 5;// every time this mineral's distance being measured, then distance+5,in order to find next best mineral 
}

Unit* WorkerManager::getBestGlobalMineral()
{
	set<BaseClass*> trybase = this->_bmc->getBaseSet();
	int bestScore;
	Unit* bestMineral;
	int shortestDis;
	for (set<BaseClass*>::iterator b=trybase.begin(); b!=trybase.end();b++)
	{	
		set<Unit*> thisBaseMineral = (*b)->getMinerals();
		bestScore = 999;
		bestMineral = 0;
		shortestDis = 999999999;
	
		//add new minerals to global set
		for each(Unit* u in thisBaseMineral)
		{
			if (_mineralsExploitation.find(u)==_mineralsExploitation.end())
				_mineralsExploitation.insert(make_pair(u,make_pair(0,(int)(*b)->getBaseLocation()->getTilePosition().getDistance(u->getTilePosition()))));	
		}
		if ((*b)->getCurrentWorkerNum()>=(*b)->getNeedWorkerNum())
			continue;
		else
		{
			for (ResourceToWorkerMap::iterator j = _mineralsExploitation.begin(); j != _mineralsExploitation.end(); j++){
				if (thisBaseMineral.find(j->first) == thisBaseMineral.end())
					continue;
				else
				{
					if (j->second.first <= bestScore)
					{
						bestScore = j->second.first;
						if ((*b)->getBaseLocation()->getTilePosition().getDistance(j->first->getTilePosition()) < shortestDis){
							shortestDis  = j->second.second;
							bestMineral = j->first;	
						}
					}
				}
			}
			if (bestMineral==0)
				return 0;
			else
			{
				//Broodwar->printf("global!");
				return bestMineral;
			}
		}	
	}	
	//Broodwar->printf("can not find bestMineral!");
	ResourceToWorkerMap::iterator j = _mineralsExploitation.begin();
	return j->first;
}

Unit* WorkerManager::getBestLocalMineral(BaseClass* b)
{
	//set<BaseClass*> trybase = this->bmc->getBaseSet();
	int bestScore;
	Unit* bestMineral;
	int shortestDis;
//	for (set<BaseClass*>::iterator b=trybase.begin(); b!=trybase.end();b++){	
		
		set<Unit*> thisBaseMineral = b->getMinerals();
		bestScore = 999;
		bestMineral = 0;
		shortestDis = 999999999;

		//add new minerals to global set
		for each(Unit* u in thisBaseMineral)
		{
			if (_mineralsExploitation.find(u)==_mineralsExploitation.end())
				_mineralsExploitation.insert(make_pair(u,make_pair(0,(int)b->getBaseLocation()->getTilePosition().getDistance(u->getTilePosition()))));	
		}

		//if this base not need worker any more, then produce worker for global use
		

		//if this base still need worker, then produce for itself
			for (ResourceToWorkerMap::iterator j = _mineralsExploitation.begin(); j != _mineralsExploitation.end(); j++){
				if(thisBaseMineral.size()<=0)
					break;
				if (thisBaseMineral.find(j->first) == thisBaseMineral.end())
					continue;
				else{
					if (j->second.first <= bestScore){
						bestScore = j->second.first;
						if (b->getBaseLocation()->getTilePosition().getDistance(j->first->getTilePosition()) < shortestDis){
							shortestDis  = j->second.second;
							bestMineral = j->first;	
						}
					}
				}
			}
			if (bestMineral==0)
				return NULL;
			else{
				//Broodwar->printf("local!");
				return bestMineral;
			}
	//Broodwar->printf("can not find bestMineral!");
	ResourceToWorkerMap::iterator j = _mineralsExploitation.begin();
	return j->first;
}

void WorkerManager::microBalance()
{
	set<BaseClass*> allBaseSet = this->_bmc->getBaseSet();
    
	if (Broodwar->getFrameCount()%30 == 0 && _WorkersPerGas != -1 && this->_rebalancing == false)
	{
		set<Unit*> baseGeysers;
		for (set<BaseClass*>::const_iterator b = allBaseSet.begin();b!= allBaseSet.end();b++)
		{	
			(*b)->setWorkerconfig();
			baseGeysers = (*b)->getGeysers();
			for(set<Unit*>::iterator g = baseGeysers.begin(); g != baseGeysers.end(); g++)
			{
				int temp = 0;
				if ((*b)->getGasWorkerNum() < this->_WorkersPerGas && (*g)->getType().isRefinery() && (*g)->getPlayer()==Broodwar->self() && (*g)->isCompleted())
				{
					temp = this->_WorkersPerGas-(*b)->getGasWorkerNum();
					std::map<BWAPI::Unit*,std::set<BWAPI::Unit*>> workerToBase = (*b)->getWorkerNearBaseSet();
					if (!workerToBase.empty())
					{
						for each(Unit* scv in workerToBase.begin()->second)
						{
							if(temp == 0)
								break;
							if (scv->isCarryingMinerals() || scv->isGatheringGas() || scv->isGatheringGas()||scv->isConstructing())
								continue;
							else
							{
								_workersTarget[scv] = (*g);
								_workerState[scv] = Gathering_Gas;
								scv->rightClick(*g);
								temp--;
							}
						}			
					}
				}
				else if ((*b)->getGasWorkerNum() > this->_WorkersPerGas&& (*g)->getType().isRefinery() && (*g)->getPlayer()==Broodwar->self() && (*g)->isCompleted())
				{
					temp = (*b)->getGasWorkerNum() - this->_WorkersPerGas;
					std::map<BWAPI::Unit*,std::set<BWAPI::Unit*>> workerToBase = (*b)->getWorkerNearBaseSet();
					if (!workerToBase.empty()){
						for each (Unit* scv in workerToBase.begin()->second)
						{
							if(temp == 0) break;
							if(scv->isGatheringGas() || scv->isCarryingGas())
							{
								_workersTarget[scv] = getBestLocalMineral(*b);
								_workerState[scv] = Gathering_Mineral;
								scv->rightClick(_workersTarget[scv]);
								temp--;
							}
						}
					}
				}
			}
		}
	}

	if (allBaseSet.size() >= 2 && Broodwar->getFrameCount()%300 == 0 && this->_rebalancing == false)
	{
		set<BaseClass*> baseNeedWorker;
		set<BaseClass*> baseProvideWorker;
		baseNeedWorker.clear();
		baseProvideWorker.clear();

		//divide bases
		for (set<BaseClass*>::const_iterator b = allBaseSet.begin();b!= allBaseSet.end();b++)
		{
			(*b)->setWorkerconfig();
			// for dangerous base or lifted base
			if ((*b)->getCurrentWorkerNum()>(*b)->getNeedWorkerNum())
			{
				baseProvideWorker.insert(*b);
				//Broodwar->printf("Base (%d,%d) over %d workers",(*b)->getBaseLocation()->getTilePosition().x(),(*b)->getBaseLocation()->getTilePosition().y(),(*b)->getOverWorkerNum());
			}		

			else if ((*b)->getCurrentWorkerNum() == (*b)->getNeedWorkerNum()) continue;
			else
			{
				baseNeedWorker.insert(*b);
				//Broodwar->printf("Base (%d,%d) lack %d workers",(*b)->getBaseLocation()->getTilePosition().x(),(*b)->getBaseLocation()->getTilePosition().y(),(*b)->getLackWorkerNum());
			} 	
		}

		for each(BaseClass* bc in baseProvideWorker)
		{
			if (bc->getOverWorkerNum() <= 0)
			{			
				continue;
			}
			else
			{
				for each(BaseClass* bc2 in baseNeedWorker)
				{
					if (bc2->getLackWorkerNum() <= 0)
					{
						continue;
					}
					else if (bc2->getLackWorkerNum() > 0)
					{
						std::set<Unit*> mPatch = bc2->getMinerals();
						std::set<Unit*> ProviderSet;
						if (!bc->getWorkerNearBaseSet().empty())
						{
							ProviderSet = bc->getWorkerNearBaseSet().begin()->second;
						}
						int i = 0;
						int j = bc->getOverWorkerNum();// limit transfer worker num, in case of too many workers transfer at one time
						for (std::set<Unit*>::const_iterator it = mPatch.begin(); it != mPatch.end(); it++){
							if (bc->getOverWorkerNum() <= 0) break;
							for (std::set<Unit*>::iterator it2 = ProviderSet.begin(); it2 != ProviderSet.end(); it2++ )
							{
								if (j <= 0) break;
								else if (_workerUnits.find(*it2) != _workerUnits.end()&& !(*it2)->isGatheringGas())
								{
									_workersTarget[*it2]=(*it);
									(*it2)->rightClick(*it);
									ProviderSet.erase(*it2);
									i++;
									j--;
									break;
								}
							}
						}
						//Broodwar->printf("%d Workers are transfered from Base (%d,%d) to Base (%d,%d)",i,bc->getBaseLocation()->getTilePosition().x(),
						//	bc->getBaseLocation()->getTilePosition().y(),bc2->getBaseLocation()->getTilePosition().x(),bc2->getBaseLocation()->getTilePosition().y());
						break;
					}
				}
			}
		}
	}
}

void WorkerManager::rebalanceGathering()
{	
	if(Broodwar->getFrameCount()%5 != 0)
		_currentNum = this->_bmc->getBaseSet().size();
	if(Broodwar->getFrameCount()%5 == 0)
		_lastNum = this->_bmc->getBaseSet().size();
	//check the change of base number
	if (_currentNum != _lastNum && _currentNum!=0 && _lastNum!=0)
	{
		if (_lastRebalanceTime==0 || Broodwar->getFrameCount()-_lastRebalanceTime>=24*60)
		{
			//setNeedGasLevel(3);
			//Broodwar->printf("Base Num is %d, begin to rebalance",this->bmc->getBaseSet().size());
			this->_rebalancing = true;
			int transferSCV=0;
			int remainSCV=0;

			set<BaseClass*> allBaseSet = this->_bmc->getBaseSet();
			set<BaseClass*> baseNeedWorker;
			set<BaseClass*> baseProvideWorker;
			baseNeedWorker.clear();
			baseProvideWorker.clear();

			//divide bases
			for (set<BaseClass*>::const_iterator b = allBaseSet.begin();b!= allBaseSet.end();b++)
			{
				(*b)->setWorkerconfig();
				if ((*b)->getNeedMoreWorker())
					baseNeedWorker.insert(*b);

				else if ((*b)->getCurrentWorkerNum() > (int)(*b)->getMinerals().size())
					baseProvideWorker.insert(*b);
			}

			//	Broodwar->printf("Divide base over!");
			for each (BaseClass* bc in baseProvideWorker)
			{
				if (bc->getCurrentWorkerNum() - (int)bc->getMinerals().size() < 5)
					continue;
				else
					transferSCV+=7;
			}

			for each(BaseClass* bc2 in baseNeedWorker)
			{
				std::set<Unit*> mPatch = bc2->getMinerals();
				UnitGroup transGroup = SelectAll()(isCompleted)(SCV).not(isGatheringGas);
				int i = transferSCV;
				for (std::set<Unit*>::const_iterator it = mPatch.begin(); it != mPatch.end(); it ++)
				{
					if (i<=0)
						break;
					for (std::set<Unit*>::iterator it2 = transGroup.begin(); it2 != transGroup.end(); it2++ )
					{
						if (_workerUnits.find(*it2) != _workerUnits.end()){
							_workersTarget[*it2]=(*it);
							(*it2)->rightClick(*it);
							transGroup.erase(*it2);
							i--;
							break;
						}
					}
				}
			}

		_lastRebalanceTime = Broodwar->getFrameCount();
	  //autoBuildWorker();
		}
	}
	this->_rebalancing =false;
}
	
void WorkerManager::setBuildOrderManager(BuildOrderManager* buildOrderManager)
{
	this->_buildOrderManager = buildOrderManager;
}		


void WorkerManager::onFrame() 
{
	//each base at least should have 10 workers, this is to prevent worker endless repair
	//ex: all workers dead for going to repair unit in dangerous place
	if (SelectAll()(isCompleted)(SCV).size() >= 10)
		workerRepair();

	if (Broodwar->getFrameCount()%12 == 0)
	{
		//for auto produing SCV
		autoTrainSCV();

		this->_allMineral = Broodwar->getMinerals();
		if(this->_bmc->checkflag == true)
		{
			rebalanceGathering();
			microBalance();
		}
		this->_mineralRate=0;
		this->_gasRate=0;
		for(map<Unit*,WorkerData>::iterator u = _workers.begin(); u != _workers.end(); u++)
		{
			Unit* i = u->first;
			if (u->second.resource != NULL)
			{
				if (u->second.resource->getType()==UnitTypes::Resource_Mineral_Field)
					_mineralRate+=8/180.0;
				else
					_gasRate+=8/180.0;
			}
		}

		//DEBUG("Update workers state");
		for(UnitSet::const_iterator worker=_workerUnits.begin(); worker!=_workerUnits.end(); worker++)
		{
			Unit* wk = *worker;
			if (wk->getOrder() == Orders::PlayerGuard)
			{
				if(_workerState[wk]==Scouting)
					continue;		
				else if (wk->isIdle() && _workerState[wk] != Scouting)
				{
					//if current worker is not null
					Unit* target = _workersTarget[wk];
					if (target)
					{
						//check if this target is not exist 
						if (target->exists())
						{
							//if target is mineral
							if (target->getType().isMineralField())
							{
								//wk->rightClick(target);
								//continue;
								//if the target is not one of our minerals
								std::set<Unit*> mineralSet = this->_bmc->getAllMineralSet();
								if (!mineralSet.empty())
								{
									if (mineralSet.find(target)==mineralSet.end())
									{
										Unit* mMineralPitch = *mineralSet.begin();
										_workersTarget[wk] = mMineralPitch;
										wk->rightClick(_workersTarget[wk]);
										continue;
									}
									else
									{
										if (wk->isCompleted())
											wk->rightClick(target);
										else
											continue;
									}									
								}
								else
								{
									if (wk->isCompleted())
										wk->rightClick(target);
									else
										continue;
								}		

							}
							else
							{
								//if it is not a mineral, then assign one of my mineral to him
								if (!this->_bmc->getAllMineralSet().empty())
								{
									Unit* mMineralPitch = (*this->_bmc->getAllMineralSet().begin());
									_workersTarget[wk] = mMineralPitch;
									wk->rightClick(_workersTarget[wk]);
								}
								//if we are mined out,find a visible mineral
								else
								{
									for (std::set<BWAPI::Unit*>::const_iterator it = _allMineral.begin(); it != _allMineral.end(); it++)
									{
										if ((*it)->isVisible() && (*it)->exists()){
											_workersTarget[wk] = (*it);
											wk->rightClick(_workersTarget[wk]);
										}
										else continue;
									}
								}
							}
						}
						//if the worker even doesn't have any target
						else
						{
							//assign one of my mineral to him
							if (!this->_bmc->getAllMineralSet().empty())
							{
								Unit* mMineralPitch = (*this->_bmc->getAllMineralSet().begin());
								_workersTarget[wk] = mMineralPitch;
								wk->rightClick(_workersTarget[wk]);
							}
							//if we are mined out,find a visible mineral to him
							else
							{
								for (std::set<BWAPI::Unit*>::const_iterator it = _allMineral.begin(); it != _allMineral.end(); it++)
								{
									if ((*it)->isVisible() && (*it)->exists())
									{
										_workersTarget[wk] = (*it);
										wk->rightClick(_workersTarget[wk]);
									}
									else continue;
								}
							}
						}
					}

					//if (_workersTarget[wk] &&_workersTarget[wk]->exists()&& _workersTarget[wk]->getResources()>0)
					//	wk->rightClick(_workersTarget[wk]);
					//else if ((!_workersTarget[wk] && (!_workersTarget[wk]->exists()||_workersTarget[wk]->getResources() == 0)) && wk->isIdle()){
					//	ResourceToWorkerMap::iterator j = _mineralsExploitation.begin();
					//	if (j->first && j->first->exists() && j->first->getResources()>0)				
					//		wk->rightClick(j->first);
					//	else{
					//		for (std::set<BWAPI::Unit*>::const_iterator it = allMineral.begin(); it != allMineral.end(); it++){
					//			if ((*it)->isVisible() && (*it)->exists()){
					//				wk->rightClick(*it);
					//				break;
					//			}
					//			else continue;
					//		}
					//	}
					//}
				}					
			}
		}
	}	
}

bool WorkerManager::needWorkers()
{	
	if (_workerUnits.size()<this->_bmc->getAllMineralSet().size())
		return true;
	else
		return false;
}

int WorkerManager::getNeedTotalWorkerNum()
{
	this->_needTotalWorkerNum = 0;

	if(this->_bmc->checkflag == true)
	{
		std::set<BaseClass*> bcset = this->_bmc->getBaseSet();
		for (std::set<BaseClass*>::const_iterator i=bcset.begin(); i!=bcset.end();i++)
		{
			if(this->_bmc->checkflag==false)
			{
				this->_needTotalWorkerNum = (int)_workers.size()+15;
				break;
			} 
			else
			{
				BaseClass* ii = *i;
				this->_needTotalWorkerNum += ii->getNeedWorkerNum();
			}
		}
	}

	if (this->_needTotalWorkerNum <80)
		return this->_needTotalWorkerNum	;
	else
	{
		this->_needTotalWorkerNum = 80;
		return this->_needTotalWorkerNum	;
	}
}

unsigned int WorkerManager::getWorkersMining()
{
	// TODO: Optimize calculating on add/delete worker on mineral spot

	int workersMining = 0;
	for (UnitSet::const_iterator _workers =_workerUnits.begin();_workers!=_workerUnits.end();_workers++)
	{
		if (_workerState[(*_workers)]== Gathering_Mineral)
			workersMining++;
		else
			continue;
	}
	/*for(ResourceToWorkerMap::const_iterator mineral=_mineralsExploitation.begin();mineral!=_mineralsExploitation.end();mineral++) {
		workersMining += mineral->second.first;
	}*/
	return workersMining;

}

void WorkerManager::onUnitDestroy(Unit* unit)
{
	// Add something here
	this->_enemyToDefend.erase(unit);
	this->_workers.erase(unit);
	_workerUnits.erase(unit);
	_scvDefendTeam.erase(unit);
	this->_repairGroup.erase(unit);	
	this->_repairList.erase(unit);
}

BWAPI::Unit* WorkerManager::getWorkerForTask(Position toPosition)
{
	return NULL;
	//to do
}

BWAPI::Position WorkerManager::getPositionToScout(Position myPos, BWTA::Region* myRegion, Position basePos, bool checkVisible)
{

	Position returnPosition;
	int maxDist = 17;
	//Broodwar->drawCircleMap(basePos.x(),basePos.y(),maxDist*TILE_SIZE,Colors::Yellow,false);
	TilePosition seedTilePos = TilePosition(myPos);
	TilePosition baseTilePos = TilePosition(basePos);
	int x      = seedTilePos.x();
	int y      = seedTilePos.y();
	int length = 1;
	int j      = 0;
	bool first = true;
	int dx     = 0;
	int dy     = 1;	
	while (length < Broodwar->mapWidth()) {
		returnPosition = Position(x*TILE_SIZE, y*TILE_SIZE);
		//check max distance
		if (returnPosition.getDistance(myPos) > maxDist*TILE_SIZE) {
			//if (x > baseTilePos.x()+maxDist && y > baseTilePos.y()+maxDist) {
			if (!checkVisible) return getPositionToScout(myPos, myRegion, basePos, true);
			else return basePos;
		}

		if (x >= 0 && x < Broodwar->mapWidth() && y >= 0 && y < Broodwar->mapHeight() && 
			myRegion == BWTA::getRegion(x,y) && Broodwar->hasPath(myPos,returnPosition) ) {
				//if (x <= baseTilePos.x()+maxDist && y <= baseTilePos.y()+maxDist) {
				if (!checkVisible) {
					if (!Broodwar->isExplored(x,y)) return returnPosition;
				} else {
					if (!Broodwar->isVisible(x,y)) return returnPosition;
				}
				//}
		}

		//otherwise, move to another position
		x = x + dx;
		y = y + dy;
		//count how many steps we take in this direction
		j++;
		if (j == length) { //if we've reached the end, its time to turn
			j = 0;	//reset step counter

			//Spiral out. Keep going.
			if (!first)
				length++; //increment step counter if needed

			first =! first; //first=true for every other turn so we spiral out at the right rate

			//turn counter clockwise 90 degrees:
			if (dx == 0) {
				dx = dy;
				dy = 0;
			} else {
				dy = -dx;
				dx = 0;
			}
		}
		//Spiral out. Keep going.
	}

	return basePos;
}

void WorkerManager::tryMiningTrick(Unit* worker)
{
	;
}

double WorkerManager::getMineralRate() const
{
	return this->_mineralRate;
}
double WorkerManager::getGasRate() const
{
	return this->_gasRate;
}

int WorkerManager::getOptimalWorkerCount() const
{
	return this->_optimalWorkerCount;
}
void WorkerManager::enableAutoBuild()
{
	this->_autoBuild=true;
}
void WorkerManager::disableAutoBuild()
{
	this->_autoBuild=false;
}
void WorkerManager::setAutoBuildPriority(int priority)
{
	this->_autoBuildPriority = priority;
}

void WorkerManager::setWorkerPerGas(int num)
{
	this->_WorkersPerGas = num;
}

void WorkerManager::setNeedGasLevel(int level)
{
	set<BaseClass*> allBaseSet = this->_bmc->getBaseSet();
	set<Unit*> baseGeysers;
	if (level == 0)
	{
		this->_WorkersPerGas = 0;
	}
	if (level == 1)
	{
		this->_WorkersPerGas = 1;
		for (set<BaseClass*>::const_iterator b = allBaseSet.begin();b!= allBaseSet.end();b++)
		{	
			baseGeysers = (*b)->getGeysers();
			for(set<Unit*>::iterator g = baseGeysers.begin(); g != baseGeysers.end(); g++)
			{
				Unit* _g = *g;
				if (Broodwar->canBuildHere(NULL,_g->getTilePosition(),UnitTypes::Terran_Refinery)&&!_g->getType().isRefinery())
				{
					//Broodwar->printf("can build refinery!");
					this->_buildOrderManager->buildAdditional(1,UnitTypes::Terran_Refinery,200,_g->getTilePosition());
				}
				//else
					//Broodwar->printf("already has refinery on it!");
			}	
		}
	}
	if (level == 2)
	{
		if(allBaseSet.size() == 1)
		{
			this->_WorkersPerGas = 3;
			for (set<BaseClass*>::const_iterator b = allBaseSet.begin();b!= allBaseSet.end();b++)
			{	
				baseGeysers = (*b)->getGeysers();
				for(set<Unit*>::iterator g = baseGeysers.begin(); g != baseGeysers.end(); g++)
				{
					Unit* _g = *g;
					if (Broodwar->canBuildHere(NULL,_g->getTilePosition(),UnitTypes::Terran_Refinery)&&!_g->getType().isRefinery())
						this->_buildOrderManager->buildAdditional(1,UnitTypes::Terran_Refinery,200,_g->getTilePosition());
				}	
			}
		}
		else if (allBaseSet.size()>1)
		{
			this->_WorkersPerGas = 2;
			for (set<BaseClass*>::const_iterator b = allBaseSet.begin();b!= allBaseSet.end();b++)
			{	
				baseGeysers = (*b)->getGeysers();
				for(set<Unit*>::iterator g = baseGeysers.begin(); g != baseGeysers.end(); g++)
				{
					Unit* _g = *g;
					if (Broodwar->canBuildHere(NULL,_g->getTilePosition(),UnitTypes::Terran_Refinery)&&!_g->getType().isRefinery())
						this->_buildOrderManager->buildAdditional(1,UnitTypes::Terran_Refinery,200,_g->getTilePosition());
				}
			}
		}
	}
	if (level == 3)
	{
		this->_WorkersPerGas = 3;
		for (set<BaseClass*>::const_iterator b = allBaseSet.begin();b!= allBaseSet.end();b++)
		{	
			baseGeysers = (*b)->getGeysers();
			for(set<Unit*>::iterator g = baseGeysers.begin(); g != baseGeysers.end(); g++)
			{
				Unit* _g = *g;
				//&&!_g->getType().isRefinery()
				if (Broodwar->canBuildHere(NULL,_g->getTilePosition(),UnitTypes::Terran_Refinery))
				{
					if (this->_buildOrderManager->getPlannedCount(UnitTypes::Terran_Refinery,90) < SelectAll()(isCompleted)(Command_Center).size())
					{
						//Broodwar->printf("Build Terran Refinery at (%d,%d) | %d",_g->getTilePosition().x(),_g->getTilePosition().y(),Broodwar->getFrameCount());
						this->_buildOrderManager->build(SelectAll()(isCompleted)(Command_Center).size(),UnitTypes::Terran_Refinery,90,_g->getTilePosition());
					}
				}				
			}	
		}
	}	
}

void WorkerManager::autoBuildWorker()
{	
	int tmp=0;
	if (this->_autoBuild == true && (int)_workerUnits.size() < (tmp=getNeedTotalWorkerNum()))
	{
		if ((int)_workerUnits.size()<80)
		{
			_optimalWorkerCount = tmp;
			BWAPI::UnitType workerType=BWAPI::Broodwar->self()->getRace().getWorker();
			//
			if (this->_buildOrderManager->getPlannedCount(workerType)<_optimalWorkerCount)
			{
				int see=this->_buildOrderManager->getPlannedCount(workerType);
				//this->buildOrderManager->deleteItem(workerType,this->autoBuildPriority);
				this->_buildOrderManager->buildAdditional(_optimalWorkerCount-see,workerType,this->_autoBuildPriority);
			}
		}
	}
	else if (this->_autoBuild ==false||(int)_workerUnits.size()>=80)
	{
		BWAPI::UnitType workerType=BWAPI::Broodwar->self()->getRace().getWorker();
		this->_buildOrderManager->deleteItem(workerType,this->_autoBuildPriority);
	}
}

std::set<BWAPI::Unit*> WorkerManager::selectSCV(int n)
{
	std::set<BWAPI::Unit*> scvSet;
	
	for (UnitSet::iterator it = _workerUnits.begin(); it != _workerUnits.end();)
	{
		Unit* scv = *it;
		if (this->constructingSCV.find(*it) != this->constructingSCV.end()
			  ||
				scv->isConstructing() || scv->isCarryingGas() || scv->isCarryingMinerals() || !scv->isCompleted()
				||
				scv->getLastCommand().getType() == UnitCommandTypes::Build || scv->getOrder() == Orders::ConstructingBuilding)
		{
			++it;
		}
		else
		{
			scvSet.insert(scv);
			_workerState[scv] = Scouting;
			_workerUnits.erase(it++);
			
			if (scvSet.size() >= n)	break;
		}
	}

	return scvSet;
}

void WorkerManager::workerRepair()
{
	//Broodwar->drawTextScreen(433,290,"Repair team: %d/%d",this->_repairGroup.size(),_repairGroupSize);
	//Broodwar->drawTextScreen(0,30,"Repair list: %d",this->_repairList.size());
	
	for each (Unit* u in this->_repairGroup)
	{
	  Position p = u->getPosition();
		UnitType ut = u->getType();
	  Broodwar->drawBoxMap(p.x()-ut.dimensionLeft()/2,p.y()-ut.dimensionUp()/2,p.x()+ut.dimensionRight()/2,p.y()+ut.dimensionDown()/2,Colors::Green,true);
	}

	for each (Unit* u in this->_repairList)
	{
		Position p = u->getPosition();
		UnitType ut = u->getType();
		Broodwar->drawBoxMap(p.x()-ut.dimensionLeft()/2,p.y()-ut.dimensionUp()/2,p.x()+ut.dimensionRight()/2,p.y()+ut.dimensionDown()/2,Colors::Red,true);
		if (u->isBeingHealed())
		{
			Broodwar->drawCircleMap(u->getPosition().x(),u->getPosition().y(),20,Colors::Green);
		}
	}

	// add damaged units to repair list
	UnitGroup attackers = ArmyManager::create()->getAttackers();
	for each (Unit* u in Broodwar->self()->getUnits())
	{
		if (this->_repairList.find(u) != this->_repairList.end()
				||
				!u->isCompleted()	|| u->isBeingConstructed() ||	u->getType().isWorker() || !u->getType().isMechanical() || u->isLifted()
				||
				u->getTilePosition().getDistance(Broodwar->self()->getStartLocation()) > 50)
		{
			continue;
		}

		if (u->getType() == UnitTypes::Terran_Vulture && !attackers.empty() && attackers.find(u) == attackers.end())
		{
			continue;
		}


		if (((u->getType() == UnitTypes::Terran_Bunker || u->getType() == UnitTypes::Terran_Missile_Turret) && u->getHitPoints() < u->getType().maxHitPoints())
			  ||
			  (u->getType().isBuilding() && u->getHitPoints() <= u->getType().maxHitPoints() / 3)
				||
				(u->getType() == UnitTypes::Terran_Command_Center && u->getHitPoints() <= u->getType().maxHitPoints() / 2)
				||
				(u->getType() != UnitTypes::Terran_Battlecruiser && !u->getType().isBuilding() && (double)u->getHitPoints() <= (double)u->getType().maxHitPoints() * 0.7)
				||
				(u->getType() == UnitTypes::Terran_Battlecruiser && u->getHitPoints() <= 200))
		{
			this->_repairList.insert(u);
		}
	}

	// remove units from repair list
	for(set<Unit*>::iterator i = _repairList.begin(); i != _repairList.end();)
	{
		Unit* u = *i;
		if (u->getHitPoints() == u->getType().maxHitPoints() ||
        _notRepairList.find(u) != _notRepairList.end())
		{
			_repairList.erase(i++);
		}
		else if (u->getType() == UnitTypes::Terran_Vulture && !attackers.empty() && attackers.find(u) == attackers.end())
		{
			_repairList.erase(i++);
			for each (Unit* scv in this->_repairGroup)
			{
				if (scv->isRepairing() && scv->getOrderTarget() == u)
				{
					scv->stop();
				}
			}
		}
		else
		{
			++i;
		}
	}

	// set the number of SCVs we need
	this->_repairGroupSize = this->_repairList.empty() ? 1 : 2;
	
	// for emergency, enlarge repair group
	Unit* bunker = SelectAll(UnitTypes::Terran_Bunker)(isCompleted).getNearest(TerrainManager::create()->mSecondChokepoint->getCenter());
	if (bunker && SelectAllEnemy()(canAttack)(isDetected).inRadius(32*10,bunker->getPosition()).size() > 1)
	{
		if (this->_mInfo->myFightingValue().first < this->_eInfo->enemyFightingValue().first
			  ||
				SelectAll()(isCompleted)(Siege_Tank).inRadius(32*10,bunker->getPosition()).empty())
		{
			this->_repairGroupSize = 5;
		}
		else
		{
			this->_repairGroupSize = 3;
		}
	}

	int num = _repairList(Battlecruiser).size();
	if (num > 1)
	{
		int need = num <= 4 ? 4 : (num <= 6 ? 6 : 8);
		this->_repairGroupSize = need > this->_repairGroupSize ? need : this->_repairGroupSize;
	}

	// remove SCVs that are scouting or constructing from repair group
	for (set<Unit*>::iterator i = this->_repairGroup.begin(); i != this->_repairGroup.end();)
	{
		Unit* scv = *i;
		if (scv->isConstructing() || scv->getLastCommand().getType() == UnitCommandTypes::Build || scv->getOrder() == Orders::ConstructingBuilding || _workerState[scv] == Scouting)
		{
			this->_workerUnits.insert(scv);
			this->_repairGroup.erase(i++);	
		}
		else
		{
			++i;
		}
	}

	if (this->_repairGroup.size() < this->_repairGroupSize)
	{
		// add SCVs to repair group, start from SCVs near the bunker
		if (bunker)
		{
			UnitGroup workers; // available workers
			for each (Unit* u in this->_workerUnits)
			{
				if (!u->isConstructing() && !u->isCarryingGas() && _workerState[u] != Scouting)
				{
					workers.insert(u);
				}
			}

			while (this->_repairGroup.size() < this->_repairGroupSize && !workers.empty())
			{
				Unit* nearestSCV = workers.getNearest(bunker->getPosition());
				if (nearestSCV)
				{
					workers.erase(nearestSCV);
					this->_repairGroup.insert(nearestSCV);
					arbitrator->setBid(this, nearestSCV, 50);
				}
			}
		}
		else
		{
			for each (Unit* u in this->_workerUnits)
			{
				if (this->_repairGroup.size() >= this->_repairGroupSize)
				{
					break;
				}
				
				if (!u->isConstructing() && !u->isCarryingGas() && _workerState[u] != Scouting)
				{
					this->_repairGroup.insert(u);
					arbitrator->setBid(this, u, 50);
				}
			}
		}
	}
	else if (this->_repairGroup.size() > this->_repairGroupSize)
	{
		// remove SCVs from repair group
		for (set<Unit*>::iterator i = this->_repairGroup.begin(); i != this->_repairGroup.end();)
		{
			if (this->_repairGroup.size() <= this->_repairGroupSize)
			{
				break;
			}

			Unit* scv = *i;
			if (!scv->isRepairing())
			{
				this->_workerUnits.insert(scv);
				this->_repairGroup.erase(i++);
			}
			else
			{
				++i;
			}
		}
	}

	int guard = 0;
	for each (Unit* u in this->_repairGroup)
	{
		if (Broodwar->getFrameCount() < 24*60*8 && bunker && !bunker->isUnderAttack() && guard < 2)
		{
			// order SCVs to wait beside the bunker in early game
			this->_workerUnits.erase(u);
			if (!u->isRepairing() && !u->isCarryingMinerals() && !u->isCarryingGas() && u->getPosition().getApproxDistance(bunker->getPosition()) > 32 * 2)
			{
				u->move(bunker->getPosition());
				guard++;
			}
		}
		else if (!_repairList.empty())
		{
			this->_workerUnits.erase(u);
		}
		else
		{
			this->_workerUnits.insert(u);
		}
	}
	
	// number of SCVs to repair each non-building unit
	int need = 0;
	if (!this->_repairList.not(isBuilding).empty())
	{
		need = this->_repairGroup.size() / this->_repairList.not(isBuilding).size();
		need = need < 1 ? 1 : need;
	}

	// repair damaged units
	for each (Unit* repairTarget in this->_repairList)
	{		
		// repair bunker first
		if (repairTarget->getType() != UnitTypes::Terran_Bunker && !_repairList(Bunker).empty() && !this->_mental->enemyInSight.empty())
		{
			continue;
		}

		// then repair dropship
		if (repairTarget->getType() != UnitTypes::Terran_Dropship && !_repairList(Dropship).empty())
		{
			continue;
		}

		for each (Unit* repairWorker in _repairGroup)
		{
			if (!repairTarget->getType().isBuilding())
			{
				int n = 0; // number of SCVs that are repairing this target
				for each (Unit* u in _repairGroup)
				{
					if (u->getLastCommand().getType() == UnitCommandTypes::Repair && u->getLastCommand().getTarget() == repairTarget)
					{
						n++;
					}
				}

				if (n >= need)
				{
					continue;
				}
			}

			if (!repairWorker->isRepairing() || (repairTarget->getType() == UnitTypes::Terran_Bunker && repairWorker->isRepairing() && repairWorker->getOrderTarget() != repairTarget))
			{
				repairWorker->repair(repairTarget);
			}													
		}
	}
}

UnitGroup WorkerManager::getRepairList()
{
	return this->_repairList;
}

bool WorkerManager::isInRepairList(Unit* u) const
{
	return (_repairList.find(u) != _repairList.end());
}


void WorkerManager::onUnitMorph(Unit* u)
{
	if (Broodwar->self()->isEnemy(u->getPlayer())){
		if (!u->getType().isBuilding() && (this->_enemyToDefend.find(u))!=this->_enemyToDefend.end())
			this->_enemyToDefend.erase(u);
	}
}

void WorkerManager::onUnitHide(BWAPI::Unit* u)
{
	this->_enemyToDefend.erase(u);
}

void WorkerManager::autoTrainSCV()
{
	if (Broodwar->self()->allUnitCount(UnitTypes::Terran_SCV) > 80)
	{
		return;
	}

	int need = (int)(_bmc->getAllMineralSet().size() * 2.5 + _bmc->getAllGeyserSet().size() * 3 + 3);
	if (need > 80)
	{
		need = 80;
	}

	if (_buildOrderManager->getPlannedCount(UnitTypes::Terran_SCV,this->_autoBuildPriority+30) < need)
	{
		this->_buildOrderManager->build(need,UnitTypes::Terran_SCV,this->_autoBuildPriority+30);
	}

	if (MentalClass::create()->STflag != MentalClass::PtechCarrier)
	{
		if (Broodwar->self()->allUnitCount(UnitTypes::Terran_SCV) < need && Broodwar->getFrameCount()%24 == 0)
		{
			for each (Unit* u in Broodwar->self()->getUnits())
			{
				if (u->getType() == UnitTypes::Terran_Command_Center && u->isCompleted() && !u->isTraining() && !u->isConstructing())
				{
					u->train(UnitTypes::Terran_SCV);
					//Broodwar->printf("Auto train SCV");
				}
			}
		}
	}
}

void WorkerManager::onUnitDiscover(BWAPI::Unit* u)
{
	if (u == NULL)
	{
		return;
	}

	if (u->getPlayer() == Broodwar->self() && u->getType().isWorker())
	{
		//addUnit(u);	 
    arbitrator->setBid(this, u, 40);
	}
}

void WorkerManager::addToNotRepairList( Unit* u )
{
  _notRepairList.insert(u);
}
