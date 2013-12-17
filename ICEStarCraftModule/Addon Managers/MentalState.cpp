#include "MentalState.h"
#include <time.h>

using namespace BWAPI;
using namespace BWTA;
using namespace std;

#ifndef _BATTLE_DEBUG
#define _BATTLE_DEBUG
#endif

MentalClass *theMentalManager = NULL;

MentalClass* MentalClass::create()
{
	if (theMentalManager) return theMentalManager;
	else return theMentalManager = new MentalClass();
}

MentalClass::MentalClass()
{
	gf = GameFlow::create();
	myInfo = NULL;
	enemyInfo = NULL;
	STflag = NotSure;
	terrainManager = NULL;
	worker = NULL;
	openCheckFlag = false;
	reactionFinish=false;
	prushTimer1 =0 ;
	mUnderAttack = false;
	emergencyFlag = false;
	baseSightRangeLimitation = 50;
	enemyInSight.clear();
	goAttack = false;
	tpTiming = TPTiming::create();
	upgradeMng = NULL;
	techMng=NULL;
	marineRushOver = false;
}
void MentalClass::setManagers(BuildOrderManager* b,UpgradeManager* up,TechManager* tech)
{
	bom = b;
	myInfo = MyInfoManager::create();
	enemyInfo =EnemyInfoManager::create();
	scm =ScoutManager::create();
	macroManager = MacroManager::create();
	terrainManager = TerrainManager::create();
	worker = WorkerManager::create();
	upgradeMng = up;
	techMng =tech;
}

void MentalClass::onFrame()
{
	setEnemyPlanFlag();
	counterMeasure();
	updateSightRange();
	attackTimingCheck();
	baseUnderAttack();
	tpTiming->CheckTiming();
}

void MentalClass::setEnemyPlanFlag()
{
	//if(STflag != NotSure)
	if(STflag != NotSure && STflag != PrushDragoon)
		//if(Broodwar->getFrameCount()>24*60*5)
		openCheckFlag = true;

	//enemy is protoss
	if (Broodwar->enemy()->getRace() == Races::Protoss && !openCheckFlag && Broodwar->getFrameCount()<24*60*7)
	{
		int ba = 0;
		int bg = 0;
		int bn = 0;
		int bp = 0;
		int by = 0;
		int bf = 0;
		int zlt = 0;
		int dr = 0;
		int vc = 0;
		int vt = 0;
		int vr = 0;
		int vb = 0;
		int bc = 0;
		bool isMainBase=false;
		bg = enemyInfo->CountEunitNum(UnitTypes::Protoss_Gateway);
		ba = enemyInfo->CountEunitNum(UnitTypes::Protoss_Assimilator);
		bp = enemyInfo->CountEunitNum(UnitTypes::Protoss_Pylon);
		bn = enemyInfo->CountEunitNum(UnitTypes::Protoss_Nexus);
		by = enemyInfo->CountEunitNum(UnitTypes::Protoss_Cybernetics_Core);
		bf = enemyInfo->CountEunitNum(UnitTypes::Protoss_Forge);
		bc = enemyInfo->CountEunitNum(UnitTypes::Protoss_Photon_Cannon);
		zlt = enemyInfo->CountEunitNum(UnitTypes::Protoss_Zealot);
		dr = enemyInfo->CountEunitNum(UnitTypes::Protoss_Dragoon);
		vc = enemyInfo->CountEunitNum(UnitTypes::Protoss_Citadel_of_Adun);
		vt = enemyInfo->CountEunitNum(UnitTypes::Protoss_Templar_Archives);
		vr = enemyInfo->CountEunitNum(UnitTypes::Protoss_Robotics_Facility);
		vb = enemyInfo->CountEunitNum(UnitTypes::Protoss_Robotics_Support_Bay);
		
		if (bn<=1)
		{
			std::map<Unit*,EnemyInfoManager::eBaseData> eNexusMap = enemyInfo->getEnemyBaseMap();
			for (std::map<Unit*,EnemyInfoManager::eBaseData>::iterator i=eNexusMap.begin();i!=eNexusMap.end();i++){
				if(i->second.isStartBase)
					isMainBase = true;
				else
					isMainBase = false;
			}
		}

		if(vc>0 ||vt>0)
		{
			STflag = PtechDK;
			//Broodwar->printf("see vc or vt");
		}

		if ((bg>=2 && isMainBase && bc==0 && ba==0 && by==0)||(zlt>=2&&ba==0&&by==0&& isMainBase && bc==0)) 
			STflag = PrushZealot;

		if ((((bg>=2 && by==1 && isMainBase)||(dr>=2&&isMainBase)||enemyInfo->drRangeUpgradeFlag) && bn<2 && vc==0) ||dr>=3)
		{
			STflag = PrushDragoon;
		}
		if(bg==1&&!enemyInfo->drRangeUpgradeFlag && vr>0 && bn==1)
			STflag = PtechReaver;	

		if((bn>=2 && Broodwar->getFrameCount()<=24*60*5)||(bn==1 && !isMainBase && Broodwar->getFrameCount()<=24*60*5))
			if (STflag != PtechCarrier)
			{
				STflag = P2Base;
			}

		if (Broodwar->getFrameCount() <= 24*60*4 && bf > 0 && bc >= 2)
			STflag = PtechCarrier;

		if (vb==1 ||(bg==1&&vr==1&&!enemyInfo->drRangeUpgradeFlag))
			STflag = PtechReaver;

	}
	// enemy is terran
	if (Broodwar->enemy()->getRace() == Races::Terran &&!openCheckFlag &&scm->enemyStartLocation!=NULL && Broodwar->getFrameCount()<24*60*6)
	{
		int bc = 0;
		int bb = 0;
		int ba = 0;
		int vf = 0;
		int vs = 0;
		int scv = 0;
		int marine = 0;
		int tank = 0;
		int vulture = 0;
		bc = enemyInfo->CountEunitNum(UnitTypes::Terran_Command_Center);
		bb = enemyInfo->CountEunitNum(UnitTypes::Terran_Barracks);
		ba = enemyInfo->CountEunitNum(UnitTypes::Terran_Academy);
		vf = enemyInfo->CountEunitNum(UnitTypes::Terran_Factory);
		vs = enemyInfo->CountEunitNum(UnitTypes::Terran_Starport);
		scv = enemyInfo->CountEunitNum(UnitTypes::Terran_SCV);
		marine = enemyInfo->CountEunitNum(UnitTypes::Terran_Marine);
		tank = enemyInfo->CountEunitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode);
		vulture = enemyInfo->CountEunitNum(UnitTypes::Terran_Vulture);

		if (Broodwar->getFrameCount()<=24*60*2){
			if(bb>0)
				STflag = TrushMarine;
		}
		if (Broodwar->getFrameCount()<=24*60*2+24*30){
			if (marine>0)
				STflag = TrushMarine;
		}

		if (Broodwar->getFrameCount()>=24*60*3){
			if (bb==0 || bb==2 || (scv>0 && scv<=11))
				STflag = TrushMarine;
		}
	}

	// enemy is zerg
	if (Broodwar->enemy()->getRace() == Races::Zerg &&!openCheckFlag && Broodwar->getFrameCount()<24*60*7)
	{
		int bh = 0;
		int bs = 0;
		int be = 0;
		int bd = 0;
		int vs = 0;
		int drone = 0;
		int zergling= 0;
		int hydralisk = 0;
		int lurker = 0;
		int lurkerEgg = 0;
		int mutalisk = 0;
		int bc = 0;
		bool isMainBase = false;
		drone = enemyInfo->CountEunitNum(UnitTypes::Zerg_Drone);
		bc = enemyInfo->CountEunitNum(UnitTypes::Zerg_Creep_Colony) + enemyInfo->CountEunitNum(UnitTypes::Zerg_Sunken_Colony);
		bh = enemyInfo->CountEunitNum(UnitTypes::Zerg_Hatchery);
		bs = enemyInfo->CountEunitNum(UnitTypes::Zerg_Spawning_Pool);
		be = enemyInfo->CountEunitNum(UnitTypes::Zerg_Extractor);
		bd = enemyInfo->CountEunitNum(UnitTypes::Zerg_Hydralisk_Den);
		vs = enemyInfo->CountEunitNum(UnitTypes::Zerg_Spire);
		zergling = enemyInfo->CountEunitNum(UnitTypes::Zerg_Zergling);
		hydralisk = enemyInfo->CountEunitNum(UnitTypes::Zerg_Hydralisk);
		lurker = enemyInfo->CountEunitNum(UnitTypes::Zerg_Lurker);
		lurkerEgg = enemyInfo->CountEunitNum(UnitTypes::Zerg_Lurker_Egg);
		mutalisk = enemyInfo->CountEunitNum(UnitTypes::Zerg_Mutalisk);

		if (Broodwar->getFrameCount()<=(24*60*2+24*40)){
			if (zergling>0)
				STflag = ZrushZergling;
		}

		if (Broodwar->getFrameCount()<=(24*60*2+24*15)){
			if ((bs>0 && enemyInfo->EnemyhasBuilt(UnitTypes::Zerg_Spawning_Pool,1))||zergling>0)
				STflag = ZrushZergling;
		}
	}
}

