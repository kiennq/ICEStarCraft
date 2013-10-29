if (Broodwar->getFrameCount()%8 == 0 && this->mental->goAttack == false && this->mental->enemyInSight.size() > 0 && Broodwar->enemy()->getRace() != Races::Terran)
{
	// marines go inside bunker
	for each (Unit* u in this->attackers(Marine))
	{
		if (!u->isCompleted() || !u->exists() || u->isLoaded())
		{
			continue;
		}

		Unit* nearestBunker = NULL;
		int minD = 999999999;
		for each (Unit* bunker in SelectAll(UnitTypes::Terran_Bunker))
		{
			if (bunker->isCompleted() && bunker->getLoadedUnits().size() < 4 && u->getPosition().getApproxDistance(bunker->getPosition()) < minD)
			{
				minD = u->getPosition().getApproxDistance(bunker->getPosition());
				nearestBunker = bunker;
			}
		}

		if (nearestBunker)
		{
			u->rightClick(nearestBunker);
		}
	}	

	this->atkTar.first = "Enemy_Army";
	this->atkTar.second = this->mental->enemyInSight(canAttack).getCenter();
	//if our army size is large enough, we should ignore small enemy group
	if (Broodwar->self()->supplyUsed()/2>=180)
	{
		//if enemy group is large, we should kill them first
		if (this->mental->enemyInSight.size()>=12)
		{
			for each(Unit* en in this->mental->enemyInSight)
			{
				//if visible enemy still far, then attack move
				if (!en||!en->exists())
					continue;
				else if (en->getPosition()==Positions::None || en->getPosition()==Positions::Invalid ||en->getPosition()==Positions::Unknown)
					continue;
				else if (Broodwar->isVisible(en->getTilePosition()) && !en->exists())
				{
					this->mental->enemyInSight.erase(en);
					break;
				}
				else if (!en->isVisible()||!en->isDetected())
					continue;
				else
					this->allUnitAttack(en,en->getPosition());
			}
		}
	}
	//else we should kill any enemy insight first
	else
	{
		//for enemy center
		//army center
		Position eCenter = this->mental->enemyInSight(canAttack).not(isBuilding,isFlyer,isLifted).getCenter();	
		//non-attack enemy
		Position backupECenter = this->mental->enemyInSight(isDetected).not(canAttack).getCenter();
		if (eCenter==Positions::None)
			eCenter = backupECenter;
		//for my center
		Position mCenter = Positions::None;
		if(this->groupCenter)
			mCenter = *this->groupCenter;
		else
			mCenter = this->attackers(canAttack).getCenter();
		//for dis
		int centerDis = mCenter.getApproxDistance(eCenter);
		//if our military power is stronger
		if ((this->mInfo->myFightingValue().first >= this->eInfo->enemyFightingValue().first) && Broodwar->self()->supplyUsed()/2>=80)
		{
			//if we are still far from eCenter, or enemy doesn't have can-attack unit
			//then move attack to that position
			if (centerDis >= 32*18 || eCenter == backupECenter)
			{
				for each(Unit* en in this->mental->enemyInSight)
				{
					//if visible enemy still far, then attack move
					if (!en||!en->exists())
						continue;
					else if (en->getPosition()==Positions::None || en->getPosition()==Positions::Invalid ||en->getPosition()==Positions::Unknown)
						continue;
					else if (Broodwar->isVisible(en->getTilePosition()) && !en->exists())
					{
						this->mental->enemyInSight.erase(en);
						break;
					}
					else if (!en->isVisible()||!en->isDetected())
						continue;
					else
						this->allUnitAttack(en,en->getPosition());
				}
			}
			else
			{
				//_T_
				// for tank
				for each (Unit* tk in this->attackers(Siege_Tank))
				{
					tankAttackMode(tk,eCenter);
				}

				// for other units
				for each(Unit* u in this->attackers.not(Siege_Tank))
				{
					keepFormationAttack(u);
				}
			}
		}
		else
		{
			Position spot = Position((this->attackers.getCenter().x()+this->mental->enemyInSight.getCenter().x())/2,(this->attackers.getCenter().y()+this->mental->enemyInSight.getCenter().y())/2);
			//if we are near best gathering point
			if (this->attackers.getCenter().getApproxDistance(this->setPoint)<=32*10)
			{
				//if enemy still far
				if (this->mental->enemyInSight.getCenter().getApproxDistance(BWTA::getStartLocation(Broodwar->self())->getPosition())>=32*35)
				{
					//_T_
					// for tank
					for each(Unit* tk in this->attackers(Siege_Tank))
					{
						tankAttackMode(tk,this->setPoint);
					}

					// for other units
					for each(Unit* u in this->attackers.not(Siege_Tank))
					{
						if (u->getPosition().getApproxDistance(this->setPoint)<=32*4)
						{
							if(Unit* bt = getBestAttackTartget(u))
								u->attack(bt);
							else
								u->attack(this->setPoint);
						}
						else
						{
							if(Unit* bt = getBestAttackTartget(u))
								u->attack(bt);
							else
								u->attack(this->setPoint);
						}					
					}
				}
				else
				{
					for each(Unit* en in this->mental->enemyInSight)
					{
						//if visible enemy still far, then attack move
						if (!en||!en->exists())
							continue;
						else if (en->getPosition()==Positions::None || en->getPosition()==Positions::Invalid ||en->getPosition()==Positions::Unknown)
							continue;
						else if (Broodwar->isVisible(en->getTilePosition()) && !en->exists()){
							this->mental->enemyInSight.erase(en);
							break;
						}
						else if (!en->isVisible()||!en->isDetected())
							continue;
						else
							this->allUnitAttack(en,en->getPosition());
					}
				}
			}
			else
			{	
				//if we are near base,then don't need to move
				if (this->attackers.getCenter().getApproxDistance(BWTA::getStartLocation(Broodwar->self())->getPosition())<=32*50)
				{
					if (this->mental->enemyInSight(canAttack).getCenter().getApproxDistance(BWTA::getStartLocation(Broodwar->self())->getPosition())>=32*40)
					{
						//_T_
						for each(Unit* tk in this->attackers(Siege_Tank))
						{
							tankAttackMode(tk,this->setPoint);
						}

						//other units
						for each(Unit* u in this->attackers.not(Siege_Tank))
						{
							if (u->getPosition().getApproxDistance(this->setPoint)<=32*4){
								if(Unit* bt = getBestAttackTartget(u))
									u->attack(bt);
								else
									u->attack(this->setPoint);
							}
							else{
								if(Unit* bt = getBestAttackTartget(u))
									u->attack(bt);
								else
									u->attack(this->setPoint);
							}					
						}
						//for sieged tank
						for each(Unit* tk in this->attackers(Siege_Tank)(isSieged)){
							if (tk->getPosition().getApproxDistance(this->setPoint)<=32*9)
								continue;
							else
								tk->unsiege();		
						}
					}
					else
					{
						for each(Unit* en in this->mental->enemyInSight){
							//if visible enemy still far, then attack move
							if (!en||!en->exists())
								continue;
							else if (en->getPosition()==Positions::None || en->getPosition()==Positions::Invalid ||en->getPosition()==Positions::Unknown)
								continue;
							else if (Broodwar->isVisible(en->getTilePosition()) && !en->exists()){
								this->mental->enemyInSight.erase(en);
								break;
							}
							else if (!en->isVisible()||!en->isDetected())
								continue;
							else
								this->allUnitAttack(en,en->getPosition());
						}
					}						
				}
				//if our army is too far from home, and weaker than enemy 
				else
				{						
					//if we can find any closest enemy expansion, then attack it
					if (!this->eInfo->eBuildingPositionMap.empty())
					{
						int dis = 0;
						Position attackP=Positions::None;
						Unit* attackTar = NULL;
						for(std::map<Unit*,std::pair<UnitType,Position>>::const_iterator i = this->eInfo->eBuildingPositionMap.begin();i != this->eInfo->eBuildingPositionMap.end();i++)
						{
							if (i->second.first.isResourceDepot()){
								if (dis==0 || i->first->getPosition().getApproxDistance(this->attackers.getCenter())<dis){
									dis = i->first->getPosition().getApproxDistance(this->attackers.getCenter());
									attackP = i->second.second;
									attackTar = i->first;
								}
							}
							else
								continue;
						}
						//go and attack
						if(attackTar&&attackP!=Positions::None)
							allUnitAttack(attackTar,attackP);

						else
						{
							//if there's no closest expansion , then go attack eMainBase
							if(this->scm->enemyStartLocation){
								//for tank
								for each(Unit* tk in this->attackers(Siege_Tank)){
									tankAttackMode(tk,this->scm->enemyStartLocation->getPosition());
								}
								//for rest
								for each(Unit* u in this->attackers.not(Siege_Tank)){
									if(Unit* bt = getBestAttackTartget(u))
										u->attack(bt);
									else
										u->attack(this->scm->enemyStartLocation->getPosition());
								}
							}
							//if we even don't know where is the eSL, then attack base to base
							else
							{
								for each(Unit* en in this->mental->enemyInSight){
									//Broodwar->printf("%s",en->getType().c_str());
									if (!en||!en->exists())
										continue;
									else if (en->getPosition()==Positions::None || en->getPosition()==Positions::Invalid ||en->getPosition()==Positions::Unknown)
										continue;
									else if (Broodwar->isVisible(en->getTilePosition()) && !en->exists()){
										this->mental->enemyInSight.erase(en);
										break;
									}
									else if (!en->isVisible()||!en->isDetected())
										continue;
									else
										this->allUnitAttack(en,en->getPosition());
								}
							}													
						}							
					}
					//if we can not find any enemy base 
					else
					{
						for each(Unit* en in this->mental->enemyInSight){
							//Broodwar->printf("%s",en->getType().c_str());
							if (!en||!en->exists())
								continue;
							else if (en->getPosition()==Positions::None || en->getPosition()==Positions::Invalid ||en->getPosition()==Positions::Unknown)
								continue;
							else if (Broodwar->isVisible(en->getTilePosition()) && !en->exists()){
								this->mental->enemyInSight.erase(en);
								break;
							}
							else if (!en->isVisible()||!en->isDetected())
								continue;
							else
								this->allUnitAttack(en,en->getPosition());
						}
					}
				}
			}
		}
	}			
}

