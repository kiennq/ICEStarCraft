#pragma once
#include <BWAPI.h>
#include <BWTA.h>

namespace ICEStarCraft
{
	class EnemyUnit
	{
	public:

		EnemyUnit(BWAPI::Unit* unit)
			:_ID(unit->getID())
			,_Unit(unit)
			,_UnitType(unit->getType())
			,_Player(unit->getPlayer())
			,_Position(unit->getPosition())
			,_HitPoints(unit->getHitPoints())
			,_Shields(unit->getShields())
			,_InterceptorCount(unit->getInterceptorCount())
			,_ScarabCount(unit->getScarabCount())
			,_LastUpdatedFrame(BWAPI::Broodwar->getFrameCount())
		{}

		int getID() const;
		BWAPI::Unit* getUnit() const;
		BWAPI::UnitType getType() const;
		BWAPI::Player* getPlayer() const;
		BWAPI::Position getPosition() const;
		int getHitPoints() const;
		int getShields() const;
		int getInterceptorCount() const;
		int	getScarabCount() const;
		int getLastUpdatedFrame() const;

		void update(bool hasGone = false);

	private:

		int _ID;
		BWAPI::Unit* _Unit;
		BWAPI::UnitType _UnitType;
		BWAPI::Player* _Player;
		BWAPI::Position _Position;
		int _HitPoints;
		int _Shields;
		int _InterceptorCount;
		int _ScarabCount;
		int _LastUpdatedFrame;
	};
}