void MentalClass::counterMeasure()
{
	//for all
	if (Broodwar->getFrameCount()>24*60*5 && !scm->enemyStartLocation)
	{
		if (SelectAll()(isCompleted)(Comsat_Station).empty() && myInfo->countUnitNum(UnitTypes::Terran_Command_Center,2) > 1)
		{
			if (bom->getPlannedCount(UnitTypes::Terran_Comsat_Station,70) < myInfo->countUnitNum(UnitTypes::Terran_Command_Center,1))
			{
				bom->build(1,UnitTypes::Terran_Academy,71);
				bom->build(myInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Comsat_Station,70);
			}
		}
	}
	
	//this part are various measures for different situation/enemy plan
	if (Broodwar->getFrameCount()%(24*2)==0)
	{
		switch(STflag)
		{
		case NotSure:
			{
				if (Broodwar->getFrameCount() >= 24*60*2.5 && Broodwar->getFrameCount() <= 24*60*5 &&
					  Broodwar->enemy()->getRace() == Races::Protoss &&
						myInfo->countUnitNum(UnitTypes::Terran_Bunker,2) < 1)
				{
					bom->build(1,UnitTypes::Terran_Bunker,106,terrainManager->buPos);
				}
				break;
			}
		case PrushZealot:
			{ 	
				//break;
				if (Broodwar->getFrameCount()<24*60*8 && !reactionFinish)
				{
					if (bom->getPlannedCount(UnitTypes::Terran_Vulture,102) < 5)
					{
						bom->build(5,UnitTypes::Terran_Vulture,102);
					}

					//build a bunker to defend zealot rush
					if (myInfo->countUnitNum(UnitTypes::Terran_Bunker,2) < 1 && myInfo->countUnitNum(UnitTypes::Terran_SCV,2) > 12)
					{
						if (terrainManager->buPos != TilePositions::None)
						{
							bom->build(1,UnitTypes::Terran_Bunker,106,terrainManager->buPos);
						}
						else
						{
							Position zealotBun = Positions::None;
							BWTA::Chokepoint* cp = terrainManager->mFirstChokepoint;
							if (cp->getWidth() >= 120)
							{
								zealotBun = terrainManager->mFirstChokepoint->getCenter();
							}
							else
							{
								ICEStarCraft::Vector2 v = cp->getSides().first - cp->getSides().second;
								ICEStarCraft::Vector2 u = ICEStarCraft::Vector2(v.y(),-v.x());
								u = u * (32.0/u.approxLen());
								zealotBun = u + cp->getCenter();
								if (BWTA::getRegion(zealotBun) != BWTA::getRegion(Broodwar->self()->getStartLocation()))
								{
									zealotBun = u*(-2) + cp->getCenter();
								}
							}
							bom->build(1,UnitTypes::Terran_Bunker,106,TilePosition(zealotBun));
						}

						if (myInfo->countUnitNum(UnitTypes::Terran_Marine,1)<5)
							bom->build(5,UnitTypes::Terran_Marine,106);							
					}

					if (enemyInSight.empty() && enemyInfo->getKilledEnemyNum() >= 8)
					{
						if (bom->getPlannedCount(UnitTypes::Terran_Vulture,73)>0)
						{
							bom->deleteItem(UnitTypes::Terran_Vulture,73);
							//Broodwar->printf("delete vulture");
						}
						reactionFinish = true;
					}								
				}
				break;
			}
		case PtechDK:
			{ 
				if (Broodwar->getFrameCount()<24*60*7 && !reactionFinish)
				{
					//Broodwar->printf("DK tech");
					bom->build(1,UnitTypes::Terran_Engineering_Bay,120);
					if (gf->bunkerPosition && (*gf->bunkerPosition)!=TilePositions::None)
					{
						bom->buildAdditional(1,UnitTypes::Terran_Missile_Turret,91,(*gf->bunkerPosition));
						bom->buildAdditional(1,UnitTypes::Terran_Missile_Turret,90,(TilePosition)terrainManager->mFirstChokepoint->getCenter());
					}
					else
					{
						bom->buildAdditional(1,UnitTypes::Terran_Missile_Turret,91,(TilePosition)terrainManager->mSecondChokepoint->getCenter());
						bom->buildAdditional(1,UnitTypes::Terran_Missile_Turret,90,(TilePosition)terrainManager->mFirstChokepoint->getCenter());
					}

					if (bom->getPlannedCount(UnitTypes::Terran_Comsat_Station)<myInfo->countUnitNum(UnitTypes::Terran_Command_Center,1))						
						bom->build(myInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Comsat_Station,80);		
					reactionFinish = true;						
				}		
				break;
			}
		case PrushDragoon:
			{ 
				//break;
				if (Broodwar->getFrameCount() < 24*60*8 && !reactionFinish)
				{
					//Broodwar->printf("DR rush");
					if (terrainManager->buPos != TilePositions::None)
					{
						bom->build(1,UnitTypes::Terran_Bunker,106,terrainManager->buPos);
					}
					if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode) && !techMng->planned(TechTypes::Tank_Siege_Mode))
						bom->research(TechTypes::Tank_Siege_Mode,95);
					if (bom->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,65)<3)
					{
						bom->buildAdditional(3,UnitTypes::Terran_Siege_Tank_Tank_Mode,95);
					}
					reactionFinish = true;
				}
				break;
			}
		case P2Base:
			{ 
				//break;
				if (Broodwar->getFrameCount()<24*60*7&&!reactionFinish)
				{
					//Broodwar->printf("2Base openning");
					if(myInfo->countUnitNum(UnitTypes::Terran_Command_Center,2) < 2 && myInfo->countUnitNum(UnitTypes::Terran_SCV,1) >= 18)
					{
						bom->autoExpand(100,2);
						if (terrainManager->buPos != TilePositions::None && Broodwar->self()->allUnitCount(UnitTypes::Terran_Bunker) < 1)
						{
							bom->build(1,UnitTypes::Terran_Bunker,106,terrainManager->buPos,false);
							bom->build(4,UnitTypes::Terran_Marine,105);
						}
					}

					if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode) && !techMng->planned(TechTypes::Tank_Siege_Mode))
					{
						bom->research(TechTypes::Tank_Siege_Mode,95);
					}
					if (bom->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,70)<3)
					{
						bom->buildAdditional(1,UnitTypes::Terran_Siege_Tank_Tank_Mode,100);
						bom->buildAdditional(3,UnitTypes::Terran_Siege_Tank_Tank_Mode,70);
					}
					if (myInfo->countUnitNum(UnitTypes::Terran_Marine,1)>=4 && bom->getPlannedCount(UnitTypes::Terran_Marine)>0)
						bom->deleteItem(UnitTypes::Terran_Marine);

					if (SelectAll()(isCompleted)(Siege_Tank,Marine).size() > 8 || Broodwar->getFrameCount()>24*60*6)
					{
						reactionFinish = true;
					}						
				}
				break;
			}
		//_T_
		case PtechCarrier:
			{
				if (Broodwar->getFrameCount()<24*60*7&&!reactionFinish)
				{
					if(myInfo->countUnitNum(UnitTypes::Terran_Command_Center,2)<2 && myInfo->countUnitNum(UnitTypes::Terran_SCV,1)>=18)
						bom->autoExpand(100,2);
					if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode) && !techMng->planned(TechTypes::Tank_Siege_Mode))
						bom->research(TechTypes::Tank_Siege_Mode,110);
					if (bom->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,105)<3)
					{
						bom->buildAdditional(3,UnitTypes::Terran_Siege_Tank_Tank_Mode,105);
					}
					if (myInfo->countUnitNum(UnitTypes::Terran_Marine,1)>=4 && bom->getPlannedCount(UnitTypes::Terran_Marine)>0)
						bom->deleteItem(UnitTypes::Terran_Marine);

					if (SelectAll()(isCompleted)(Siege_Tank).size()>2 || Broodwar->getFrameCount()>24*60*6){
						reactionFinish = true;
					}
				}
				break;
			}
		case ZrushZergling:
			{
				if (!reactionFinish)
				{
					if ((gf->bunkerPosition == NULL && myInfo->countUnitNum(UnitTypes::Terran_Bunker,1) < 1) ||
						 myInfo->countUnitNum(UnitTypes::Terran_Marine,1) < 4)
					{
						// train marine
						if (bom->getPlannedCount(UnitTypes::Terran_Marine,80) < 15)
						{
							bom->build(15,UnitTypes::Terran_Marine,80);
						}
						if (bom->getPlannedCount(UnitTypes::Terran_Barracks) < 2)
						{
							bom->build(2,UnitTypes::Terran_Barracks,75);
						}
						// build a bunker
						if (myInfo->countUnitNum(UnitTypes::Terran_Bunker,2) < 1)
						{									
							Position pos = Positions::None;
							BWTA::Chokepoint* cp = terrainManager->mFirstChokepoint;
							if (cp->getWidth() >= 200)
							{
								pos = terrainManager->mFirstChokepoint->getCenter();
							}
							else
							{
								ICEStarCraft::Vector2 v = cp->getSides().first - cp->getSides().second;
								ICEStarCraft::Vector2 u = ICEStarCraft::Vector2(v.y(),-v.x());
								u = u * (32.0 * 4 /u.approxLen());
								pos = u + cp->getCenter();
								if (BWTA::getRegion(pos) == terrainManager->mNearestBase->getRegion())
								{
									pos = -u + cp->getCenter();
								}
							}
							bom->build(1,UnitTypes::Terran_Bunker,106,TilePosition(pos));
						}

						if (Broodwar->getFrameCount() >= 24*60*10 || myInfo->countUnitNum(UnitTypes::Terran_Marine,1) >= 12)
						{
							bom->deleteItem(UnitTypes::Terran_Marine);
							bom->deleteItem(UnitTypes::Terran_Barracks,75);
							reactionFinish = true;
						}
					}
				}
				break;
			}
		case TrushMarine:
			{
				if (!reactionFinish)
				{
					if (bom->getPlannedCount(UnitTypes::Terran_Marine,80) < 8)
					{
						bom->build(8,UnitTypes::Terran_Marine,80);
					}
					else
					{
						reactionFinish = true;
					}

					if (Broodwar->getFrameCount() > 24*60*6)
					{
						reactionFinish = true;
					}
				}
			}
			break;
		}
	}

	//if enemy is zerg
	if (Broodwar->enemy()->getRace() == Races::Zerg)
	{
		//defend early rush
		if (Broodwar->getFrameCount() <= 24*60*8 && myInfo->countUnitNum(UnitTypes::Terran_Bunker,1) > 0)
		{
			if (myInfo->myFightingValue().second <= enemyInfo->enemyFightingValue().first)
			{
				if (myInfo->countUnitNum(UnitTypes::Terran_Factory,2) > 0 && myInfo->countUnitNum(UnitTypes::Terran_Academy,2) > 0)
				{
					if (bom->getPlannedCount(UnitTypes::Terran_Marine,60) < 8)
					{
						bom->build(8,UnitTypes::Terran_Marine,200);
					}
					if (bom->getPlannedCount(UnitTypes::Terran_Bunker,60) < 2)
					{
						bom->buildAdditional(1,UnitTypes::Terran_Bunker,201,*gf->bunkerPosition);
					}
				}
			}
		}

		//based on enemy tech tree
		if (Broodwar->getFrameCount() <= 24*60*15)
		{
			if (enemyInfo->EnemyhasBuilt(UnitTypes::Zerg_Mutalisk,1) || enemyInfo->EnemyhasBuilt(UnitTypes::Zerg_Spire,2))
			{
				TilePosition buildPos = TilePosition(terrainManager->mSecondChokepoint->getCenter());
				if (bom->getPlannedCount(UnitTypes::Terran_Missile_Turret,100) < 4)
				{
					if (myInfo->countUnitNum(UnitTypes::Terran_Engineering_Bay,2) < 1)
					{
						bom->build(1,UnitTypes::Terran_Engineering_Bay,110);
					}
					bom->build(4,UnitTypes::Terran_Missile_Turret,110,buildPos,false);
				}
				bom->build(2,UnitTypes::Terran_Bunker,120,buildPos,false);
				bom->build(8,UnitTypes::Terran_Marine,119);
				bom->build(12,UnitTypes::Terran_Goliath,100);
				bom->build(1,UnitTypes::Terran_Starport,90);
				bom->build(1,UnitTypes::Terran_Control_Tower,88);
				bom->build(6,UnitTypes::Terran_Valkyrie,86);
				bom->build(24,UnitTypes::Terran_Goliath,85);
				if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Charon_Boosters) < 1)
				{
					bom->upgrade(1,UpgradeTypes::Charon_Boosters,80);
				}
				if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Terran_Ship_Weapons) < 1)
				{
					bom->upgrade(1,UpgradeTypes::Terran_Ship_Weapons,65);
				}		
			}
			else if (enemyInfo->EnemyhasBuilt(UnitTypes::Zerg_Hydralisk,1) || enemyInfo->EnemyhasBuilt(UnitTypes::Zerg_Hydralisk_Den,2))
			{
				//research siege mode
				if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode) && !techMng->planned(TechTypes::Tank_Siege_Mode))
					bom->research(TechTypes::Tank_Siege_Mode,200);

				if (bom->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,197) < 6)
				{
					bom->buildAdditional(1,UnitTypes::Terran_Siege_Tank_Tank_Mode,198);
				}

				//machine shop
				if (bom->getPlannedCount(UnitTypes::Terran_Machine_Shop,199) < 2)
				{
					bom->buildAdditional(1,UnitTypes::Terran_Machine_Shop,199);
				}
				//missile turret for invisible units
				if (bom->getPlannedCount(UnitTypes::Terran_Missile_Turret,94) < 2 && gf->bunkerPosition && myInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,1) > 0)
				{
					bom->build(2,UnitTypes::Terran_Missile_Turret,94,*gf->bunkerPosition);
				}
			}				
		}

		//for lurker
		if (enemyInfo->EnemyhasBuilt(UnitTypes::Zerg_Lurker,1) && Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) > 1)
		{
			if (bom->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,68) < 12)
			{
				bom->build(12,UnitTypes::Terran_Siege_Tank_Tank_Mode,68);
			}

			if (Broodwar->self()->supplyUsed()/2 > 85 && Broodwar->self()->allUnitCount(UnitTypes::Terran_Science_Vessel) < 2)
			{
				if (bom->getPlannedCount(UnitTypes::Terran_Science_Vessel,104) < 2)
				{
					if (bom->getPlannedCount(UnitTypes::Terran_Starport,120)         < 1) bom->build(1,UnitTypes::Terran_Starport,120);
					if (bom->getPlannedCount(UnitTypes::Terran_Control_Tower,115)    < 1) bom->build(1,UnitTypes::Terran_Control_Tower,115);
					if (bom->getPlannedCount(UnitTypes::Terran_Science_Facility,110) < 1) bom->build(1,UnitTypes::Terran_Science_Facility,110);
					if (bom->getPlannedCount(UnitTypes::Terran_Science_Vessel,105)   < 2) bom->build(2,UnitTypes::Terran_Science_Vessel,105);
				}
				if (myInfo->countUnitNum(UnitTypes::Terran_Science_Facility,1) > 0)
				{
					if (!bom->plannedTech(TechTypes::Irradiate) && !Broodwar->self()->hasResearched(TechTypes::Irradiate))
					{
						bom->research(TechTypes::Irradiate,100);
					}
					if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Titan_Reactor) < 1)
					{
						bom->upgrade(1,UpgradeTypes::Titan_Reactor,80);
					}
				}
			}
		}
	}

	//for against protoss carrier
	if ((enemyInfo->EnemyhasBuilt(UnitTypes::Protoss_Carrier,1) || enemyInfo->EnemyhasBuilt(UnitTypes::Protoss_Fleet_Beacon,2)) &&
		 myInfo->countUnitNum(UnitTypes::Terran_Command_Center,1) >= 2 && myInfo->countUnitNum(UnitTypes::Terran_Factory,1) >= 3)
	{
		//	Broodwar->printf("produce goliath as soon as possible");
		if (bom->getPlannedCount(UnitTypes::Terran_Goliath,81)<32)
		{
			bom->build(32,UnitTypes::Terran_Goliath,81);
		}
		if (myInfo->countUnitNum(UnitTypes::Terran_Goliath,1) >= 12 && bom->getPlannedCount(UnitTypes::Terran_Factory,75) < 6)
		{
			bom->build(6,UnitTypes::Terran_Factory,75);
		}
		if(upgradeMng->getPlannedLevel(UpgradeTypes::Charon_Boosters)<1)
			bom->upgrade(1,UpgradeTypes::Charon_Boosters,72);
		if (bom->getPlannedCount(UnitTypes::Terran_Armory)<2)
		{
			bom->build(2,UnitTypes::Terran_Armory,72);
		}
		if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons)<1)
		{
			if(upgradeMng->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons)<1)
				bom->upgrade(1,UpgradeTypes::Terran_Vehicle_Weapons,71);
		}			
		else
		{
			if(upgradeMng->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons)<3)
				bom->upgrade(3,UpgradeTypes::Terran_Vehicle_Weapons,70);
		}
		if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Plating)<1){
			if(upgradeMng->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Plating)<1)
				bom->upgrade(1,UpgradeTypes::Terran_Vehicle_Plating,71);
		}		
		else
		{
			if(upgradeMng->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Plating)<3)
				bom->upgrade(3,UpgradeTypes::Terran_Vehicle_Plating,70);
		}
	}

	//for against protoss arbiter
	if ((enemyInfo->EnemyhasBuilt(UnitTypes::Protoss_Arbiter,1) || enemyInfo->EnemyhasBuilt(UnitTypes::Protoss_Arbiter_Tribunal,2)) &&
		  myInfo->countUnitNum(UnitTypes::Terran_Command_Center,1) >= 2)
	{
		if (Broodwar->getFrameCount()%(24*30) == 9)
		{
			//Broodwar->printf("Arbiter!");
		}

		if (upgradeMng->getPlannedLevel(UpgradeTypes::Charon_Boosters) < 1)
		{
			bom->upgrade(1,UpgradeTypes::Charon_Boosters,72);
		}

		if (bom->getPlannedCount(UnitTypes::Terran_Goliath,71) < 15)
		{
			bom->build(15,UnitTypes::Terran_Goliath,71);
		}

		if (bom->getPlannedCount(UnitTypes::Terran_Starport,110) < 1)
		{
			bom->build(1,UnitTypes::Terran_Starport,110);
		}

		if (myInfo->countUnitNum(UnitTypes::Terran_Starport,1) > 0)
		{
			if (bom->getPlannedCount(UnitTypes::Terran_Science_Facility,106) < 1)	bom->build(1,UnitTypes::Terran_Science_Facility,106);
			if (bom->getPlannedCount(UnitTypes::Terran_Control_Tower,108)    < 1) bom->build(1,UnitTypes::Terran_Control_Tower,108);
		}
	
		int need = enemyInfo->countUnitNum(UnitTypes::Protoss_Arbiter) > 3 ? enemyInfo->countUnitNum(UnitTypes::Protoss_Arbiter) : 2;
		if (bom->getPlannedCount(UnitTypes::Terran_Science_Vessel,104) < need)
		{
			bom->build(need,UnitTypes::Terran_Science_Vessel,104);
		}
		
		if (myInfo->countUnitNum(UnitTypes::Terran_Science_Facility,1) > 0)
		{
			if (!bom->plannedTech(TechTypes::EMP_Shockwave) && !Broodwar->self()->hasResearched(TechTypes::EMP_Shockwave))
			{
				bom->research(TechTypes::EMP_Shockwave,75);
			}

			if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Titan_Reactor) < 1)
			{
				bom->upgrade(1,UpgradeTypes::Titan_Reactor,72);
			}
		}
	}

	//for against protoss dark templar
	if (enemyInfo->EnemyhasBuilt(UnitTypes::Protoss_Dark_Templar,1) ||
		  (enemyInfo->EnemyhasBuilt(UnitTypes::Protoss_Templar_Archives,2) && Broodwar->getFrameCount() < 24*60*8))
	{
		if (Broodwar->getFrameCount()%(24*30) == 9)
		{
			//Broodwar->printf("Dark Templar!");
		}
		
		if (Broodwar->self()->supplyUsed()/2 > 100)
		{
			if (bom->getPlannedCount(UnitTypes::Terran_Science_Vessel,104) < 2)
			{
				if (bom->getPlannedCount(UnitTypes::Terran_Starport,110)         < 1) bom->build(1,UnitTypes::Terran_Starport,110);
				if (bom->getPlannedCount(UnitTypes::Terran_Control_Tower,108)    < 1) bom->build(1,UnitTypes::Terran_Control_Tower,108);
				if (bom->getPlannedCount(UnitTypes::Terran_Science_Facility,106) < 1) bom->build(1,UnitTypes::Terran_Science_Facility,106);
				if (bom->getPlannedCount(UnitTypes::Terran_Science_Vessel,104)   < 2) bom->build(2,UnitTypes::Terran_Science_Vessel,104);
			}
		}
		else
		{
			if (bom->getPlannedCount(UnitTypes::Terran_Academy,102) < 1) bom->build(1,UnitTypes::Terran_Academy,102);
			if (bom->getPlannedCount(UnitTypes::Terran_Comsat_Station,100) < myInfo->countUnitNum(UnitTypes::Terran_Command_Center,1))
				bom->build(myInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Comsat_Station,100);
		}
	}

	//for against enemy air unit
	if (enemyInfo->EnemyhasBuilt(UnitTypes::Protoss_Stargate,2) || 
		  enemyInfo->EnemyhasBuilt(UnitTypes::Terran_Starport,2) || 
		  enemyInfo->EnemyhasBuilt(UnitTypes::Zerg_Spire,2))
	{
		if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Armory) > 0)
		{
			if (upgradeMng->getPlannedLevel(UpgradeTypes::Charon_Boosters) < 1)
			{
				bom->upgrade(1,UpgradeTypes::Charon_Boosters,72);
			}
			if (bom->getPlannedCount(UnitTypes::Terran_Goliath,71) < 8)
			{
				bom->build(8,UnitTypes::Terran_Goliath,71);
			}
		}
		else
		{
			if (bom->getPlannedCount(UnitTypes::Terran_Armory,71) < 1)
			{
				bom->build(1,UnitTypes::Terran_Armory,71);
			}
		}
	}
}

