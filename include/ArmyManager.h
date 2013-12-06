#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include <string>
#include <Arbitrator.h>
#include "Common.h"
#include "GameFlow.h"
#include "Helper.h"
#include "InformationManager.h"
#include "MentalState.h"
#include "MicroUnitControl.h"
#include "PFFunctions.h"
#include "ScoutManager.h"
#include "TerrainManager.h"
#include "UnitGroupManager.h"

class GameFlow;

namespace ICEStarCraft
{
	enum ArmyState
	{
		ArmyGuard,
		ArmyDefend,
		ArmyAttack
	};
}

class AttackTarget
{
public:

	AttackTarget(BWAPI::Unit* unit, BWAPI::Position position, std::string type)
		:unit(unit)
		,position(position)
		,type(type)
	{

	}

	BWAPI::Unit* getUnit()        const {return unit;}
	BWAPI::Position getPosition() const {return position;}
	std::string getType()         const {return type;}

private:

	BWAPI::Unit* unit;
	BWAPI::Position position;
	std::string type;
};

class ArmyManager : public Arbitrator::Controller<BWAPI::Unit*,double>
{
public:

	static ArmyManager* create();
	static void destroy();

	void setArbitrator(Arbitrator::Arbitrator<BWAPI::Unit*,double>* arbitrator) {this->arbitrator = arbitrator;}

	std::string getName() const {return "ArmyManager";}

	void onOffer(std::set<BWAPI::Unit*>);
	void onRevoke(BWAPI::Unit*, double);
	void update();
	void onUnitDestroy(BWAPI::Unit*);

	UnitGroup               getAttackers()          {return attackers;}
	AttackTarget*           getAttackTarget() const {return attackTarget;}
	ICEStarCraft::ArmyState getArmyState()    const {return state;}
	BWAPI::Position         getSetPoint()     const {return setPoint;}
	BWAPI::Position         getSiegePoint()   const {return siegePoint;}
	BWAPI::Position         getGatherPoint()  const {return gatherPoint;}

	// get enemy units that we can/should attack
	static bool isAttackTarget(BWAPI::Unit*);
	static UnitGroup getAttackTargets();
	static UnitGroup getAttackTargets(BWAPI::Position,int);
	static UnitGroup getAttackTargets(UnitGroup&);
	static UnitGroup getAttackTargets(UnitGroup&,BWAPI::Position,int);

	void showDebugInfo();

protected:

	ArmyManager();
	~ArmyManager();

private:

  GameFlow* gameFlow;	
	MyInfoManager* mInfo;
	EnemyInfoManager* eInfo;
	MentalClass* mental;
	ScoutManager* scoutManager;
	TerrainManager* terrainManager;
	
	Arbitrator::Arbitrator<BWAPI::Unit*,double>* arbitrator;
	AttackTarget* attackTarget;
	ICEStarCraft::ArmyState state;

	UnitGroup attackers;
	UnitGroup vessels;

	BWAPI::Position setPoint;
	BWAPI::Position setPoint2;
	BWAPI::Position siegePoint;
	BWAPI::Position gatherPoint;

	void updateArmyUnits();
	void updateSetPoint();
	void updateSiegePoint();

	void ArmyGuard();
	void ArmyDefend();
	void ArmyAttack();
	
	void allUnitsAttack(BWAPI::Position, bool needTank = true);
	bool allUnitsGather(BWAPI::Position, bool needTank = true);
	void allUnitsAvoidPsionicStorm();
	void allUnitsAvoidNuclearMissile();
	void controlScienceVessels();

	int lastAttackFrame;
	int startGatheringFrame;
	bool shouldGatherBeforeAttack;
	bool hasScoutedBeforeAttack;
	Unit* scoutUnit;
};