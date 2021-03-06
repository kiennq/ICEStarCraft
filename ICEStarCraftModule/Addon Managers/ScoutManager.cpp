#include "ScoutManager.h"
#include "math.h"
#include <time.h>
#include "MentalState.h"

using namespace BWAPI;
using namespace BWTA;
using namespace ICEStarCraft;

ScoutManager* theScoutManager = NULL;

ScoutManager* ScoutManager::create()
{
  if (theScoutManager) return theScoutManager;
  else return theScoutManager = new ScoutManager();
}

ScoutManager::ScoutManager()
{
  this->myStartLocation=getStartLocation(Broodwar->self());
  this->enemyStartLocation=NULL;
  for each(BaseLocation* BL in BWTA::getStartLocations())
	{
    if (BL == this->myStartLocation)
      continue;
    
		this->startLocationsToScout.insert(BL);
    this->currentSearchTarget=BL;
  }
  this->baseLocationsExplored.clear();
  this->LocationsHasEnemy.clear();
  this->workerMG = NULL;
  this->bmc = NULL;
  this->terrainManager = NULL;
  this->scoutGroup.clear();
  this->needScoutNum = 1;
  this->ScoutUnitPurposeMap.clear();
  this->lastExplorPosition=BWAPI::Positions::None;
  this->ScoutUnitLastPurposeMap.clear();
  this->returnTimes = 0;
  this->lockedPositionSet.clear();
  this->expansionToScout.clear();
  this->lastScanTime = 0;
  this->scannedPositions.clear();
  this->needMoreScan=false;
  this->taskStartTime =0;
  this->previousPositionSet.clear();
  this->PreWarningPositionSet.clear();
  this->mapBaseLocations = BWTA::getBaseLocations();	
  this->baseLocationNeedToScout.clear();
  this->expansionScouting=false;
  this->currentLocationTarget=NULL;
  this->mental = NULL;
  this->currentFinish=true;
	this->currentStartFrame = 0;
  this->bestPrewarningpo = Positions::None;
  this->startLocationsExplored.clear();
  this->essentialPostions.clear();

  mInfo=NULL;
  eInfo=NULL;

	_fSwitchRegion = false;
	_nextTargetBase = NULL;
  _fChangedRegion = false;
  _lastSwitchRegionFrame = 0;
	
	this->deadScoutUnitCount = 0;
};


void ScoutManager::setManagers()
{
  this->workerMG = WorkerManager::create();
  this->bmc = BaseManager::create();
  this->terrainManager = TerrainManager::create();
  this->mental = MentalClass::create();
  this->eInfo = EnemyInfoManager::create();
  this->mInfo = MyInfoManager::create();
}

void ScoutManager::SCVScout(ScoutPurpose purpose)
{
  if ((int)this->scoutGroup.size() >= this->needScoutNum)
	{
    return;
  }

	// send only 1 SCV to scout
	for (map<Unit*,ScoutPurpose>::iterator i = this->ScoutUnitPurposeMap.begin(); i != this->ScoutUnitPurposeMap.end(); i++)
	{
		if (i->first && i->first->exists() && i->second == purpose)
		{
			return;
		}
	}
		
  std::set<Unit*> tempGroup = this->workerMG->selectSCV(1);
  for each (Unit* u in tempGroup)
	{
    if (u->isConstructing())
		{
      continue;
    }
    if (this->scoutGroup.find(u) == this->scoutGroup.end())
      this->scoutGroup.insert(u);
  }

  for each (Unit* u in this->scoutGroup)
	{
    if (u->isConstructing())
		{
      continue;
    }
    if (this->ScoutUnitPurposeMap.find(u) == this->ScoutUnitPurposeMap.end())
      this->ScoutUnitPurposeMap[u] = purpose;
  }
}

