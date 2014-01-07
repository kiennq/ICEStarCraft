#include "MicroUnitControl.h"

#define MELEE_RANGE 64

using namespace BWAPI;
using namespace BWTA;
using namespace ICEStarCraft;
using namespace std;

MicroUnitControl::MicroUnitControl()
{
	
}

MicroUnitControl::~MicroUnitControl()
{

}

bool MicroUnitControl::isBlocked(Unit* u, Position p)
{
	if (!p.isValid() || !Broodwar->isWalkable(p.x() >> 3, p.y() >> 3))
	{
		return true;
	}

	Vector2 v = Vector2((double)(p.x() - u->getPosition().x()),(double)(p.y() - u->getPosition().y()));
	Position tmp;
	UnitType ut = u->getType();
	set<Unit*> units;
	set<Unit*>::iterator _u;

	for (int i = 1; i <= 4; i++)
	{
		tmp = v * 0.25 * i + u->getPosition();
		units = Broodwar->getUnitsInRectangle(tmp.x()-ut.dimensionLeft(),tmp.y()-ut.dimensionUp(),tmp.x()+ut.dimensionRight(),tmp.y()+ut.dimensionDown());
		
		_u = units.find(u);
		if (_u != units.end())
			units.erase(_u);

		if (!units.empty())
			return true;
	}

	return false;
}

UnitGroup MicroUnitControl::getBlockingUnits(Unit *u, Position p)
{
	Vector2 v = Vector2((double)(p.x() - u->getPosition().x()),(double)(p.y() - u->getPosition().y()));
	Position tmp;
	UnitType ut = u->getType();
	UnitGroup units;

	for (int i = 1; i <= 4; i++)
	{
		tmp = v * 0.25 * i + u->getPosition();
		for each (Unit* t in Broodwar->getUnitsInRectangle(tmp.x()-ut.dimensionLeft(),tmp.y()-ut.dimensionUp(),tmp.x()+ut.dimensionRight(),tmp.y()+ut.dimensionDown()))
			units.insert(t);
	}	
	
	set<Unit*>::iterator _u = units.find(u);
	if (_u != units.end())
		units.erase(_u);

	return units;
}

Position MicroUnitControl::getDestination(Unit*u, UnitGroup& myUnits, UnitGroup& enemyUnits)
{
	Position destination;
	Vector2 v, v1, v2;

	v1 = enemyUnits.size() > 1 ? Vector2(myUnits.getCenter()) : Vector2(u->getPosition());
	v2 = Vector2(enemyUnits.getCenter());

	//uniform flow
	v = v2 - v1;
	if (v.approxLen() > 32*8)
	{
		v = v * (32*8.0 / v.approxLen());
	}
	
	for each (Unit* e in enemyUnits)
	{
		if (e->getPosition().getDistance(u->getPosition()) <= u->getType().sightRange())
			v += PFFunctions::getVelocitySource(e->getPosition(), u->getPosition()) * 1000;
	}

	v = v * (96.0 / v.approxLen());
	destination = v + u->getPosition();

	if (!isBlocked(u, destination))
	{
		return destination;
	}

	Position p = u->getPosition();
	UnitType ut = u->getType();
	//Broodwar->drawBoxMap(p.x()-ut.dimensionLeft(),p.y()-ut.dimensionUp(),p.x()+ut.dimensionRight(),p.y()+ut.dimensionDown(),Colors::Red);

	Vector2 _v = Vector2();
	int left  = 0;
	int right = 0;
	int r = 64;

	set<Unit*> surroundUnits;;
	for each (Unit* i in u->getUnitsInRadius(r))
	{
		if (i->getPosition().getDistance(enemyUnits.getCenter()) - 32 < u->getPosition().getDistance(enemyUnits.getCenter()))
		{
			if (i->getPlayer() == Broodwar->self() && (i->isAttacking() || i->getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode || i->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode))
			{		
				surroundUnits.insert(i);
			}
			if (i->getType().isInvincible() || (i->getType().isBuilding() && !i->isLifted()) || i->getType().isAddon())
			{
				surroundUnits.insert(i);
			}
		}
	}

	if (surroundUnits.empty())
	{
		return destination;
	}

	for each (Unit* i in surroundUnits)
	{
		Vector2 v1 = v;
		Vector2 v2 = Vector2(i->getPosition().x() - u->getPosition().x(), i->getPosition().y() - u->getPosition().y());

		if (v1.x() * v2.y() - v2.x() * v1.y() > 0)
			left++;
		else
			right++;

		//Broodwar->drawLineMap(u->getPosition().x(),u->getPosition().y(),i->getPosition().x(),i->getPosition().y(),Colors::Orange);
		_v += PFFunctions::getVelocityVortex(i->getPosition(),u->getPosition()) * 1000;
	}

	_v = _v * (96.0 / _v.approxLen());
	Position p1 = -_v + u->getPosition();
	Position p2 = _v + u->getPosition();
	
	if(ICEStarCraft::Helper::isWalkable(p,p1) && ICEStarCraft::Helper::isWalkable(p,p2))
	{
		return left > right ? p1 : p2;
	}

	if(ICEStarCraft::Helper::isWalkable(p,p1) && !ICEStarCraft::Helper::isWalkable(p,p2))
	{
		return p1;
	}

	if(!ICEStarCraft::Helper::isWalkable(p,p1) && ICEStarCraft::Helper::isWalkable(p,p2))
	{
		return p2;
	}

	//Broodwar->drawTextMap(u->getPosition().x(),u->getPosition().y(),"nowhere to go");
	return Positions::None;
}

