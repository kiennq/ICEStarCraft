#include "ICEStarCraft.h"
#include <iostream>
#include <fstream>
#include <ctime>

using namespace BWAPI;
using namespace ICEStarCraft;

bool analyzed;
BWTA::Region* home;
BWTA::Region* enemy_base;

void ICEStarCraftModule::onStart()
{
	if (Broodwar->isReplay()) return;

	BWTA::readMap();
	BWTA::analyze();
	analyzed=true;
	
	Broodwar->printf("%s", Broodwar->mapName().c_str());

  if (Config::i().DEBUG_ALL())
  {
    Broodwar->enableFlag(Flag::UserInput);
  }
	Broodwar->setLocalSpeed(0);

	unitGroupManager = UnitGroupManager::create();
	workerManager = WorkerManager::create();
	this->upgradeManager = new UpgradeManager(&this->arbitrator);
	this->techManager = new TechManager(&this->arbitrator);
	this->buildManager = new BuildManager(&this->arbitrator);
	this->supplyManager = SupplyManager::create();
	this->scoutManager = ScoutManager::create();
	this->macroManager = MacroManager::create();

	this->buildOrderManager  = new BuildOrderManager(this->buildManager,this->techManager,this->upgradeManager,workerManager,this->supplyManager);
	//this->buildOrderManager->setDebugMode(false);
	if (Broodwar->enemy()->getRace() != Races::Zerg)
	{
		this->buildManager->setBuildDistance(0);
	}

	this->terrainManager = TerrainManager::create();
	this->gameFlow = GameFlow::create();
	this->mental = MentalClass::create();
	this->supplyManager->setBuildManager(this->buildManager);
	this->supplyManager->setBuildOrderManager(this->buildOrderManager);
	this->techManager->setBuildingPlacer(this->buildManager->getBuildingPlacer());
	this->upgradeManager->setBuildingPlacer(this->buildManager->getBuildingPlacer());

	this->buildOrderManager->enableDependencyResolver();

	this->mInfo = MyInfoManager::create();
	this->eInfo = EnemyInfoManager::create();

	//productionManager = new ProductionManager();
	baseManager = BaseManager::create();
	baseManager->expandPlan();
	this->mInfo->setUsefulManagers(baseManager);
	this->buildOrderManager->setBaseManagerClass(baseManager);
	workerManager->setBaseManagerClass(baseManager);
	workerManager->setBuildOrderManager(this->buildOrderManager);
	baseManager->setManagers(buildOrderManager);
	this->scoutManager->setManagers();
	this->terrainManager->setScoutManager(scoutManager);
	this->mental->setManagers(this->buildOrderManager,upgradeManager,techManager);
	this->tpTiming = TPTiming::create();

	/********autobuild worker********/


	workerManager->setArbitrator(&this->arbitrator);
	workerManager->enableAutoBuild();
	workerManager->setAutoBuildPriority(70);
	
	this->gameFlow->setManagers(this->buildOrderManager,this->upgradeManager);

	BWAPI::Race race = Broodwar->self()->getRace();
	this->simulator = new Simulator();

	this->macroManager->setManagers(this->buildOrderManager);
	this->macroManager->setArbitrator(&this->arbitrator);

	terrainGraph = TerrainGraph::create();

	extest=1;
	this->buildOrderManager->buildAdditional(2,UnitTypes::Terran_SCV,300);
	this->drawObjects = false;
	draw_self_targets = false;
	draw_enemy_targets = false;
  _showAllDebug = Config::i().DEBUG_ALL();

	//_T_
#ifdef _TIME_DEBUG
	CalculationTime.clear();
#endif
}