void ScoutManager::VultureScout(ScoutPurpose purpose)
{
	//_T_
	for each (Unit* u in this->scoutGroup)
	{
		if (u->getType() == UnitTypes::Terran_Vulture && u->getSpiderMineCount() == 0)
		{
			//Broodwar->printf("delete vulture from scout group");
			this->scoutGroup.erase(u);
			this->ScoutUnitPurposeMap.erase(u);
		}

		if (u->getType() == UnitTypes::Terran_SCV && this->ScoutUnitPurposeMap[u] == ScoutManager::EnemyExpansion)
		{
			//Broodwar->printf("delete SCV from scout group");
			this->scoutGroup.erase(u);
			this->ScoutUnitPurposeMap.erase(u);
			this->workerMG->_workerUnits.insert(u);
			this->workerMG->_workerState.erase(u);
		}
	}

  UnitGroup vultureGroup = SelectAll()(isCompleted)(Vulture)(HitPoints,">=",70).not(isAttacking);
	
	int MineMinNum = 3;
	while ((int)this->scoutGroup.size() < this->needScoutNum && MineMinNum >= 0)
	{
		//Broodwar->printf("add vultures to scout group");
		for each(Unit* u in vultureGroup)
		{
			if (u->getSpiderMineCount() < MineMinNum)
			{
				continue;
			}
			
			if(this->scoutGroup.find(u) == this->scoutGroup.end())
			{
				this->scoutGroup.insert(u);
				if ((int)this->scoutGroup.size() >= this->needScoutNum)
				{
					break;
				}
			}	
		}
		MineMinNum--;
	}//_T_
	
  for each(Unit* u in this->scoutGroup)
	{
    if(this->ScoutUnitPurposeMap.find(u) == this->ScoutUnitPurposeMap.end())
		{
      this->ScoutUnitPurposeMap[u] = purpose;
		}
  }
}
void ScoutManager::ScannerScout(ScoutPurpose purpose,int reserveTimes)
{
  //select all scanner whose energy is more than 50
  switch (purpose)//scan purpose
  {
  case EnemyStartLocation:
    {
      //if we already know enemy start location ,then break
      if (this->enemyStartLocation){
        //	Broodwar->printf("already know enemy start location");
        return;
      }

      else{
        //scan all start locations(except myself) in the map
        for each(BaseLocation* bl in this->startLocationsToScout){
          UnitGroup availableScanner;
          availableScanner= SelectAll()(isCompleted)(Comsat_Station)(Energy,">=",50);
          if (this->enemyStartLocation || availableScanner.size()==0)	
            break;
          else if (Broodwar->isVisible(bl->getTilePosition())){
            UnitGroup tempUG = SelectAllEnemy().inRadius(32*8,bl->getPosition());
            if (tempUG(isResourceDepot).size()>0 
              || tempUG(isCompleted,isWorker).size()>=3
              || tempUG(isBuilding).size()>=1){
                this->enemyStartLocation = bl;
                this->startLocationsToScout.clear();
                return;
            }
            else{
              this->startLocationsToScout.erase(bl);
              return;
            }
          }
          else{
            //for each scanner
            for each(Unit* scanner in availableScanner){
              if (scanner->getEnergy()<50)
                continue;
              else{
                int energy=scanner->getEnergy();
                //scan this baselocation
                scanner->useTech(TechTypes::Scanner_Sweep,bl->getPosition());
                return;
              }								
            }
          }
        }
      }
    }
    break;
  case EnemyOpening:
    {
			//if we don't know enemy start location, return
      if (this->enemyStartLocation == NULL)
        return;
      else
			{
        if (this->essentialPostions.size()<1)
				{
          //add some important positions to scan
          if (Broodwar->enemy()->getRace()==Races::Terran)
					{
            for(std::map<Unit*,EnemyInfoManager::eBaseData>::iterator i= this->eInfo->enemyBaseMap.begin(); i!=this->eInfo->enemyBaseMap.end();i++){
              this->essentialPostions.insert(i->second.position);
            }
          }
          else
					{
            this->essentialPostions.insert(this->enemyStartLocation->getPosition());
            this->essentialPostions.insert(this->enemyStartLocation->getRegion()->getCenter());
            this->essentialPostions.insert(this->terrainManager->eNearestBase->getPosition());
						this->essentialPostions.insert(this->terrainManager->eSecondChokepoint->getCenter());
						this->essentialPostions.insert(this->terrainManager->eThirdChokepoint->getCenter());
						this->essentialPostions.insert(this->terrainManager->eFirstChokepoint->getCenter());
          }		
        }
        else
				{
          if (Broodwar->getFrameCount()-this->lastScanTime>24*5)
					{
            UnitGroup availableScanner = SelectAll()(isCompleted)(Comsat_Station)(Energy,">=",50);
            int totalEnergy = 0;
            if (availableScanner.size()==0)
              return;
            for each(Unit* sc in availableScanner)
						{
              //save some energy for invisible units
              totalEnergy += sc->getEnergy();
            }
            for each(Unit* sc in availableScanner)
						{
              //each time for scanner will cost 50 energies
              if (totalEnergy/50 <= reserveTimes)
                return;
              else if (sc->getEnergy()<50)
                continue;
              else
							{
                for(std::set<Position>::iterator i=this->essentialPostions.begin(); i!=this->essentialPostions.end();)
								{
                  if (Broodwar->isVisible((*i).x()/32,(*i).y()/32))
									{
                    i = this->essentialPostions.erase(i);
                  }
                  else
									{
                    sc->useTech(TechTypes::Scanner_Sweep,*i);
                    i = this->essentialPostions.erase(i);
                    this->lastScanTime = Broodwar->getFrameCount();
                    return;
                  }
                }					
              }
            }	
          }
        }						
      }
    }
    break;
  case EnemyExpansion:
    {
			//_T_
			if (this->enemyStartLocation == NULL)
				return;
			else
			{
				if (this->essentialPostions.size()<1)
				{
					//add some important positions to scan
					for each (BWTA::BaseLocation* base in this->baseLocationNeedToScout)
					{
						if (this->baseLocationsExplored.find(base) != this->baseLocationsExplored.end())
						{
							continue;
						}
				
						if (Broodwar->isVisible(base->getTilePosition()))
						{
							continue;
						}
						this->essentialPostions.insert(base->getPosition());
					}
				}
				else
				{
					if (Broodwar->getFrameCount()-this->lastScanTime>24*5)
					{
						UnitGroup availableScanner = SelectAll()(isCompleted)(Comsat_Station)(Energy,">=",50);
						int totalEnergy = 0;
						if (availableScanner.size()==0)
							return;
						for each(Unit* sc in availableScanner)
						{
							//save some energy for invisible units
							totalEnergy += sc->getEnergy();
						}
						for each(Unit* sc in availableScanner)
						{
							//each time for scanner will cost 50 energies
							if (totalEnergy/50 <= reserveTimes)
								return;
							else if (sc->getEnergy()<50)
								continue;
							else
							{
								for(std::set<Position>::iterator i=this->essentialPostions.begin(); i!=this->essentialPostions.end();)
								{
									if (Broodwar->isVisible((*i).x()/32,(*i).y()/32))
									{
										i = this->essentialPostions.erase(i);
									}
									else
									{
										sc->useTech(TechTypes::Scanner_Sweep,*i);
										i = this->essentialPostions.erase(i);
										this->lastScanTime = Broodwar->getFrameCount();
										return;
									}
								}					
							}
						}	
					}
				}						
			}
    }
    break;
  case EnemyTech:
    {

    }
    break;
  }

}

void ScoutManager::onUnitDestroy(Unit* unit)
{
  if (this->scoutGroup.find(unit) != this->scoutGroup.end())
	{
    this->scoutGroup.erase(unit);
		this->deadScoutUnitCount++;
    // Try to infer enemy start location
    BWTA::Region* curReg = BWTA::getRegion(unit->getPosition());
    if (!enemyStartLocation &&
        !curReg->getBaseLocations().empty())
    {
      // find closest start location
      BaseLocation* start = NULL;
      double minDist = 99999;
      for each (BaseLocation* bl in BWTA::getBaseLocations())
      {
        if (bl == myStartLocation ||
            bl == TerrainManager::create()->mNearestBase ||
            (bl->getRegion() == curReg && !bl->isStartLocation())) 
        {
          continue;
        }
        double dist = getGroundDistance(unit->getTilePosition(), bl->getTilePosition());
        if (dist >= 0 && dist < minDist)
        {
          minDist = dist;
          start = bl;
        }
      }
      enemyStartLocation = start && start->isStartLocation() ? start : NULL;
      if (enemyStartLocation)
        Broodwar->printf("Infered enemy start location");
    }
  }
  if (this->ScoutUnitPurposeMap.find(unit) != this->ScoutUnitPurposeMap.end())
	{
    this->ScoutUnitPurposeMap.erase(unit);
  }
  ScoutController::create()->onUnitDestroy(unit);
}