Position MicroUnitControl::getDestinationNearChokepoint(Unit*u, UnitGroup& myUnits, Position atkTar, BWTA::Chokepoint* theChokePoint)
{
	if (!u || !u->exists() || myUnits.empty() || atkTar == Positions::None || !theChokePoint)
	{
		return Positions::None;
	}
	
	Position p = u->getPosition();
	BWTA::Region* reg = BWTA::getRegion(p);
	BWTA::Region* targetReg = BWTA::getRegion(atkTar);

	if (!reg || !targetReg)
	{
		return Positions::None;
	}

	if (u->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode && p.getDistance(theChokePoint->getCenter()) < 32*3)
	{
		u->unsiege();
		return Positions::None;
	}
	
	if (u->isAttacking() || u->isAttackFrame() || u->getLastCommand().getType() == UnitCommandTypes::Attack_Unit || u->getOrder() == Orders::AttackUnit)
	{
		if (p.getDistance(theChokePoint->getCenter()) > 32*3)
		{
			return Positions::None;
		}
	}
	
	if (p.getDistance(theChokePoint->getCenter()) > 32*6)
	{
		return Positions::None;
	}

	if (reg != targetReg && !ICEStarCraft::Helper::isDirectlyConnected(reg,targetReg))
	{
		return Positions::None;
	}

	UnitGroup insideUnits = myUnits.inRegion(targetReg);
	double radius = 32*4*(1.5-0.8*insideUnits.size()/myUnits.size());
	//Broodwar->drawCircleMap(theChokePoint->getCenter().x(),theChokePoint->getCenter().y(),(int)radius,Colors::Red);
	if (reg == targetReg && p.getDistance(theChokePoint->getCenter()) > radius)
	{
		return Positions::None;
	}

	Vector2 v1 = theChokePoint->getSides().first - theChokePoint->getSides().second;
	Vector2 v2 = Vector2(v1.y(),-v1.x());
	Position regcen = v2 + theChokePoint->getCenter();
	if (BWTA::getRegion(regcen) != targetReg)
	{
		regcen = -v2 + theChokePoint->getCenter();
	}
	//Broodwar->drawCircleMap(regcen.x(),regcen.y(),5,Colors::Black,true);


	Vector2 b = Vector2();
	Vector2 b_tmp = Vector2();
	set<Position> vertices = TerrainManager::create()->regionVertices[reg];
	
	for (set<Position>::iterator i = vertices.begin(); i != vertices.end(); i++)
	{
		Broodwar->drawCircleMap((*i).x(),(*i).y(),2,Colors::Yellow);
		if (!ICEStarCraft::Helper::isChokePointCenter(*i) && p.getDistance(*i) <= 64)
		{
			//Broodwar->drawLineMap((*i).x(),(*i).y(),p.x(),p.y(),Colors::Yellow);
			b_tmp = PFFunctions::getVelocityVortex(*i,p)*1000;

			v1 = (*i) - regcen;
			v2 = theChokePoint->getCenter() - regcen;
			if(v1.x()*v2.y() - v1.y()*v2.x() < 0)
				b += b_tmp;
			else
				b -= b_tmp;
		}
	}
	if (b == Vector2(0,0))
	{
		return Positions::None;
	}
	b = b * (96.0 / b.approxLen());
	return (b + p).makeValid();
}

bool MicroUnitControl::isFiring(Unit* u)
{
	if (u == NULL || !u->exists())
	{
		return false;
	}

	for each (Bullet* b in Broodwar->getBullets())
	{
		if (b->exists() && b->getSource() == u)
		{
			return true;
		}
	}

	return false;
}

/**********************************************************************************/

WeaponType MicroUnitControl::getAirWeapon(UnitType type)
{
	if (type == UnitTypes::Protoss_Carrier)
	{
		return UnitTypes::Protoss_Interceptor.airWeapon();
	}

	if (type == UnitTypes::Terran_Bunker)
	{
		return UnitTypes::Terran_Marine.airWeapon();
	}

	return type.airWeapon();
}

WeaponType MicroUnitControl::getGroundWeapon(UnitType type)
{
	if (type == UnitTypes::Protoss_Carrier)
	{
		return UnitTypes::Protoss_Interceptor.groundWeapon();
	}

	if (type == UnitTypes::Protoss_Reaver)
	{
		return UnitTypes::Protoss_Scarab.groundWeapon();
	}

	if (type == UnitTypes::Terran_Bunker)
	{
		return UnitTypes::Terran_Marine.groundWeapon();
	}

	return type.groundWeapon();
}

int MicroUnitControl::getAirCooldown(UnitType type)
{
	if (type == UnitTypes::Protoss_Carrier)
	{
		return 30;
	}

	return getAirWeapon(type).damageCooldown();
}

int MicroUnitControl::getGroundCooldown(UnitType type)
{
	if (type == UnitTypes::Protoss_Reaver)
	{
		// reavers launch scarabs every 60 frames
		return 60;
	}

	return getGroundWeapon(type).damageCooldown();
}

int MicroUnitControl::getAirHits(UnitType type)
{
	if (type == UnitTypes::Protoss_Carrier)
	{
		return UnitTypes::Protoss_Interceptor.maxAirHits();
	}

	if (type == UnitTypes::Terran_Bunker)
	{
		return UnitTypes::Terran_Marine.maxAirHits();
	}

	if (type == UnitTypes::Terran_Goliath)
	{
		// BWAPI returns 1
		return 2;
	}

	return type.maxAirHits();
}

int MicroUnitControl::getGroundHits(UnitType type)
{
	if (type == UnitTypes::Protoss_Carrier)
	{
		return UnitTypes::Protoss_Interceptor.maxGroundHits();
	}

	if (type == UnitTypes::Protoss_Reaver)
	{
		return UnitTypes::Protoss_Scarab.maxGroundHits();
	}

	if (type == UnitTypes::Terran_Bunker)
	{
		return UnitTypes::Terran_Marine.maxGroundHits();
	}

	return type.maxGroundHits();
}

int MicroUnitControl::getDamage(UnitType type, Player* player)
{
	return max(getAirDamage(type,player),getGroundDamage(type,player));
}

int MicroUnitControl::getAirDamage(UnitType type, Player* player)
{
	WeaponType weapon = getAirWeapon(type);

	if (weapon == WeaponTypes::None)
	{
		return 0;
	}
	
	int upgradeLevel = player == NULL ? 0 : player->getUpgradeLevel(weapon.upgradeType());

	int damage = getAirHits(type) * (weapon.damageAmount() + upgradeLevel * weapon.damageFactor() * weapon.damageBonus());

	if (type == UnitTypes::Terran_Goliath)
	{
		// damage bonus = 1 but BWAPI returns 2
		damage = getAirHits(type) * (weapon.damageAmount() + upgradeLevel * weapon.damageFactor());
	}

	if (type == UnitTypes::Protoss_Carrier)
	{
		damage *= 8;
	}

	if (type == UnitTypes::Terran_Bunker)
	{
		damage *= 4;
	}

	return damage;
}

int MicroUnitControl::getGroundDamage(UnitType type, Player* player)
{
	WeaponType weapon = getGroundWeapon(type);

	if (weapon == WeaponTypes::None)
	{
		return 0;
	}
	
	int upgradeLevel = player == NULL ? 0 : player->getUpgradeLevel(weapon.upgradeType());

	int damage = getGroundHits(type) * (weapon.damageAmount() + upgradeLevel * weapon.damageFactor() * weapon.damageBonus());

	if (type == UnitTypes::Protoss_Carrier)
	{
		damage *= 8;
	}

	if (type == UnitTypes::Terran_Bunker)
	{
		damage *= 4;
	}

	return damage;
}

double MicroUnitControl::getDPF(UnitType type, Player* player)
{
	return max(getAirDPF(type,player),getGroundDPF(type,player));
}

double MicroUnitControl::getAirDPF(UnitType type, Player* player)
{
	if (getAirCooldown(type) == 0)
	{
		return 0;
	}

	return 1.0 * getAirDamage(type,player) / getAirCooldown(type);
}

double MicroUnitControl::getGroundDPF(UnitType type, Player* player)
{
	if (getGroundCooldown(type) == 0)
	{
		return 0;
	}

	return 1.0 * getGroundDamage(type,player) / getGroundCooldown(type);
}

/**********************************************************************************/

WeaponType MicroUnitControl::getAirWeapon(Unit* unit)
{
	if (!unit || !unit->exists())
	{
		return WeaponTypes::None;
	}
	
	return getAirWeapon(unit->getType());
}