void ICEStarCraftModule::onEnd(bool isWinner)
{
	MentalClass::destroy();
	WorkerManager::destroy();
	ScoutManager::destroy();
	UnitGroupManager::destroy();
	TerrainManager::destroy();
	MacroManager::destroy();
	MyInfoManager::destroy();
	EnemyInfoManager::destroy();
	GameFlow::destroy();
	BaseManager::destroy();
	TPTiming::destroy();
	if (isWinner)
	{
		//log win to file
	}

#ifdef _LOG_TO_FILE
	ofstream fout("result.csv", ios::app);
	if (fout.is_open())
	{
		fout << Broodwar->self()->getRace().c_str() << "(" << Broodwar->self()->getName() << ")"  << ",";
		fout << Broodwar->enemy()->getRace().c_str() << "(" << Broodwar->enemy()->getName() << ")"  << ",";
		fout << Broodwar->mapFileName().c_str() << ",";
		fout << isWinner << ",";
		fout << Broodwar->getFrameCount() << endl;
		fout.close();
	}
#endif
}

void ICEStarCraftModule::onFrame()
{
	if (Broodwar->isReplay()) return;

	if (Broodwar->getFrameCount() == 24*5)
	{
		Broodwar->sendText("gl hf");
	}

#ifndef _TIME_DEBUG
	if (this->drawObjects)
	{
		enhancedUI->update();
		enhancedUI->drawUnitsAttackRange();
		terrainGraph->draw();
	}

	if (draw_enemy_targets) enhancedUI->drawUnitTargets(Broodwar->enemy());
	if (draw_self_targets) enhancedUI->drawUnitTargets(Broodwar->self());

	this->terrainManager->onFrame();
	this->gameFlow->onFrame();
	this->mental->onFrame();
	this->macroManager->onFrame();
	this->eInfo->showTypeToTimeMap();
	this->eInfo->showUnitToTypeMap();
	this->eInfo->showBuildingToPositionMap();
	this->eInfo->showBaseToDataMap();
	this->eInfo->onFrame();
	this->mInfo->onFrame();
	this->scoutManager->onFrame();
	workerManager->onFrame();
	baseManager->update();
	this->buildManager->update();
	this->buildOrderManager->update();
	this->supplyManager->update();
	this->arbitrator.update();
	this->upgradeManager->update();
	this->techManager->update();
	if (Broodwar->enemy()->getRace() != Races::Zerg)
	{
		if ((SelectAll()(Supply_Depot).size() > 1 && SelectAll()(Bunker).size() > 0) ||	Broodwar->getFrameCount() > 24*60*4)
		{
			this->buildManager->setBuildDistance(1);
		}
	}
	showDebugInfo();

#endif //_TIME_DEBUG


#ifdef _TIME_DEBUG
	clock_t t_start = clock();
	clock_t t = t_start;

	if (this->drawObjects)
	{
		enhancedUI->update();
		enhancedUI->drawUnitsAttackRange();
		terrainGraph->draw();
	}

	if (draw_enemy_targets) enhancedUI->drawUnitTargets(Broodwar->enemy());
	if (draw_self_targets) enhancedUI->drawUnitTargets(Broodwar->self());

	CalculationTime["Others"] = (float)(1000*(clock()-t))/CLOCKS_PER_SEC;

	t = clock();
	this->terrainManager->onFrame();
	CalculationTime["TerrainManager"] = (float)(1000*(clock()-t))/CLOCKS_PER_SEC;

	t = clock();
	this->gameFlow->onFrame();
	CalculationTime["GameFlow"] = (float)(1000*(clock()-t))/CLOCKS_PER_SEC;

	t = clock();
	this->mental->onFrame();
	CalculationTime["MentalState"] = (float)(1000*(clock()-t))/CLOCKS_PER_SEC;

	t = clock();
	this->macroManager->onFrame();
	CalculationTime["MacroManager"] = (float)(1000*(clock()-t))/CLOCKS_PER_SEC;

	t = clock();
	this->eInfo->showTypeToTimeMap();
	this->eInfo->showUnitToTypeMap();
	this->eInfo->showBuildingToPositionMap();
	this->eInfo->showBaseToDataMap();
	this->eInfo->onFrame();
	this->mInfo->onFrame();
	CalculationTime["InformationManager"] = (float)(1000*(clock()-t))/CLOCKS_PER_SEC;

	t = clock();
	this->scoutManager->onFrame();
	CalculationTime["ScoutManager"] = (float)(1000*(clock()-t))/CLOCKS_PER_SEC;

	t = clock();
	workerManager->onFrame();
	CalculationTime["WorkerManager"] = (float)(1000*(clock()-t))/CLOCKS_PER_SEC;

	t = clock();
	baseManager->update();
	CalculationTime["BaseManager"] = (float)(1000*(clock()-t))/CLOCKS_PER_SEC;

	t = clock();
	this->buildManager->update();
	CalculationTime["Others"] += (float)(1000*(clock()-t))/CLOCKS_PER_SEC;
	//CalculationTime["BuildManager"] = (float)(1000*(clock()-t))/CLOCKS_PER_SEC;

	t = clock();
	this->buildOrderManager->update();
	CalculationTime["BuildOrderManager"] = (float)(1000*(clock()-t))/CLOCKS_PER_SEC;

	t = clock();
	this->supplyManager->update();
	CalculationTime["Others"] += (float)(1000*(clock()-t))/CLOCKS_PER_SEC;
	//CalculationTime["SupplyManager"] = (float)(1000*(clock()-t))/CLOCKS_PER_SEC;

	t = clock();
	this->arbitrator.update();
	CalculationTime["Others"] += (float)(1000*(clock()-t))/CLOCKS_PER_SEC;
	//CalculationTime["Arbitrator"] = (float)(1000*(clock()-t))/CLOCKS_PER_SEC;

	t = clock();
	this->upgradeManager->update();
	CalculationTime["Others"] += (float)(1000*(clock()-t))/CLOCKS_PER_SEC;
	//CalculationTime["UpgradeManager"] = (float)(1000*(clock()-t))/CLOCKS_PER_SEC;

	t = clock();
	this->techManager->update();
	CalculationTime["Others"] += (float)(1000*(clock()-t))/CLOCKS_PER_SEC;
	//CalculationTime["TechManager"] = (float)(1000*(clock()-t))/CLOCKS_PER_SEC;
	
	t = clock();
	if (Broodwar->enemy()->getRace() != Races::Zerg)
	{
		if ((SelectAll()(Supply_Depot).size() > 1 && SelectAll()(Bunker).size() > 0) ||	Broodwar->getFrameCount() > 24*60*4)
		{
			this->buildManager->setBuildDistance(1);
		}
	}
	CalculationTime["Others"] += (float)(1000*(clock()-t))/CLOCKS_PER_SEC;

#ifdef _BATTLE_DEBUG
	t = clock();
	showDebugInfo();
	CalculationTime["DrawInfo"] = (float)(1000*(clock()-t))/CLOCKS_PER_SEC;
#endif // _BATTLE_DEBUG

	Broodwar->drawTextScreen(190,335,"\x07 Frame: %d",Broodwar->getFrameCount());
	Broodwar->drawTextScreen(270,335,"\x07 FPS: %d",Broodwar->getFPS());
	Broodwar->drawTextScreen(330,335,"\x07 Last frame: %.0f ms",(float)(1000*(clock()-t_start))/CLOCKS_PER_SEC);

	int x = 200;
	int line = 1;
	float max = 0;

	for (map<string,float>::iterator i = CalculationTime.begin(); i != CalculationTime.end(); i++)
	{
		if (i->second >= max)
		{
			max = i->second;
		}
	}
	for (map<string,float>::iterator i = CalculationTime.begin(); i != CalculationTime.end(); i++)
	{
		if (i->second == max && max > 0)
		{
			Broodwar->drawTextScreen(x,10*(++line),"\x06 %-25s",i->first.c_str());
			Broodwar->drawTextScreen(x+110,10*line,"\x06 %.0f",i->second);
		}
		else
		{
			Broodwar->drawTextScreen(x,10*(++line)," %-25s",i->first.c_str());
			Broodwar->drawTextScreen(x+110,10*line," %.0f",i->second);
		}
	}
	
#endif // _TIME_DEBUG

#ifdef _LOG_TO_FILE
	Broodwar->drawTextScreen(0,50,"%d games left",reTryTimes+1);
#endif // _LOG_TO_FILE
}