void ScoutManager::onUnitDiscover(Unit* unit)
{
  ScoutController::create()->onUnitDiscover(unit);
}

void ScoutManager::scoutEnemyLocation(Unit* u)
{
	if(this->enemyStartLocation != NULL||Broodwar->getFrameCount()>24*60*6)
	{
		this->ScoutUnitPurposeMap[u] = EnemyOpening;
		return;
	}
	
	if (u->getType() != UnitTypes::Terran_SCV && u->getType() != UnitTypes::Terran_Vulture)
	{
		return;
	}

	if (u->isConstructing())
	{
		return;
	}

	if(this->ScoutUnitPurposeMap[u] == EnemyStartLocation)
	{
		UnitGroup inRadiusSet = SelectAllEnemy().inRadius(u->getType().sightRange(),u->getPosition());
		if (inRadiusSet(isBuilding).size() >= 1)
		{
			for each (BaseLocation* bl in startLocationsToScout)
			{
				if (getRegion(inRadiusSet(isBuilding).getCenter()) == getRegion(bl->getPosition()))
				{
					this->enemyStartLocation = bl;	
					this->ScoutUnitPurposeMap[u] = EnemyOpening;	
					//Broodwar->printf("Enemy StartLocation is (%d,%d)",bl->getPosition().x(),bl->getPosition().y());
					return;
				}
			}
		}

		for each(BaseLocation* bl in startLocationsToScout)
		{
			if (startLocationsExplored.find(bl) != startLocationsExplored.end())
				continue;
			else if (startLocationsToScout.size() - startLocationsExplored.size() <= 1)
			{
				this->enemyStartLocation = bl;
				currentSearchTarget = bl;
				this->ScoutUnitPurposeMap[u] = EnemyOpening;	
				//Broodwar->printf("The rest location must be EnemyStartLocation!");
			}
			else
			{
				currentSearchTarget = bl;
				if(u->getLastCommand().getType() == UnitCommandTypes::Move && (Broodwar->getFrameCount()%36!=0))
					return;
				else
				{
					//u->move(bl->getPosition());
					
					//_T_
					// move to first choke point first
					// to avoid being trapped on map Electric Circuit, start location 5 hour
					if (BWTA::getRegion(u->getPosition()) == BWTA::getRegion(Broodwar->self()->getStartLocation()) &&
						  u->getPosition().getApproxDistance(TerrainManager::create()->mFirstChokepoint->getCenter()) > 32*2)
					{
						u->move(TerrainManager::create()->mFirstChokepoint->getCenter());
					}
					else
					{
						u->move(bl->getPosition());
					}

					if(u->getPosition().getApproxDistance(bl->getPosition()) <= u->getType().sightRange())
					{								
						std::set<Unit*> onTileSet = Broodwar->getUnitsOnTile(bl->getTilePosition().x(),bl->getTilePosition().y());
						for each(Unit* onTileUnit in onTileSet)
						{
							if(onTileUnit->getPlayer() == Broodwar->enemy() && onTileUnit->getType().isResourceDepot())
							{
								this->enemyStartLocation = bl;
								this->ScoutUnitPurposeMap[u] = EnemyOpening;
								//Broodwar->printf("Enemy StartLocation is (%d,%d)",bl->getPosition().x(),bl->getPosition().y());
								return;
							}										
						}
						this->startLocationsExplored.insert(bl);
						//Broodwar->printf("No enemy here, move to next location!");
						return;					
					}
				}			
			}
		}
	}
}