WeaponType MicroUnitControl::getGroundWeapon(Unit* unit)
{
	if (!unit || !unit->exists())
	{
		return WeaponTypes::None;
	}

	return getGroundWeapon(unit->getType());
}

int MicroUnitControl::getAirCooldown(Unit* unit)
{
	if (unit->getType() == UnitTypes::Protoss_Carrier)
	{
		return 30;
	}

	return getAirWeapon(unit).damageCooldown();
}

int MicroUnitControl::getGroundCooldown(Unit* unit)
{
	if (unit->getType() == UnitTypes::Protoss_Reaver)
	{
		// reavers launch scarabs every 60 frames
		return 60;
	}

	return getGroundWeapon(unit).damageCooldown();
}

int MicroUnitControl::getAirHits(Unit* unit)
{
	if (!unit || !unit->exists())
	{
		return 0;
	}

	return getAirHits(unit->getType());
}

int MicroUnitControl::getGroundHits(Unit* unit)
{
	if (!unit || !unit->exists())
	{
		return 0;
	}

	return getGroundHits(unit->getType());
}

int MicroUnitControl::getDamage(Unit* unit)
{
	return max(getAirDamage(unit),getGroundDamage(unit));
}

int MicroUnitControl::getAirDamage(Unit* unit)
{
	if (!unit || !unit->exists())
	{
		return 0;
	}

	if (unit->isUnderDisruptionWeb() || unit->isStasised() || unit->isLockedDown() || unit->isMaelstrommed())
	{
		return 0;
	}

	WeaponType weapon = getAirWeapon(unit);

	if (weapon == WeaponTypes::None)
	{
		return 0;
	}
	
	int damage = getAirHits(unit) * (weapon.damageAmount() + unit->getPlayer()->getUpgradeLevel(weapon.upgradeType()) * weapon.damageFactor() * weapon.damageBonus());
	
	if (unit->getType() == UnitTypes::Terran_Goliath)
	{
		// damage bonus = 1 but BWAPI returns 2
		damage = getAirHits(unit) * (weapon.damageAmount() + unit->getPlayer()->getUpgradeLevel(weapon.upgradeType()) * weapon.damageFactor());
	}

	if (unit->getType() == UnitTypes::Protoss_Carrier)
	{
		damage *= unit->getInterceptorCount();
	}

	if (unit->getType() == UnitTypes::Terran_Bunker)
	{
		if (unit->getPlayer() == Broodwar->self())
		{
			damage *= unit->getLoadedUnits().size();
		}
		else
		{
			damage *= 4;
		}
	}

	return damage;
}

int MicroUnitControl::getGroundDamage(Unit* unit)
{
	if (!unit || !unit->exists())
	{
		return 0;
	}

	if (unit->isUnderDisruptionWeb() || unit->isStasised() || unit->isLockedDown()  || unit->isMaelstrommed())
	{
		return 0;
	}

	WeaponType weapon = getGroundWeapon(unit);

	if (weapon == WeaponTypes::None)
	{
		return 0;
	}

	int damage = getGroundHits(unit) * (weapon.damageAmount() + unit->getPlayer()->getUpgradeLevel(weapon.upgradeType()) * weapon.damageFactor() * weapon.damageBonus());

	if (unit->getType() == UnitTypes::Protoss_Carrier)
	{
		damage *= unit->getInterceptorCount();
	}

	if (unit->getType() == UnitTypes::Terran_Bunker)
	{
		if (unit->getPlayer() == Broodwar->self())
		{
			damage *= unit->getLoadedUnits().size();
		}
		else
		{
			damage *= 4;
		}
	}

	return damage;
}

double MicroUnitControl::getDamage(Unit* unit, Unit* target)
{
	//http://wiki.teamliquid.net/starcraft/Damage
	//TODO: consider Protoss Shields regeneration for Protoss units and HP regeneration for Zerg units

	if (!unit || !unit->exists() || !target || !target->exists() || target->isStasised() || !target->isDetected())
	{
		return 0;
	}

	if (getDamage(unit) == 0)
	{
		return 0;
	}

	int amor = target->getType().armor() + target->getPlayer()->getUpgradeLevel(target->getType().armorUpgrade());
	int shields = target->getShields();
	int shieldUpgrade = target->getPlayer()->getUpgradeLevel(UpgradeTypes::Protoss_Plasma_Shields);

	// target is air unit
	if (target->getType().isFlyer())
	{
		double damage = (double)getAirDamage(unit);

		if (damage == 0)
		{
			return 0;
		}

		WeaponType weapon = getAirWeapon(unit);

		if (weapon.damageType() != DamageTypes::Ignore_Armor)
		{
			amor *= getAirHits(unit);
			shieldUpgrade *= getAirHits(unit);

			if (unit->getType() == UnitTypes::Protoss_Carrier)
			{
				amor *= unit->getInterceptorCount();
				shieldUpgrade *= unit->getInterceptorCount();
			}

			if (unit->getType() == UnitTypes::Terran_Bunker)
			{
				if (unit->getPlayer() == Broodwar->self())
				{
					amor *= unit->getLoadedUnits().size();
					shieldUpgrade *= unit->getLoadedUnits().size();
				}
				else
				{
					amor *= 4;
					shieldUpgrade *= 4;
				}
			}

			if (shields < 1)
			{
				damage -= amor;
			}
			else if (shields >= damage - shieldUpgrade)
			{
				damage -= shieldUpgrade;
			}
			else
			{
				// accurate ?
				damage -= (shieldUpgrade + amor);
			}
			
			if (damage <= 0)
			{
				return 0;
			}
		}

		if (weapon.damageType() == DamageTypes::Concussive)
		{
			if (target->getType().size() == UnitSizeTypes::Large)
			{
				return damage * 0.25;
			}
			if (target->getType().size() == UnitSizeTypes::Medium)
			{
				return damage * 0.5;
			}
		}

		if (weapon.damageType() == DamageTypes::Explosive)
		{
			if (target->getType().size() == UnitSizeTypes::Small)
			{
				return damage * 0.5;
			}
			if (target->getType().size() == UnitSizeTypes::Medium)
			{
				return damage * 0.75;
			}
		}

		return damage;
	}

	// target is ground unit
	if (!target->getType().isFlyer())
	{
		double damage = (double)getGroundDamage(unit);

		if (damage == 0)
		{
			return 0;
		}

		WeaponType weapon = getGroundWeapon(unit);

		if (weapon.damageType() != DamageTypes::Ignore_Armor)
		{
			amor *= getGroundHits(unit);
			shieldUpgrade *= getAirHits(unit);

			if (unit->getType() == UnitTypes::Protoss_Carrier)
			{
				amor *= unit->getInterceptorCount();
				shieldUpgrade *= unit->getInterceptorCount();
			}

			if (unit->getType() == UnitTypes::Terran_Bunker)
			{
				if (unit->getPlayer() == Broodwar->self())
				{
					amor *= unit->getLoadedUnits().size();
					shieldUpgrade *= unit->getLoadedUnits().size();
				}
				else
				{
					amor *= 4;
					shieldUpgrade *= 4;
				}
			}

			if (shields < 1)
			{
				damage -= amor;
			}
			else if (shields >= damage - shieldUpgrade)
			{
				damage -= shieldUpgrade;
			}
			else
			{
				// accurate ?
				damage -= (shieldUpgrade + amor);
			}

			if (damage <= 0)
			{
				return 0;
			}
		}

		if (target->isUnderDarkSwarm())
		{
			if (unit->getType() != UnitTypes::Terran_Siege_Tank_Siege_Mode &&
				  unit->getType() != UnitTypes::Zerg_Lurker &&
				  unit->getType() != UnitTypes::Protoss_Reaver)
			{
				if (weapon.maxRange() >= MELEE_RANGE)
				{
					return 0;
				}
			}
		}

		double ratio = 1.0;
		if (!unit->getType().isFlyer() && Broodwar->getGroundHeight(unit->getTilePosition()) < Broodwar->getGroundHeight(target->getTilePosition()))
		{
			if (weapon.maxRange() >= MELEE_RANGE || unit->getType() == UnitTypes::Protoss_Reaver)
			{
				ratio = 0.53125;
			}
		}

		if (weapon.damageType() == DamageTypes::Concussive)
		{
			if (target->getType().size() == UnitSizeTypes::Large)
			{
				return damage * 0.25 * ratio;
			}
			if (target->getType().size() == UnitSizeTypes::Medium)
			{
				return damage * 0.5 * ratio;
			}
		}

		if (weapon.damageType() == DamageTypes::Explosive)
		{
			if (target->getType().size() == UnitSizeTypes::Small)
			{
				return damage * 0.5 * ratio;
			}
			if (target->getType().size() == UnitSizeTypes::Medium)
			{
				return damage * 0.75 * ratio;
			}
		}

		return damage * ratio;
	}

	return 0;
}