void MentalClass::baseUnderAttack()
{
	if (Broodwar->getFrameCount()%24 != 0)
	{
		return;
	}

	enemyInSight.clear();
	map<Position,int> centers;
	for each (Unit* cc in SelectAll(UnitTypes::Terran_Command_Center))
	{
		if (cc->getTilePosition() == Broodwar->self()->getStartLocation())
		{
			centers.insert(make_pair(cc->getPosition(),32*25));
		}
		else
		{
			centers.insert(make_pair(cc->getPosition(),32*20));
		}
	}
	if (terrainManager->mNearestBase)      centers.insert(make_pair(terrainManager->mNearestBase->getPosition(),32*20));
	if (terrainManager->mFirstChokepoint)  centers.insert(make_pair(terrainManager->mFirstChokepoint->getCenter(),32*10));
	if (terrainManager->mSecondChokepoint) centers.insert(make_pair(terrainManager->mSecondChokepoint->getCenter(),32*10));

	for (map<Position,int>::iterator i = centers.begin(); i != centers.end(); i++)
	{
		for each (Unit* u in Broodwar->getUnitsInRadius(i->first,i->second))
		{
			if (u->getPlayer() != Broodwar->enemy())
			{
				continue;
			}

			UnitType type = u->getType();
			if (type == UnitTypes::Protoss_Scarab
			    ||
			    type == UnitTypes::Terran_Vulture_Spider_Mine
			    ||
			    type == UnitTypes::Terran_Nuclear_Missile
			    ||
			    type == UnitTypes::Protoss_Interceptor)
			{
				continue;
			}

			if (type.canAttack()
			    ||
			    type == UnitTypes::Protoss_Carrier
			    ||
			    type == UnitTypes::Protoss_Reaver
			    ||
			    type == UnitTypes::Terran_Bunker
			    ||
			    type == UnitTypes::Protoss_Shuttle
			    ||
			    type == UnitTypes::Terran_Dropship)
			{
				enemyInSight.insert(u);	
			}
		}
	}

	for each(Unit* u in Broodwar->self()->getUnits())
	{
		if (!u->getType().isBuilding())
		{
			continue;
		}

		if (u->isUnderAttack() && u->getHitPoints() < 50)
		{
			if (u->isBeingConstructed())
			{
				u->cancelConstruction();
			}
			
			if (u->isConstructing())
			{
				u->cancelAddon();
			}
		
			if (u->isResearching() && u->getRemainingResearchTime() > 24)
			{
				u->cancelResearch();
			}

			if (u->isUpgrading() && u->getRemainingUpgradeTime() > 24)
			{
				u->cancelUpgrade();
			}
		}
	}
}