void ScoutManager::scoutEnemyOpening(Unit* u)
{
  if (this->enemyStartLocation == NULL)
    return;
 
	if (u->getType() == UnitTypes::Terran_SCV ||u->getType() == UnitTypes::Terran_Vulture)
	{
		if (u->isConstructing())
		{
			return;
		}

		if (this->ScoutUnitPurposeMap[u] == EnemyOpening)
		{       
			Chokepoint* waypoint = TerrainManager::create()->eFirstChokepoint;
			Chokepoint* chkPoint = BWTA::getNearestChokepoint(u->getPosition());
      _nextTargetBase = TerrainManager::create()->eNearestBase;
      BWTA::Region* curReg = BWTA::getRegion(u->getPosition());
			if (!_fSwitchRegion &&
          _nextTargetBase &&
				  chkPoint &&
				  u->getPosition().getApproxDistance(chkPoint->getCenter()) > 96)
			{

        if (curReg == enemyStartLocation->getRegion() ||
            curReg == _nextTargetBase->getRegion())
        {
          if (Broodwar->getFrameCount() > _lastSwitchRegionFrame+24*40 && 
              curReg == enemyStartLocation->getRegion() &&
              _fChangedRegion)
          {
            //ScoutController::create()->removeFromScoutSet(u);
            _fSwitchRegion = true;
            _fChangedRegion = false;
            ScoutController::create()->setTargetRegion(_nextTargetBase->getRegion());
            ScoutController::create()->getAttractPoints().insert(waypoint->getCenter());
            // Broodwar->printf("Go to next base");
          }
          else
          {
            if (curReg == enemyStartLocation->getRegion() && !_fChangedRegion)
            {
              ScoutController::create()->getAttractPoints().erase(waypoint->getCenter());
              _fChangedRegion = true;
              _lastSwitchRegionFrame = Broodwar->getFrameCount();
            }
            ScoutController::create()->addToScoutSet(u);
            ScoutController::create()->onFrame();
          }
        }
        else
        {
          ScoutController::create()->setTargetRegion(enemyStartLocation->getRegion());
          ScoutController::create()->getAttractPoints().insert(waypoint->getCenter());
          u->move(enemyStartLocation->getPosition());
        }
			}
			else if (_nextTargetBase && 
				       _fSwitchRegion &&
				       chkPoint &&
				       u->getPosition().getApproxDistance(chkPoint->getCenter()) > 96)
	
      {
        if (curReg == enemyStartLocation->getRegion() ||
            curReg == _nextTargetBase->getRegion())
        {
          if (Broodwar->getFrameCount() > _lastSwitchRegionFrame+24*10 &&
              curReg == _nextTargetBase->getRegion() &&
              _fChangedRegion)
          {
            //ScoutController::create()->removeFromScoutSet(u);
            _fSwitchRegion = false;
            _fChangedRegion = false;
            ScoutController::create()->setTargetRegion(enemyStartLocation->getRegion());
            ScoutController::create()->getAttractPoints().insert(waypoint->getCenter());
            // Broodwar->printf("Go to next base");
          }
          else
          {
            if (curReg == _nextTargetBase->getRegion() && !_fChangedRegion)
            {
              ScoutController::create()->getAttractPoints().erase(waypoint->getCenter());
              _fChangedRegion = true;
              _lastSwitchRegionFrame = Broodwar->getFrameCount();
            }
            ScoutController::create()->addToScoutSet(u);
            ScoutController::create()->onFrame();
          }
        }
        else
        {
          ScoutController::create()->setTargetRegion(_nextTargetBase->getRegion());
          ScoutController::create()->getAttractPoints().insert(waypoint->getCenter());
          u->move(_nextTargetBase->getPosition());
        }
      }
      // If not reach, move directly to enemy base
			else if (!_fSwitchRegion)
			{
        ScoutController::create()->setTargetRegion(enemyStartLocation->getRegion());
				u->move(enemyStartLocation->getPosition());
			}
      // If not reach, move directly to next enemy base
			else
			{
        ScoutController::create()->setTargetRegion(_nextTargetBase->getRegion());
				u->move(_nextTargetBase->getPosition());
			}

			if (_nextTargetBase) Broodwar->drawCircleMap(_nextTargetBase->getPosition().x(), _nextTargetBase->getPosition().x(), 5, Colors::Orange, true);
		}
	}
}

void ScoutManager::scoutEnemyExpansion(Unit* u)
{		
	for each (BWTA::BaseLocation* base in this->baseLocationNeedToScout)
	{
		//Broodwar->drawCircleMap(base->getPosition().x(),base->getPosition().y(),30,Colors::Yellow);
	}
	//Broodwar->drawTextScreen(5,60,"BaseLocationNeedToScout: %d | %d",this->baseLocationNeedToScout.size(),Broodwar->getFrameCount());
	//Broodwar->drawTextScreen(5,70,"BaseLocationExplored: %d",this->baseLocationsExplored.size());
	
	if (this->ScoutUnitPurposeMap[u] != EnemyExpansion)
	{
		return;
	}

	if (u->getType() != UnitTypes::Terran_SCV && u->getType() != UnitTypes::Terran_Vulture)
	{
		return;
	}

	if (u->isConstructing())
	{
		return;
	}

	if (this->baseLocationsExplored.size() == this->baseLocationNeedToScout.size() || this->baseLocationNeedToScout.size() == 0)
	{
		this->expansionScouting = false;
		this->baseLocationsExplored.clear();
	}
	else
	{
		this->expansionScouting = true;
	}

	if (!this->expansionScouting)
		setExpansionToScout();

	if (this->baseLocationNeedToScout.empty())
	{
		this->scoutGroup.erase(u);
		this->ScoutUnitPurposeMap.erase(u);
		return;
	}

	if(this->currentLocationTarget == NULL || this->currentFinish == true)
	{
		int shortestdis =0;
		for each(BaseLocation* bl in this->baseLocationNeedToScout){
			if(this->baseLocationsExplored.find(bl)!=this->baseLocationsExplored.end())
				continue;
			if(shortestdis==0||u->getPosition().getApproxDistance(bl->getPosition())<shortestdis){
				shortestdis= u->getPosition().getApproxDistance(bl->getPosition());
				this->currentLocationTarget=bl;
			}
		}
		this->currentFinish=false;
		this->currentStartFrame = Broodwar->getFrameCount();
	}	

	//_T_
	if (this->currentLocationTarget)
	{
		Position tar = this->currentLocationTarget->getPosition();
		//Broodwar->drawLineMap(u->getPosition().x(),u->getPosition().y(),tar.x(),tar.y(),Colors::White);
	}

	if (!unitInDanger(u))
	{
		if (Broodwar->getFrameCount()%(24*3)==0)
			u->move(this->currentLocationTarget->getPosition());

		//_T_
		//int range = u->getType() == UnitTypes::Terran_Vulture ? (u->getSpiderMineCount() > 0 ? 32 : u->getType().sightRange()) : u->getType().sightRange();
		//if (range == 32 && !SelectAll()(isBuilding,SCV).inRadius(32*2,this->currentLocationTarget->getPosition()).empty())
		//{
		//	range = u->getType().sightRange();
		//}
		int range = 64;

		if(u->getPosition().getApproxDistance(this->currentLocationTarget->getPosition()) <= range)
		{
			if(hasArmyKeep(u,this->currentLocationTarget,32*10,3)||seeResourceDepot(u,this->currentLocationTarget))
			{
				this->LocationsHasEnemy.insert(this->currentLocationTarget);
			}
			this->baseLocationsExplored.insert(this->currentLocationTarget);
			this->currentFinish=true;						
		}
		// if the scout unit cannot reach this location after a long time then skip
		// this may happen on Electric Circuit
		else if (Broodwar->getFrameCount() - this->currentStartFrame > 24*30)
		{
			//Broodwar->printf("skip scouting Base(%d,%d)",this->currentLocationTarget->getTilePosition().x(),this->currentLocationTarget->getTilePosition().y());
			this->baseLocationsExplored.insert(this->currentLocationTarget);
			this->currentFinish = true;
		}
	}
	else
	{
		//Broodwar->printf("scout unit in danger");
		if (u->getPosition().getApproxDistance(this->currentLocationTarget->getPosition())<=u->getType().sightRange()*2)//3
		{
			this->LocationsHasEnemy.insert(this->currentLocationTarget);
			this->baseLocationsExplored.insert(this->currentLocationTarget);
			this->currentFinish=true;
		}
		u->move(this->myStartLocation->getPosition());
	}
}

