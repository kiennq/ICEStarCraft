#include "ScoutManager.h"
#include "math.h"
#include <time.h>
#include "MentalState.h"

using namespace BWAPI;
using namespace BWTA;
using namespace ICEStarCraft;


ScoutManager* theScoutManager = NULL;

static const Position Up = Position(0,-48);
static const Position Down = Position(0,48);
static const Position Left = Position(-48,0);
static const Position Right = Position(48,0);
static const Position UpLeft = Position(-48,-48);
static const Position UpRight = Position(48,-48);
static const Position DownLeft = Position(-48,48);
static const Position DownRight = Position(48,48);

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
  this->ScoutUnitPuporseMap.clear();
  this->lastExplorPosition=BWAPI::Positions::None;
  this->ScoutUnitLastPuporseMap.clear();
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
  this->onceCommand = new issueOnce();
  this->bestPrewarningpo = Positions::None;
  this->startLocationsExplored.clear();
  this->essentialPostions.clear();

  mInfo=NULL;
  eInfo=NULL;

	_switchRegionFlag = false;
	_nextTargetBase = NULL;

  this->ScoutUpPosition = BWAPI::Positions::None;
  this->ScoutDownPosition = BWAPI::Positions::None;
  this->ScoutLeftPosition = BWAPI::Positions::None;
  this->ScoutRightPosition = BWAPI::Positions::None;
  this->ScoutUpRightPosition = BWAPI::Positions::None;
  this->ScoutUpLeftPosition = BWAPI::Positions::None;
  this->ScoutDownRightPosition = BWAPI::Positions::None;
  this->ScoutDownLeftPosition = BWAPI::Positions::None;
  this->ScoutCenterPosition = BWAPI::Positions::None;
  this->SMD = 32;
  this->moveLevel = 0;
  //12;
	
	this->deadScoutUnitCount = 0;
};


