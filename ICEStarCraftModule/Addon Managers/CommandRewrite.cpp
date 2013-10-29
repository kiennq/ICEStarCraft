#include "CommandRewrite.h"
#include "time.h"

issueOnce::issueOnce()
{
	this->mLastIssueTime = 0;
	this->mLastAttackPosition = BWAPI::Positions::None;
	this->mLastMovePosition = BWAPI::Positions::None;
	this->mLastTarget = NULL;
	this->lastMinePosition = BWAPI::Positions::None;
}
void issueOnce::attackMoveOnce(Unit* u,Position p, int accuracy)
{
	if (u->exists()){
		Position targetPosition = p;
		int dis = u->getDistance(p);

		if(u->getOrder() == Orders::AttackMove){
			if (dis < accuracy)
				return;
		}
		if (u->getLastCommand().getType()==UnitCommandTypes::Attack_Move){
			if(mLastAttackPosition ==targetPosition)
				return;
		}

		if(u->isCompleted()){
			u->attack(targetPosition);
			mLastIssueTime = Broodwar->getFrameCount()+Broodwar->getRemainingLatencyFrames();
			mLastAttackPosition = targetPosition;
			return;
		}
}
}

void issueOnce::attackTargetOnce(Unit* u,Unit* target)
{
	if(u->exists()&&target->exists()){
		Unit* mTarget = target;
	
		if(u->getOrder()==Orders::AttackTile||u->getOrder()==Orders::AttackTile)
			return;
		if(u->getLastCommand().getType()==UnitCommandTypes::Attack_Unit){
			if(target==mLastTarget)
				return;
		}
		if(u->isCompleted()){
			u->attack(mTarget);
			mLastIssueTime = Broodwar->getFrameCount()+Broodwar->getRemainingLatencyFrames();
			mLastTarget = mTarget;
			return;
		}
	}
}

void issueOnce::moveOnce(Unit* u,Position p,int accuracy)
{
	if (u->exists()){
		Position targetPosition = p;
		int dis = u->getDistance(p);

		if(u->getOrder() == Orders::Move){
			if (dis < accuracy)
				return;
		}
		if (u->getLastCommand().getType()==UnitCommandTypes::Move){
			if(mLastMovePosition==targetPosition)
				return;
		}

		if(u->isCompleted()){
			u->move(targetPosition);
			mLastIssueTime = Broodwar->getFrameCount()+Broodwar->getRemainingLatencyFrames();
			mLastMovePosition = targetPosition;
			return;
		}

}
}
void issueOnce::stopOnce(Unit* u)
{
	if (u->exists()){
		if (u->getOrder()==Orders::Stop)
			return;
		if (u->getLastCommand().getType()==UnitCommandTypes::Stop)
			return;
		if (u->isCompleted()){
			u->stop();
			mLastIssueTime = Broodwar->getFrameCount()+Broodwar->getRemainingLatencyFrames();
			return;
		}
	}
}

void issueOnce::holdOnce(Unit* u)
{
	if (u->exists()){
		if (u->getOrder()==Orders::HoldPosition)
			return;
		if (u->getLastCommand().getType()==UnitCommandTypes::Hold_Position)
			return;
		if (u->isCompleted()){
			u->holdPosition();
			mLastIssueTime = Broodwar->getFrameCount()+Broodwar->getRemainingLatencyFrames();
			return;
		}
	}
}

void issueOnce::patrolOnce(Unit* u, Position p)
{
	if(u->exists()){
		Position patrolPosition = p;
		if (u->getOrder()==Orders::Patrol)
			return;
		if(u->getLastCommand().getType()==UnitCommandTypes::Patrol){
			if (patrolPosition == mLastPatrolPosition)
				return;
		}
		if (u->isCompleted()){
			u->patrol(patrolPosition);
			mLastIssueTime = Broodwar->getFrameCount()+Broodwar->getRemainingLatencyFrames();
			mLastPatrolPosition = patrolPosition;
			return;
		}
	}
}
void issueOnce::repairOnce(Unit* u,Unit* target)
{
	if (u->exists() && target->exists()){
		Unit* repairTarget = target;
		if (u->getOrder()==Orders::Repair){
			return;
		}
		if (u->getLastCommand().getType()==UnitCommandTypes::Repair){
			if (repairTarget==mLastRepairTarget)
				return;
		}
		if (u->isCompleted()){
			u->repair(repairTarget);
			mLastRepairTarget = repairTarget;
			mLastIssueTime = Broodwar->getFrameCount()+Broodwar->getRemainingLatencyFrames();
			return;
		}		
	}
}

