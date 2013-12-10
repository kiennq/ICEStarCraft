#pragma once
#include <BWAPI.h>
#include <BWTA.h>

namespace ICEStarCraft
{
	class EnemyUnit
	{
	public:

		EnemyUnit(BWAPI::Unit* unit, int fadeTime = -1)
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
      ,_isBeingConstructed(unit->isBeingConstructed())
      ,_isInvincible(unit->isInvincible())
      ,_isLifted(unit->isLifted())
      ,_isBurrowed(unit->isBurrowed())
      ,_target(unit->getTarget() ? unit->getTarget() : unit->getOrderTarget())
      ,_fadeTime(fadeTime)
		{}

    EnemyUnit() {}

		int getID() const;
		BWAPI::Unit* getUnit() const;
		BWAPI::UnitType getType() const;
		BWAPI::Player* getPlayer() const;
		BWAPI::Position getPosition() const;
    BWAPI::Unit* getTarget() const;
		int getHitPoints() const;
		int getShields() const;
		int getInterceptorCount() const;
		int	getScarabCount() const;
		int getLastUpdatedFrame() const;
    bool isBeingConstructed() const;
    bool isInvincible() const;
    bool isLifted() const;
    bool isBurrowed() const;
    int getFadeTime() const;

		void update(bool hasGone = false);

    bool operator==(const EnemyUnit& other) const;
    bool operator<(const EnemyUnit& other) const;
    // This one is crazy
    EnemyUnit* operator->();

	private:

		int _ID;
		BWAPI::Unit* _Unit;
    BWAPI::Unit* _target;
		BWAPI::UnitType _UnitType;
		BWAPI::Player* _Player;
		BWAPI::Position _Position;
		int _HitPoints;
		int _Shields;
		int _InterceptorCount;
		int _ScarabCount;
		int _LastUpdatedFrame;
    bool _isBeingConstructed;
    int _fadeTime;
    bool _isInvincible;
    bool _isLifted;
    bool _isBurrowed;
	};
}