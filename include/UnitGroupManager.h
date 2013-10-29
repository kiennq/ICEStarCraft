#pragma once
#include <BWAPI.h>
#include <UnitGroup.h>
class UnitGroupManager
{
public:
	static UnitGroupManager* create();
	static void destroy();
	void onUnitDiscover(BWAPI::Unit* unit);
	void onUnitEvade(BWAPI::Unit* unit);
	void onUnitMorph(BWAPI::Unit* unit);
	void onUnitRenegade(BWAPI::Unit* unit);
protected:
	UnitGroupManager();
	~UnitGroupManager();
};
extern UnitGroupManager* TheUnitGroupManager;
UnitGroup AllUnits();
UnitGroup SelectAll();
UnitGroup SelectAll(BWAPI::UnitType type);
UnitGroup SelectAllEnemy();
UnitGroup SelectAllEnemy(BWAPI::UnitType type);
UnitGroup SelectAll(BWAPI::Player* player, BWAPI::UnitType type);
// Best pratice using in 1v1 battle
UnitGroup SelectAll(BWAPI::Player* player);