void issueOnce::healMoveOnce(Unit* u,Position p,int accuracy)
{
	if (u->exists()&& u->getType()==UnitTypes::Terran_Medic){
		Position targetPosition = p;
		int dis = u->getDistance(p);
		if(u->getOrder() == Orders::HealMove){
			if (dis < accuracy)
				return;
		}
		if (u->getLastCommand().getType()==UnitCommandTypes::Attack_Move ||u->getLastCommand().getType()==UnitCommandTypes::Move){
			if(mLastHealMovePosition ==targetPosition)
				return;
		}

		if(u->isCompleted()){
			u->useTech(TechTypes::Healing,targetPosition);
			mLastIssueTime = Broodwar->getFrameCount()+Broodwar->getRemainingLatencyFrames();
			mLastHealMovePosition = targetPosition;
			return;
		}
	}
}

void issueOnce::followOnce(Unit* u,Unit* target)
{
	if (u->exists() && target->exists()){
		Unit* followTarget = target;
		if (u->getOrder()==Orders::Follow){
			return;
		}
		if (u->getLastCommand().getType()==UnitCommandTypes::Follow){
			if (followTarget==mLastFollowTarget)
				return;
		}
		if (u->isCompleted()){
			u->follow(followTarget);
			mLastFollowTarget = followTarget;
			mLastIssueTime = Broodwar->getFrameCount()+Broodwar->getRemainingLatencyFrames();
			return;
		}		
	}
}

void issueOnce::stuckSolution(Unit* u)
{
	if (u->exists()){
		if (u->isStuck()){
			stopOnce(u);
			return;
		}
		else
			return;
	}
}

void issueOnce::siegeOnce(Unit*u,Position p)
{ 
	if (u->exists()){
		int dis = u->getDistance(p);
		if (u->getType()==UnitTypes::Terran_Siege_Tank_Tank_Mode && Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode)){
			time_t t;
			srand((unsigned)time(&t)); // time seed	
			int rangeX = 20+rand()%(70-20+1);// generate a random number between 50-65
			srand((unsigned)time(&t)); 
			int rangeY = 20+rand()%(70-20+1);
			Position* randomPosition = new Position(rangeX,rangeY);
			if((u->getLastCommand().getType() == UnitCommandTypes::Siege))
				//&&u->isSieged()
				//|| u->getLastCommand().getType() == UnitCommandTypes::Move
				return;
			else if(u->isCompleted()){
				if (dis>144){
					moveOnce(u,p+*randomPosition,40);
				}
				else{
					u->siege();
					return;
				}
					
			}			
		}
	}
}

void issueOnce::tankMoveHold(Unit* u)
{
	if (u->exists()&&u->getType()==UnitTypes::Terran_Siege_Tank_Tank_Mode){
		time_t t;
		srand((unsigned)time(&t)); // time seed	
		int rangeX = 32+rand()%(64-32+1);// generate a random number between 50-65
		srand((unsigned)time(&t)); 
		int rangeY = 32+rand()%(64-32+1);
		Position* randomPosition = new Position(rangeX,rangeY);
		if (u->getGroundWeaponCooldown()!=0){
			moveOnce(u,*randomPosition,50);
		}
		else{
			holdOnce(u);
		}
}
}