void ICEStarCraftModule::onSendText(std::string text)
{
	if (text=="/drawoff"){
		this->drawObjects = false;
	}
	if (text=="/drawon"){
		this->drawObjects = true;
	}

	if (text == "/e"){
		draw_enemy_targets = !draw_enemy_targets;
	}
	if (text == "/o"){
		draw_self_targets = !draw_self_targets;
	}

	if (text=="/a"){
		Broodwar->sendText("show me the money");
		Broodwar->sendText("operation cwal");
		Broodwar->sendText("food for thought");
		//Broodwar->sendText("power overwhelming");
	}
	if (text=="/wudi"){
		Broodwar->sendText("power overwhelming");
	}
	if (text=="/tt"){
		this->eInfo->showTypeToTime=true;
		this->eInfo->showUnitToType=false;
		this->eInfo->showBuildingToPosition=false;
		this->eInfo->showBaseToData=false;
	}
	if (text=="/ut"){
		this->eInfo->showTypeToTime=false;
		this->eInfo->showUnitToType=true;
		this->eInfo->showBuildingToPosition=false;
		this->eInfo->showBaseToData=false;
	}
	if (text=="/bp"){
		this->eInfo->showTypeToTime=false;
		this->eInfo->showUnitToType=false;
		this->eInfo->showBuildingToPosition=true;
		this->eInfo->showBaseToData=false;
	}

	if (text=="/bd"){
		this->eInfo->showTypeToTime=false;
		this->eInfo->showUnitToType=false;
		this->eInfo->showBuildingToPosition=false;
		this->eInfo->showBaseToData=true;
	}
	if (text=="/tableoff"){
		this->eInfo->showTypeToTime=false;
		this->eInfo->showUnitToType=false;
		this->eInfo->showBuildingToPosition=false;
		this->eInfo->showBaseToData=false;
	}
	/*if (text=="/ston"){
		this->macroManager->setScanTableFlag(true);
	}
	if (text=="/stoff"){
		this->macroManager->setScanTableFlag(false);
	}*/

	if (text=="/c")
	{
		for each(BaseClass* bc in baseManager->getBaseSet())
		{
			Broodwar->printf("need:%d, current:%d\n",bc->getNeedWorkerNum(),bc->getCurrentWorkerNum());
		}	  
	}

	if (text == "/simu"){
		simulator->ProtossSimu(Broodwar->getFrameCount(),pEconomic,50);
	}
	
	if (text =="/ex")
	{
		buildOrderManager->autoExpand(40,++extest);
	}
	if (text=="/g3"){
		workerManager->setNeedGasLevel(3);
	}
	if (text=="/g2"){
		workerManager->setNeedGasLevel(2);
	}
	if (text=="/g1"){
		workerManager->setNeedGasLevel(1);
	}
	if (text=="/g0"){
		workerManager->setNeedGasLevel(0);
	}

	if (text=="/autoscv"){
		Broodwar->printf("enable auto build scv");
		workerManager->enableAutoBuild();
	}

	if (text=="/stopscv"){
		Broodwar->printf("disable auto build scv");
		workerManager->disableAutoBuild();

	}

	if (text == "/bom on")
	{
		this->buildOrderManager->setDebugMode(true);
	}
	if (text == "/bom off")
	{
		this->buildOrderManager->setDebugMode(false);
	}

	if (text == "/plan on")
	{
		this->gameFlow->debug = true;
	}
	if (text == "/plan off")
	{
		this->gameFlow->debug = false;
	}

	if (text=="/show bullets")
	{
		show_bullets = !show_bullets;
	} else if (text=="/show players")
	{
		showPlayers();
	} else if (text=="/show forces")
	{
		showForces();
	} else if (text=="/show visibility")
	{
		show_visibility_data=!show_visibility_data;
	} else if (text=="/analyze")
	{
		if (analyzed == false)
		{
			Broodwar->printf("Analyzing map... this may take a minute");
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AnalyzeThread, NULL, 0, NULL);
		}
	} else
	{
		//  Broodwar->printf("You typed '%s'!",text.c_str());
		Broodwar->sendText("%s",text.c_str());
	}
}

