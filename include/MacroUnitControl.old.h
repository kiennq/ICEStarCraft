#pragma once
#include <BWTA.h>
#include <BWAPI.h>
#include <string>
#include "UnitGroup.h"
#include "UnitGroupManager.h"
#include "BuildOrderManager.h"
#include "TerrainManager.h"
#include "CommandRewrite.h"
#include "ScoutManager.h"
#include "InformationManager.h"
#include "MentalState.h"
#include "GameFlow.h"
#include "BaseManager.h"
#include "Base.h"
#include "DropManager.h"

using namespace std;
using namespace BWAPI;
using namespace BWTA;

class ScoutManager;
class MentalClass;
class BaseManager;
class GameFlow;
class TerrainManager;
class DropManager;

class MacroManager
{
public:
	static MacroManager* create();
	static void destroy();

	void setManagers(BuildOrderManager* bom);
	

	void allUnitAttack(Unit*, Position, bool needTank = true);

	void onUnitDestroy(Unit* u);
	void onUnitDiscover(Unit* unit);
	void onFrame();
	
	issueOnce* onceCommand;
	void tankAttackMode(Unit* tk, Position p, int reachRange = 6);
	void allBuildingSetPoint(Position setposition);
	void ScanInvisibleEnemy();
	void findRestEnemy();
	void ScienceVesselController();
	void showScanPositionTable();
	bool setScanTableFlag(bool b);
	void minetest();
	void unitAvoidMine(UnitGroup ug,Position p);
	int unitGroupCenterLimitation;
	UnitGroup vultureGroup;
	TilePosition startTP;
	TilePosition endTP;
	void MiningRouteInitialization();
	std::pair<std::string,Position> atkTar;
	TilePosition lastMineStart;
	TilePosition lastMineEnd;
	std::map<Unit*,Position> TankDefBase;
	void TvTTankProtectBase();

	UnitGroup attackers;
	//_T_
	UnitGroup myBattleUnits;
	UnitGroup enemyBattleUnits;
	void BattleController();
	void NormalBattleController();
	void NormalBattleController(UnitGroup& units);
	void ChokepointBattleController(Position atkTar, BWTA::Chokepoint* theChokePoint);
	void ChokepointBattleController(UnitGroup& units, Position atkTar, BWTA::Chokepoint* theChokePoint);

	void keepFormationAttack(Unit* u);
	void BattleCruiserController();

	Unit* theMarine;
	Unit* toGoIn;

	//new
	
	Position getSetPoint() const;
	Position getSiegePoint() const;

	DropManager* dropManager;
protected:
	MacroManager();

private:

	BaseManager* baseManager;
	int mLastCommandTime;
	Position mLastCommandPosition;
	BuildOrderManager* bom;
	EnemyInfoManager* eInfo;
	MyInfoManager* mInfo;
	MentalClass* mental;
	TerrainManager* terrainManager;
	std::set<Unit*> mDefendTeam;
	int DefendTeamSize;
	int btCount;
	std::set<TilePosition> receivedBTPositions;
	std::set<Unit*> allEnemyUnits;
	UnitGroup allVisibleEnemy;
	Position CCPosition;
	int highGroundTankLimitation;
	Position nextSetPoint;

	int lastScanTime;
	Position lastScanPosition;
	int lastTourMapTime;
	std::map<Position,std::pair<bool,int>> positionTOattack;
	std::set<Position> lastAttackPosition;
	int lastAllUnitAttack;
	ScoutManager* scm;
	bool desEnemyMainBase;
	std::set<Unit*> searchingUnits;
	// for allunitattack() function
	bool scanTableFlag;
	Position* groupCenter;
	GameFlow* gf;
	Position gatherPoint;
	//std::map<Unit*,int> liftedB;

	//new
	
	Position setPoint;
	Position setPoint2;
	Position siegePoint;
	
	void updateSetPoint();
	void updateSiegePoint();

	void LiftedBuildingController();
	void ArmyGuard();
	void ArmyDefend();
	void ArmyAttack();
};

