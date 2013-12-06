#pragma once
#include "Common.h"
#include "Helper.h"
#include "InformationManager.h"
#include "PFFunctions.h"
#include "TerrainManager.h"
#include "UnitGroup.h"
#include "UnitGroupManager.h"

class MicroUnitControl
{
public:

	MicroUnitControl();
	~MicroUnitControl();

	static bool isBlocked(BWAPI::Unit*, BWAPI::Position);
	static UnitGroup getBlockingUnits(BWAPI::Unit*, BWAPI::Position);
	static BWAPI::Position getDestination(BWAPI::Unit*, UnitGroup&, UnitGroup&);
	static BWAPI::Position getDestinationNearChokepoint(BWAPI::Unit*, UnitGroup&, BWAPI::Position, BWTA::Chokepoint*);
	
	static bool isFiring(BWAPI::Unit*);

	// consider unit state, unit type, upgrades and number of hits per attack
	static WeaponType getAirWeapon(BWAPI::UnitType);
	static WeaponType getGroundWeapon(BWAPI::UnitType);
	static int getAirCooldown(BWAPI::UnitType);
	static int getGroundCooldown(BWAPI::UnitType);
	static int getAirHits(BWAPI::UnitType);
	static int getGroundHits(BWAPI::UnitType);
	static int getDamage(BWAPI::UnitType, BWAPI::Player* player = NULL);
	static int getAirDamage(BWAPI::UnitType, BWAPI::Player* player = NULL);
	static int getGroundDamage(BWAPI::UnitType, BWAPI::Player* player = NULL);

	static double getDPF(BWAPI::UnitType, BWAPI::Player* player = NULL);
	static double getAirDPF(BWAPI::UnitType, BWAPI::Player* player = NULL);
	static double getGroundDPF(BWAPI::UnitType, BWAPI::Player* player = NULL);

	static WeaponType getAirWeapon(BWAPI::Unit*);
	static WeaponType getGroundWeapon(BWAPI::Unit*);
	static int getAirCooldown(BWAPI::Unit*);
	static int getGroundCooldown(BWAPI::Unit*);
	static int getAirHits(BWAPI::Unit*);
	static int getGroundHits(BWAPI::Unit*);
	static int getDamage(BWAPI::Unit*);
	static int getAirDamage(BWAPI::Unit*);
	static int getGroundDamage(BWAPI::Unit*);

	static double getDamage(BWAPI::Unit*,BWAPI::Unit*);
	static bool canKillTarget(BWAPI::Unit*,BWAPI::Unit*);

	static double getDPF(BWAPI::Unit*);
	static double getAirDPF(BWAPI::Unit*);
	static double getGroundDPF(BWAPI::Unit*);
	static double getDPF(BWAPI::Unit*,BWAPI::Unit*);

	static BWAPI::Unit* getBestAttackTartget(BWAPI::Unit*, UnitGroup&);

	// unit command
	static bool attack(BWAPI::Unit*,BWAPI::Unit*);
	static bool attack(BWAPI::Unit*,BWAPI::Position);
	static bool move(BWAPI::Unit*,BWAPI::Position);

	static void tankAttack(BWAPI::Unit*,BWAPI::Position, int reachRange = 32*6);
	static void battlecruiserAttack(BWAPI::Unit*,BWAPI::Position);
	static void scienceVesselMicro(BWAPI::Unit*);
	
private:
	
	// use tech
	static bool useTech(BWAPI::Unit*,BWAPI::TechType);
	static bool useTech(BWAPI::Unit*,BWAPI::TechType,BWAPI::Unit*);
	static bool useTech(BWAPI::Unit*,BWAPI::TechType,BWAPI::Position);
	static bool useYamatoGun(BWAPI::Unit*, UnitGroup&);
	static bool useIrradiate(BWAPI::Unit*, UnitGroup&);
	static bool useEMPShockwave(BWAPI::Unit*, UnitGroup&);
	static bool useDefensiveMatrix(BWAPI::Unit*, UnitGroup&);
};