Position unitMoveLeft(Unit*u)
{
	return Position(u->getPosition().x()-32*4,u->getPosition().y());
}
Position unitMoveRight(Unit*u)
{
	return Position(u->getPosition().x()+32*4,u->getPosition().y());
}
Position unitMoveUp(Unit*u)
{
	return Position(u->getPosition().x(),u->getPosition().y()-32*4);
}
Position unitMoveDown(Unit*u)
{
	return Position(u->getPosition().x(),u->getPosition().y()+32*4);
}
Position unitMoveTopLeft(Unit*u)
{
	return Position(u->getPosition().x()-32*4,u->getPosition().y()-32*4);
}
Position unitMoveTopRight(Unit*u)
{
	return Position(u->getPosition().x()+32*4,u->getPosition().y()-32*4);
}
Position unitMoveBottomLeft(Unit*u)
{
	return Position(u->getPosition().x()-32*4,u->getPosition().y()+32*4);
}
Position unitMoveBottomRight(Unit*u)
{
	return Position(u->getPosition().x()+32*4,u->getPosition().y()+32*4);
}

Position unitNextRunningPosition(Unit*u)
{ 
	std::set<Unit*> MineralGroup = Broodwar->getAllUnits();
	Unit* m=NULL;
	for each(Unit* u in MineralGroup){
		if(u->getType()==UnitTypes::Resource_Mineral_Field && u->isVisible()){
			m=u;
			break;
		}
	}
	if (u->isStuck()){
		u->rightClick(m);
		return m->getPosition();
	}
		
	UnitGroup dangerousBuiliding = SelectAllEnemy()(isCompleted,isVisible)(Sunken_Colony,Ion_Cannon).inRadius(u->getType().sightRange()*2,u->getPosition());
	UnitGroup enemyAround = SelectAllEnemy()(isCompleted,isVisible,canAttack).not(isBuilding,Larva,Overlord,Egg).inRadius(u->getType().sightRange()*4,u->getPosition())+dangerousBuiliding;
	for each(Unit* eu in dangerousBuiliding){
		if(u->getPosition().getApproxDistance(eu->getPosition())<u->getType().sightRange()+48){
			u->rightClick(m);
			return m->getPosition();
		}
		
	}
	for each(Unit* eu in enemyAround){
		int dangerDis = eu->getType().groundWeapon().maxRange()*2;
		int see1 = eu->getPosition().getApproxDistance(u->getPosition());
		// && (eu->isAttacking()||eu->getTarget()==u||eu->getLastCommand().getTarget()==u)
		if(eu->getPosition().getApproxDistance(u->getPosition())<dangerDis){	
			if (eu->getPosition().getApproxDistance(u->getPosition())<=dangerDis/2+32){
				u->rightClick(m);
				u->rightClick(BWTA::getStartLocation(Broodwar->self())->getPosition());
				continue;
			}
			std::set<Position> mPosition;
			mPosition.clear();
			Position moveleft = unitMoveLeft(u);
			Position moveright = unitMoveRight(u);
			Position moveup = unitMoveUp(u);
			Position movedown = unitMoveDown(u);
			Position movetopleft = unitMoveTopLeft(u);
			Position movetopright = unitMoveTopRight(u);
			Position movebottomleft = unitMoveBottomLeft(u);
			Position movebottomright = unitMoveBottomRight(u);
			int currentDis = eu->getPosition().getApproxDistance(enemyAround.getCenter());
			Position bestPosition = BWAPI::Positions::None;
			int surroundUnit = 0;
			mPosition.insert(moveleft);
			mPosition.insert(moveright);
			mPosition.insert(moveup);
			mPosition.insert(movedown);
			mPosition.insert(movetopleft);
			mPosition.insert(movetopright);
			mPosition.insert(movebottomleft);
			mPosition.insert(movebottomright);
			for each(Position po in mPosition){
				//int predictSize = SelectAllEnemy()(isCompleted).not(isBuilding).inRadius(u->getType().sightRange()*4,u->getPosition()).size();
				BWAPI::Broodwar->drawCircleMap(po.x(), po.y(), 16,Colors::Blue);
				//Broodwar->isWalkable(po.x(),po.y())
				//&& Broodwar->isWalkable(po.x(),po.y()) && predictSize<surroundUnit
				// &&Broodwar->getUnitsOnTile(po.x()/32,po.y()/32).size()==0 
				if (po.getApproxDistance(enemyAround.getCenter())>currentDis&&isTileWalkable((TilePosition)po)&&
					Broodwar->getUnitsOnTile(po.x()/32,po.y()/32).size()==0){
					currentDis = po.getApproxDistance(eu->getPosition());
					bestPosition = po;
					continue;
					//surroundUnit = predictSize;
					//BWAPI::Broodwar->drawCircleMap(bestPosition.x(), bestPosition.y(), 16,Colors::Red);
					//return bestPosition;
				}
				else if (bestPosition == BWAPI::Positions::None || !Broodwar->isWalkable(bestPosition.x()/16,bestPosition.y()/16)){
					bestPosition = BWTA::getStartLocation(Broodwar->self())->getPosition();
					//Broodwar->printf("best position is home");
				}
					
			}
			//Broodwar->printf("can find best Position");
			BWAPI::Broodwar->drawCircleMap(bestPosition.x(), bestPosition.y(), 16,Colors::Red);
			return bestPosition;
		}
		else
			return BWTA::getStartLocation(Broodwar->self())->getPosition();
	}
	return BWAPI::Positions::None;
}