void ScoutManager::scoutEnemyTech(Unit* u)
{

}
void ScoutManager::scoutEnemyArmyNum(Unit* u)
{

}

void ScoutManager::scoutMyMainBase(Unit* u)
{
	if (u->getType() != UnitTypes::Terran_SCV || this->ScoutUnitPurposeMap[u] != MyMainBase)
	{
		return;
	}
	
	vector<Position> vertices = BWTA::getRegion(Broodwar->self()->getStartLocation())->getPolygon();
	static int current = 0;
	static int lastOrderFrame = 0;

	if (Broodwar->getFrameCount() >= 24*60*5
			||
			!SelectAllEnemy().inRegion(BWTA::getRegion(Broodwar->self()->getStartLocation())).empty()
			||
			MentalClass::create()->STflag != MentalClass::NotSure
		  ||
			u->getOrder() == Orders::ConstructingBuilding || u->isConstructing() || u->isAttacking() || u->isRepairing())
	{
		this->ScoutUnitPurposeMap.erase(u);
		this->scoutGroup.erase(u);
		this->workerMG->_workerUnits.insert(u);
		this->workerMG->_workerState.erase(u);
		this->setScoutNum(1);
		lastOrderFrame = 0;
		return;
	}
	
	//Broodwar->drawLineMap(u->getPosition().x(),u->getPosition().y(),vertices[current].x(),vertices[current].y(),Colors::Green);
		
	if (Broodwar->isVisible(TilePosition(vertices[current]))
		  ||
		  (lastOrderFrame > 0 && Broodwar->getFrameCount() - lastOrderFrame > 24*5)) // SCV is stuck or cannot reach this position
	{
		current++;
		current = current % vertices.size();
		lastOrderFrame = Broodwar->getFrameCount();
	}
	else
	{
		u->move(vertices[current]);
	}
}

void ScoutManager::AsPreWarning(Unit* u)
{
	
}

void ScoutManager::onFrame()
{
  if (Broodwar->getFrameCount()>=24*60*3 && (Broodwar->enemy()->getRace()==Races::Zerg))
	{
    // enemy doesn't has lurker,or doesn't have intention to produce lurker,then we don't need to save scanner's energy
    // if time <=10 mins
    // if no lurker , no lurker egg, no lair + (hydralisk den or hydralisk) 
    if (this->mental->goAttack ||
        (this->eInfo->EnemyhasBuilt(UnitTypes::Zerg_Lurker,2))||(this->eInfo->EnemyhasBuilt(UnitTypes::Zerg_Lurker_Egg,2))||
        ((this->eInfo->EnemyhasBuilt(UnitTypes::Zerg_Lair,2))&&((this->eInfo->EnemyhasBuilt(UnitTypes::Zerg_Hydralisk_Den,2))||(this->eInfo->EnemyhasBuilt(UnitTypes::Zerg_Hydralisk,2)))))
    {
      ScannerScout(EnemyOpening,4);
    }			
    else if (Broodwar->getFrameCount() <= 24*60*8)
      ScannerScout(EnemyOpening,0);
    else
      ScannerScout(EnemyOpening,1);
  }

  if(SelectAll()(isCompleted)(Comsat_Station).size()>0&& Broodwar->enemy()->getRace()==Races::Protoss)
	{		
    if (this->eInfo->EnemyhasBuilt(UnitTypes::Protoss_Arbiter,1)||this->eInfo->EnemyhasBuilt(UnitTypes::Protoss_Arbiter_Tribunal,2) ||
			  this->eInfo->EnemyhasBuilt(UnitTypes::Protoss_Dark_Templar,1)||this->eInfo->EnemyhasBuilt(UnitTypes::Protoss_Templar_Archives,2))
      ScannerScout(EnemyOpening,4);
    else
      ScannerScout(EnemyOpening,0);
  }

  if (Broodwar->enemy()->getRace()==Races::Terran && Broodwar->getFrameCount()<=24*60*15 && SelectAll()(isCompleted)(Comsat_Station).size()>0)
	{
    if (this->eInfo->EnemyhasBuilt(UnitTypes::Terran_Wraith,1) || this->eInfo->EnemyhasBuilt(UnitTypes::Terran_Ghost,1))
      ScannerScout(EnemyOpening,4);
    else
      ScannerScout(EnemyOpening,2);
  }

  if(Broodwar->self()->supplyUsed()/2 >= 8 && deadScoutUnitCount < 2 && Broodwar->enemy()->getRace() == Races::Zerg)
	  SCVScout(EnemyStartLocation);
  if(Broodwar->self()->supplyUsed()/2 >= 8 && deadScoutUnitCount < 2 && Broodwar->getFrameCount() < 24*60*5.5 && Broodwar->enemy()->getRace() == Races::Protoss)
	  SCVScout(EnemyStartLocation);
  if(Broodwar->self()->supplyUsed()/2 == 9 && Broodwar->enemy()->getRace() == Races::Terran)
    SCVScout(EnemyStartLocation);
  if (!this->enemyStartLocation && Broodwar->getFrameCount() >= 24*60*4 && Broodwar->getFrameCount() <= 24*60*7)
    SCVScout(EnemyStartLocation);

  if (Broodwar->getFrameCount() >= 24*60*5 && this->enemyStartLocation == NULL)
	{
    ScannerScout(EnemyStartLocation,0);
  }

	//_T_
	// scout main base to prevent canon rush
	if (Broodwar->enemy()->getRace() == Races::Protoss
		  &&
		  SelectAllEnemy().inRegion(BWTA::getRegion(Broodwar->self()->getStartLocation())).empty()
			&&
			MentalClass::create()->STflag == MentalClass::NotSure
			&&
		  Broodwar->getFrameCount() >= 24*60*2 && Broodwar->getFrameCount() <= 24*60*5 && Broodwar->getFrameCount()%(24*10) == 0)
	{
		this->setScoutNum(2);
		SCVScout(MyMainBase);
	}
	
	// find enemy expansion
	if (Broodwar->getFrameCount() >= 24*60*6 && Broodwar->getFrameCount()%(24*15) == 0)
	{
		this->setScoutNum(1);
		this->SCVScout(ScoutManager::EnemyExpansion);
	}

	if (Broodwar->getFrameCount() >= 24*60*8 && Broodwar->enemy()->getRace() == Races::Zerg)
	{
		//Broodwar->printf("scan enemy expansion");
		if(this->eInfo->EnemyhasBuilt(UnitTypes::Zerg_Lurker,2) ||
			this->eInfo->EnemyhasBuilt(UnitTypes::Zerg_Lurker_Egg,2) ||
			(this->eInfo->EnemyhasBuilt(UnitTypes::Zerg_Lair,2) && (this->eInfo->EnemyhasBuilt(UnitTypes::Zerg_Hydralisk_Den,2) || this->eInfo->EnemyhasBuilt(UnitTypes::Zerg_Hydralisk,2))))
			this->ScannerScout(ScoutManager::EnemyExpansion,4);
		else
			this->ScannerScout(ScoutManager::EnemyExpansion,2);
	}//_T_

  if ((this->LocationsHasEnemy.find(this->enemyStartLocation)==this->LocationsHasEnemy.end()) && this->enemyStartLocation!=NULL)
    this->LocationsHasEnemy.insert(this->enemyStartLocation);

	for (std::map<Unit*,ScoutPurpose>::iterator j = this->ScoutUnitPurposeMap.begin(); j != this->ScoutUnitPurposeMap.end();)
	{
		std::map<Unit*, ScoutPurpose>::iterator i = j++;
		switch (i->second)
		{
		case EnemyStartLocation:
			//Broodwar->printf("Current state:EnemyStartLocation");
			scoutEnemyLocation(i->first);
			break;
		case EnemyOpening:
			//Broodwar->printf("Current state:EnemyOpening");
			scoutEnemyOpening(i->first);
			break;
		case EnemyExpansion:
			//Broodwar->printf("Current state:EnemyExpansion");
			scoutEnemyExpansion(i->first);
			break;
		case EnemyTech:
			//Broodwar->printf("Current state:EnemyTech");
			scoutEnemyTech(i->first);
			break;
		case EnemyArmyNum:
			scoutEnemyArmyNum(i->first);
			break;
		case MyMainBase:
			scoutMyMainBase(i->first);
			break;
		case PreWarning:
			//Broodwar->printf("Current state:AsPreWarning");
			AsPreWarning(i->first);
			break;
		case Running:
			//Broodwar->printf("Current state:Running, Previous state: %d",this->ScoutUnitLastPuporseMap[(*i).first]);
			unitFlee(i->first);
			break;				
    }
  }
}