bool MicroUnitControl::canKillTarget(Unit* unit, Unit* target)
{
	// TODO: consider Protoss Shields regeneration for Protoss units and HP regeneration for Zerg units

	double damage = getDamage(unit,target);

	if (damage == 0)
	{
		return false;
	}
	
	int hp = target->getHitPoints() + target->getShields();
	if (target->isDefenseMatrixed())
	{
		hp += 250;
	}
	
	return damage > hp + 1;
}

double MicroUnitControl::getDPF(Unit* unit)
{
	return max(getAirDPF(unit),getGroundDPF(unit));
}

double MicroUnitControl::getAirDPF(Unit* unit)
{
	if (getAirCooldown(unit) == 0)
	{
		return 0;
	}
	
	return 1.0 * getAirDamage(unit) / getAirCooldown(unit);
}

double MicroUnitControl::getGroundDPF(Unit* unit)
{
	if (getGroundCooldown(unit) == 0)
	{
		return 0;
	}
	
	return 1.0 * getGroundDamage(unit) / getGroundCooldown(unit);
}

double MicroUnitControl::getDPF(Unit* unit, Unit* target)
{
	if (target->getType().isFlyer() && getAirCooldown(unit) > 0)
	{
		return 1.0 * getDamage(unit,target) / getAirCooldown(unit);
	}

	if (!target->getType().isFlyer() && getGroundCooldown(unit) > 0)
	{
		return 1.0 * getDamage(unit,target) / getGroundCooldown(unit);
	}

	return 0;
}

Unit* MicroUnitControl::getBestAttackTartget(Unit* unit, UnitGroup& targets)
{
	if (!unit || !unit->exists() || targets.empty())
	{
		return (Unit*)NULL;
	}

	// enemy simulated hitpoints used to avoid overkill
	//static map<Unit*,double> hitpoints;
	//static int lastUpdateFrame = 0;

	/*for each (Unit* e in targets)
	{
		if (!ArmyManager::isAttackTarget(e))
		{
			continue;
		}

		if (lastUpdateFrame != Broodwar->getFrameCount())
		{
			lastUpdateFrame = Broodwar->getFrameCount();
		  hitpoints[e] = e->getHitPoints() + e->getShields();
		  if (e->isDefenseMatrixed())
		  {
			  hitpoints[e] += 250;
		  }
		}
		else if (hitpoints.find(e) == hitpoints.end())
		{
			hitpoints[e] = e->getHitPoints() + e->getShields();
			if (e->isDefenseMatrixed())
			{
				hitpoints[e] += 250;
			}
		}
	}*/

	Unit* target = NULL;
	Unit* target_bak = NULL;

	int airDamage    = getAirDamage(unit);
	int groundDamage = getGroundDamage(unit);
	
	if (airDamage == 0 && groundDamage == 0)
	{
		return target;
	}

	double maxAV = 0;
	double maxAV_bak = 0;

	// can attack only air units
	if (groundDamage == 0 && airDamage > 0)
	{
		for each (Unit* e in targets)
		{
			if (!e->getType().isFlyer()
				  ||
					e->getPosition().getDistance(unit->getPosition()) > unit->getType().sightRange()
					||
					!ArmyManager::isAttackTarget(e)
					||
					getDamage(unit,e) == 0)
			{
				continue;
			}

			//if (hitpoints[e] <= 0)
			//{
			//	//Broodwar->printf("skip target %s %d | %.1f %d",e->getType().getName().c_str(),e->getID(),hitpoints[e],e->getHitPoints()+e->getShields());
			//	continue;
			//}

			if (e->getType() == UnitTypes::Protoss_Arbiter)
			{
				target = e;
				break;
			}

			double hp = 1.0 + e->getHitPoints() + e->getShields();
			double AV = getDPF(e) / hp;

			if (getDamage(e,unit) == 0)
			{
				AV = AV / 1000;
			}
			
			if (AV > maxAV && e->getPosition().getDistance(unit->getPosition()) < unit->getType().seekRange())
			{
				maxAV = AV;
				target = e;
			}

			if (AV > maxAV_bak)
			{
				maxAV_bak = AV;
				target_bak = e;
			}
		}
	}

	// can attack only ground units
	if (groundDamage > 0 && airDamage == 0)
	{
		for each (Unit* e in targets)
		{
			int maxRange = unit->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode ? unit->getType().groundWeapon().maxRange() : unit->getType().sightRange()*3/2;

			if (e->getType().isFlyer()
				  ||
					e->getPosition().getDistance(unit->getPosition()) > maxRange
					||
					!ArmyManager::isAttackTarget(e)
					||
					getDamage(unit,e) == 0)
			{
				continue;
			}

			//if (hitpoints[e] <= 0)
			//{
			//	//Broodwar->printf("skip target %s %d | %.1f %d",e->getType().getName().c_str(),e->getID(),hitpoints[e],e->getHitPoints()+e->getShields());
			//	if (unit->isSelected())
			//	{
			//		//Broodwar->drawLineMap(unit->getPosition().x(),unit->getPosition().y(),e->getPosition().x(),e->getPosition().y(),Colors::Green);
			//		Broodwar->drawTextMap(e->getPosition().x(),e->getPosition().y()-10,"5 %.1f",hitpoints[e]);
			//	}
			//	continue;
			//}

			if (e->getType() == UnitTypes::Protoss_High_Templar || e->getType() == UnitTypes::Zerg_Defiler)
			{
				target = e;
				break;
			}

			double hp = 1.0 + e->getHitPoints() + e->getShields();
			double AV = getDPF(e) / hp;

			if (getDamage(e,unit) == 0)
			{
				AV = AV / 1000;
			}

			if (unit->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode)
			{
				// avoid attacking enemy units that are close to our units
				int count = 0;
				for each (Unit* i in Broodwar->getUnitsInRadius(e->getPosition(),48))
				{
					if (i->getPlayer() == Broodwar->self() && !i->getType().isFlyer() && !i->isLifted() && !i->isStasised())
					{
						count++;
					}
				}
				AV = AV / (count + 1.0);
			}

			if (AV > maxAV && e->getPosition().getDistance(unit->getPosition()) < unit->getType().seekRange())
			{
				maxAV = AV;
				target = e;
			}

			if (AV > maxAV_bak)
			{
				maxAV_bak = AV;
				target_bak = e;
			}
		}
	}

	// can attack both air and ground units
	if (groundDamage > 0 && airDamage > 0)
	{
		for each (Unit* e in targets)
		{
			if (e->getPosition().getDistance(unit->getPosition()) > unit->getType().sightRange()
				  ||
				  !ArmyManager::isAttackTarget(e)
				  ||
					getDamage(unit,e) == 0)
			{
				continue;
			}
			
			//if (hitpoints[e] <= 0)
			//{
			//	//Broodwar->printf("skip target %s %d | %.1f %d",e->getType().getName().c_str(),e->getID(),hitpoints[e],e->getHitPoints()+e->getShields());
			//	continue;
			//}

			if (e->getType() == UnitTypes::Protoss_High_Templar || e->getType() == UnitTypes::Protoss_Arbiter || e->getType() == UnitTypes::Zerg_Defiler)
			{
				target = e;
				break;
			}

			double hp = 1.0 + e->getHitPoints() + e->getShields();
			double AV = getDPF(e) / hp;

			if (getDamage(e,unit) == 0)
			{
				AV = AV / 1000;
			}

			if (AV > maxAV && e->getPosition().getDistance(unit->getPosition()) < unit->getType().seekRange())
			{
				maxAV = AV;
				target = e;
			}

			if (AV > maxAV_bak)
			{
				maxAV_bak = AV;
				target_bak = e;
			}
		}
	}

	if (!target)
	{
		target = target_bak;
	}

	if (target)
	{
		Broodwar->drawLineMap(unit->getPosition().x(),unit->getPosition().y(),target->getPosition().x(),target->getPosition().y(),Colors::Red);
		//Broodwar->printf("%s %d ---> %s %d | %d",unit->getType().getName().c_str(),unit->getID(),target->getType().getName().c_str(),target->getID(),Broodwar->getFrameCount());
		//hitpoints[target] -= (getDamage(unit,target) - 1);
		/*if (hitpoints[target] <= 0)
		{
			Broodwar->printf("%s %d ---> %s %d %.1f",unit->getType().getName().c_str(),unit->getID(),target->getType().getName().c_str(),target->getID(),hitpoints[target]);
			Broodwar->drawLineMap(target->getPosition().x()-10,target->getPosition().y()-10,target->getPosition().x()+10,target->getPosition().y()+10,Colors::Red);
			Broodwar->drawLineMap(target->getPosition().x()-10,target->getPosition().y()+10,target->getPosition().x()+10,target->getPosition().y()-10,Colors::Red);
		}*/
	}
	
	return target;
}

