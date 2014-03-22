#include <time.h>
#include "BattleManager.h"

#define MELEE_RANGE 64

using namespace std;
using namespace BWAPI;
using namespace BWTA;

BattleManager* theBattleManager = NULL;

BattleManager* BattleManager::create() 
{
	if (theBattleManager) return theBattleManager;
	else return theBattleManager = new BattleManager();
}

void BattleManager::destroy()
{
	if (theBattleManager)
	{
		delete theBattleManager;
		theBattleManager = NULL;
	}
}

BattleManager::BattleManager()
{
	eInfo = EnemyInfoManager::create();
	mInfo = MyInfoManager::create();
	arbitrator = NULL;

	mUnits.clear();
	eUnits.clear();
}

BattleManager::~BattleManager()
{

}

void BattleManager::onOffer(set<Unit*> units)
{
	for each (Unit* u in units)
	{
		arbitrator->accept(this, u);
		if (mUnits.find(u) == mUnits.end())
		{
			mUnits.insert(u);
		}
	}
}

void BattleManager::onRevoke(BWAPI::Unit* unit, double bid)
{
	mUnits.erase(unit);
}

void BattleManager::onUnitDestroy(BWAPI::Unit* unit)
{
	mUnits.erase(unit);
}

void BattleManager::update()
{
	// MY UNITS IN BATTLE
	UnitGroup unitsInBattle;
	UnitGroup unitsNotInBattle;
	
	UnitGroup fighters = mUnits + ArmyManager::create()->getAttackers();
	for each (Unit* u in fighters)
	{
		if (u->isAttacking() || u->getLastCommand().getType() == UnitCommandTypes::Attack_Unit || u->getOrder() == Orders::AttackUnit || u->isUnderAttack())
		{
			unitsInBattle.insert(u);
		}
		else if (MicroUnitControl::isFiring(u))
		{
			unitsInBattle.insert(u);
		}
		else
		{
			unitsNotInBattle.insert(u);
		}
	}

	if (unitsInBattle.empty())
	{
		mUnits.clear();
		arbitrator->removeAllBids(this);
		return;
	}

	Position center = unitsInBattle.getCenter();
	for each (Unit* u in unitsNotInBattle)
	{
		if (u->getPosition().getApproxDistance(center) <= 32*10)
		{
			unitsInBattle.insert(u);
		}
	}

	// ENEMY UNITS IN BATTLE
	eUnits.clear();
	eUnits = ArmyManager::getAttackTargets(unitsInBattle.getCenter(),32*20);

	if (eUnits.empty())
	{
		mUnits.clear();
		arbitrator->removeAllBids(this);
		return;
	}

	arbitrator->setBid(this, unitsInBattle, 500);

	if (mUnits.empty())
	{
		return;
	}

	// CONTROL UNITS

	BWTA::Chokepoint* chokepoint = getNarrowChokepointInBattle(mUnits,ArmyManager::create()->getAttackTarget());
	Position eTarget = eUnits.getTargetPosition();
	Position attackPosition;

	if (ArmyManager::create()->getArmyState() == ICEStarCraft::ArmyAttack && ArmyManager::create()->getGatherPoint() != Positions::None)
	{
		attackPosition = ArmyManager::create()->getGatherPoint();
	}
	else if (ArmyManager::create()->getAttackTarget()->getPosition() != Positions::None)
	{
		attackPosition = ArmyManager::create()->getAttackTarget()->getPosition();
	}
	else
	{
		attackPosition = eUnits.getCenter();
	}

	for each (Unit* u in mUnits)
	{
		if (u->isLoaded())
		{
			continue;
		}

		if (chokepoint && !u->getType().isFlyer())
		{
			Position des = MicroUnitControl::getDestinationNearChokepoint(u,mUnits,ArmyManager::create()->getAttackTarget()->getPosition(),chokepoint);
			if (des != Positions::None)
			{
				MicroUnitControl::move(u,des);
				continue;
			}
		}

		if (u->getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode || u->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode)
		{
		  MicroUnitControl::tankAttack(u,attackPosition);
			continue;
		}

		if (u->getType() == UnitTypes::Terran_Battlecruiser)
		{
			MicroUnitControl::battlecruiserAttack(u,attackPosition);
			continue;
		}

		if (u->getType() == UnitTypes::Terran_Marine && Broodwar->self()->completedUnitCount(UnitTypes::Terran_Bunker) > 0)
		{
			int minD = -1;
			Unit* bunker = NULL;
			for each (Unit* i in Broodwar->self()->getUnits())
			{
				if (!i->isCompleted() ||
					  i->getType() != UnitTypes::Terran_Bunker ||
						i->getLoadedUnits().size() >= 4 ||
						u->getPosition().getApproxDistance(i->getPosition()) > 32*12)
				{
					continue;
				}

				if (!eUnits.inRadius(32*10,i->getPosition()).empty() || (eTarget != Positions::None && eTarget.getApproxDistance(i->getPosition()) < 32*6))
				{
					if (minD == -1 || u->getPosition().getApproxDistance(i->getPosition()) < minD)
					{
						minD = u->getPosition().getApproxDistance(i->getPosition());
						bunker = i;
					}
				}
			}

			if (bunker)
			{
				u->rightClick(bunker);
				continue;
			}
		}

		// retreat
		if (!u->getType().isFlyer() && u->getType().canMove() && u->getType().groundWeapon().maxRange() > MELEE_RANGE)
		{
			Vector2 v = Vector2();
			for each (Unit* e in eUnits)
			{
				if (e->getType().isFlyer() || e->getType().groundWeapon() == WeaponTypes::None || e->getType().groundWeapon().maxRange() > MELEE_RANGE)
				{
					continue;
				}

				if (u->getGroundWeaponCooldown() > 1 && u->getAirWeaponCooldown() > 1)
				{
					if (e->getPosition().getDistance(u->getPosition()) <= 32*4)
					{
						v += PFFunctions::getVelocitySource(e->getPosition(),u->getPosition()) * 1000;
					}
				}
				else
				{
					if (e->getPosition().getDistance(u->getPosition()) <= 32*2 && MicroUnitControl::getDamage(e,u) > MicroUnitControl::getDamage(u,e))
					{
						// run away, don't care about cooldown
						v += PFFunctions::getVelocitySource(e->getPosition(),u->getPosition()) * 1000;
					}
				}
			}

			if (v != Vector2())
			{
				v = v * (128.0 / v.approxLen());
				Position des = (v + u->getPosition()).makeValid();

				MicroUnitControl::move(u,des);
				continue;
			}
		}

		// place mine
		if (u->getType() == UnitTypes::Terran_Vulture && u->getSpiderMineCount() > 0 && Broodwar->self()->hasResearched(TechTypes::Spider_Mines) && u->getGroundWeaponCooldown() > 1)
		{
			Unit* target = eUnits.not(isFlyer,isBuilding).getNearest(u->getPosition());
			if (target)
			{
				//Broodwar->printf("place mine");
				Position pos = target->getPosition() + u->getPosition();
				pos = Position(pos.x()/2,pos.y()/2);
				if (!u->isIdle() &&
					  u->getLastCommand().getType() == UnitCommandTypes::Use_Tech_Position &&
					  u->getLastCommand().getTargetPosition().getDistance(pos) < 32 &&
					  u->getLastCommandFrame() > Broodwar->getFrameCount() - 24*5)
				{
					continue;
				}
				u->useTech(TechTypes::Spider_Mines,pos);
				continue;
			}
		}

		// attack
		if (u->getGroundWeaponCooldown() <= 1 || u->getAirWeaponCooldown() <= 1)
		{
			Unit* target = MicroUnitControl::getBestAttackTartget(u,eUnits);

			if (target)
			{
				MicroUnitControl::attack(u,target);
			}
			else
			{
				MicroUnitControl::attack(u,attackPosition);
			}
		}
	}
}