void ICEStarCraftModule::onReceiveText(BWAPI::Player* player, std::string text)
{
	Broodwar->printf("%s said '%s'", player->getName().c_str(), text.c_str());
}

void ICEStarCraftModule::onPlayerLeft(BWAPI::Player* player)
{
	Broodwar->sendText("%s left the game.",player->getName().c_str());
}

void ICEStarCraftModule::onNukeDetect(BWAPI::Position target)
{
	if (target!=Positions::Unknown)
		Broodwar->printf("Nuclear Launch Detected at (%d,%d)",target.x(),target.y());
	else
		Broodwar->printf("Nuclear Launch Detected");
}

void ICEStarCraftModule::onUnitDiscover(BWAPI::Unit* unit)
{
	if (Broodwar->isReplay())return;

	unitGroupManager->onUnitDiscover(unit);
	workerManager->onUnitDiscover(unit);

	this->mInfo->onUnitDiscover(unit);
	this->eInfo->onUnitDiscover(unit);
	this->macroManager->onUnitDiscover(unit);
	//this->scoutManager->OnUnitDiscover(unit);
}

void ICEStarCraftModule::onUnitEvade(BWAPI::Unit* unit)
{
	unitGroupManager->onUnitEvade(unit);
	this->mental->onUnitEvade(unit);
	if (Broodwar->self()->isEnemy(unit->getPlayer()) &&
		  unit->getRegion()->isHigherGround() &&
		  (unit->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode || unit->getType()==UnitTypes::Protoss_Reaver || unit->getType()==UnitTypes::Protoss_High_Templar))
	{
		UnitGroup allMyScanner = SelectAll()(isCompleted)(Comsat_Station)(Energy,">=",50);
		if (!allMyScanner.empty())
		{
			BWAPI::Unit* scanner = (*allMyScanner.begin());
			scanner->useTech(TechTypes::Scanner_Sweep,unit->getPosition());
		}
	}
	//_T_
	this->eInfo->onUnitEvade(unit);
}