bool MicroUnitControl::attack(Unit* unit, Unit* target)
{
	if (!unit || !unit->exists() || !target || !target->exists())
	{
		return false;
	}
	
	if (!unit->isIdle() &&
		  unit->getLastCommand().getType() == UnitCommandTypes::Attack_Unit &&
		  unit->getLastCommand().getTarget() == target &&
		  unit->getLastCommandFrame() > Broodwar->getFrameCount() - 24*5)
	{
		return false;
	}
	
	return unit->attack(target);
}

bool MicroUnitControl::attack(Unit* unit, Position target)
{
	if (!unit || !unit->exists() || target == Positions::None || target == Positions::Invalid || target == Positions::Unknown)
	{
		return false;
	}

	if (!unit->isIdle() &&
		  unit->getLastCommand().getType() == UnitCommandTypes::Attack_Move &&
		  unit->getLastCommand().getTargetPosition().getDistance(target) < 32 * 2 &&
		  unit->getLastCommandFrame() > Broodwar->getFrameCount() - 24*5)
	{
		return false;
	}
	
	return unit->attack(target);
}

bool MicroUnitControl::move(Unit* unit, Position target)
{
	if (!unit || !unit->exists() || target == Positions::None || target == Positions::Invalid || target == Positions::Unknown)
	{
		return false;
	}

	if (!unit->isIdle() &&
		  unit->getLastCommand().getType() == UnitCommandTypes::Move &&
		  unit->getLastCommand().getTargetPosition().getDistance(target) < 32 * 2 &&
		  unit->getLastCommandFrame() > Broodwar->getFrameCount() - 24*5)
	{
		return false;
	}

	return unit->move(target);
}