bool lastDangerFlag = false;
bool unitInDanger(Unit* u)
{
	if (u->exists()&&u->isCompleted()&&u->getPlayer()==Broodwar->self()&&!u->getType().isBuilding())
	{
		UnitGroup dangerousBuiliding = SelectAllEnemy()(isCompleted)(Sunken_Colony,Photon_Cannon).inRadius(u->getType().sightRange()*2,u->getPosition());
		UnitGroup enemyAround = SelectAllEnemy()(isCompleted)(isVisible)(canAttack).not(isBuilding,Larva,Overlord,Egg).inRadius(u->getType().sightRange()*2,u->getPosition())+dangerousBuiliding;
		for each(Unit* eu in dangerousBuiliding)
		{
			if(u->getPosition().getApproxDistance(eu->getPosition())<u->getType().sightRange()+48)
				return true;
		}
		for each(Unit* eu in enemyAround){
			int threatenUnitNum = 0;
			if(!eu->getType().canAttack()||eu->getType().isBuilding())
				continue;
			if(!eu->getType().isWorker()&&!eu->getType().isBuilding()&&!eu->getType().canAttack())
				threatenUnitNum++;
			int dangerousDis = eu->getType().groundWeapon().maxRange()+48;
			int see1 =eu->getPosition().getApproxDistance(u->getPosition());
			bool see2 = eu->isAttacking();
			//eu->getPosition().getApproxDistance(u->getPosition())<dangerousDis
			//(eu->getOrder()==Orders::AttackUnit||eu->getOrder()==Orders::AttackMove||eu->getTarget()==u||eu->isAttacking())
			//(eu->getLastCommand().getType()==UnitCommandTypes::Attack_Unit&&eu->getLastCommand().getTarget()==u)
			int see3 =Broodwar->getFrameCount()-eu->getLastCommandFrame(); 
			if(eu->getPosition().getApproxDistance(u->getPosition())<dangerousDis && (eu->isAttacking()||u->isUnderAttack()||eu->getTarget()==u)){
				lastDangerFlag =true;
				//Broodwar->printf("Dangerous,run!");
				return true;
			}	
			else if(threatenUnitNum >= 2){
				lastDangerFlag =true;
				return true;
			}

			else if(lastDangerFlag && SelectAllEnemy()(isCompleted).not(isBuilding).inRadius(u->getType().sightRange()*4,u->getPosition()).size()!=0){
				//Broodwar->printf("Dangerous,run!");
				return true;
			}
			
			else{
				lastDangerFlag=false;
				continue;
			}
		}
	}
	else{
		lastDangerFlag=false;
		return false;
	}
	lastDangerFlag=false;
	return false;
}


bool isTileWalkable(TilePosition location)
{
	return isTileWalkable(location.x(), location.y());
}

bool isTileWalkable(int x, int y)
{
	for(int nx = x * 4; nx < x * 4 + 4; ++nx)
	{
		for(int ny = y * 4; ny < y * 4 + 4; ++ny)
		{
			if(!BWAPI::Broodwar->isWalkable(nx, ny) || !BWAPI::Broodwar->getUnitsOnTile(x, y).empty())
				return false;
		}
	}

	return true;
}