void ICEStarCraftModule::onUnitShow(BWAPI::Unit* unit)
{
	this->eInfo->onUnitDiscover(unit);
}

void ICEStarCraftModule::onUnitHide(BWAPI::Unit* unit)
{
	workerManager->onUnitHide(unit);
}

void ICEStarCraftModule::onUnitCreate(BWAPI::Unit* unit)
{

}

void ICEStarCraftModule::onUnitDestroy(BWAPI::Unit* unit)
{
	baseManager->onUnitDestroy(unit);
	workerManager->onUnitDestroy(unit);
	this->mental->onUnitDestroy(unit);
	this->macroManager->onUnitDestroy(unit);
	this->buildOrderManager->onUnitDestroy(unit);
	this->eInfo->onUnitDestroy(unit);
	this->mInfo->onUnitDestroy(unit);
	this->scoutManager->onUnitDestroy(unit);
	this->arbitrator.onRemoveObject(unit);
	this->buildManager->onRemoveUnit(unit);
	this->upgradeManager->onRemoveUnit(unit);
	this->techManager->onRemoveUnit(unit);
}

void ICEStarCraftModule::onUnitMorph(BWAPI::Unit* unit)
{
	if (!Broodwar->isReplay())
	{
		this->mental->onUnitMorph(unit);
		this->eInfo->onUnitMorph(unit);
		workerManager->onUnitMorph(unit);
	}
}

void ICEStarCraftModule::onUnitRenegade(BWAPI::Unit* unit)
{
	if (Broodwar->isReplay()) return;
	unitGroupManager->onUnitRenegade(unit);
}

void ICEStarCraftModule::onSaveGame(std::string gameName)
{
	Broodwar->printf("The game was saved to \"%s\".", gameName.c_str());
}