void MicroUnitControl::tankAttack(BWAPI::Unit* u, BWAPI::Position p, int reachRange /*= 32*6*/)
{
	if (!u || !u->exists() || p == Positions::None
		  ||
		  (u->getType() != UnitTypes::Terran_Siege_Tank_Tank_Mode && u->getType() != UnitTypes::Terran_Siege_Tank_Siege_Mode))
	{
		return;
	}

	if (u->getOrder() == Orders::Sieging || u->getOrder() == Orders::Unsieging)
	{
		return;
	}

	Position siegePoint = ArmyManager::create()->getSiegePoint();
	int attackRange = UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().maxRange();

	UnitGroup enemyInRange;
	for each (Unit* e in Broodwar->enemy()->getUnits())
	{
		if (!ArmyManager::isAttackTarget(e))
		{
			continue;
		}

		if (e->getType().isFlyer() || e->isLifted() || e->getPosition().getDistance(u->getPosition()) > attackRange)
		{
			continue;
		}

		enemyInRange.insert(e);
	}

	if (u->isSelected())
	{
		Broodwar->drawLineMap(u->getPosition().x(),u->getPosition().y(),p.x(),p.y(),Colors::White);
		Broodwar->drawTextMap(u->getPosition().x(),u->getPosition().y()+10,"%d %d",u->getGroundWeaponCooldown(),enemyInRange.size());
	}

	// if attack position is near siege point then just stay there
	if (siegePoint != Positions::None)
	{
		if (ArmyManager::create()->getArmyState() != ICEStarCraft::ArmyAttack &&
			  p.getDistance(siegePoint) < attackRange*1.5 &&
			  BWTA::getRegion(p) != BWTA::getRegion(Broodwar->self()->getStartLocation()))
		{
			p = siegePoint;
		}
	}

	if (p == siegePoint)
	{
		reachRange = 32*3;
		BWTA::Region* reg = BWTA::getRegion(p);

		int count = 0;
		for each (Unit* i in Broodwar->self()->getUnits())
		{
			//if (i == u || !i->isCompleted() || (i->getType() != UnitTypes::Terran_Siege_Tank_Tank_Mode && i->getType() != UnitTypes::Terran_Siege_Tank_Siege_Mode))
			if (i == u || !i->isCompleted() || i->getType() != UnitTypes::Terran_Siege_Tank_Siege_Mode)
			{
				continue;
			}
			if (i->getPosition().getDistance(p) <= reachRange && (!reg || BWTA::getRegion(i->getPosition()) == reg))
			{
				count++;
			}
		}

		if (count >= 4)
		{
			reachRange = 32*6;
			count = 0;
			for each (Unit* i in Broodwar->self()->getUnits())
			{
				//if (i == u || !i->isCompleted() || (i->getType() != UnitTypes::Terran_Siege_Tank_Tank_Mode && i->getType() != UnitTypes::Terran_Siege_Tank_Siege_Mode))
				if (i == u || !i->isCompleted() || i->getType() != UnitTypes::Terran_Siege_Tank_Siege_Mode)
				{
					continue;
				}
				if (i->getPosition().getDistance(p) <= reachRange && (!reg || BWTA::getRegion(i->getPosition()) == reg))
				{
					count++;
				}
			}
			
			if (count >= 7)
			{
				p = ArmyManager::create()->getSetPoint();
			}
		}
	}

	/************************************************************************/
	/* Tank Mode                                                            */
	/************************************************************************/

	if (u->getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode)
	{
		if (u->isUnderDisruptionWeb())
		{
			u->move(Position(Broodwar->self()->getStartLocation()));
			return;
		}

		if (enemyInRange.empty())
    {
			// destroy buildings that block its way
			Unit* bs = NULL;
			if (Broodwar->getFrameCount() > 24*60*10 &&
				  TerrainManager::create()->bsPos != TilePositions::None &&
				  TerrainManager::create()->bsPos.getDistance(u->getTilePosition()) < 8)
			{
				for each (Unit* i in Broodwar->self()->getUnits())
				{
          if (i->getTilePosition().getDistance(TerrainManager::create()->bsPos) > 2)
          {
            continue;
          }
					if (i->getType() == UnitTypes::Terran_Supply_Depot ||
              (i->getType() == UnitTypes::Terran_Bunker && ArmyManager::create()->getArmyState() != ICEStarCraft::ArmyGuard))
					{
						bs = i;
						break;
					}
				}
			}

			if (bs)
			{
        workerManager->addToNotRepairList(bs);
				attack(u,bs);
				return;
			}

			if (u->getPosition().getDistance(p) > reachRange)
			{
				// move
				if (Broodwar->enemy()->getRace() != Races::Terran)
				{
					MicroUnitControl::attack(u,p);
					return;
				}

				// there may be invisible tanks. be careful!
				set<EnemyUnit*> invisibleEnemy;
				for each (EnemyUnit* e in EnemyInfoManager::create()->getAllEnemyUnits())
				{
					if (Broodwar->getFrameCount() - e->getLastUpdatedFrame() > 24*60*3)
					{
						continue;
					}

					if (e->getPosition() == Positions::Unknown)
					{
						continue;
					}

					if (e->getType() != UnitTypes::Terran_Siege_Tank_Siege_Mode && e->getType() != UnitTypes::Terran_Siege_Tank_Tank_Mode)
					{
						continue;
					}

					if (e->getPosition().getDistance(u->getPosition()) < attackRange + 32*2)
					{
						Vector2 v1 = Vector2(e->getPosition()) - Vector2(u->getPosition());
						Vector2 v2 = Vector2(p) - Vector2(u->getPosition());
						if (v1.angle(v2) < 90) // this tank will get closer to enemy if move towards p
						{
							invisibleEnemy.insert(e);
						}
					}
				}
				if (!invisibleEnemy.empty())
				{
					//Broodwar->printf("be careful !!!");
					//Broodwar->drawBoxMap(u->getPosition().x()-15,u->getPosition().y()-15,u->getPosition().x()+15,u->getPosition().y()+15,Colors::Orange);
					//Broodwar->drawBoxMap(u->getPosition().x()-20,u->getPosition().y()-20,u->getPosition().x()+20,u->getPosition().y()+20,Colors::Orange);
					u->siege(); // or move back ?
				}
				else
				{
					attack(u,p);
					return;
				}
			}
			else
			{
				// siege
				BWTA::Chokepoint* nearestCp = BWTA::getNearestChokepoint(u->getPosition());
				if (nearestCp && nearestCp->getWidth() < 120 && u->getPosition().getDistance(nearestCp->getCenter()) < 32*4)
				{
					if (u->getLastCommand().getType() != UnitCommandTypes::Attack_Move)
					{
						attack(u,p);
						return;
					}
				}
				else
				{
          if (p != siegePoint)
          {
            Vector2 v = Vector2();
            for each (Unit* i in Broodwar->self()->getUnits())
            {
              if (!i->isCompleted() || i->getPosition().getApproxDistance(u->getPosition()) > 64)
              {
                continue;
              }
              if (i->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode
                  ||
                  (i->getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode && i->getOrder() == Orders::Sieging))
              {
                v += PFFunctions::getVelocitySource(i->getPosition(),u->getPosition()) * 1000;
              }
            }
            if (v != Vector2())
            {
              v = v * (96.0 / v.approxLen());
              move(u,(v + u->getPosition()).makeValid());
              return;
            }
          }
					u->siege();
				}
			}
		}
		else // enemy in range not empty
		{
			//don't siege if there are enemy units close to this tank
			int count = 0;
			for each (Unit* e in u->getUnitsInRadius(32*2))
			{
				if (e->getPlayer() == Broodwar->enemy() &&
					  e->isCompleted() &&
					  !e->getType().isFlyer() &&
					  (e->getType().canAttack() || e->getType() == UnitTypes::Protoss_Reaver) &&
					  e->getType() != UnitTypes::Terran_Vulture_Spider_Mine &&
					  e->getType() != UnitTypes::Protoss_Scarab)
				{
					count++;
				}
			}

			if (count == 0)
			{
				BWTA::Chokepoint* nearestCp = BWTA::getNearestChokepoint(u->getPosition());
				if (nearestCp && nearestCp->getWidth() < 120 && u->getPosition().getDistance(nearestCp->getCenter()) < 32*4)
				{
					attack(u,enemyInRange.getCenter());
				}
				else
				{
					u->siege();
				}
				return;
			}

			if (u->getGroundWeaponCooldown() > 1)
			{
				return;
			}

			// attack in Tank mode
			Unit* target = getBestAttackTartget(u,enemyInRange);
			if (target)
			{
				attack(u,target);
			}
		}
	}

	/************************************************************************/
	/* Siege Mode                                                           */
	/************************************************************************/

	if (u->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode)
	{
		if (u->isUnderDisruptionWeb())
		{
			u->unsiege();
			return;
		}

		// tank should not siege near narrow choke point
		BWTA::Chokepoint* nearestCp = BWTA::getNearestChokepoint(u->getPosition());
		if (nearestCp && nearestCp->getWidth() < 120 && u->getPosition().getDistance(nearestCp->getCenter()) < 32*4)
		{
			u->unsiege();
			return;
		}

		if (enemyInRange.empty() && u->getPosition().getDistance(p) > reachRange)
		{
			if (Broodwar->enemy()->getRace() != Races::Terran)
			{	
				u->unsiege();
				return;
			}

			// there may be invisible tanks. be careful!
			set<EnemyUnit*> invisibleEnemy;
			for each (EnemyUnit* e in EnemyInfoManager::create()->getAllEnemyUnits())
			{
				if (Broodwar->getFrameCount() - e->getLastUpdatedFrame() > 24*60*3)
				{
					continue;
				}

				if (e->getPosition() == Positions::Unknown)
				{
					continue;
				}

				if (e->getType() != UnitTypes::Terran_Siege_Tank_Siege_Mode && e->getType() != UnitTypes::Terran_Siege_Tank_Tank_Mode)
				{
					continue;
				}

				if (e->getPosition().getDistance(u->getPosition()) < attackRange + 32*2)
				{
					Vector2 v1 = Vector2(e->getPosition()) - Vector2(u->getPosition());
					Vector2 v2 = Vector2(p) - Vector2(u->getPosition());
					if (v1.angle(v2) < 90) // this tank will get closer to enemy if move towards p
					{
						invisibleEnemy.insert(e);
					}
				}
			}
			if (!invisibleEnemy.empty())
			{
				//Broodwar->drawBoxMap(u->getPosition().x()-15,u->getPosition().y()-15,u->getPosition().x()+15,u->getPosition().y()+15,Colors::Orange);
				//Broodwar->drawBoxMap(u->getPosition().x()-20,u->getPosition().y()-20,u->getPosition().x()+20,u->getPosition().y()+20,Colors::Orange);
			}
			else
			{
				u->unsiege();
				return;
			}
		}

		if (!enemyInRange.empty())
		{
			// unsiege if enemy melee units are close to this tank
			for each (Unit* e in u->getUnitsInRadius(32*2))
			{
				if (e->getPlayer() == Broodwar->enemy() &&
					  e->isCompleted() &&
					  !e->getType().isFlyer() &&
					  (e->getType().canAttack() || e->getType() == UnitTypes::Protoss_Reaver) &&
					  e->getType() != UnitTypes::Terran_Vulture_Spider_Mine &&
					  e->getType() != UnitTypes::Protoss_Scarab)
				{
					u->unsiege();
					return;
				}
			}

			if (u->getGroundWeaponCooldown() > 1)
			{
				return;
			}

			Unit* target = getBestAttackTartget(u,enemyInRange);
			if (target)
			{
				attack(u,target);
			}
			else
			{
				//Broodwar->printf("SiegeTank no target, enemyInRange %d",enemyInRange.size());
			}
		}
	}
}