bool isSurroundingWalkable(int x, int y)
{
	for(int nx = (x - 5*32); nx <= (x + 5*32); nx=nx+4)
	{
		for(int ny = (y - 5*32); ny <= (y + 5*32); ny=ny+4)
		{
			if(!BWAPI::Broodwar->isWalkable(nx/32, ny/32))
				return false;
		}
	}

	return true;
}



Position avoidEnemyAttackMove(Unit* u,Position finalgoal)
{
	 if(u->getPosition().getApproxDistance(finalgoal)<7*32)
		return BWTA::getStartLocation(Broodwar->self())->getPosition();	
	UnitGroup dangerousBuiliding = SelectAllEnemy()(isCompleted,isVisible)(Sunken_Colony,Ion_Cannon).inRadius(u->getType().sightRange()*2,u->getPosition());
	UnitGroup enemyAround = SelectAllEnemy()(isCompleted,isVisible,canAttack).not(isBuilding,Larva,Overlord,Egg).inRadius(u->getType().sightRange()*4,u->getPosition())+dangerousBuiliding;
	for each(Unit* eu in dangerousBuiliding){
		std::set<Position> mPosition;
		mPosition.clear();
		Position moveleft = unitMoveLeft(u);
		Position moveright = unitMoveRight(u);
		Position moveup = unitMoveUp(u);
		Position movedown = unitMoveDown(u);
		Position movetopleft = unitMoveTopLeft(u);
		Position movetopright = unitMoveTopRight(u);
		Position movebottomleft = unitMoveBottomLeft(u);
		Position movebottomright = unitMoveBottomRight(u);
		int currentDisToEnemy = u->getPosition().getApproxDistance(eu->getPosition());
		int currentDisToGoal = u->getPosition().getApproxDistance(finalgoal);
		Position bestPosition = BWAPI::Positions::None;
		int surroundUnit = 0;
		mPosition.insert(moveleft);
		mPosition.insert(moveright);
		mPosition.insert(moveup);
		mPosition.insert(movedown);
		mPosition.insert(movetopleft);
		mPosition.insert(movetopright);
		mPosition.insert(movebottomleft);
		mPosition.insert(movebottomright);
	//	isTileWalkable((TilePosition)po)
		for each(Position po in mPosition){
			BWAPI::Broodwar->drawCircleMap(po.x(), po.y(),13,Colors::Red);
			if(po.getApproxDistance(eu->getPosition())>eu->getType().groundWeapon().maxRange()+10&&po.getApproxDistance(finalgoal)<currentDisToGoal
				&&po.getApproxDistance(eu->getPosition())>currentDisToEnemy &&isSurroundingWalkable(po.x(),po.y())&& Broodwar->getUnitsInRadius(po,16).size()==0){
					currentDisToEnemy = po.getApproxDistance(eu->getPosition());
					bestPosition = po;
					continue;
			}
			else if (bestPosition == BWAPI::Positions::None || !isTileWalkable((TilePosition)po)){
				bestPosition = BWTA::getStartLocation(Broodwar->self())->getPosition();					
			}
		}
		BWAPI::Broodwar->drawCircleMap(bestPosition.x(), bestPosition.y(),13,Colors::Green);
		return bestPosition;
	}

	//for army

	Unit* nearestEnemy = NULL;
	int currentDisToEnemy = 0; 
	int enemyAttackRange = 0;
	for each(Unit* eu in enemyAround){
		if(currentDisToEnemy==0 || u->getPosition().getApproxDistance(eu->getPosition())<currentDisToEnemy){
			nearestEnemy = eu;
			currentDisToEnemy = u->getPosition().getApproxDistance(eu->getPosition());
			enemyAttackRange = eu->getType().groundWeapon().maxRange()+12;
		}
	}
	std::set<Position> mPosition;
	mPosition.clear();
	Position moveleft = unitMoveLeft(u);
	Position moveright = unitMoveRight(u);
	Position moveup = unitMoveUp(u);
	Position movedown = unitMoveDown(u);
	Position movetopleft = unitMoveTopLeft(u);
	Position movetopright = unitMoveTopRight(u);
	Position movebottomleft = unitMoveBottomLeft(u);
	Position movebottomright = unitMoveBottomRight(u);

	int currentDisToGoal = u->getPosition().getApproxDistance(finalgoal);
	Position bestPosition = BWAPI::Positions::None;
	int surroundUnit = 0;
	mPosition.insert(moveleft);
	mPosition.insert(moveright);
	mPosition.insert(moveup);
	mPosition.insert(movedown);
	mPosition.insert(movetopleft);
	mPosition.insert(movetopright);
	mPosition.insert(movebottomleft);
	mPosition.insert(movebottomright);

	for each(Position po in mPosition){
		BWAPI::Broodwar->drawCircleMap(po.x(), po.y(),13,Colors::Red);
		if(po.getApproxDistance(nearestEnemy->getPosition()) > enemyAttackRange && (po.getApproxDistance(finalgoal)<currentDisToGoal)
			&& po.getApproxDistance(nearestEnemy->getPosition())>currentDisToEnemy &&isSurroundingWalkable(po.x(),po.y())){
				currentDisToEnemy = po.getApproxDistance(nearestEnemy->getPosition());
				currentDisToGoal = po.getApproxDistance(finalgoal);
				bestPosition = po;
				continue;
		}
		else if (bestPosition == BWAPI::Positions::None || !isTileWalkable((TilePosition)po)){
			bestPosition = BWTA::getStartLocation(Broodwar->self())->getPosition();	
		}
	
	}
	BWAPI::Broodwar->drawCircleMap(bestPosition.x(), bestPosition.y(),13,Colors::Green);
	return bestPosition;
}