bool MentalClass::getUnderAttackFlag()
{
	if (enemyInSight.size()>0)
	{
		mUnderAttack = true;
		return mUnderAttack;
	}
	else
	{
		mUnderAttack = false;
		return mUnderAttack;
	}
}

void MentalClass::onUnitDestroy(BWAPI::Unit* u)
{
	enemyInSight.erase(u);
}

void MentalClass::onUnitMorph(BWAPI::Unit* u)
{
	enemyInSight.erase(u);	
}

void MentalClass::onUnitEvade(BWAPI::Unit* u)
{
	enemyInSight.erase(u);
}
void MentalClass::destroy()
{
	if (theMentalManager) delete theMentalManager;
}

void MentalClass::updateSightRange()
{
	baseSightRangeLimitation = 30 + Broodwar->self()->supplyUsed()/4;
}

void MentalClass::attackTimingCheck()
{
	//for zerg timing
	if (Broodwar->enemy()->getRace() == Races::Zerg)
	{
		//Broodwar->drawTextScreen(460,30,"\x11 ME: %d | ENEMY: %d",(int)myInfo->myFightingValue(),(int)enemyInfo->enemyFightingValue());
		//calculate timing
		if (STflag==ZrushZergling && SelectAll()(Marine)(isCompleted).not(isLoaded).size()>=10 && Broodwar->getFrameCount()<(24*60*7+24*30))
			goAttack = true;
		else
		{
			if (SelectAll()(Marine)(isCompleted).not(isLoaded).size() < 4 || Broodwar->getFrameCount()>(24*60*7+24*30))
				goAttack = false;
		}

		if (enemyInfo->allenemyFighter.size()>5 || Broodwar->getFrameCount() > 24*60*6)
		{
			int detectorCount = SelectAll()(isCompleted)(Comsat_Station)(Energy,">=",50).size() + SelectAll()(isCompleted)(Science_Vessel).size();
			if (goAttack)
			{
				if ((!SelectAllEnemy().not(isDetected)(isAttacking).empty() && detectorCount < 1)
					  ||
					  myInfo->myFightingValue().first < enemyInfo->enemyFightingValue().second) 
				{
					goAttack = false;
				}
			}
			else if (detectorCount > 0)
			{
				if (myInfo->myFightingValue().first > enemyInfo->enemyFightingValue().first * 2.4 &&
					  myInfo->myFightingValue().first > enemyInfo->enemyFightingValue().second &&
					  myInfo->getMyArmyNum() >= 30)
				{
					goAttack = true;
				}
				else if (myInfo->getMyArmyNum() >= 50 || Broodwar->self()->supplyUsed()/2 >= 180 || Broodwar->getFrameCount() > 24*60*50)
				{
					goAttack = true;
				}
			}
		}
	}
	//for protoss timng
	else if (Broodwar->enemy()->getRace() == Races::Protoss)
	{
		if (STflag == PtechCarrier)
		{
			if ((Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode) || Broodwar->self()->isResearching(TechTypes::Tank_Siege_Mode)) &&
				  SelectAll()(Siege_Tank)(isCompleted).size() > 0)
			{
				goAttack = true;
			}
			else
			{
				goAttack = false;
			}
		}
		else
		{
			if (goAttack)
			{
				if(Broodwar->self()->supplyUsed()/2 < 190 && myInfo->myFightingValue().first < 1.0 * enemyInfo->enemyFightingValue().first) //1.2
				{
					goAttack = false;
				}
			}
			else
			{
				if (tpTiming->attackTimingMap[1] || tpTiming->attackTimingMap[2] || tpTiming->attackTimingMap[3] || tpTiming->attackTimingMap[4])
				{
					goAttack = true;
				}
			}
		}
	}
	//for terran timing
	else if (Broodwar->enemy()->getRace() == Races::Terran)
	{
		//for 8bb rush
		int killedMarine = Broodwar->self()->deadUnitCount(UnitTypes::Terran_Marine);
		if (Broodwar->getFrameCount()<=24*60*8 && myInfo->countUnitNum(UnitTypes::Terran_Marine,1)>=2 && !marineRushOver)
		{
			//if we lost too many marine and barely kill any enemy
			if (killedMarine>=4 && enemyInfo->killedEnemyNum < killedMarine)
			{
				goAttack = false;	
				marineRushOver = true;
				//cancel rush bunker
				if (bom->getPlannedCount(UnitTypes::Terran_Bunker)>0){
					bom->deleteItem(UnitTypes::Terran_Bunker,95, BWTA::getRegion(scm->enemyStartLocation->getTilePosition()));
				}
				//delete marine
				//&& bom->getPlannedCount(UnitTypes::Terran_Marine)>0
				if (SelectAll()(isCompleted)(Marine).size()>=4)
					bom->deleteItem(UnitTypes::Terran_Marine);
			}					
			//if our fight value is larger & enemy doesn't have bunker & enemy number <7 
			//else if (myInfo->myFightingValue().first > enemyInfo->enemyFightingValue().first &&
			//_T_
			else if (myInfo->myFightingValue().first * 2 > enemyInfo->enemyFightingValue().first &&
				!enemyInfo->EnemyhasBuilt(UnitTypes::Terran_Bunker,1) &&
				SelectAllEnemy()(isCompleted)(canAttack).not(isWorker,isBuilding).size()<7)
			{
					//if we already have bunker
					if (myInfo->countUnitNum(UnitTypes::Terran_Bunker,1)>0)
					{
						int mrInBk = SelectAll()(isCompleted)(isLoaded)(Marine).size();
						//if we have marine in bunker, then continue rush
						if (mrInBk > 0){
							goAttack = true;
							bom->build(2,UnitTypes::Terran_Siege_Tank_Tank_Mode,140);
							if(!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode))
								bom->research(TechTypes::Tank_Siege_Mode,135);
						}

						//if bunker is empty, and enemy has no vulture, continue rush
						else if (mrInBk == 0 && !enemyInfo->EnemyhasBuilt(UnitTypes::Terran_Vulture,1))
							goAttack = true;
						bom->build(2,UnitTypes::Terran_Siege_Tank_Tank_Mode,140);
						if(!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode))
							bom->research(TechTypes::Tank_Siege_Mode,135);
					}
					//if don't have bunker yet
					else
					{
						//enemy has vulture
						if (enemyInfo->EnemyhasBuilt(UnitTypes::Terran_Vulture,1) 
							&& myInfo->countUnitNum(UnitTypes::Terran_Marine,1)/enemyInfo->CountEunitNum(UnitTypes::Terran_Vulture)<3){
								goAttack = false;
								marineRushOver = true;
								//cancel bunker plan
								if (bom->getPlannedCount(UnitTypes::Terran_Bunker)>0){
									bom->deleteItem(UnitTypes::Terran_Bunker,95, BWTA::getRegion(scm->enemyStartLocation->getTilePosition()));
								}
								// delete marine
								if (SelectAll()(isCompleted)(Marine).size()>=4 && bom->getPlannedCount(UnitTypes::Terran_Marine)>0)
									bom->deleteItem(UnitTypes::Terran_Marine);
						}
						//_T_
						//enemy has medic or firebat
						//else if (enemyInfo->EnemyhasBuilt(UnitTypes::Terran_Medic,1) || enemyInfo->EnemyhasBuilt(UnitTypes::Terran_Firebat,1))
						//{
						//	goAttack = false;
						//	marineRushOver = true;
						//	//cancel bunker plan
						//	if (bom->getPlannedCount(UnitTypes::Terran_Bunker)>0){
						//		bom->deleteItem(UnitTypes::Terran_Bunker,95, BWTA::getRegion(scm->enemyStartLocation->getTilePosition()));
						//	}
						//	// delete marine
						//	if (SelectAll()(isCompleted)(Marine).size()>=4 && bom->getPlannedCount(UnitTypes::Terran_Marine)>0)
						//		bom->deleteItem(UnitTypes::Terran_Marine);
						//}
						else
							goAttack = true;
					}
			}				
			else
			{
				goAttack = false;
				marineRushOver = true;
				int see = bom->getPlannedCount(UnitTypes::Terran_Bunker,95);
				if (bom->getPlannedCount(UnitTypes::Terran_Bunker)>0){
					bom->deleteItem(UnitTypes::Terran_Bunker,95, BWTA::getRegion(scm->enemyStartLocation->getTilePosition()));
					see = bom->getPlannedCount(UnitTypes::Terran_Bunker,95);
				}

				if (SelectAll()(isCompleted)(Marine).size()>=4 && bom->getPlannedCount(UnitTypes::Terran_Marine)>0)
					bom->deleteItem(UnitTypes::Terran_Marine);
			}				
		}
		//_T_
		else
		{
			if (goAttack)
			{
				if (Broodwar->self()->supplyUsed()/2 < 180 &&
						SelectAll()(isCompleted)(Battlecruiser)(HitPoints,">",400).size() < 6 &&
					  myInfo->myFightingValue().first < enemyInfo->enemyFightingValue().first)
				{
					goAttack = false;
				}
			}
			else
			{
				if (Broodwar->getFrameCount() > 24*60*10 &&
					  Broodwar->getFrameCount() < 24*60*14 &&
					  Broodwar->self()->supplyUsed()/2 > 120 &&
						myInfo->myFightingValue().first > 1.3 * enemyInfo->enemyFightingValue().first)
				{
					goAttack = true;
				}
				else if (SelectAll()(isCompleted)(Battlecruiser)(HitPoints,">=",400).size() >= 6)
				{
					goAttack = true;
				}
				else if (Broodwar->self()->supplyUsed()/2 >= 180 || Broodwar->getFrameCount() > 24*60*45)
				{
					goAttack = true;
				}
			}
		}
	}
}