void ScoutManager::explorEnemyBase(Unit* u)
{
  if (this->lastExplorPosition!=BWAPI::Positions::None && (u->getPosition().getApproxDistance(this->lastExplorPosition)>32*3+16)){
    if(Broodwar->getFrameCount()%24==0)
      this->returnTimes++;
    if (this->returnTimes>=8){
      u->move(this->enemyStartLocation->getPosition());
      this->lastExplorPosition = this->enemyStartLocation->getPosition();
      this->returnTimes = 0;
    }		
    else
      return;
  }
  //if unit is stuck
  if (u->getOrder()==Orders::Move && !u->isMoving()){
    if(Broodwar->getFrameCount()%24==0){
      u->move(this->enemyStartLocation->getPosition());
      this->lastExplorPosition = this->enemyStartLocation->getPosition();
      return;
    }

  }	

  if(this->lastExplorPosition == this->enemyStartLocation->getPosition()){
    if(Broodwar->getFrameCount()%48 == 0)
      this->lastExplorPosition=Positions::None;
  }


  this->previousPositionSet.clear();
  Position enemyBaseCenter = BWTA::getRegion(this->enemyStartLocation->getPosition())->getCenter();
  if(this->lastExplorPosition!=Positions::None){
    BWAPI::Broodwar->drawTextMap(u->getPosition().x()-16,u->getPosition().y()-16,"\x1D Distance: %d",u->getPosition().getApproxDistance(this->lastExplorPosition));
    BWAPI::Broodwar->drawLineMap(u->getTargetPosition().x(),u->getTargetPosition().y(),u->getPosition().x(),u->getPosition().y(),Colors::Yellow);
  }



  //draw center position

  if (Broodwar->getFrameCount()%2==0){
    BWAPI::Broodwar->drawTriangleMap(enemyBaseCenter.x()-20,enemyBaseCenter.y()+20,enemyBaseCenter.x()+20,enemyBaseCenter.y()+20,enemyBaseCenter.x(),enemyBaseCenter.y()-20,Colors::Orange);
    BWAPI::Broodwar->drawCircleMap(enemyBaseCenter.x(),enemyBaseCenter.y(),2,Colors::Orange,true);
    BWAPI::Broodwar->drawTextMap(enemyBaseCenter.x()-20,enemyBaseCenter.y()-20,"\x11 Region Center Position:(%d,%d)",enemyBaseCenter.x(),enemyBaseCenter.y());
  }
  else {
    BWAPI::Broodwar->drawTriangleMap(enemyBaseCenter.x()-20,enemyBaseCenter.y()+20,enemyBaseCenter.x()+20,enemyBaseCenter.y()+20,enemyBaseCenter.x(),enemyBaseCenter.y()-20,Colors::Red);
    BWAPI::Broodwar->drawCircleMap(enemyBaseCenter.x(),enemyBaseCenter.y(),2,Colors::Red,true);
    BWAPI::Broodwar->drawTextMap(enemyBaseCenter.x()-20,enemyBaseCenter.y()-20,"\x08 Region Center Position:(%d,%d)",enemyBaseCenter.x(),enemyBaseCenter.y());
  }
  int mapheight=Broodwar->mapHeight();
  int mapwidth = Broodwar->mapWidth(); 

  //draw a circle by the center of enemyBaseCenter, and radius is 25 tilePosition length
  for(int x=(enemyBaseCenter.x()/32-25);x<(enemyBaseCenter.x()/32+25);x++){
    for (int y=(enemyBaseCenter.y()/32-25);y<(enemyBaseCenter.y()/32+25);y++){
      bool conditionflag = false;
      Position* newp= new Position(x*32,y*32);
      if(x<0||y<0||x>=Broodwar->mapHeight()||y>=Broodwar->mapWidth())
        continue;
      else if((newp->getApproxDistance(enemyBaseCenter)<32*10&&newp->getApproxDistance(enemyBaseCenter)>32*9&&Broodwar->getUnitsInRadius(*newp,16).size()==0)||(newp->getApproxDistance(enemyBaseCenter)<32*15 && newp->getApproxDistance(enemyBaseCenter)>32*14 && Broodwar->getUnitsInRadius(*newp,16).size()==0 )){
        //the first position
        if (this->previousPositionSet.size()==0){
          this->previousPositionSet.insert(*newp);
          this->previousPositionSet.insert(Position((this->enemyStartLocation->getTilePosition().x()-2)*32,(this->enemyStartLocation->getTilePosition().y()-2)*32));
        }

        //other positions
        if (this->previousPositionSet.size()!=0){
          for each(Position pp in this->previousPositionSet){
            /*		if(newp->getApproxDistance(pp)<32*5){
            conditionflag =true;
            }*/
            if((newp->getApproxDistance(enemyBaseCenter)<32*10&&newp->getApproxDistance(enemyBaseCenter)>32*9)
              &&(pp.getApproxDistance(enemyBaseCenter)<32*10&&pp.getApproxDistance(enemyBaseCenter)>32*9))
            {
              if (newp->getApproxDistance(pp)<32*10)
                conditionflag = true;							
            }
            else{
              if (newp->getApproxDistance(pp)<32*7)
                conditionflag = true;	
            }
          }
          if(conditionflag)
            continue;
          else{
            if(this->previousPositionSet.find(*newp)==this->previousPositionSet.end())
              this->previousPositionSet.insert(*newp);
          }						
        }
      }
      else
        continue;
    }
  }
  if (this->previousPositionSet.size()!=0){
    //draw positions
    for each(Position pp in this->previousPositionSet){
      if(u->getTargetPosition()==pp){
        BWAPI::Broodwar->drawTextMap(pp.x()-20,pp.y()-20,"Current Target:(%d,%d)",pp.x(),pp.y());
        BWAPI::Broodwar->drawCircleMap(pp.x(),pp.y(),2,Colors::Green,true);
        BWAPI::Broodwar->drawCircleMap(pp.x(),pp.y(),8,Colors::Green);
      }
      else{
        BWAPI::Broodwar->drawTextMap(pp.x()-20,pp.y()-20,"Position:(%d,%d)",pp.x(),pp.y());
        BWAPI::Broodwar->drawCircleMap(pp.x(),pp.y(),2,Colors::Blue,true);
        BWAPI::Broodwar->drawCircleMap(pp.x(),pp.y(),8,Colors::Blue);
      }


    }


    //unit go to position
    if(u->getLastCommand().getType()==UnitCommandTypes::Move&&(u->getPosition().getApproxDistance(this->lastExplorPosition)>32*3+16)&&this->lastExplorPosition!=Positions::None){
      if (Broodwar->getFrameCount()%24==0)
        u->move(this->lastExplorPosition);
      return;
    }	


    else{
      for each(Position _position in this->previousPositionSet){
        if(this->lockedPositionSet.size()==this->previousPositionSet.size()){
          this->lockedPositionSet.clear();
          return;
        }

        else if(this->lockedPositionSet.find(_position)!=this->lockedPositionSet.end())
          continue;
        else{					
          if(this->lastExplorPosition==Positions::None ||(_position.getApproxDistance(this->lastExplorPosition)>5 && _position.getApproxDistance(this->lastExplorPosition)<15*32)){
            this->lockedPositionSet.insert(this->lastExplorPosition);
            u->move(_position);
            this->lastExplorPosition = _position;
            return;
          }	
          else
            continue;
        }	
      }
    }
  }
}