void issueOnce::vultureMine(Unit* u, Position p1)
{
	if (u->getPlayer()==Broodwar->self()&&u->getType()==UnitTypes::Terran_Vulture){
		if (Broodwar->self()->hasResearched(TechTypes::Spider_Mines)&&u->getSpiderMineCount()>0){
			time_t t;
			srand((unsigned)time(&t)); // time seed	
			int rangeX = -32*2+rand()%(32*2-(-32*2)+1);// generate a random number between 50-65
			srand((unsigned)time(&t)); 
			int rangeY = -32*2+rand()%(32*2-(-32*2)+1);
			if(this->lastMinePosition==BWAPI::Positions::None)
				this->lastMinePosition = p1;
			if (this->lastMinePosition.getApproxDistance(p1)>32*6)
				this->lastMinePosition = p1;
			Position* randomPosition = new Position(this->lastMinePosition.x()+rangeX,this->lastMinePosition.y()+rangeY);
			if(isTileWalkable(randomPosition->x()/32,randomPosition->y()/32)){
				u->useTech(TechTypes::Spider_Mines,*randomPosition);	
				this->lastMinePosition = *randomPosition;
			}
			else{
				u->useTech(TechTypes::Spider_Mines,u->getPosition());	
				this->lastMinePosition = p1;
			}				
		}
	}
}


//void vultureAttackMode(Unit* u)
//{
//	if (u->getType()==UnitTypes::Terran_Vulture){
//		UnitGroup Punits= SelectAllEnemy()(isCompleted)(Zergling,Ultralisk,Infested_Terran,Drone,Zealot,Dark_Templar,High_Templar,Archon,Marine,Firebat,Ghost,SCV,Probe).inRadius(u->getType().sightRange(),u->getPosition());
//		UnitGroup Aunits= SelectAllEnemy()(isCompleted,canAttack).not(isBuilding,isFlyer) - Punits;
//		
//	}
//
//
//}

bool enemyInSCVSightRange(Unit* u)
{
	bool r = false;
	int sightRange = UnitTypes::Terran_SCV.sightRange()+5;
	std::set<Unit*> unitInRange =Broodwar->getUnitsInRadius(u->getPosition(),sightRange*2);
	for each(Unit* e in unitInRange){
		if (Broodwar->self()->isEnemy(e->getPlayer()) && e->getType().canAttack()){
			if (!e->getType().isWorker())
				return true;
			else{
				if(e->getTarget()==u)
					return true;
			}
		}
		else if(e->getType()==UnitTypes::Protoss_Photon_Cannon || e->getType()==UnitTypes::Zerg_Sunken_Colony){
			return true;
		}
	}
	return false;
}


