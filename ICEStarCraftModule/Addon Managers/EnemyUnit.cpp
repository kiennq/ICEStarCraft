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

BWAPI::Order EnemyUnit::getOrder() const
{
	return _order;
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

bool EnemyUnit::isBeingConstructed() const
{
  return _isBeingConstructed;
}

bool EnemyUnit::isMorphing() const
{
  return _isMorphing;
}

void EnemyUnit::update(bool hasGone /*= false*/)
{
  if (!_Unit) return;
	if (!hasGone && _Unit->exists())
	{
		_ID                 = _Unit->getID();
		_UnitType           = _Unit->getType();
    _order              = _Unit->getOrder();
		_Player             = _Unit->getPlayer();
		_Position           = _Unit->getPosition();
		_HitPoints          = _Unit->getHitPoints();
		_Shields            = _Unit->getShields();
		_InterceptorCount   = _Unit->getInterceptorCount();
		_ScarabCount        = _Unit->getScarabCount();
		_LastUpdatedFrame   = Broodwar->getFrameCount();
    _isBeingConstructed = _Unit->isBeingConstructed();
    _isMorphing         = _Unit->isMorphing();
    _isInvincible       = _Unit->isInvincible();
    _isLifted           = _Unit->isLifted();
    _isBurrowed         = _Unit->isBurrowed();
    _isCompleted        = _Unit->isCompleted();
    _isConstructing     = _Unit->isConstructing();
    _target             = _Unit->getTarget() ? _Unit->getTarget() 
                                             : _Unit->getOrderTarget();
	}
	else
	{
		_Position         = Positions::Unknown;
		_LastUpdatedFrame = Broodwar->getFrameCount();
	}
}

bool EnemyUnit::operator==(const EnemyUnit& other) const
{
  return _Unit == other._Unit;
}

bool EnemyUnit::operator<(const EnemyUnit& other) const
{
  return _Unit < other._Unit;
}

int EnemyUnit::getFadeTime() const
{
  return _fadeTime;
}

bool EnemyUnit::isInvincible() const
{
  return _isInvincible;
}

bool EnemyUnit::isBurrowed() const
{
  return _isBurrowed;
}

bool EnemyUnit::isLifted() const
{
  return _isLifted;
}

BWAPI::Unit* EnemyUnit::getTarget() const
{
  return _target;
}

bool EnemyUnit::isCompleted() const
{
  return _isCompleted;
}

bool EnemyUnit::isConstructing() const
{
  return _isConstructing; 
}

EnemyUnit* EnemyUnit::operator->()
{
  return this;
}