void MicroUnitControl::battlecruiserAttack(BWAPI::Unit* u, BWAPI::Position p)
{
	if (!u || !u->exists() || p == Positions::None || u->getType() != UnitTypes::Terran_Battlecruiser)
	{
		return;
	}

	if (workerManager->isInRepairList(u) || u->getHitPoints() <= 200 || u->isBeingHealed())
	{
		if (u->getPosition().getDistance(TerrainManager::create()->mSecondChokepoint->getCenter()) > 32*3)
		{
			u->move(TerrainManager::create()->mSecondChokepoint->getCenter());
		}
		return;
	}

	UnitGroup targets = ArmyManager::getAttackTargets(u->getPosition(),u->getType().sightRange()*3/2);
	UnitGroup targets_;
	
	if (useYamatoGun(u,targets) == true)
	{
		return;
	}

	Unit* target = getBestAttackTartget(u,targets);
	if (target)
	{
		attack(u,target);
	}
	else
	{
		attack(u,p);
	}
}
bool MicroUnitControl::useYamatoGun(Unit* u, UnitGroup& targets)
{
	if (u->getOrder() == Orders::FireYamatoGun)
	{
		return true;
	}

	if (!Broodwar->self()->hasResearched(TechTypes::Yamato_Gun)
		  ||
		  TechTypes::Yamato_Gun.whatUses().find(u->getType()) == TechTypes::Yamato_Gun.whatUses().end()
		  ||
		  u->getEnergy() < TechTypes::Yamato_Gun.energyUsed())
	{
		return false;
	}

	Unit* target = NULL;
	double dis = -1;
	
	for each (Unit* e in targets)
	{
		if (e->getHitPoints() < 100)
		{
			continue;
		}
		
		if (dis == -1 || u->getPosition().getDistance(e->getPosition()) < dis)
		{
			dis = u->getPosition().getDistance(e->getPosition());
			target = e;
		}
	}
	
	if (target)
	{
		return useTech(u,TechTypes::Yamato_Gun,target);
	}
	
	return false;
}

void MicroUnitControl::scienceVesselMicro(Unit* u)
{
	if (!u || !u->exists() || u->getType() != UnitTypes::Terran_Science_Vessel)
	{
		return;
	}

	Vector2 v = Vector2(0,0);
	for each (Unit* e in Broodwar->enemy()->getUnits())
	{
		if (e->getPosition().getDistance(u->getPosition()) > u->getType().sightRange())
		{
			continue;
		}

		if (e->getOrder() == Orders::AttackUnit && e->getOrderTarget() == u)
		{
			v += PFFunctions::getVelocitySource(e->getPosition(),u->getPosition()) * 1000;
		}
	}

	if (v != Vector2(0,0))
	{
		for each (Unit* i in Broodwar->self()->getUnits())
		{
			if (!i->isCompleted())
			{
				continue;
			}
			
			if (i->getType().airWeapon() != WeaponTypes::None || (i->getType() == UnitTypes::Terran_Bunker && !i->getLoadedUnits().empty()))
			{
				v += PFFunctions::getVelocitySource(i->getPosition(),u->getPosition()) * (-1000);
			}
		}

		v = v * (128.0 / v.approxLen());
		Position p = (v + u->getPosition()).makeValid();
		move(u,p);
		return;
	}

	UnitGroup units = BattleManager::create()->getMyUnits();
	if (units.empty())
	{
		units = ArmyManager::create()->getAttackers().inRadius(32*30,u->getPosition());
		if (units.empty())
		{
			units = ArmyManager::create()->getAttackers();
		}
	}

	Position targetPos = Positions::None;
	if (units.empty())
	{
		targetPos = ArmyManager::create()->getSetPoint();
	}
	else
	{
		UnitGroup unDetectedEnemy = SelectAllEnemy()(isCompleted)(Lurker,Dark_Templar,Ghost,Wraith,Arbiter).inRadius(32*20,units.getCenter());
		unDetectedEnemy += SelectAllEnemy()(isCompleted)(canAttack,Carrier,Reaver).not(isDetected).inRadius(32*20,units.getCenter());

		if (unDetectedEnemy.getNearest(u->getPosition()))
		{
			targetPos = unDetectedEnemy.getNearest(u->getPosition())->getPosition();
		}
		else
		{
			targetPos = units.getCenter();
		}
	}

	// use tech
	UnitGroup targets = UnitGroup::getUnitGroup(u->getUnitsInRadius(u->getType().sightRange()*3/2));
	if (useIrradiate(u,targets))
	{
		//Broodwar->printf("use Irradiate %d %d",u->getID(),Broodwar->getFrameCount());
		return;
	}

	if (useEMPShockwave(u,targets))
	{
		//Broodwar->printf("use EMP Shockwave %d %d",u->getID(),Broodwar->getFrameCount());
		return;
	}

	if (useDefensiveMatrix(u,targets))
	{
		//Broodwar->printf("use Defensive Matrix %d %d",u->getID(),Broodwar->getFrameCount());
		return;
	}

	move(u,targetPos);
}