BWTA::Chokepoint* BattleManager::getNarrowChokepointInBattle(UnitGroup& units, AttackTarget* target)
{
	if (units.size() < 12 || !target || target->getPosition() == Positions::None)
	{
		return (BWTA::Chokepoint*)NULL;
	}

	string tarType  = target->getType();
	Position tarPos = target->getPosition();

	if (tarType == "MainBase" || tarType == "OnlyMainBase" || tarType == "Expansion")
	{
		BWTA::Region* startReg  = BWTA::getRegion(Broodwar->self()->getStartLocation());
		BWTA::Region* secondReg = TerrainManager::create()->mNearestBase ? TerrainManager::create()->mNearestBase->getRegion() : (BWTA::Region*)NULL;
		BWTA::Region* tarReg    = BWTA::getRegion(tarPos);
		if (!startReg || !secondReg || !tarReg)
		{
			return (BWTA::Chokepoint*)NULL;
		}

		int minD = -1;
		BWTA::Chokepoint* chokepoint = NULL;
		Position center = units.getCenter();
		for each (BWTA::Chokepoint* cp in tarReg->getChokepoints())
		{
			if (minD == -1 || cp->getCenter().getApproxDistance(center) < minD)
			{
				minD = cp->getCenter().getApproxDistance(center);
				chokepoint = cp;
			}
		}

		if (chokepoint && chokepoint->getWidth() < 120)
		{
			return chokepoint;
		}
	}
	
	return (BWTA::Chokepoint*)NULL;
}

UnitGroup BattleManager::getMyUnits()
{
	return mUnits;
}

UnitGroup BattleManager::getEnemyUnits()
{
	return eUnits;
}

void BattleManager::showDebugInfo()
{
	for each (Unit* u in mUnits)
	{
		Broodwar->drawCircleMap(u->getPosition().x(),u->getPosition().y(),10,Colors::Green);
	}

	for each (Unit* u in eUnits)
	{
		Broodwar->drawCircleMap(u->getPosition().x(),u->getPosition().y(),10,Colors::Red);
	}
}