if (Broodwar->getFrameCount()%8 == 0 && this->mental->goAttack == false && Broodwar->enemy()->getRace()==Races::Terran)
{
	//for sight range
	UnitGroup tmp = SelectAll()(isCompleted)(Barracks,Engineering_Bay)(isLifted);
	UnitGroup en ;
	//(canAttack)
	if (this->groupCenter)
		en = SelectAllEnemy().inRadius(32*25,*this->groupCenter).not(isLifted,isBuilding, isWorker);
	for each(Unit* liftU in tmp)
	{
		en += SelectAllEnemy().inRadius(liftU->getType().sightRange() + 32*6, liftU->getPosition())(canAttack).not(isFlyer, isBuilding, isWorker);
	}
	//en+=this->mental->enemyInSight;
	en=this->mental->enemyInSight;
	//for drawing
	this->atkTar.first = "Enemy_Army";
	this->atkTar.second = en(canAttack).getCenter();
	//if we see enemy
	if (!en.empty()&&this->groupCenter)
	{
		//if enemy still far , go for the enemy
		if (groupCenter->getApproxDistance(en.getCenter())>32*18)
		{
			for each(Unit* tk in attackers(Siege_Tank))
			{
				tankAttackMode(tk,en.getCenter());
			}

			for each(Unit* ou in attackers.not(Siege_Tank,Marine))
			{
				if (ou->isMoving()||ou->isAttacking())
					continue;
				else
					ou->attack(en.getCenter());
			}
		}
		//if close enough
		else
		{			
			//compare the fight value
			//&&en(Siege_Tank)(isSieged).size()<8 
			if (this->mInfo->myFightingValue().first >= this->eInfo->enemyFightingValue().first*5 &&
				this->eInfo->CountEunitNum(UnitTypes::Terran_Siege_Tank_Siege_Mode) < 8 &&
				this->mInfo->myDeadArmy<12){
					for each(Unit* tk in attackers(Siege_Tank))
					{
						tankAttackMode(tk,en.getCenter());
					}

					for each(Unit* ou in attackers.not(Siege_Tank,Marine))
					{
						if (ou->isMoving()||ou->isAttacking())
							continue;
						else
							ou->attack(en.getCenter());
					}
			}
			else
			{
				int deadArmy = Broodwar->self()->deadUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode)+Broodwar->self()->deadUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)+Broodwar->self()->deadUnitCount(UnitTypes::Terran_Goliath);
				if (this->eInfo->CountEunitNum(UnitTypes::Terran_Siege_Tank_Siege_Mode)<4 && deadArmy<=8){
					for each(Unit* ene in en)
					{
						UnitGroup surroundTank = SelectAllEnemy()(isSieged).inRadius(32*6,ene->getPosition());
						if (ene->getType()==UnitTypes::Terran_Siege_Tank_Siege_Mode)
							continue;
						else if (surroundTank.size()>=1)
							continue;
						else
							allUnitAttack(ene,ene->getPosition());								
					}
				}
				else
				{
					if (groupCenter)
					{
						//_T_
						//for tank
						for each(Unit* tk in attackers(Siege_Tank))
						{
							tankAttackMode(tk,*groupCenter);
						}

						//for other unit
						for each(Unit* ou in attackers.not(Siege_Tank,Marine)){
							if (ou->getPosition().getApproxDistance(*groupCenter)>32*4)
								ou->attack(*groupCenter);
							else{
								if (ou->isHoldingPosition())
									continue;
								else
									ou->holdPosition();
							}								
						}
					}
				}												
			}
		}
	}
}