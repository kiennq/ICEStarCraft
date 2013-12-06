#pragma once
#include <math.h>
#include <BWTA.h>
#include <BWAPI.h>
#include "BuildOrderManager.h"
#include "Common.h"
#include "EnemyUnit.h"
#include "Helper.h"
#include "MentalState.h"
#include "PFFunctions.h"
#include "ScoutManager.h"
#include "TerrainManager.h"
#include "UnitGroupManager.h"
#include "Vector2.h"

namespace ICEStarCraft
{
	enum DropPurpose
	{
		HighGround = 1,
		Harrass,
		DropNone
	};

	enum DropperState
	{
		Idle = 1,
		Waiting,
		Loading,
		Moving,
		Dropping,
		Returning
	};
}

class Dropper
{
public:
	Dropper(BWAPI::Unit*);
	~Dropper();
	
	BWAPI::Unit* dropship;
	UnitGroup unitsToLoad;
	ICEStarCraft::DropPurpose purpose;
	ICEStarCraft::DropperState state;
	BWAPI::Position dropPos;
	BWTA::BaseLocation* target;
	std::vector<BWAPI::Position> path;
	int current;

	void update();
	std::string getStateString();

private:

	bool debug;
};

class DropTarget
{
public:

	DropTarget(BWTA::BaseLocation* base, BWAPI::Position dropPos, ICEStarCraft::DropPurpose purpose, std::vector<BWAPI::Position> path)
		:base(base)
		,dropPos(dropPos)
		,purpose(purpose)
		,path(path)
	{
	}

	BWTA::BaseLocation* base;
	BWAPI::Position dropPos;
	ICEStarCraft::DropPurpose purpose;
	std::vector<BWAPI::Position> path;
};

class DropManager : public Arbitrator::Controller<BWAPI::Unit*,double>
{
public:

	static DropManager* create();
	static void destroy();

	void setManagers(BuildOrderManager* buildOrderManager) {this->buildOrderManager = buildOrderManager;}
	void setArbitrator(Arbitrator::Arbitrator<BWAPI::Unit*,double>* arbitrator) {this->arbitrator = arbitrator;}

	std::string getName() const {return "DropManager";}

	void onOffer(std::set<BWAPI::Unit*>);
	void onRevoke(BWAPI::Unit*, double);
	void update();
	void onUnitDestroy(BWAPI::Unit*);

	std::map<BWAPI::Unit*, Dropper*> requestedUnits;
	UnitGroup allUnitsToLoad;
	UnitGroup unitsToControl;
	std::set<Dropper*> droppers;
	int dropperMaxNum;
	std::set<DropTarget*> dropTargets;
	bool isDropTarget(BWTA::BaseLocation*);
	void selectUnitsForDropper(Dropper*);
	void controlDroppedUnits();
	std::vector<BWAPI::Position> getBestFlightPath(BWAPI::Position,BWAPI::Position,std::set<ICEStarCraft::EnemyUnit*>);

	void showDebugInfo();

protected:

	DropManager();
	~DropManager();

private:

	bool debug;

	TerrainManager*    terrainManager;
	BuildOrderManager* buildOrderManager;
	MentalClass*       mental;
	ScoutManager*      scoutManager;
	EnemyInfoManager*  eInfo;
	MyInfoManager*     mInfo;

	Arbitrator::Arbitrator<BWAPI::Unit*,double>* arbitrator;
};
