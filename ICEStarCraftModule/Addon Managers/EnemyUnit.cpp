#include <EnemyUnit.h>

using namespace BWAPI;
using namespace BWTA;
using namespace std;
using namespace ICEStarCraft;

int EnemyUnit::getID() const
{
	return _ID;
}

BWAPI::Unit* EnemyUnit::getUnit() const
{
	return _Unit;
}

BWAPI::UnitType EnemyUnit::getType() const
{
	return _UnitType;
}

BWAPI::Player* EnemyUnit::getPlayer() const
{
	return _Player;
}

BWAPI::Position EnemyUnit::getPosition() const
{
	return _Position;
}

int EnemyUnit::getHitPoints() const
{
	return _HitPoints;
}

int EnemyUnit::getShields() const
{
	return _Shields;
}

int EnemyUnit::getInterceptorCount() const
{
	if (_UnitType != UnitTypes::Protoss_Carrier)
	{
		return 0;
	}
	return _InterceptorCount;
}

int	EnemyUnit::getScarabCount() const
{
	if (_UnitType != UnitTypes::Protoss_Reaver)
	{
		return 0;
	}
	return _ScarabCount;
}

int EnemyUnit::getLastUpdatedFrame() const
{
	return _LastUpdatedFrame;
}

void EnemyUnit::update(bool hasGone /*= false*/)
{
	if (!hasGone && _Unit->exists())
	{
		_ID               = _Unit->getID();
		_UnitType         = _Unit->getType();
		_Player           = _Unit->getPlayer();
		_Position         = _Unit->getPosition();
		_HitPoints        = _Unit->getHitPoints();
		_Shields          = _Unit->getShields();
		_InterceptorCount = _Unit->getInterceptorCount();
		_ScarabCount      = _Unit->getScarabCount();
		_LastUpdatedFrame = Broodwar->getFrameCount();
	}
	else
	{
		_Position         = Positions::Unknown;
		_LastUpdatedFrame = Broodwar->getFrameCount();
	}
}