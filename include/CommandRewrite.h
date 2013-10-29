#pragma once
#include <BWAPI.h>
#include "UnitGroup.h"
#include "UnitGroupManager.h"
#include <map>

using namespace BWAPI;
using namespace std;

class issueOnce{
public:
	issueOnce();
	void moveOnce(Unit* u,Position p,int accuracy);
	void attackMoveOnce(Unit* u,Position p, int accuracy);
	void attackTargetOnce(Unit* u,Unit* target);
	void stopOnce(Unit* u);
	void holdOnce(Unit* u);
	void patrolOnce(Unit* u, Position p);
	void repairOnce(Unit* u,Unit* target);
	void healMoveOnce(Unit* u,Position p,int accuracy);
	void followOnce(Unit* u,Unit* target);
	void stuckSolution(Unit* u);
	void siegeOnce(Unit*u,Position p);
	void tankMoveHold(Unit*u);
	void vultureMine(Unit* u,Position p1);


private:
	int mLastIssueTime;
	Position mLastAttackPosition;
	Position mLastMovePosition;
	Position mLastPatrolPosition;
	Position mLastHealMovePosition;
	Unit* mLastTarget;
	Unit* mLastFollowTarget;
	Unit* mLastRepairTarget;

	Position lastMinePosition;
};

Position unitMoveLeft(Unit*u);
Position unitMoveRight(Unit*u);
Position unitMoveUp(Unit*u);
Position unitMoveDown(Unit*u);
Position unitMoveTopLeft(Unit*u);
Position unitMoveTopRight(Unit*u);
Position unitMoveBottomLeft(Unit*u);
Position unitMoveBottomRight(Unit*u);
Position unitNextRunningPosition(Unit*u);
bool unitInDanger(Unit* u);
Position avoidEnemyAttackMove(Unit* u,Position finalgoal);

bool isTileWalkable(TilePosition location);
bool isTileWalkable(int x, int y);
bool isSurroundingWalkable(int x, int y);
void vultureAttackMode(Unit* u);
bool enemyInSCVSightRange(Unit* u);
Unit* getBestAttackTartget(Unit* myFighter);

namespace Helper
{
	Position isUnderInvAtk (Unit *u);
}

BWAPI::Position getGroupTargetPosition(UnitGroup ug);