DWORD WINAPI AnalyzeThread()
{
	BWTA::analyze();

	//self start location only available if the map has base locations
	if (BWTA::getStartLocation(BWAPI::Broodwar->self())!=NULL)
	{
		home       = BWTA::getStartLocation(BWAPI::Broodwar->self())->getRegion();
	}
	//enemy start location only available if Complete Map Information is enabled.
	if (BWTA::getStartLocation(BWAPI::Broodwar->enemy())!=NULL)
	{
		enemy_base = BWTA::getStartLocation(BWAPI::Broodwar->enemy())->getRegion();
	}
	analyzed   = true;
	return 0;
}

void ICEStarCraftModule::drawStats()
{
	std::set<Unit*> myUnits = Broodwar->self()->getUnits();
	Broodwar->drawTextScreen(5,0,"I have %d units:",myUnits.size());
	std::map<UnitType, int> unitTypeCounts;
	for(std::set<Unit*>::iterator i=myUnits.begin();i!=myUnits.end();i++)
	{
		if (unitTypeCounts.find((*i)->getType())==unitTypeCounts.end())
		{
			unitTypeCounts.insert(std::make_pair((*i)->getType(),0));
		}
		unitTypeCounts.find((*i)->getType())->second++;
	}
	int line=1;
	for(std::map<UnitType,int>::iterator i=unitTypeCounts.begin();i!=unitTypeCounts.end();i++)
	{
		Broodwar->drawTextScreen(5,16*line,"- %d %ss",(*i).second, (*i).first.getName().c_str());
		line++;
	}
}

void ICEStarCraftModule::drawBullets()
{
	std::set<Bullet*> bullets = Broodwar->getBullets();
	for(std::set<Bullet*>::iterator i=bullets.begin();i!=bullets.end();i++)
	{
		Position p=(*i)->getPosition();
		double velocityX = (*i)->getVelocityX();
		double velocityY = (*i)->getVelocityY();
		if ((*i)->getPlayer()==Broodwar->self())
		{
			Broodwar->drawLineMap(p.x(),p.y(),p.x()+(int)velocityX,p.y()+(int)velocityY,Colors::Green);
			Broodwar->drawTextMap(p.x(),p.y(),"\x07%s",(*i)->getType().getName().c_str());
		}
		else
		{
			Broodwar->drawLineMap(p.x(),p.y(),p.x()+(int)velocityX,p.y()+(int)velocityY,Colors::Red);
			Broodwar->drawTextMap(p.x(),p.y(),"\x06%s",(*i)->getType().getName().c_str());
		}
	}
}

void ICEStarCraftModule::drawVisibilityData()
{
	for(int x=0;x<Broodwar->mapWidth();x++)
	{
		for(int y=0;y<Broodwar->mapHeight();y++)
		{
			if (Broodwar->isExplored(x,y))
			{
				if (Broodwar->isVisible(x,y))
					Broodwar->drawDotMap(x*32+16,y*32+16,Colors::Green);
				else
					Broodwar->drawDotMap(x*32+16,y*32+16,Colors::Blue);
			}
			else
				Broodwar->drawDotMap(x*32+16,y*32+16,Colors::Red);
		}
	}
}