void ScoutManager::unitFlee(Unit* u)
{
  
}

void ScoutManager::setExpansionToScout()
{
  std::set<BaseLocation*> allmyBaseLocation;
  std::set<BaseClass*> allmyBaseClass = this->bmc->getBaseSet();
  this->baseLocationNeedToScout.clear();
  for each(BaseClass* bc in allmyBaseClass)
	{
    allmyBaseLocation.insert(bc->getBaseLocation());
  }
  for each(BWTA::BaseLocation* bl in this->mapBaseLocations)
	{
    if (allmyBaseLocation.find(bl) != allmyBaseLocation.end())
      continue;
    if (this->LocationsHasEnemy.find(bl)!=this->LocationsHasEnemy.end())
      continue;
    if(bl->isIsland())
      continue;
		//_T_
		if (this->terrainManager->getGroundDistance(Broodwar->self()->getStartLocation(), bl->getTilePosition()) < 0)
			continue;
		if (ICEStarCraft::Helper::isDirectlyConnected(bl->getRegion(),BWTA::getRegion(Broodwar->self()->getStartLocation())))
			continue;
		if (this->eInfo->isEnemyBase(bl) && bl != terrainManager->eNearestBase)
			continue;
		if (this->enemyStartLocation && ICEStarCraft::Helper::isDirectlyConnected(bl->getRegion(),this->enemyStartLocation->getRegion()))
		{
			if (terrainManager->gameMap == Maps::Andromeda || terrainManager->gameMap == Maps::Electric_Circuit)
			{
				continue;
			}
		}

		if (Broodwar->isVisible(bl->getTilePosition()))
		{
			TilePosition tp = bl->getTilePosition();
			if (Broodwar->isVisible(TilePosition(tp.x()-6,tp.y()).makeValid()) &&
				  Broodwar->isVisible(TilePosition(tp.x()+6,tp.y()).makeValid()) &&
					Broodwar->isVisible(TilePosition(tp.x(),tp.y()-6).makeValid()) &&
					Broodwar->isVisible(TilePosition(tp.x(),tp.y()+6).makeValid()))
			{
				continue;
			}
		}//_T_
    
		this->baseLocationNeedToScout.insert(bl);
  }
}