Unit* getBestAttackTartget(Unit* myFighter)
{
	int surroundingEnemy=0;
	double MaxValue=0;
	Unit* bestTarget = NULL;
	Position mPosition = myFighter->getPosition();
	//first check if this unit can attack flyer
	if (myFighter->getType().airWeapon()!=WeaponTypes::None)
	{
		double attackRange = myFighter->getType().groundWeapon().maxRange() > myFighter->getType().airWeapon().maxRange() ? myFighter->getType().groundWeapon().maxRange():myFighter->getType().airWeapon().maxRange();////*1.3
		UnitGroup enemyInRange = SelectAllEnemy()(isDetected).inRadius(attackRange,mPosition);
		// _T_ 29/6
		// if there are enemy units and buildings then this unit should attack only enemy units
		if (enemyInRange(canAttack).size() > 0)
		{
			enemyInRange -= enemyInRange.not(canAttack);
		}

		for each(Unit* u in enemyInRange){
			double attackValue = 0;
			if (u->getType()==UnitTypes::Protoss_High_Templar || u->getType()==UnitTypes::Zerg_Defiler 
				|| u->getType()==UnitTypes::Zerg_Lurker || u->getType()==UnitTypes::Protoss_Reaver ||u->getType()==UnitTypes::Terran_Siege_Tank_Tank_Mode
				||u->getType()==UnitTypes::Terran_Siege_Tank_Siege_Mode || u->getType()==UnitTypes::Protoss_Carrier){
					bestTarget = u;
					//if (bestTarget)
					//{
					//	Broodwar->drawLineMap(mPosition.x(),mPosition.y(),bestTarget->getPosition().x(),bestTarget->getPosition().y(),Colors::Red);
					//	Broodwar->printf("%s ---> %s",myFighter->getType().getName().c_str(),bestTarget->getType().getName().c_str());
					//}
					return bestTarget;
			}
			else if (u->getType().canAttack()){
				if(u->exists()){
					double hp = u->getHitPoints()+u->getShields();
					double dpf = u->getType().groundWeapon().damageAmount()*1.0/u->getType().groundWeapon().damageCooldown();
					double price = u->getType().mineralPrice()+u->getType().gasPrice();
					//dpf 50% , HP  35%  ,price 15%
					attackValue = dpf*0.5*price*0.15/hp*0.35;
				}												
				else
					attackValue = 0;
				if (MaxValue==0 || attackValue>MaxValue){
					MaxValue = attackValue;
					bestTarget = u;
				}
			}

			else if (!u->getType().canAttack()){
				if(u->exists()){
					double hp = u->getHitPoints()+u->getShields();
					double price = u->getType().mineralPrice()+u->getType().gasPrice();
					//HP  60%  ,price 40%
					attackValue = price*0.4/hp*0.6*0.5;
				}												
				else
					attackValue = 0;

				if (MaxValue==0 || attackValue>MaxValue){
					MaxValue = attackValue;
					bestTarget = u;
				}
			}
		}
		//if (bestTarget)
		//{
		//	Broodwar->drawLineMap(mPosition.x(),mPosition.y(),bestTarget->getPosition().x(),bestTarget->getPosition().y(),Colors::Red);
		//	Broodwar->printf("%s ---> %s",myFighter->getType().getName().c_str(),bestTarget->getType().getName().c_str());
		//}
		return bestTarget;
	}
	else
	{
		double attackRange = myFighter->getType().groundWeapon().maxRange();//*1.3
		UnitGroup enemyInRange = SelectAllEnemy()(isDetected).not(isFlyer).inRadius(attackRange,mPosition);
		// _T_ 29/6
		// if there are enemy units and buildings then this unit should attack only enemy units
		if (enemyInRange(canAttack).size() > 0)
		{
			enemyInRange -= enemyInRange.not(canAttack);
		}


		for each(Unit* u in enemyInRange){
			double attackValue = 0;
			if (u->getType()==UnitTypes::Protoss_High_Templar || u->getType()==UnitTypes::Zerg_Defiler 
				|| u->getType()==UnitTypes::Zerg_Lurker || u->getType()==UnitTypes::Protoss_Reaver ||u->getType()==UnitTypes::Terran_Siege_Tank_Tank_Mode
				||u->getType()==UnitTypes::Terran_Siege_Tank_Siege_Mode){
					bestTarget = u;
					//if (bestTarget)
					//{
					//	Broodwar->drawLineMap(mPosition.x(),mPosition.y(),bestTarget->getPosition().x(),bestTarget->getPosition().y(),Colors::Red);
					//	Broodwar->printf("%s ---> %s",myFighter->getType().getName().c_str(),bestTarget->getType().getName().c_str());
					//}
					return bestTarget;
			}
			else if (u->getType().canAttack()){
				if(u->exists()){
					double hp = u->getHitPoints()+u->getShields();
					double dpf = u->getType().groundWeapon().damageAmount()*1.0/u->getType().groundWeapon().damageCooldown();
					double price = u->getType().mineralPrice()+u->getType().gasPrice();
					//HP  60%  ,price 40%, if can not attack then value/2
					attackValue = dpf*0.5*price*0.15/hp*0.35;
				}												
				else
					attackValue = 0;
				if (MaxValue==0 || attackValue>MaxValue){
					MaxValue = attackValue;
					bestTarget = u;
				}
			}

			else if (!u->getType().canAttack()){
				if(u->exists()){
					double hp = u->getHitPoints()+u->getShields();
					double price = u->getType().mineralPrice()+u->getType().gasPrice();
					//HP  60%  ,price 40%, if can not attack then value/2
					attackValue = price*0.4/hp*0.6*0.5;
				}												
				else
					attackValue = 0;

				if (MaxValue==0 || attackValue>MaxValue){
					MaxValue = attackValue;
					bestTarget = u;
				}
			}
		}
		//if (bestTarget)
		//{
		//	Broodwar->drawLineMap(mPosition.x(),mPosition.y(),bestTarget->getPosition().x(),bestTarget->getPosition().y(),Colors::Red);
		//	Broodwar->printf("%s ---> %s",myFighter->getType().getName().c_str(),bestTarget->getType().getName().c_str());
		//}
		return bestTarget;
	}
}