void ScoutManager::setManagers(){
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
	for (map<Unit*,ScoutPurpose>::iterator i = this->ScoutUnitPuporseMap.begin(); i != this->ScoutUnitPuporseMap.end(); i++)
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
    else if (this->scoutGroup.find(u) == this->scoutGroup.end())
      this->scoutGroup.insert(u);
  }

  for each (Unit* u in this->scoutGroup)
	{
    if (u->isConstructing())
		{
      continue;
    }
    else if (this->ScoutUnitPuporseMap.find(u) == this->ScoutUnitPuporseMap.end())
      this->ScoutUnitPuporseMap[u] = purpose;
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
			this->ScoutUnitPuporseMap.erase(u);
		}

		if (u->getType() == UnitTypes::Terran_SCV && this->ScoutUnitPuporseMap[u] == ScoutManager::EnemyExpansion)
		{
			//Broodwar->printf("delete SCV from scout group");
			this->scoutGroup.erase(u);
			this->ScoutUnitPuporseMap.erase(u);
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
    if(this->ScoutUnitPuporseMap.find(u) == this->ScoutUnitPuporseMap.end())
		{
      this->ScoutUnitPuporseMap[u] = purpose;
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
  }
  if (this->ScoutUnitPuporseMap.find(unit) != this->ScoutUnitPuporseMap.end())
	{
    this->ScoutUnitPuporseMap.erase(unit);
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
		this->ScoutUnitPuporseMap[u] = EnemyOpening;
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

	if(this->ScoutUnitPuporseMap[u] == EnemyStartLocation)
	{
		UnitGroup inRadiusSet = SelectAllEnemy().inRadius(u->getType().sightRange(),u->getPosition());
		if (inRadiusSet(isBuilding).size() >= 1)
		{
			for each (BaseLocation* bl in startLocationsToScout)
			{
				if (getRegion(inRadiusSet(isBuilding).getCenter()) == getRegion(bl->getPosition()))
				{
					this->enemyStartLocation = bl;	
					this->ScoutUnitPuporseMap[u] = EnemyOpening;	
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
				this->ScoutUnitPuporseMap[u] = EnemyOpening;	
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
								this->ScoutUnitPuporseMap[u] = EnemyOpening;
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
  if(this->enemyStartLocation==NULL)
    return;
  //for each(Unit* u in this->scoutGroup)
	{
    // for scv and vulture
    if(u->getType()==UnitTypes::Terran_SCV ||u->getType()==UnitTypes::Terran_Vulture){
      if (u->isConstructing()){
        return;
      }
      if(this->ScoutUnitPuporseMap[u]==EnemyOpening)
			{       
				if (!_switchRegionFlag && BWTA::getRegion(u->getPosition()) == enemyStartLocation->getRegion())
				{
					if (Broodwar->getFrameCount() % (24*60) == 0)
					{
						if (!_nextTargetBase)
						{
              double dis = 9999;
              for each (BaseLocation *b in BWTA::getBaseLocations()){
                if (b!=enemyStartLocation)
								{
                  double tdis = BWTA::getGroundDistance(b->getTilePosition(),enemyStartLocation->getTilePosition());
                  if (dis>tdis && !(BWTA::getShortestPath(b->getTilePosition(), enemyStartLocation->getTilePosition()).empty()) && !b->getGeysers().empty()) {
                    dis = tdis;
                    _nextTargetBase = b;
                  } 
                }
              }
						}
						ScoutController::create()->removeFromScoutSet(u);
						_switchRegionFlag = true;
						u->move(_nextTargetBase->getPosition());
						// Broodwar->printf("Go to next base");
					}
					else
					{
						ScoutController::create()->addToScoutSet(u);
						ScoutController::create()->onFrame();
					}
				}
				else if (_nextTargetBase && _switchRegionFlag && BWTA::getRegion(u->getPosition()) == _nextTargetBase->getRegion())
				{
					if (Broodwar->getFrameCount() % (24*10) == 0)
					{
						ScoutController::create()->removeFromScoutSet(u);
						_switchRegionFlag = false;
						u->move(enemyStartLocation->getPosition());
						// Broodwar->printf("Go to enemy base");
					}
					else
					{
						ScoutController::create()->addToScoutSet(u);
						ScoutController::create()->onFrame();
					}
				}
				else if (!_switchRegionFlag)
				{
					u->move(enemyStartLocation->getPosition());
        }
				else
				{
					u->move(_nextTargetBase->getPosition());
        }
        
        if (!_nextTargetBase)
				{
          double dis = 9999;
          for each (BaseLocation *b in BWTA::getBaseLocations()){
            if (b!=enemyStartLocation){
              double tdis = BWTA::getGroundDistance(b->getTilePosition(),enemyStartLocation->getTilePosition());
              if (dis>tdis && !(BWTA::getShortestPath(b->getTilePosition(), enemyStartLocation->getTilePosition()).empty()) && !b->getGeysers().empty()) {
                dis = tdis;
                _nextTargetBase = b;
              } 
            }
          }
        }

        if (_nextTargetBase) Broodwar->drawCircleMap(_nextTargetBase->getPosition().x(), _nextTargetBase->getPosition().x(), 5, Colors::Orange, true);
       
        //if(!enemyInSCVSightRange(u)/*unitInDanger(u)==false*/){
        //  explorEnemyBase(u);										
        //}
        //else if(enemyInSCVSightRange(u)/*unitInDanger(u)*/){
        //  testScoutRun(u);
        //}

      }
    }


    //else if (u->getType()==UnitTypes::Spell_Scanner_Sweep && this->ScoutUnitPuporseMap[u]==EnemyOpening){
    //	//for scanner
    //	std::set<Position> placeToScan;
    //	placeToScan.clear();
    //	placeToScan.insert(this->enemyStartLocation->getPosition());
    //	placeToScan.insert(this->terrainManager->eBaseCenter);
    //	placeToScan.insert(this->terrainManager->efirstChokepoint->getCenter());
    //	for each(Position p in placeToScan){
    //		if(this->needMoreScan){
    //			if(Broodwar->getFrameCount()-this->lastScanTime>=24*30)
    //				this->scannedPositions.clear();
    //			if(u->getEnergy()>=50){
    //				if(this->scannedPositions.find(p)!=this->scannedPositions.end())
    //					continue;
    //				else{
    //					u->useTech(TechTypes::Scanner_Sweep,p);
    //					this->scannedPositions.insert(p);
    //					this->lastScanTime = Broodwar->getFrameCount();
    //					continue;
    //				}

    //			}
    //			else
    //				break;
    //		}	
    //	}	
    //	if(this->scannedPositions.size()>=placeToScan.size())
    //		this->needMoreScan=false;
    //	else
    //		continue;
    //}
  }
}

void ScoutManager::scoutEnemyExpansion(Unit* u)
{		
	for each (BWTA::BaseLocation* base in this->baseLocationNeedToScout)
	{
		Broodwar->drawCircleMap(base->getPosition().x(),base->getPosition().y(),30,Colors::Yellow,true);
	}
	//Broodwar->drawTextScreen(5,60,"BaseLocationNeedToScout: %d | %d",this->baseLocationNeedToScout.size(),Broodwar->getFrameCount());
	//Broodwar->drawTextScreen(5,70,"BaseLocationExplored: %d",this->baseLocationsExplored.size());
	
	if (this->ScoutUnitPuporseMap[u] != EnemyExpansion)
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
		this->ScoutUnitPuporseMap.erase(u);
		Broodwar->printf("Finish scouting");
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
		Broodwar->drawLineMap(u->getPosition().x(),u->getPosition().y(),tar.x(),tar.y(),Colors::White);
	}

	if (unitInDanger(u) == false)
	{
		if(Broodwar->getFrameCount()%(24*3)==0)
			u->move(this->currentLocationTarget->getPosition());

		//_T_
		int range = u->getType() == UnitTypes::Terran_Vulture ? (u->getSpiderMineCount() > 0 ? 32 : u->getType().sightRange()) : u->getType().sightRange();
		if (range == 32 && !SelectAll()(isBuilding,SCV).inRadius(32*2,this->currentLocationTarget->getPosition()).empty())
		{
			range = u->getType().sightRange();
		}

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
			Broodwar->printf("skip scouting Base(%d,%d)",this->currentLocationTarget->getTilePosition().x(),this->currentLocationTarget->getTilePosition().y());
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
	if (u->getType() != UnitTypes::Terran_SCV || this->ScoutUnitPuporseMap[u] != MyMainBase)
	{
		return;
	}
	
	vector<Position> vertices = BWTA::getRegion(Broodwar->self()->getStartLocation())->getPolygon();
	static int current = 0;
	static int lastOrderFrame = 0;

	if (Broodwar->getFrameCount() >= 24*60*4
			||
			!SelectAllEnemy().inRegion(BWTA::getRegion(Broodwar->self()->getStartLocation())).empty()
			||
			MentalClass::create()->STflag != MentalClass::NotSure
		  ||
			u->getOrder() == Orders::ConstructingBuilding || u->isConstructing() || u->isAttacking() || u->isRepairing())
	{
		this->ScoutUnitPuporseMap.erase(u);
		this->scoutGroup.erase(u);
		this->workerMG->_workerUnits.insert(u);
		this->workerMG->_workerState.erase(u);
		this->setScoutNum(1);
		lastOrderFrame = 0;
		return;
	}
	
	Broodwar->drawLineMap(u->getPosition().x(),u->getPosition().y(),vertices[current].x(),vertices[current].y(),Colors::Green);
		
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
  if(this->PreWarningPositionSet.size()==0){
    if(this->terrainManager->eNearestBase!=NULL)
      this->PreWarningPositionSet.insert(this->terrainManager->eNearestBase->getPosition());		

    Position mapcenter = Position(Broodwar->mapWidth()/2*32,Broodwar->mapHeight()/2*32);
    std::set<Position> pset;
    pset.clear();

    for (int x=1;x<Broodwar->mapWidth();x++){
      for(int y=1;y<Broodwar->mapHeight();y++){
        Position newposition = Position(x*32,y*32);
        if(newposition.getApproxDistance(this->terrainManager->eThirdChokepoint->getCenter())>32*13
          &&newposition.getApproxDistance(this->terrainManager->eThirdChokepoint->getCenter())<32*15
          &&newposition.getApproxDistance(this->enemyStartLocation->getPosition())>32*32
          &&newposition.getApproxDistance(mapcenter)<mapcenter.getApproxDistance(this->terrainManager->eThirdChokepoint->getCenter())
          &&isTileWalkable((TilePosition)newposition)){
            pset.insert(newposition);
        }		
      }
    }
    int sdis=0;

    for each(Position p in pset){
      if((sdis==0 || p.getApproxDistance(this->terrainManager->mThirdChokepoint->getCenter())<sdis)&&isSurroundingWalkable(p.x(),p.y())){
        sdis = p.getApproxDistance(this->terrainManager->mThirdChokepoint->getCenter());
        this->bestPrewarningpo = p;
      }
    }

    this->PreWarningPositionSet.insert(this->bestPrewarningpo);
    this->PreWarningPositionSet.insert(mapcenter);
  }

  //for each(Unit* u in this->scoutGroup)
	{
    if (u->getType()==UnitTypes::Terran_SCV || u->getType()==UnitTypes::Terran_Vulture ){
      if(this->ScoutUnitPuporseMap[u]==PreWarning){
        if (Broodwar->getFrameCount()<24*60*5 && Broodwar->getFrameCount()%24*25==0){
          this->ScoutUnitPuporseMap[u]=EnemyOpening;
          return;
        }
        if(unitInDanger(u)==false){
          //if (this->lastExplorPosition!=NULL)					
          //	Broodwar->drawCircleMap(this->lastExplorPosition.x(),this->lastExplorPosition.y(),16,Colors::Red,true);
          if (this->LocationsHasEnemy.size()<2||Broodwar->getFrameCount()<24*60*6){
            for each(Position p in this->PreWarningPositionSet){
              if (this->lastExplorPosition!=this->terrainManager->eNearestBase->getPosition()){
                this->lastExplorPosition = this->terrainManager->eNearestBase->getPosition();
              }
            }

            if(u->getPosition().getApproxDistance(this->lastExplorPosition)>32*4){
              if (Broodwar->getFrameCount()%24==0)
                u->move(this->lastExplorPosition);						
            }

            else{
              if (u->getLastCommand().getType()==UnitCommandTypes::Patrol && Broodwar->getFrameCount()%24!=0)
                return;
              else
                this->onceCommand->patrolOnce(u,this->lastExplorPosition);									
            }

          }
          else if(this->LocationsHasEnemy.size()>2||(Broodwar->getFrameCount()>24*60*5&&Broodwar->getFrameCount()<24*60*10)){

            this->lastExplorPosition = this->bestPrewarningpo;	
            if (Broodwar->getFrameCount()<6 && Broodwar->getFrameCount()%24*30==0){
              this->lastExplorPosition = this->terrainManager->eNearestBase->getPosition();
              return;
            }

            if(u->getPosition().getApproxDistance(this->lastExplorPosition)>32*4){
              if (Broodwar->getFrameCount()%24==0)
                u->move(this->lastExplorPosition);
            }

            else{
              if (u->getLastCommand().getType()==UnitCommandTypes::Patrol && Broodwar->getFrameCount()%24!=0)
                return;
              else
                this->onceCommand->patrolOnce(u,this->lastExplorPosition);									
            }
          }
          else if (Broodwar->getFrameCount()>24*60*10||this->PreWarningPositionSet.size()==1){

            if (Broodwar->getFrameCount()<9 && Broodwar->getFrameCount()%24*30==0){
              this->lastExplorPosition = this->bestPrewarningpo;
              return;
            }
            this->lastExplorPosition = Position(Broodwar->mapWidth()/2*32,Broodwar->mapHeight()/2*32);			

            if(u->getPosition().getApproxDistance(this->lastExplorPosition)>32*4){
              if (Broodwar->getFrameCount()%24==0)
                u->move(this->lastExplorPosition);
            }

            else{
              if (u->getLastCommand().getType()==UnitCommandTypes::Patrol && Broodwar->getFrameCount()%24!=0)
                return;
              else
                this->onceCommand->patrolOnce(u,this->lastExplorPosition);									
            }
          }
        }

        else if(unitInDanger(u)){

          if (Broodwar->getFrameCount()%12==0)
            unitNextRunningPosition(u);

          if (this->lastExplorPosition==this->terrainManager->eNearestBase->getPosition()){
            if(hasArmyKeep(u,this->terrainManager->eNearestBase,u->getType().sightRange()*2,2)||seeResourceDepot(u,this->terrainManager->eNearestBase)){
              this->lastExplorPosition=this->bestPrewarningpo;
              this->LocationsHasEnemy.insert(this->terrainManager->eNearestBase);
              if(Broodwar->getFrameCount()>=24*60*6){
                this->PreWarningPositionSet.erase(this->terrainManager->eNearestBase->getPosition());
                return;
              }							
            }				
          }
          else if(this->lastExplorPosition==this->bestPrewarningpo){
            if(hasArmyKeep(u)){
              this->lastExplorPosition=Position(Broodwar->mapWidth()/2*32,Broodwar->mapHeight()/2*32);
              if(Broodwar->getFrameCount()>=24*60*8){
                this->PreWarningPositionSet.erase(this->bestPrewarningpo);
                return;
              }

            }
          }
          //if(this->ScoutUnitLastPuporseMap.size()==0 || this->ScoutUnitLastPuporseMap.find(u)==this->ScoutUnitLastPuporseMap.end()){
          //	if (this->ScoutUnitPuporseMap[u]!=Running){
          //		this->ScoutUnitLastPuporseMap[u]=this->ScoutUnitPuporseMap[u];
          //	}						
          //}
          //this->ScoutUnitPuporseMap[u]=Running;
          //return;	
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        //			for each(Position p in this->PreWarningPositionSet){
        //				//give first position
        //				if (this->lastExplorPosition!=this->terrainManager->eNearestBase->getPosition()){
        //						this->lastExplorPosition = this->terrainManager->esecondChokepoint->getCenter();
        //				}
        //				// check whether reach 2nd terrainManager
        //				else if(this->lastExplorPosition == this->terrainManager->esecondChokepoint->getCenter()){
        //					if(this->lastExplorPosition==this->terrainManager->esecondChokepoint->getCenter() && u->getPosition().getApproxDistance(this->lastExplorPosition)<2*32+16){
        //						this->lastExplorPosition = this->terrainManager->eThirdChokepoint->getCenter();
        //						break;
        //					}
        //					else
        //						fixMovingStuck(u);					
        //				}
        //				// check whether reach 3rd terrainManager
        //				else if(this->lastExplorPosition == this->terrainManager->eThirdChokepoint->getCenter()){
        //					if(this->lastExplorPosition==this->terrainManager->eThirdChokepoint->getCenter() && u->getPosition().getApproxDistance(this->lastExplorPosition)<2*32+16){
        //						this->lastExplorPosition = Position(Broodwar->mapHeight()/2*32,Broodwar->mapWidth()/2*32);
        //						break;
        //					}
        //					else
        //						fixMovingStuck(u);
        //				}

        //				//check whether reach map center
        //				else if(this->lastExplorPosition == Position(Broodwar->mapHeight()/2*32,Broodwar->mapWidth()/2*32)){
        //					if(this->lastExplorPosition==Position(Broodwar->mapHeight()/2*32,Broodwar->mapWidth()/2*32) && u->getPosition().getApproxDistance(this->lastExplorPosition)<2*32+16){
        //						this->lastExplorPosition = this->terrainManager->esecondChokepoint->getCenter();
        //						break;
        //					}						
        //					else
        //						fixMovingStuck(u);
        //				}
        //			}		
        //		}
        //	
        //		else if(unitInDanger(u)){
        //			if(this->ScoutUnitLastPuporseMap.size()==0 || this->ScoutUnitLastPuporseMap.find(u)==this->ScoutUnitLastPuporseMap.end()){
        //				if (this->ScoutUnitPuporseMap[u]!=Running){
        //					this->ScoutUnitLastPuporseMap[u]=this->ScoutUnitPuporseMap[u];
        //				}						
        //			}
        //			this->ScoutUnitPuporseMap[u]=Running;
        //			return;	
        //		}
        //	}
        //	//if time > 5 min
        //	else if(Broodwar->getFrameCount()>24*60*5 && this->ScoutUnitPuporseMap[u]==PreWarning){
        //		if(unitInDanger(u)==false){
        //			for each(Position p in this->PreWarningPositionSet){
        //				//give first position
        //				if (this->lastExplorPosition!=this->terrainManager->eThirdChokepoint->getCenter()&&
        //					this->lastExplorPosition!=Position(Broodwar->mapHeight()/2*32,Broodwar->mapWidth()/2*32)){
        //						this->lastExplorPosition = this->terrainManager->eThirdChokepoint->getCenter();
        //				}
        //				// check whether reach 2nd terrainManager
        //				else if(this->lastExplorPosition == this->terrainManager->esecondChokepoint->getCenter()){
        //					this->lastExplorPosition = this->terrainManager->eThirdChokepoint->getCenter();
        //				}			
        //				// check whether reach 3rd terrainManager
        //				else if(this->lastExplorPosition == this->terrainManager->eThirdChokepoint->getCenter()){
        //					if(this->lastExplorPosition==this->terrainManager->eThirdChokepoint->getCenter() && u->getPosition().getApproxDistance(this->lastExplorPosition)<3*32+16){
        //						this->lastExplorPosition = Position(Broodwar->mapHeight()/2*32,Broodwar->mapWidth()/2*32);
        //						break;
        //					}
        //					else
        //						fixMovingStuck(u);
        //				}

        //				//check whether reach map center
        //				else if(this->lastExplorPosition == Position(Broodwar->mapHeight()/2*32,Broodwar->mapWidth()/2*32)){
        //					if(this->lastExplorPosition==Position(Broodwar->mapHeight()/2*32,Broodwar->mapWidth()/2*32) && u->getPosition().getApproxDistance(this->lastExplorPosition)<3*32+16){
        //						this->lastExplorPosition = this->terrainManager->eThirdChokepoint->getCenter();
        //						break;
        //					}						
        //					else
        //						fixMovingStuck(u);
        //				}
        //			}		
        //		}
        //		else if(unitInDanger(u)){
        //			if(this->ScoutUnitLastPuporseMap.size()==0 || this->ScoutUnitLastPuporseMap.find(u)==this->ScoutUnitLastPuporseMap.end()){
        //				if (this->ScoutUnitPuporseMap[u]!=Running){
        //					this->ScoutUnitLastPuporseMap[u]=this->ScoutUnitPuporseMap[u];
        //				}						
        //			}
        //			this->ScoutUnitPuporseMap[u]=Running;
        //			return;	
        //		}
        //	}
        //	else{
        //		return;
        //	}
      }
    }
  }
}
void ScoutManager::onFrame()
{
  if(Broodwar->getFrameCount()>=24*60*3 && (Broodwar->enemy()->getRace()==Races::Zerg))
	{
    // enemy doesn't has lurker,or doesn't have intention to produce lurker,then we don't need to save scanner's energy
    // if time <=10 mins
    // if no lurker , no lurker egg, no lair + (hydralisk den or hydralisk) 
    if(this->mental->goAttack ||
      (this->eInfo->EnemyhasBuilt(UnitTypes::Zerg_Lurker,2))||(this->eInfo->EnemyhasBuilt(UnitTypes::Zerg_Lurker_Egg,2))||
      ((this->eInfo->EnemyhasBuilt(UnitTypes::Zerg_Lair,2))&&((this->eInfo->EnemyhasBuilt(UnitTypes::Zerg_Hydralisk_Den,2))||(this->eInfo->EnemyhasBuilt(UnitTypes::Zerg_Hydralisk,2))))){
        ScannerScout(EnemyOpening,4);
    }			
    else if(Broodwar->getFrameCount()<=24*60*8)
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
		  Broodwar->getFrameCount() >= 24*60*2 && Broodwar->getFrameCount() <= 24*60*4 && Broodwar->getFrameCount()%(24*10) == 0)
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

	for(std::map<Unit*,ScoutPurpose>::iterator i=this->ScoutUnitPuporseMap.begin();i!=this->ScoutUnitPuporseMap.end();i++)
	{
		switch ((*i).second)
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
			unitFlee((*i).first);
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
  if (this->ScoutUnitPuporseMap[u]==Running && unitInDanger(u))
	{
    if(this->ScoutUnitLastPuporseMap.find(u)!=this->ScoutUnitLastPuporseMap.end()){
      //if(this->ScoutUnitLastPuporseMap[u]==EnemyOpening){
      //	if(scoutFinish(u,this->enemyStartLocation)){
      //		this->ScoutUnitLastPuporseMap[u]=PreWarning;
      //		(u)->rightClick(unitNextRunningPosition(u));
      //		return;
      //	}
      //	else if(Broodwar->getFrameCount()%12==0 && u->getPosition().getApproxDistance(this->enemyStartLocation->getPosition())>32*3+16){
      //		(u)->rightClick(avoidEnemyAttackMove(u,this->enemyStartLocation->getPosition()));
      //		return;
      //	}
      //	else{
      //		(u)->rightClick(unitNextRunningPosition(u));
      //		return;
      //	}	
      //}	

      //else{
      if(Broodwar->getFrameCount()%12==0)
        (u)->rightClick(unitNextRunningPosition(u));
      return;
      //}					
    }		
  }

  else{
    this->ScoutUnitPuporseMap[u] = this->ScoutUnitLastPuporseMap[u];
    this->ScoutUnitLastPuporseMap.erase(u);
    return;
  }
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
		if (this->eInfo->isEnemyBase(bl) && !(this->enemyStartLocation && ICEStarCraft::Helper::isDirectlyConnected(bl->getRegion(),this->enemyStartLocation->getRegion())))
			continue;
		if (this->enemyStartLocation && ICEStarCraft::Helper::isDirectlyConnected(bl->getRegion(),this->enemyStartLocation->getRegion()) && bl->isMineralOnly())
			continue;
		if (Broodwar->isVisible(bl->getTilePosition()))
			continue;//_T_
    
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
    if(ScoutUnitPuporseMap[u]==EnemyOpening){
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


void ScoutManager::testScoutRun(Unit* u)
{
  double BestP;
  Position NextPosition = BWAPI::Positions::None;

  if (Broodwar->getFrameCount()%8==0){
    //Broodwar->printf("Scout Position (%d,%d)",u->getPosition().x(),u->getPosition().y());
    //Broodwar->printf("EBase Position (%d,%d)",enemyStartLocation->getPosition().x(),enemyStartLocation->getPosition().y());
    //Broodwar->printf("Distance (%f)",u->getTilePosition().getDistance(enemyStartLocation->getTilePosition()));
    //decide detect range
    int sightRange = UnitTypes::Terran_SCV.sightRange()+32;
    Position scvPosition = u->getPosition();
    std::set<Unit*> unitInRange =Broodwar->getUnitsInRadius(scvPosition,sightRange);


    BWTA::Region* R=BWTA::getRegion(this->enemyStartLocation->getPosition());

    Position NearestPerimeter = R->getPolygon().getNearestPoint(u->getPosition());

    for each(Unit* e in unitInRange){
      if (e->getType().isBuilding() || e->getType().isInvincible()){
        double size = sqrt(1.0*e->getType().dimensionUp()*e->getType().dimensionDown() + 1.0*e->getType().dimensionRight()*e->getType().dimensionLeft());
        if (e->getPosition().getApproxDistance(u->getPosition())<=size*2){
          SMD = 16;
          break;
        }			
        else{
          SMD = 64;
        }					
      }
    }
    if (u->getPosition().getApproxDistance(NearestPerimeter) <= 32*3){
      SMD = 16;
    }			
    else{
      SMD = 64;
    }			

    ScoutUpPosition.x() = u->getPosition().x();
    ScoutUpPosition.y() = u->getPosition().y()-SMD;

    ScoutDownPosition.x() = u->getPosition().x();
    ScoutDownPosition.y() = u->getPosition().y()+SMD;

    ScoutLeftPosition.x() = u->getPosition().x()-SMD;
    ScoutLeftPosition.y() = u->getPosition().y();

    ScoutRightPosition.x() = u->getPosition().x()+SMD;
    ScoutRightPosition.y() = u->getPosition().y();

    ScoutUpRightPosition.x() = u->getPosition().x()+SMD;
    ScoutUpRightPosition.y() = u->getPosition().y()-SMD;

    ScoutUpLeftPosition.x() = u->getPosition().x()-SMD;
    ScoutUpLeftPosition.y() = u->getPosition().y()-SMD;

    ScoutDownRightPosition.x() = u->getPosition().x()+SMD;
    ScoutDownRightPosition.y() = u->getPosition().y()+SMD;

    ScoutDownLeftPosition.x() = u->getPosition().x()-SMD;
    ScoutDownLeftPosition.y() = u->getPosition().y()+SMD;

    ScoutCenterPosition.x() = u->getPosition().x();
    ScoutCenterPosition.y() = u->getPosition().y();

    //movement range limitation
    UpP = CalculationPotential(ScoutUpPosition,enemyStartLocation->getPosition());
    DownP = CalculationPotential(ScoutDownPosition,enemyStartLocation->getPosition());
    LeftP = CalculationPotential(ScoutLeftPosition,enemyStartLocation->getPosition());
    RightP = CalculationPotential(ScoutRightPosition,enemyStartLocation->getPosition());
    UpRightP = CalculationPotential(ScoutUpRightPosition,enemyStartLocation->getPosition());
    UpLeftP  = CalculationPotential(ScoutUpLeftPosition,enemyStartLocation->getPosition());
    DownRightP = CalculationPotential(ScoutDownRightPosition,enemyStartLocation->getPosition());
    DownLeftP = CalculationPotential(ScoutDownLeftPosition,enemyStartLocation->getPosition());
    CenterP = CalculationPotential(ScoutCenterPosition,enemyStartLocation->getPosition())-1;

    double mUpU = -100;
    double mDownU = -100;
    double mLeftU = -100;
    double mRightU = -100;
    double mUpRightU = -100;
    double mUpLeftU = -100;
    double mDownRightU = -100;
    double mDownLeftU = -100;

    for each(Unit* e in unitInRange){
      if (e->getType().isInvincible()){
        UpP = UpP+CalculationPotentialBuilding(ScoutUpPosition,e);
        DownP = DownP+CalculationPotentialBuilding(ScoutDownPosition,e);
        LeftP = LeftP+CalculationPotentialBuilding(ScoutLeftPosition,e);
        RightP = RightP+CalculationPotentialBuilding(ScoutRightPosition,e);
        UpRightP = UpRightP+CalculationPotentialBuilding(ScoutUpRightPosition,e);
        UpLeftP = UpLeftP+CalculationPotentialBuilding(ScoutUpLeftPosition,e);
        DownRightP = DownRightP+CalculationPotentialBuilding(ScoutDownRightPosition,e);
        DownLeftP = DownLeftP+CalculationPotentialBuilding(ScoutDownLeftPosition,e);
        CenterP = CenterP+CalculationPotentialBuilding(ScoutCenterPosition,e);
      }
      else if (!Broodwar->self()->isEnemy(e->getPlayer())){
        continue;
      }
      else if(e->getType().isWorker() && !e->isAttacking()){
        continue;
      }
      else if(e->getType().isBuilding()){
        if(e->getType()==UnitTypes::Protoss_Photon_Cannon || e->getType()==UnitTypes::Zerg_Sunken_Colony)
        {
          UpP = UpP+CalculationPotentialDangerousBuilding(ScoutUpPosition,e);
          DownP = DownP+CalculationPotentialDangerousBuilding(ScoutDownPosition,e);
          LeftP = LeftP+CalculationPotentialDangerousBuilding(ScoutLeftPosition,e);
          RightP = RightP+CalculationPotentialDangerousBuilding(ScoutRightPosition,e);
          UpRightP = UpRightP+CalculationPotentialDangerousBuilding(ScoutUpRightPosition,e);
          UpLeftP = UpLeftP+CalculationPotentialDangerousBuilding(ScoutUpLeftPosition,e);
          DownRightP = DownRightP+CalculationPotentialDangerousBuilding(ScoutDownRightPosition,e);
          DownLeftP = DownLeftP+CalculationPotentialDangerousBuilding(ScoutDownLeftPosition,e);
          CenterP = CenterP+CalculationPotentialDangerousBuilding(ScoutCenterPosition,e);
        }
        else
        {
          UpP = UpP+CalculationPotentialBuilding(ScoutUpPosition,e);
          DownP = DownP+CalculationPotentialBuilding(ScoutDownPosition,e);
          LeftP = LeftP+CalculationPotentialBuilding(ScoutLeftPosition,e);
          RightP = RightP+CalculationPotentialBuilding(ScoutRightPosition,e);
          UpRightP = UpRightP+CalculationPotentialBuilding(ScoutUpRightPosition,e);
          UpLeftP = UpLeftP+CalculationPotentialBuilding(ScoutUpLeftPosition,e);
          DownRightP = DownRightP+CalculationPotentialBuilding(ScoutDownRightPosition,e);
          DownLeftP = DownLeftP+CalculationPotentialBuilding(ScoutDownLeftPosition,e);
          CenterP = CenterP+CalculationPotentialBuilding(ScoutCenterPosition,e);
        }
      }
      else if(!e->getType().isBuilding()){
        mUpU = max(mUpU,CalculationPotentialUnit(ScoutUpPosition,e));
        mDownU = max(mDownU, CalculationPotentialUnit(ScoutDownPosition,e));
        mLeftU = max(mLeftU,CalculationPotentialUnit(ScoutLeftPosition,e));
        mRightU = max(mRightU, CalculationPotentialUnit(ScoutRightPosition,e));
        mUpRightU = max(mUpRightU, CalculationPotentialUnit(ScoutUpRightPosition,e));
        mUpLeftU = max(mUpLeftU, CalculationPotentialUnit(ScoutUpLeftPosition,e));
        mDownRightU = max(mDownRightU, CalculationPotentialUnit(ScoutDownRightPosition,e));
        mDownLeftU = max(mDownLeftU, CalculationPotentialUnit(ScoutDownLeftPosition,e));

        //UpP = UpP+CalculationPotentialUnit(ScoutUpPosition,e);
        //DownP = DownP+CalculationPotentialUnit(ScoutDownPosition,e);
        //LeftP = LeftP+CalculationPotentialUnit(ScoutLeftPosition,e);
        //RightP = RightP+CalculationPotentialUnit(ScoutRightPosition,e);
        //UpRightP = UpRightP+CalculationPotentialUnit(ScoutUpRightPosition,e);
        //UpLeftP = UpLeftP+CalculationPotentialUnit(ScoutUpLeftPosition,e);
        //DownRightP = DownRightP+CalculationPotentialUnit(ScoutDownRightPosition,e);
        //DownLeftP = DownLeftP+CalculationPotentialUnit(ScoutDownLeftPosition,e);
        //CenterP = CenterP+CalculationPotentialUnit(ScoutCenterPosition,e);
      }
    }

    UpP += mUpU;
    DownP += mDownU;
    LeftP += mLeftU;
    RightP += mRightU;
    UpLeftP += mUpLeftU;
    UpRightP += mUpRightU;
    DownLeftP += mDownLeftU;
    DownRightP += mDownRightU;

    //UpP = UpP+PolygonPotentialValue(ScoutUpPosition);
    //DownP = DownP+PolygonPotentialValue(ScoutDownPosition);
    //LeftP = LeftP+PolygonPotentialValue(ScoutLeftPosition);
    //RightP = RightP+PolygonPotentialValue(ScoutRightPosition);
    //UpRightP = UpRightP+PolygonPotentialValue(ScoutUpRightPosition);
    //UpLeftP = UpLeftP+PolygonPotentialValue(ScoutUpLeftPosition);
    //DownRightP = DownRightP+PolygonPotentialValue(ScoutDownRightPosition);
    //DownLeftP = DownLeftP+PolygonPotentialValue(ScoutDownLeftPosition);
    //CenterP = CenterP+PolygonPotentialValue(ScoutCenterPosition);
    UpP = UpP+CalculationPotentialPerimeter(ScoutUpPosition);
    DownP = DownP+CalculationPotentialPerimeter(ScoutDownPosition);
    LeftP = LeftP+CalculationPotentialPerimeter(ScoutLeftPosition);
    RightP = RightP+CalculationPotentialPerimeter(ScoutRightPosition);
    UpRightP = UpRightP+CalculationPotentialPerimeter(ScoutUpRightPosition);
    UpLeftP = UpLeftP+CalculationPotentialPerimeter(ScoutUpLeftPosition);
    DownRightP = DownRightP+CalculationPotentialPerimeter(ScoutDownRightPosition);
    DownLeftP = DownLeftP+CalculationPotentialPerimeter(ScoutDownLeftPosition);
    CenterP = CenterP+CalculationPotentialPerimeter(ScoutCenterPosition);


    srand((unsigned)time(NULL));

    BestP = CenterP - 1;
    NextPosition = ScoutCenterPosition;
    Broodwar->drawCircleMap(ScoutLeftPosition.x(),ScoutLeftPosition.y(),2,Colors::Green);
    Broodwar->drawCircleMap(ScoutRightPosition.x(),ScoutRightPosition.y(),2,Colors::Green);
    Broodwar->drawCircleMap(ScoutUpPosition.x(),ScoutUpPosition.y(),2,Colors::Green);
    Broodwar->drawCircleMap(ScoutDownPosition.x(),ScoutDownPosition.y(),2,Colors::Green);
    Broodwar->drawCircleMap(ScoutUpLeftPosition.x(),ScoutUpLeftPosition.y(),2,Colors::Green);
    Broodwar->drawCircleMap(ScoutUpRightPosition.x(),ScoutUpRightPosition.y(),2,Colors::Green);
    Broodwar->drawCircleMap(ScoutDownLeftPosition.x(),ScoutDownLeftPosition.y(),2,Colors::Green);
    Broodwar->drawCircleMap(ScoutDownRightPosition.x(),ScoutDownRightPosition.y(),2,Colors::Green);
    if(UpP + (isWalkTileWalkable(ScoutUpPosition)?0:-100) + ((double) rand() / (RAND_MAX+1)) > BestP + ((double) rand() / (RAND_MAX+1))){
      BestP = UpP;
      if (isWalkTileWalkable((ScoutCenterPosition + Up)))
        NextPosition = ScoutCenterPosition + Up;
    }
    if(DownP + (isWalkTileWalkable(ScoutDownPosition)?0:-100) + ((double) rand() / (RAND_MAX+1)) >BestP + ((double) rand() / (RAND_MAX+1))){
      BestP = DownP;
      if (isWalkTileWalkable((ScoutCenterPosition + Down)))
        NextPosition = ScoutCenterPosition + Down;
    }
    if(LeftP +(isWalkTileWalkable(ScoutLeftPosition)?0:-100) + ((double) rand() / (RAND_MAX+1))>BestP+((double) rand() / (RAND_MAX+1))){
      BestP = LeftP;
      if (isWalkTileWalkable((ScoutCenterPosition + Left)))
        NextPosition = ScoutCenterPosition + Left;
    }
    if(RightP +(isWalkTileWalkable(ScoutRightPosition)?0:-100)+((double) rand() / (RAND_MAX+1)) >BestP+((double) rand() / (RAND_MAX+1))){
      BestP = RightP;
      if (isWalkTileWalkable((ScoutCenterPosition + Right)))
        NextPosition = ScoutCenterPosition + Right;
    }
    if(DownRightP +(isWalkTileWalkable(ScoutDownRightPosition)?0:-100)+((double) rand() / (RAND_MAX+1)) >BestP+((double) rand() / (RAND_MAX+1))){
      BestP = DownRightP;
      if (isWalkTileWalkable((ScoutCenterPosition + DownRight)))
        NextPosition = ScoutCenterPosition + DownRight;
    }
    if(DownLeftP +(isWalkTileWalkable(ScoutDownLeftPosition)?0:-100)+((double) rand() / (RAND_MAX+1)) > BestP+((double) rand() / (RAND_MAX+1))){
      BestP = DownLeftP;
      if (isWalkTileWalkable((ScoutCenterPosition + DownLeft)))
        NextPosition = ScoutCenterPosition + DownLeft;
    }
    if(UpRightP +(isWalkTileWalkable(ScoutUpRightPosition)?0:-100)+((double) rand() / (RAND_MAX+1)) >BestP+((double) rand() / (RAND_MAX+1))){
      BestP = UpRightP;
      if (isWalkTileWalkable((ScoutCenterPosition + UpRight)))
        NextPosition = ScoutCenterPosition + UpRight;
    }
    if(UpLeftP +(isWalkTileWalkable(ScoutUpLeftPosition)?0:-100)+((double) rand() / (RAND_MAX+1)) > BestP+((double) rand() / (RAND_MAX+1))){
      BestP = UpLeftP;
      if (isWalkTileWalkable((ScoutCenterPosition + UpLeft)))
        NextPosition = ScoutCenterPosition + UpLeft;
    }

    Broodwar->printf("BestP (%f)",BestP);
    if (NextPosition!=ScoutCenterPosition){
      if (u->isStuck()){
        if(this->workerMG->_workersTarget[u])
          u->rightClick(this->workerMG->_workersTarget[u]);
      }
      else
        u->move(NextPosition);
    }		
    else{

      if(this->workerMG->_workersTarget[u])
        u->rightClick(this->workerMG->_workersTarget[u]);
    }
    //Distance=(sqrt((ux-ex)^2+(uy-ey)^2)/32)

    /*np.x() = u->getPosition().x()+30;
    np.y() = u->getPosition().y();
    u->move(np);
    }
    else{
    np.x() = u->getPosition().x();
    np.y() = u->getPosition().y()+30;
    u->move(np);
    }*/

  }
}

double ScoutManager::CalculationPotential(Position m,Position e)
{
  double D;
  double x,y,d;
  double P = 0;

  // distance between m and e
  x = (m.x()-e.x())*(m.x()-e.x());
  y = (m.y()-e.y())*(m.y()-e.y());
  //d = x+y;
  d = m.getDistance(e);
  // from position distance to tileposition distance,so divide 32
  D=(d)/32;


  if (D>30)
    P = -100;
  else
    P = 0;




  //if(D<2){
  //	P = D*3-2;
  //}
  //else if(D>=2&&D<10){
  //	P = 10;
  //}
  //else if(D>=10){
  //	P = 20 - D;
  //}

  return P;
}

double ScoutManager::CalculationPotentialUnit(Position m,Unit* e)
{
  double D;
  double x,y,d;
  double P = 0;
  double AttackRange = e->getType().groundWeapon().maxRange()*1.0/32.0 + 0.5;


  x = (m.x()-e->getPosition().x())*(m.x()-e->getPosition().x());
  y = (m.y()-e->getPosition().y())*(m.y()-e->getPosition().y());
  //d = x+y;
  d = m.getDistance(e->getPosition());

  D=(d)/32;

  Broodwar->drawCircleMap(e->getPosition().x(),e->getPosition().y(),(int)(AttackRange*32),Colors::White);
  Broodwar->drawCircleMap(e->getPosition().x(),e->getPosition().y(),(int)(AttackRange*32 + 2*32),Colors::Red);
  Broodwar->drawTextMap(e->getPosition().x(),e->getPosition().y(),"%lf",D);

  if(D<=AttackRange){
    P = -exp(AttackRange+3-D);
  }
  else if(D<=AttackRange+2){
    //P = D;
    P = 2*log(D-AttackRange);
  } else {
    P = 2*log(D);
  }

  return P;
}

double ScoutManager::CalculationPotentialBuilding(Position m,Unit* e)
{
  //return 0;
  double D;
  double x,y,d;
  double P=0;
  double Size = 2.5;
  Size = sqrt(1.0*e->getType().dimensionUp()*e->getType().dimensionDown() + 1.0*e->getType().dimensionRight()*e->getType().dimensionLeft())/32.0;

  x = (m.x()-e->getPosition().x())*(m.x()-e->getPosition().x());
  y = (m.y()-e->getPosition().y())*(m.y()-e->getPosition().y());
  //d = x+y;
  d = m.getDistance(e->getPosition());

  D=(d)/32;

  Broodwar->drawCircleMap(e->getPosition().x(), e->getPosition().y(), (int)(Size*32+8), Colors::White);

  if(D<=Size+0.25){
    P = D-Size - 1.25;
  }
  return P;
}

double ScoutManager::CalculationPotentialDangerousBuilding(Position m,Unit* e)
{
  double D;
  double x,y,d;
  double P;
  int AttackRange = e->getType().groundWeapon().maxRange()/32+1;

  x = (m.x()-e->getPosition().x())*(m.x()-e->getPosition().x());
  y = (m.y()-e->getPosition().y())*(m.y()-e->getPosition().y());
  //d = x+y;
  d = m.getDistance(e->getPosition());

  D=(d)/32;

  if(D<=AttackRange){
    P = D/AttackRange-5;
  }
  else if(D>AttackRange){
    P = 0;
  }

  return P;
}

void ScoutManager::destroy()
{
  if (theScoutManager) delete theScoutManager;

}


double ScoutManager::CalculationPotentialPerimeter(Position m)
{
  double D;
  double x,y,d;
  double P;
  double num= 0 ;
  BWTA::Region* R=BWTA::getRegion(this->enemyStartLocation->getPosition());
  Position NearestPerimeter;

  NearestPerimeter = R->getPolygon().getNearestPoint(m);

  x = (m.x()-NearestPerimeter.x())*(m.x()-NearestPerimeter.x());
  y = (m.y()-NearestPerimeter.y())*(m.y()-NearestPerimeter.y());
  //d = x+y;
  d = m.getDistance(NearestPerimeter);

  D=(d)/32;

  //Broodwar->drawCircleMap(NearestPerimeter.x(),NearestPerimeter.y(),32,Colors::Orange);
  Broodwar->drawCircleMap(NearestPerimeter.x(),NearestPerimeter.y(),2*32,Colors::Yellow);

  if(D<=1.75){
    P = -exp(4-D)-1;
  }
  else P = 1 + 2*log(D);

  return P;
}

double ScoutManager::PolygonPotentialValue(Position po)
{
  return 0;
  double D;
  double P=0;
  double num = 0;
  BWTA::Region* R=BWTA::getRegion(this->enemyStartLocation->getPosition());
  Position NearestPerimeter = R->getPolygon().getNearestPoint(po); 

  BWTA::Polygon polygonVector = R->getPolygon();

  D = po.getDistance(NearestPerimeter)/32;

  for(Polygon::iterator i = polygonVector.begin(); i != polygonVector.end();i++){
    if (NearestPerimeter.getDistance(*i)<=32){
      Broodwar->drawCircleMap(i->x(),i->y(),32,Colors::Orange);
      //Broodwar->drawCircleMap(i->x(),i->y(),32*3,Colors::Yellow);
      num++;
    }
  }

  if (D<=1){
    P = (D-2)*num;
  } else if(D<=3){
    P = (D-1)*num;
  }
  //else P = 3*num;*/
  return P;

}