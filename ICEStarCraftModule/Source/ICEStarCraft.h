#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include <windows.h>
#include <boost/shared_ptr.hpp>
#include "EnhancedUI.h"
#include "UnitGroupManager.h"
#include "UnitGroup.h"
#include "WorkerManager.h"
#include "Common.h"
#include "ProductionManager.h"
#include "Base.h"
#include "BaseManager.h"
#include "SupplyManager.h"
#include "TechManager.h"
#include "UpgradeManager.h"
#include "BuildOrderManager.h"
#include "TerrainManager.h"
#include "Simulator.h"
#include "MacroUnitControl.h"
#include "ScoutManager.h"
#include "InformationManager.h"
#include "GameFlow.h"
#include "MentalState.h"
#include "TerrainGraph.h"
#include "vsProtossRules.h"
#include "DropManager.h"
#include "Helper.h"
#include "PFFunctions.h"
#include "Config.h"

#define _BATTLE_DEBUG
//#define _LOG_TO_FILE
#define _TIME_DEBUG

extern bool analyzed;
extern BWTA::Region* home;
extern BWTA::Region* enemy_base;
DWORD WINAPI AnalyzeThread();

class ICEStarCraftModule : public BWAPI::AIModule
{
public:
  virtual void onStart();
  virtual void onEnd(bool isWinner);
  virtual void onFrame();
  virtual void onSendText(std::string text);
  virtual void onReceiveText(BWAPI::Player* player, std::string text);
  virtual void onPlayerLeft(BWAPI::Player* player);
  virtual void onNukeDetect(BWAPI::Position target);
  virtual void onUnitDiscover(BWAPI::Unit* unit);
  virtual void onUnitEvade(BWAPI::Unit* unit);
  virtual void onUnitShow(BWAPI::Unit* unit);
  virtual void onUnitHide(BWAPI::Unit* unit);
  virtual void onUnitCreate(BWAPI::Unit* unit);
  virtual void onUnitDestroy(BWAPI::Unit* unit);
  virtual void onUnitMorph(BWAPI::Unit* unit);
  virtual void onUnitRenegade(BWAPI::Unit* unit);
  virtual void onSaveGame(std::string gameName);
  virtual void onUnitComplete(BWAPI::Unit *unit);
  void drawStats(); //not part of BWAPI::AIModule
  void drawBullets();
  void drawVisibilityData();
  void drawTerrainData();
  void showPlayers();
  void showForces();
  bool show_bullets;
  bool show_visibility_data;
  bool drawObjects;
	void showDebugInfo();

  //Managers
  Arbitrator::Arbitrator<BWAPI::Unit*,double> arbitrator;
  BuildManager* buildManager;
  BuildOrderManager* buildOrderManager;
  SupplyManager* supplyManager;
  TechManager* techManager;
  UpgradeManager* upgradeManager;
  TerrainManager* terrainManager;
  MacroManager* macroManager;
  MentalClass* mental;
  //BaseManagerClass* baseManager;
  Simulator* simulator;
  ScoutManager* scoutManager;
  BWAPI::Position attackpoint;	
  EnhancedUI* enhancedUI;
  UnitGroup allArmy;
  std::set<BWAPI::Unit*> scanSet;
  std::set<BWAPI::Unit*> munit;
  MyInfoManager* mInfo;
  EnemyInfoManager* eInfo;
  GameFlow* gameFlow;
  int extest;
  TPTiming* tpTiming;

	//Terrain mapping
	TerrainGraph* terrainGraph;

#ifdef _LOG_TO_FILE
	int reTryTimes;
	int reTryTimesMax;
#endif

private:
	bool draw_enemy_targets;
	bool draw_self_targets;
  bool _showAllDebug;

#ifdef _TIME_DEBUG
	std::map<std::string,float> CalculationTime;
#endif
};