string MentalClass::getSTflag()
{
	switch (STflag)
	{
	case MentalClass::NotSure:
		return "Unknown";
	case MentalClass::PrushZealot:
		return "PrushZealot";
	case MentalClass::PrushDragoon:
		return "PrushDragoon";
	case MentalClass::PtechDK:
		return "PtechDK";
	case MentalClass::PtechReaver:
		return "PtechReaver";
	case MentalClass::BeCareful:
		return "BeCareful";
	case MentalClass::P2Base:
		return "P2Base";
	case MentalClass::PtechCarrier:
		return "PtechCarrier";
	case MentalClass::ZrushZergling:
		return "ZrushZergling";
	case MentalClass::Ztech:
		return "Ztech";
	case MentalClass::Zexpansion:
		return "Zexpansion";
	case MentalClass::TrushMarine:
		return "TrushMarine";
	case MentalClass::Ttech:
		return "Ttech";
	case MentalClass::Texpansion:
		return "Texpansion";
	default:
		return "Unknown";
	}
}

void MentalClass::showDebugInfo()
{
	string   atkTarType = MacroManager::create()->getAttackTarget()->getType();
	Position atkTarPos  = MacroManager::create()->getAttackTarget()->getPosition();
	
	if (!goAttack)
	{		
		Broodwar->drawTextScreen(433,15,"\x1C Defend");
		if (atkTarPos != Positions::None)
		{
			Broodwar->drawTextScreen(473,15,"\x1C %s | (%d,%d)",atkTarType.c_str(),atkTarPos.x()/32,atkTarPos.y()/32);
		}
		else
		{
			Broodwar->drawTextScreen(473,15,"\x1C %s",atkTarType.c_str());
		}
	}
	else
	{
		Broodwar->drawTextScreen(433,15,"\x08 Attack");
		if (atkTarPos != Positions::None)
		{
			Broodwar->drawTextScreen(473,15,"\x08 %s | (%d,%d)",atkTarType.c_str(),atkTarPos.x()/32,atkTarPos.y()/32);
		}
		else
		{
			Broodwar->drawTextScreen(473,15,"\x08 %s",atkTarType.c_str());
		}
	}

	// second line
	if (myInfo->myFightingValue().first>enemyInfo->enemyFightingValue().second)
	{
		Broodwar->drawTextScreen(433,25,"\x07 My_FV: %d > Enemy_DV: %d",(int)myInfo->myFightingValue().first,(int)enemyInfo->enemyFightingValue().second);
	}
	else if (myInfo->myFightingValue().first==enemyInfo->enemyFightingValue().second)
	{
		Broodwar->drawTextScreen(433,25,"\x19 My_FV: %d = Enemy_DV: %d",(int)myInfo->myFightingValue().first,(int)enemyInfo->enemyFightingValue().second);
	}
	else
	{
		Broodwar->drawTextScreen(433,25,"\x08 My_FV: %d < Enemy_DV: %d",(int)myInfo->myFightingValue().first,(int)enemyInfo->enemyFightingValue().second);
	}

	// third line
	if (myInfo->myFightingValue().second > enemyInfo->enemyFightingValue().first)
	{
		Broodwar->drawTextScreen(433,35,"\x07 My_DV: %d > Enemy_FV: %d",(int)myInfo->myFightingValue().second,(int)enemyInfo->enemyFightingValue().first);
	}
	else if (myInfo->myFightingValue().second == enemyInfo->enemyFightingValue().first)
	{
		Broodwar->drawTextScreen(433,35,"\x19 My_DV: %d = Enemy_FV: %d",(int)myInfo->myFightingValue().second,(int)enemyInfo->enemyFightingValue().first);
	}
	else
	{
		Broodwar->drawTextScreen(433,35,"\x08 My_DV: %d < Enemy_FV: %d",(int)myInfo->myFightingValue().second,(int)enemyInfo->enemyFightingValue().first);
	}

	// enemy opening strategy
	switch(STflag)
	{
	case MentalClass::NotSure:
		Broodwar->drawTextScreen(120,0,"\x07 NotSure");
		break;
	case MentalClass::PrushZealot:
		Broodwar->drawTextScreen(120,0,"\x07 PrushZealot");
		break;
	case MentalClass::PrushDragoon:
		Broodwar->drawTextScreen(120,0,"\x07 PrushDragoon");
		break;
	case MentalClass::PtechDK:
		Broodwar->drawTextScreen(120,0,"\x07 PtechDK");
		break;
	case MentalClass::PtechReaver:
		Broodwar->drawTextScreen(120,0,"\x07 PtechReaver");
		break;
	case MentalClass::BeCareful:
		Broodwar->drawTextScreen(120,0,"\x07 BeCareful");
		break;
	case MentalClass::P2Base:
		Broodwar->drawTextScreen(120,0,"\x07 P2Base");
		break;
	case MentalClass::PtechCarrier:
		Broodwar->drawTextScreen(120,0,"\x07 PtechCarrier");
		break;
	case MentalClass::ZrushZergling:
		Broodwar->drawTextScreen(120,0,"\x07 ZrushZergling");
		break;
	case MentalClass::Ztech:
		Broodwar->drawTextScreen(120,0,"\x07 Ztech");
		break;
	case MentalClass::Zexpansion:
		Broodwar->drawTextScreen(120,0,"\x07 Zexpansion");
		break;
	case MentalClass::TrushMarine:
		Broodwar->drawTextScreen(120,0,"\x07 TrushMarine");
		break;
	case MentalClass::Ttech:
		Broodwar->drawTextScreen(120,0,"\x07 Ttech");
		break;
	case MentalClass::Texpansion:
		Broodwar->drawTextScreen(120,0,"\x07 Texpansion");
		break;
	default:
		break;
	}

	// enemy in sight
	Broodwar->drawTextScreen(433,145,"\x08 EnemyInSight: %d",enemyInSight.size());
}