// Run away
Position Helper::isUnderInvAtk( Unit *u )
{ 
	UnitGroup ug = SelectAllEnemy().not(isDetected).inRadius(UnitTypes::Zerg_Lurker.groundWeapon().maxRange()+32*8, u->getPosition())(canAttack);
	Position center(ug.getCenter());
	Position cur(u->getPosition());
	int runDist = 0;
	Unit* cl = NULL;
	for each (Unit* unit in ug) {		
		int dis = unit->getType().groundWeapon().maxRange() - u->getPosition().getApproxDistance(unit->getPosition());
		if (dis > runDist) {
			runDist = dis;
			cl = unit;
		}
	}
	if (cl) {
		double dis = center.getApproxDistance(cur);
		double scaleX = (-center.x() + cur.x()*1.0)/dis;
		double scaleY = (-center.y() + cur.y()*1.0)/dis;
		double dScaleX = scaleX*32;
		double dScaleY = scaleY*32;
		double diffX = scaleX*runDist + dScaleX*6;
		double diffY = scaleY*runDist + dScaleY*6;
		int count = 8;
		while (!isTileWalkable((int)(cur.x() + diffX)/32, (int)(cur.y() + diffY)/32)) {
			diffX += dScaleX;
			diffY += dScaleY;
			if ((--count)==0) break;
		} 
		return Position(cur.x() + (int)diffX, cur.y() + (int)diffY).makeValid();
	}
	return Positions::None;
}



BWAPI::Position getGroupTargetPosition(UnitGroup ug)
{
	int groupTarX = 0;
	int groupTarY = 0;
	int gSize = ug.size();
	if (gSize>0){
		for each(Unit* u in ug){
			Position Tar = u->getTargetPosition();
			if (Tar==Positions::None)
				continue;
			else{
				groupTarX += Tar.x();
				groupTarY += Tar.y();
			}
		}

		return Position(groupTarX/gSize,groupTarY/gSize);
	}
	else{
		Broodwar->printf("empty unit group, can not get target position!");
		return Positions::None;
	}
}