void ScoutManager::setNeedScan(bool needscan)
{
  this->needMoreScan = needscan;
}

bool ScoutManager::hasArmyKeep(Unit* u ,BaseLocation* bl,int Radius,int armyNum)
{	int i =0;
if(u->getPosition().getApproxDistance(bl->getPosition())<=u->getType().sightRange()){								
  std::set<Unit*> inRadiusSet = Broodwar->getUnitsInRadius(bl->getPosition(),Radius);
  for each(Unit* inRadiusUnit in inRadiusSet){
    if (inRadiusUnit->getPlayer()==Broodwar->enemy()&& inRadiusUnit->getType().isWorker())
      return true;
    else if(inRadiusUnit->getPlayer()==Broodwar->enemy() && !inRadiusUnit->getType().isWorker() &&!inRadiusUnit->getType().isBuilding()){
      i++;
      if (i>=armyNum)
        return true;	
    }				
  }
}
return false;
}

bool ScoutManager::hasArmyKeep(Unit* u)
{	
  UnitGroup egroup = SelectAllEnemy(canAttack).not(isBuilding).inRadius(u->getType().sightRange()*2,u->getPosition());
  if (egroup.size()>=1 && u->isUnderAttack())
    return true;
  else
    return false;
}

bool ScoutManager::seeResourceDepot(Unit* u ,BaseLocation* bl)
{
  if(u->getPosition().getApproxDistance(bl->getPosition())<=u->getType().sightRange()){								
    std::set<Unit*> onTileSet = Broodwar->getUnitsOnTile(bl->getTilePosition().x(),bl->getTilePosition().y());
    for each(Unit* onTileUnit in onTileSet){
      if(onTileUnit->getPlayer()==Broodwar->enemy()&&onTileUnit->getType().isResourceDepot())
        return true;									
    }
  }
  return false;
}

bool ScoutManager::scoutFinish(Unit* u ,BaseLocation* bl)
{
  if (u->getType()==UnitTypes::Terran_Vulture ||u->getType()==UnitTypes::Terran_SCV){
    if(ScoutUnitPurposeMap[u]==EnemyOpening){
      if(Broodwar->getFrameCount()>=24*60*6)
        return true;
      else if (SelectAllEnemy()(Dragoon,Vulture,Siege_Tank,Sunken_Colony,Ion_Cannon).inRadius(u->getType().sightRange()*2,bl->getPosition()).size()>=1)
        return true;
      else if (SelectAllEnemy()(canAttack).not(isBuilding,isWorker).inRadius(u->getType().sightRange()*2,bl->getPosition()).size()>=3)
        return true;
      else if (unitInDanger(u)&&u->getType().maxHitPoints()/u->getHitPoints()>=3)
        return true;
      else 
        return false;
    }
  }
  else if (u->getType()==UnitTypes::Spell_Scanner_Sweep){
    if(this->needMoreScan==false)
      return true;
    else
      return false;
  }

  return false;
}

void ScoutManager::fixMovingStuck(Unit* u)
{
  if (u->getOrder()==Orders::Move && u->getLastCommand().getType()==UnitCommandTypes::Move && u->getPosition().getApproxDistance(this->lastExplorPosition)>32*2+16){
    if(Broodwar->getFrameCount()%24*2==0)
      this->returnTimes++;
    if (this->returnTimes>=16){
      u->move(this->myStartLocation->getPosition());
      this->returnTimes = 0;
    }		
  }
  if(u->getLastCommand().getType()==UnitCommandTypes::Move && !u->isMoving()){
    if(Broodwar->getFrameCount()%24==0)
      u->move(this->lastExplorPosition);
  }

  else{
    if(Broodwar->getFrameCount()%24==0)
      u->move(this->lastExplorPosition);
  }
}

void ScoutManager::setScoutNum(int num)
{
  this->needScoutNum = num;

}

bool isWalkTileWalkable(Position pos)
{
  return Broodwar->isWalkable(pos.x()/8, pos.y()/8) && SelectAllEnemy().not(isFlyer,isLifted).inRadius(16,pos).empty();
}

void ScoutManager::destroy()
{
  ScoutController::destroy();
  if (theScoutManager) delete theScoutManager;
}

bool ScoutManager::unitInDanger(Unit* u)
{
	if (u->isUnderAttack())
	{
		return true;
	}

	for each (Unit* e in Broodwar->enemy()->getUnits())
	{
		if (!e->isCompleted() || e->getPosition().getApproxDistance(u->getPosition()) > u->getType().sightRange())
		{
			continue;
		}

		UnitType type = e->getType();

		if (type.isBuilding())
		{
			if (type != UnitTypes::Protoss_Photon_Cannon && type != UnitTypes::Zerg_Sunken_Colony)
			{
				continue;
			}
		}
		
		if (type.canAttack())
		{
			if (type == UnitTypes::Terran_Vulture_Spider_Mine || type == UnitTypes::Protoss_Scarab)
			{
				continue;
			}
		}
		else
		{
			if (type != UnitTypes::Protoss_Reaver && type != UnitTypes::Protoss_Carrier)
			{
				continue;
			}
		}

		return true;
	}

	return false;
}