void ICEStarCraftModule::drawTerrainData()
{
	//we will iterate through all the base locations, and draw their outlines.
	for(std::set<BWTA::BaseLocation*>::const_iterator i=BWTA::getBaseLocations().begin();i!=BWTA::getBaseLocations().end();i++)
	{
		TilePosition p=(*i)->getTilePosition();
		Position c=(*i)->getPosition();

		//draw outline of center location
		Broodwar->drawBox(CoordinateType::Map,p.x()*32,p.y()*32,p.x()*32+4*32,p.y()*32+3*32,Colors::Blue,false);

		//draw a circle at each mineral patch
		for(std::set<BWAPI::Unit*>::const_iterator j=(*i)->getStaticMinerals().begin();j!=(*i)->getStaticMinerals().end();j++)
		{
			Position q=(*j)->getInitialPosition();
			Broodwar->drawCircle(CoordinateType::Map,q.x(),q.y(),30,Colors::Cyan,false);
		}

		//draw the outlines of vespene geysers
		for(std::set<BWAPI::Unit*>::const_iterator j=(*i)->getGeysers().begin();j!=(*i)->getGeysers().end();j++)
		{
			TilePosition q=(*j)->getInitialTilePosition();
			Broodwar->drawBox(CoordinateType::Map,q.x()*32,q.y()*32,q.x()*32+4*32,q.y()*32+2*32,Colors::Orange,false);
		}

		//if this is an island expansion, draw a yellow circle around the base location
		if ((*i)->isIsland())
			Broodwar->drawCircle(CoordinateType::Map,c.x(),c.y(),80,Colors::Yellow,false);
	}

	//we will iterate through all the regions and draw the polygon outline of it in green.
	for(std::set<BWTA::Region*>::const_iterator r=BWTA::getRegions().begin();r!=BWTA::getRegions().end();r++)
	{
		BWTA::Polygon p=(*r)->getPolygon();
		for(int j=0;j<(int)p.size();j++)
		{
			Position point1=p[j];
			Position point2=p[(j+1) % p.size()];
			Broodwar->drawLine(CoordinateType::Map,point1.x(),point1.y(),point2.x(),point2.y(),Colors::Green);
		}
	}

	//we will visualize the chokepoints with red lines
	for(std::set<BWTA::Region*>::const_iterator r=BWTA::getRegions().begin();r!=BWTA::getRegions().end();r++)
	{
		for(std::set<BWTA::Chokepoint*>::const_iterator c=(*r)->getChokepoints().begin();c!=(*r)->getChokepoints().end();c++)
		{
			Position point1=(*c)->getSides().first;
			Position point2=(*c)->getSides().second;
			Broodwar->drawLine(CoordinateType::Map,point1.x(),point1.y(),point2.x(),point2.y(),Colors::Red);
		}
	}
}

void ICEStarCraftModule::showPlayers()
{
	
}

void ICEStarCraftModule::showForces()
{
	
}

void ICEStarCraftModule::onUnitComplete(BWAPI::Unit *unit)
{
	
}

void ICEStarCraftModule::showDebugInfo()
{
	Broodwar->drawTextScreen(5,15,"Time: %02d:%02d",(Broodwar->getFrameCount()/24)/60,(Broodwar->getFrameCount()/24)%60);
	Broodwar->drawTextScreen(5,25,"%s | %s",ArmyManager::create()->getArmyStateString().c_str(),ArmyManager::create()->getAttackTarget()->getType().c_str());
	Broodwar->drawTextScreen(5,35,"EnemyOpening: %s",MentalClass::create()->getSTflag().c_str());
	Broodwar->drawTextScreen(5,45,"EnemyInSight: %d",MentalClass::create()->enemyInSight.size());

	if (!_showAllDebug) return;
	ArmyManager::create()->showDebugInfo();
	BattleManager::create()->showDebugInfo();
	DropManager::create()->showDebugInfo();
	EnemyInfoManager::create()->showDebugInfo();
	GameFlow::create()->showDebugInfo();
	MentalClass::create()->showDebugInfo();
	MineManager::create()->showDebugInfo();
	MyInfoManager::create()->showDebugInfo();
	TerrainManager::create()->showDebugInfo();

	for each (Unit* u in Broodwar->self()->getUnits())
	{
		if (u->isSelected())
		{
			Broodwar->drawTextMap(u->getPosition().x(),u->getPosition().y(),"%s",u->getOrder().getName().c_str());
      if (arbitrator.hasBid(u))
      {
        Broodwar->drawTextMap(u->getPosition().x(),u->getPosition().y()-10,"%s %.2f", arbitrator.getHighestBidder(u).first->getName().c_str(), arbitrator.getHighestBidder(u).second);
      }
			
			for each (Unit* e in Broodwar->enemy()->getUnits())
			{
				Broodwar->drawTextMap(e->getPosition().x(),e->getPosition().y(),"%.2f %.2f",MicroUnitControl::getDPF(u,e),MicroUnitControl::getDamage(u,e));
			}
		}
	}
}