bool MicroUnitControl::useIrradiate(Unit* u, UnitGroup& targets)
{
	if (u->getOrder() == Orders::CastIrradiate)
	{
		return true;
	}

	if (!Broodwar->self()->hasResearched(TechTypes::Irradiate)
		  ||
			TechTypes::Irradiate.whatUses().find(u->getType()) == TechTypes::Irradiate.whatUses().end()
		  ||
		  u->getEnergy() < TechTypes::Irradiate.energyUsed())
	{
		return false;
	}

	Unit* target = NULL;
	Unit* target1 = NULL;
	int maxHP = 50;
	int maxHP1 = 50;

	for each (Unit* e in targets)
	{
		if (e->getPlayer() != Broodwar->enemy()
			  ||
				e->getType().isBuilding()
				||
				!e->getType().isOrganic()
				||
				e->isIrradiated()
				||
				!e->isCompleted())
		{
			continue;
		}

		if (e->getType().canAttack())
		{
			if (e->getHitPoints() >= maxHP)
			{
				maxHP = e->getHitPoints();
				target = e;
			}
		}
		
		if (e->getHitPoints() >= maxHP1)
		{
			maxHP1 = e->getHitPoints();
			target1 = e;
		}
	}

	if (!target)
	{
		target = target1;
	}
	
	if (target)
	{
		//Broodwar->printf("use Irradiate on %s",target->getType().getName().c_str());
		//Broodwar->drawLineMap(u->getPosition().x(),u->getPosition().y(),target->getPosition().x(),target->getPosition().y(),Colors::Orange);
		return useTech(u,TechTypes::Irradiate,target);
	}
	
	return false;
}

bool MicroUnitControl::useEMPShockwave(Unit* u, UnitGroup& targets)
{
	if (u->getOrder() == Orders::CastEMPShockwave)
	{
		return true;
	}

	if (!Broodwar->self()->hasResearched(TechTypes::EMP_Shockwave)
		  ||
		  TechTypes::EMP_Shockwave.whatUses().find(u->getType()) == TechTypes::EMP_Shockwave.whatUses().end()
		  ||
		  u->getEnergy() < TechTypes::EMP_Shockwave.energyUsed())
	{
		return false;
	}

	targets = targets(Broodwar->enemy())(isCompleted).not(Observer);
	if (targets.empty())
	{
		return false;
	}

	Unit* arbiter = targets(Arbiter).getNearest(u->getPosition());
	Unit* hightemplar = targets(High_Templar).getNearest(u->getPosition());
	
	Position target = Positions::None;
	if (arbiter)
	{
		target = arbiter->getPosition();
	}
	else if (hightemplar)
	{
		target = hightemplar->getPosition();
	}
	else
	{
		target = targets.getCenter();
	}
	//Broodwar->printf("use EMP Shockwave");
	//Broodwar->drawLineMap(u->getPosition().x(),u->getPosition().y(),target.x(),target.y(),Colors::Orange);
	return useTech(u,TechTypes::EMP_Shockwave,target);
}

bool MicroUnitControl::useDefensiveMatrix(Unit* u, UnitGroup& targets)
{
	if (u->getOrder() == Orders::CastDefensiveMatrix)
	{
		return true;
	}

	if (!Broodwar->self()->hasResearched(TechTypes::Defensive_Matrix)
		  ||
		  TechTypes::Defensive_Matrix.whatUses().find(u->getType()) == TechTypes::Defensive_Matrix.whatUses().end()
		  ||
		  u->getEnergy() < TechTypes::Defensive_Matrix.energyUsed())
	{
		return false;
	}

	Unit* target = NULL;
	int max = 0;

	for each (Unit* i in targets)
	{
		if (i->getPlayer() != Broodwar->self()
			  ||
				!i->isCompleted()
				||
				!i->getType().canAttack()
				||
				i->getType().isBuilding()
				||
				i->getType().isWorker()
				||
				i->getType() == UnitTypes::Terran_Vulture_Spider_Mine
				||
				i->getType() == UnitTypes::Terran_Nuclear_Missile
				||
				i->isDefenseMatrixed()
				||
				i->getHitPoints() < i->getType().maxHitPoints()/3
				||
				(!i->isAttacking() && !i->isUnderAttack()))
		{
			continue;
		}

		int damage = max(i->getType().groundWeapon().damageAmount(),i->getType().airWeapon().damageAmount());
		if (i->getType().maxHitPoints() * damage > max)
		{
			max = i->getType().maxHitPoints() * damage;
			target = i;
		}
	}

	if (target)
	{
		//Broodwar->printf("use Defensive Matrix on %s",target->getType().getName().c_str());
		//Broodwar->drawLineMap(u->getPosition().x(),u->getPosition().y(),target->getPosition().x(),target->getPosition().y(),Colors::Orange);
		return useTech(u,TechTypes::Defensive_Matrix,target);
	}

	return false;
}

bool MicroUnitControl::useTech(Unit* unit, TechType tech)
{
	if (!unit || !unit->exists() || tech == TechTypes::None)
	{
		return false;
	}
	
	if (!unit->isIdle() &&
		  unit->getLastCommand().getTechType() == tech &&
		  unit->getLastCommandFrame() > Broodwar->getFrameCount() - 24*5)
	{
		return true;
	}
	
	return unit->useTech(tech);
}

bool MicroUnitControl::useTech(Unit* unit, TechType tech, Unit* target)
{
	if (!unit || !unit->exists() || !target || !target->exists() || tech == TechTypes::None)
	{
		return false;
	}

	if (!unit->isIdle() &&
		  unit->getLastCommand().getTechType() == tech &&
			unit->getLastCommand().getTarget() == target &&
		  unit->getLastCommandFrame() > Broodwar->getFrameCount() - 24*5)
	{
		return true;
	}

	return unit->useTech(tech,target);
}

bool MicroUnitControl::useTech(Unit* unit, TechType tech, Position target)
{
	if (!unit || !unit->exists() || target == Positions::None || target == Positions::Unknown || target == Positions::Invalid || tech == TechTypes::None)
	{
		return false;
	}

	if (!unit->isIdle() &&
		  unit->getLastCommand().getTechType() == tech &&
		  unit->getLastCommand().getTargetPosition().getDistance(target) < 32 &&
		  unit->getLastCommandFrame() > Broodwar->getFrameCount() - 24*5)
	{
		return true;
	}

	return unit->useTech(tech,target);
}