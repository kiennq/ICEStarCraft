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
	this->gf = GameFlow::create();
	myInfo = NULL;
	enemyInfo = NULL;
	STflag = NotSure;
	terrainManager = NULL;
	this->worker = NULL;
	openCheckFlag = false;
	reactionFinish=false;
	prushTimer1 =0 ;
	mUnderAttack = false;
	emergencyFlag = false;
	baseSightRangeLimitation = 50;
	this->enemyInSight.clear();
	this->goAttack = false;
	this->tpTiming = TPTiming::create();
	this->upgradeMng = NULL;
	this->techMng=NULL;
	this->marineRushOver = false;
}
void MentalClass::setManagers(BuildOrderManager* b,UpgradeManager* up,TechManager* tech)
{
	this->bom = b;
	this->myInfo = MyInfoManager::create();
	this->enemyInfo =EnemyInfoManager::create();
	this->scm =ScoutManager::create();
	this->macroManager = MacroManager::create();
	this->terrainManager = TerrainManager::create();
	this->worker = WorkerManager::create();
	this->upgradeMng = up;
	this->techMng =tech;
}

void MentalClass::onFrame()
{
	setEnemyPlanFlag();
	counterMeasure();
	upDateSightRange();
	attackTimingCheck();
	baseUnderAttack();
	this->tpTiming->CheckTiming();
	
	//if (!enemyInSight.empty())
	//{
	//	LastTimeEnemyAttacked = Broodwar->getFrameCount();
	//}
}

void MentalClass::setEnemyPlanFlag()
{
	//if(this->STflag != NotSure)
	if(this->STflag != NotSure && this->STflag != PrushDragoon)
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
			this->STflag = PtechDK;
			//Broodwar->printf("see vc or vt");
		}

		if ((bg>=2 && isMainBase && bc==0 && ba==0 && by==0)||(zlt>=2&&ba==0&&by==0&& isMainBase && bc==0)) 
			this->STflag = PrushZealot;

		if ((((bg>=2 && by==1 && isMainBase)||(dr>=2&&isMainBase)||this->enemyInfo->drRangeUpgradeFlag) && bn<2 && vc==0) ||dr>=3)
		{
			this->STflag = PrushDragoon;
		}
		if(bg==1&&!this->enemyInfo->drRangeUpgradeFlag && vr>0 && bn==1)
			this->STflag = PtechReaver;	

		//if((bn>=2 && Broodwar->getFrameCount()<=24*60*5)||(bn==1 && !isMainBase && Broodwar->getFrameCount()<=24*60*5))
		//	this->STflag = P2Base;

		//_T_
		if((bn>=2 && Broodwar->getFrameCount()<=24*60*5)||(bn==1 && !isMainBase && Broodwar->getFrameCount()<=24*60*5))
			if (this->STflag != PtechCarrier)
			{
				this->STflag = P2Base;
			}

		if (Broodwar->getFrameCount() <= 24*60*4 && bf > 0 && bc >= 2)
			this->STflag = PtechCarrier;

		if (vb==1 ||(bg==1&&vr==1&&!this->enemyInfo->drRangeUpgradeFlag))
			this->STflag = PtechReaver;

	}
	// enemy is terran
	if (Broodwar->enemy()->getRace() == Races::Terran &&!openCheckFlag &&this->scm->enemyStartLocation!=NULL && Broodwar->getFrameCount()<24*60*6)
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
				this->STflag = TrushMarine;
		}
		if (Broodwar->getFrameCount()<=24*60*2+24*30){
			if (marine>0)
				this->STflag = TrushMarine;
		}

		if (Broodwar->getFrameCount()>=24*60*3){
			if (bb==0 || bb==2 || (scv>0 && scv<=11))
				this->STflag = TrushMarine;
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

		//for 5d zergling rush
		//if (Broodwar->getFrameCount()<=(24*60*4+24*30)){
		//	if ((bs>0||zergling>0) && drone>=3 && drone<8 && bh==1 && bc==0)
		//		this->STflag = ZrushZergling;
		//}
		if (Broodwar->getFrameCount()<=(24*60*2+24*40)){
			if (zergling>0)
				this->STflag = ZrushZergling;
		}

		if (Broodwar->getFrameCount()<=(24*60*2+24*15)){
			if ((bs>0 && this->enemyInfo->EnemyhasBuilt(UnitTypes::Zerg_Spawning_Pool,1))||zergling>0)
				this->STflag = ZrushZergling;
		}
	}
}

void MentalClass::counterMeasure()
{
	//for all
	if (Broodwar->getFrameCount()>24*60*5 && !this->scm->enemyStartLocation)
	{
		if (SelectAll()(isCompleted)(Comsat_Station).empty()&&this->myInfo->countUnitNum(UnitTypes::Terran_Command_Center,2)>1){
			if (this->bom->getPlannedCount(UnitTypes::Terran_Comsat_Station,70)<this->myInfo->countUnitNum(UnitTypes::Terran_Command_Center,1)){
				this->bom->build(1,UnitTypes::Terran_Academy,71);
				this->bom->build(this->myInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Comsat_Station,70);
			}
		}
	}
	// this part is for under enemy attack, flag is mUnderAttack

	//if enemy is zerg
	if (Broodwar->enemy()->getRace()==Races::Zerg)
	{
		//defend early rush
		if (Broodwar->getFrameCount()<=24*60*8 && this->myInfo->countUnitNum(UnitTypes::Terran_Bunker,1)>0)
		{
			if (this->myInfo->myFightingValue().second <=this->enemyInfo->enemyFightingValue().first)
			{
				//produce marine
				if (this->bom->getPlannedCount(UnitTypes::Terran_Marine,60)<8 && this->myInfo->countUnitNum(UnitTypes::Terran_Factory,2)>0
					&& this->myInfo->countUnitNum(UnitTypes::Terran_Academy,2)>0)
					this->bom->build(8,UnitTypes::Terran_Marine,200);
				//build another bunker
				if (this->bom->getPlannedCount(UnitTypes::Terran_Bunker,60)<2&&this->myInfo->countUnitNum(UnitTypes::Terran_Factory,2)>0
					&& this->myInfo->countUnitNum(UnitTypes::Terran_Academy,2)>0){
						this->bom->buildAdditional(1,UnitTypes::Terran_Bunker,201,*this->gf->bunkerPosition);
				}	
			}

			//based on enemy tech tree
			if (Broodwar->getFrameCount()<=24*60*15)//13
			{
				if (this->enemyInfo->EnemyhasBuilt(UnitTypes::Zerg_Mutalisk,1)||this->enemyInfo->EnemyhasBuilt(UnitTypes::Zerg_Spire,2))
				{
					//_T_
					//TilePosition buildPos = SelectAll(UnitTypes::Terran_Bunker).empty()?BWTA::getNearestBaseLocation(this->chokeclass->mSecondChokepoint->getCenter())->getTilePosition():TilePosition(SelectAll(UnitTypes::Terran_Bunker).getCenter()).makeValid();
					TilePosition buildPos = TilePosition(this->terrainManager->mSecondChokepoint->getCenter());
					if (this->bom->getPlannedCount(UnitTypes::Terran_Missile_Turret,100) < 4)
					{
						if (this->myInfo->countUnitNum(UnitTypes::Terran_Engineering_Bay,2) < 1)
						{
							this->bom->build(1,UnitTypes::Terran_Engineering_Bay,110);
						}
						this->bom->build(4,UnitTypes::Terran_Missile_Turret,110,buildPos,false);
					}
					this->bom->build(2,UnitTypes::Terran_Bunker,120,buildPos,false);
					this->bom->build(8,UnitTypes::Terran_Marine,119);
					this->bom->build(30,UnitTypes::Terran_Goliath,100);
					this->bom->upgrade(1,UpgradeTypes::Charon_Boosters,90);
					this->bom->build(1,UnitTypes::Terran_Starport,90);
					this->bom->build(1,UnitTypes::Terran_Control_Tower,88);
					this->bom->build(6,UnitTypes::Terran_Valkyrie,85);
					this->bom->upgrade(1,UpgradeTypes::Terran_Ship_Weapons,65);
				}
				else if (this->enemyInfo->EnemyhasBuilt(UnitTypes::Zerg_Hydralisk,1)||this->enemyInfo->EnemyhasBuilt(UnitTypes::Zerg_Hydralisk_Den,2))
				{
					//	one more bunker

					//research siege mode
					if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode)&&!this->techMng->planned(TechTypes::Tank_Siege_Mode))
						this->bom->research(TechTypes::Tank_Siege_Mode,200);
					//build tank
					//||this->myInfo->CountUnitNum(UnitTypes::Terran_Siege_Tank_Tank_Mode,1)<6
					if (this->bom->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,197)<6){
						this->bom->buildAdditional(1,UnitTypes::Terran_Siege_Tank_Tank_Mode,198);
						//Broodwar->printf("planned tank %d",this->bom->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,198));
					}
					else{
						this->bom->deleteItem(UnitTypes::Terran_Siege_Tank_Tank_Mode,198);
						//Broodwar->printf("delete tank");
					}

					//machine shop
					if (this->bom->getPlannedCount(UnitTypes::Terran_Machine_Shop,199)<2){
						this->bom->buildAdditional(1,UnitTypes::Terran_Machine_Shop,199);
					}
					//missile turret for invisible units
					if (this->bom->getPlannedCount(UnitTypes::Terran_Missile_Turret,94)<1&&this->gf->bunkerPosition&&this->myInfo->countUnitNum(UnitTypes::Terran_Machine_Shop,1)>=1)
						this->bom->build(2,UnitTypes::Terran_Missile_Turret,94,*this->gf->bunkerPosition);
				}				
			}
		}
		//for lurker
		if (this->enemyInfo->EnemyhasBuilt(UnitTypes::Zerg_Lurker,1) &&
			  Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) > 1)
		{
			if (this->bom->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,68) < 12)
			{
				this->bom->build(12,UnitTypes::Terran_Siege_Tank_Tank_Mode,68);
			}

			if (Broodwar->self()->supplyUsed()/2 > 85 && Broodwar->self()->allUnitCount(UnitTypes::Terran_Science_Vessel) < 2)
			{
				this->bom->build(1,UnitTypes::Terran_Starport,120);
				this->bom->build(1,UnitTypes::Terran_Control_Tower,115);
				this->bom->build(1,UnitTypes::Terran_Science_Facility,110);
				this->bom->build(2,UnitTypes::Terran_Science_Vessel,105);
				if (!this->bom->plannedTech(TechTypes::Irradiate) && !Broodwar->self()->hasResearched(TechTypes::Irradiate))
				{
					this->bom->research(TechTypes::Irradiate,100);
				}
				if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Titan_Reactor) < 1)
				{
					this->bom->upgrade(1,UpgradeTypes::Titan_Reactor,80);
				}
			}
		}
	}
	//this part are various measures for different situation/enemy plan
	if(Broodwar->getFrameCount()%(24*2)==0)
	{
		switch(this->STflag)
		{
		case NotSure:
			{
				//_T_
				if (Broodwar->getFrameCount() >= 24*60*2.5 && Broodwar->getFrameCount() <= 24*60*5 &&
					  Broodwar->enemy()->getRace() == Races::Protoss &&
					  //!this->scm->enemyStartLocation &&
						this->myInfo->countUnitNum(UnitTypes::Terran_Bunker,2) < 1)
				{
					this->bom->build(1,UnitTypes::Terran_Bunker,106,this->terrainManager->buPos);
				}//_T_

				if (Broodwar->getFrameCount()>=24*60*5 && !reactionFinish && Broodwar->enemy()->getRace() == Races::Protoss)
				{
					if (this->bom->getPlannedCount(UnitTypes::Terran_Marine,40)<8)
						this->bom->buildAdditional(1,UnitTypes::Terran_Marine,66);
					//build another bunker
					if (this->bom->getPlannedCount(UnitTypes::Terran_Bunker,60)<2 && this->gf->bunkerPosition)
					{
						this->bom->buildAdditional(1,UnitTypes::Terran_Bunker,71,*this->gf->bunkerPosition);
					}
					reactionFinish = true;
					return;
				}
				break;
			}
		case PrushZealot:
			{ 	
				//break;
				if (Broodwar->getFrameCount()<24*60*8 && !reactionFinish)
				{
					//Broodwar->printf("Kill enemy units %d",this->enemyInfo->getKilledEnemyNum() );
					if (Broodwar->getFrameCount()%(24*15)==0)
						Broodwar->printf("zealot rush");
					//if (!Broodwar->self()->hasResearched(TechTypes::Spider_Mines)&& !this->techMng->planned(TechTypes::Spider_Mines))
					//	this->bom->research(TechTypes::Spider_Mines,78);//101
					if (this->bom->getPlannedCount(UnitTypes::Terran_Vulture,102) < 5)
					{
						this->bom->build(5,UnitTypes::Terran_Vulture,102);
					}

					//build a bunker to defend zealot rush
					if (this->myInfo->countUnitNum(UnitTypes::Terran_Bunker,2) < 1 && this->myInfo->countUnitNum(UnitTypes::Terran_SCV,2) > 12)
					{
						if (this->terrainManager->buPos != TilePositions::None)
						{
							this->bom->build(1,UnitTypes::Terran_Bunker,106,this->terrainManager->buPos);
						}
						else
						{
							Position zealotBun = Positions::None;
							BWTA::Chokepoint* cp = this->terrainManager->mFirstChokepoint;
							if (cp->getWidth() >= 120)
							{
								zealotBun = this->terrainManager->mFirstChokepoint->getCenter();
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
							this->bom->build(1,UnitTypes::Terran_Bunker,106,TilePosition(zealotBun));
						}

						if (this->myInfo->countUnitNum(UnitTypes::Terran_Marine,1)<5)
							this->bom->build(5,UnitTypes::Terran_Marine,106);							
					}

					if (this->enemyInSight.empty() && this->enemyInfo->getKilledEnemyNum() >= 8)
					{
						if (this->bom->getPlannedCount(UnitTypes::Terran_Vulture,73)>0)
						{
							this->bom->deleteItem(UnitTypes::Terran_Vulture,73);
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
					Broodwar->printf("DK tech");
					this->bom->build(1,UnitTypes::Terran_Engineering_Bay,120);
					if (this->gf->bunkerPosition && (*this->gf->bunkerPosition)!=TilePositions::None)
					{
						this->bom->buildAdditional(1,UnitTypes::Terran_Missile_Turret,91,(*this->gf->bunkerPosition));//_T_ 71
						this->bom->buildAdditional(1,UnitTypes::Terran_Missile_Turret,90,(TilePosition)this->terrainManager->mFirstChokepoint->getCenter());//70
					}
					else
					{
						this->bom->buildAdditional(1,UnitTypes::Terran_Missile_Turret,91,(TilePosition)this->terrainManager->mSecondChokepoint->getCenter());//71
						this->bom->buildAdditional(1,UnitTypes::Terran_Missile_Turret,90,(TilePosition)this->terrainManager->mFirstChokepoint->getCenter());//70
					}

					if (this->bom->getPlannedCount(UnitTypes::Terran_Comsat_Station)<this->myInfo->countUnitNum(UnitTypes::Terran_Command_Center,1))						
						this->bom->build(this->myInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Comsat_Station,80);//100				
					reactionFinish = true;						
				}		
				break;
			}
		case PrushDragoon:
			{ 
				//break;
				if (Broodwar->getFrameCount() < 24*60*8 && !reactionFinish)
				{
					Broodwar->printf("DR rush");
					if (this->terrainManager->buPos != TilePositions::None)
					{
						this->bom->build(1,UnitTypes::Terran_Bunker,106,this->terrainManager->buPos);
					}
					if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode) && !this->techMng->planned(TechTypes::Tank_Siege_Mode))
						this->bom->research(TechTypes::Tank_Siege_Mode,95);//75
					if (this->bom->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,65)<3)
					{
						this->bom->buildAdditional(3,UnitTypes::Terran_Siege_Tank_Tank_Mode,95);//75
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
					if(this->myInfo->countUnitNum(UnitTypes::Terran_Command_Center,2) < 2 && this->myInfo->countUnitNum(UnitTypes::Terran_SCV,1) >= 18)//18
					{
						this->bom->autoExpand(100,2);//200
						if (terrainManager->buPos != TilePositions::None && Broodwar->self()->allUnitCount(UnitTypes::Terran_Bunker) < 1)
						{
							this->bom->build(1,UnitTypes::Terran_Bunker,106,terrainManager->buPos,false);
							this->bom->build(4,UnitTypes::Terran_Marine,105);
						}
					}

					if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode) && !this->techMng->planned(TechTypes::Tank_Siege_Mode))
					{
						this->bom->research(TechTypes::Tank_Siege_Mode,95);//75
					}
					if (this->bom->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,70)<3)
					{
						this->bom->buildAdditional(1,UnitTypes::Terran_Siege_Tank_Tank_Mode,100);
						this->bom->buildAdditional(3,UnitTypes::Terran_Siege_Tank_Tank_Mode,70);
						//this->bom->buildAdditional(3,UnitTypes::Terran_Siege_Tank_Tank_Mode,70);
					}
					if (this->myInfo->countUnitNum(UnitTypes::Terran_Marine,1)>=4 && this->bom->getPlannedCount(UnitTypes::Terran_Marine)>0)
						this->bom->deleteItem(UnitTypes::Terran_Marine);

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
					if(this->myInfo->countUnitNum(UnitTypes::Terran_Command_Center,2)<2 && this->myInfo->countUnitNum(UnitTypes::Terran_SCV,1)>=18)
						this->bom->autoExpand(100,2);
					if (!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode) && !this->techMng->planned(TechTypes::Tank_Siege_Mode))
						this->bom->research(TechTypes::Tank_Siege_Mode,110);
					if (this->bom->getPlannedCount(UnitTypes::Terran_Siege_Tank_Tank_Mode,105)<3)
					{
						this->bom->buildAdditional(3,UnitTypes::Terran_Siege_Tank_Tank_Mode,105);
					}
					if (this->myInfo->countUnitNum(UnitTypes::Terran_Marine,1)>=4 && this->bom->getPlannedCount(UnitTypes::Terran_Marine)>0)
						this->bom->deleteItem(UnitTypes::Terran_Marine);

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
					if((this->gf->bunkerPosition==NULL && this->myInfo->countUnitNum(UnitTypes::Terran_Bunker,1) < 1) ||
						 this->myInfo->countUnitNum(UnitTypes::Terran_Marine,1) < 4)
					{
						//for train marine
						if (this->bom->getPlannedCount(UnitTypes::Terran_Marine,100) < 15)
						{
							this->bom->build(15,UnitTypes::Terran_Marine,80);
						}
						if (this->bom->getPlannedCount(UnitTypes::Terran_Barracks) < 2)
						{
							this->bom->build(2,UnitTypes::Terran_Barracks,75);
						}
						//for build a bunker
						if (this->myInfo->countUnitNum(UnitTypes::Terran_Bunker,2) < 1)
						{									
							Position re =  BWTA::getRegion(Broodwar->self()->getStartLocation())->getCenter();
							Position zergBun =Position((re.x()+this->terrainManager->mFirstChokepoint->getCenter().x())/2,(re.y()+this->terrainManager->mFirstChokepoint->getCenter().y())/2);
							this->bom->build(1,UnitTypes::Terran_Bunker,106,TilePosition(zergBun.x()/32,zergBun.y()/32));
							this->gf->bunkerPosition = new TilePosition(zergBun.x()/32,zergBun.y()/32);							
						}

						if((this->gf->bunkerPosition && this->myInfo->countUnitNum(UnitTypes::Terran_Marine,1) >= 6) || 
							(Broodwar->getFrameCount()>=24*60*10 && this->myInfo->countUnitNum(UnitTypes::Terran_Marine,1) >= 12))
						{
							this->bom->deleteItem(UnitTypes::Terran_Marine);
							this->bom->deleteItem(UnitTypes::Terran_Barracks,75);
							reactionFinish = true;
							break;
						}
					}
				}
				break;
			}
		case TrushMarine:
			{
			}
			break;
		}
	}

	//for against protoss carrier
	if((this->enemyInfo->EnemyhasBuilt(UnitTypes::Protoss_Carrier,1)||this->enemyInfo->EnemyhasBuilt(UnitTypes::Protoss_Fleet_Beacon,2)) 
		&& (this->myInfo->countUnitNum(UnitTypes::Terran_Command_Center,1)>=2) && (this->myInfo->countUnitNum(UnitTypes::Terran_Factory,1)>=3))
	{
		//	Broodwar->printf("produce goliath as soon as possible");
		if (this->bom->getPlannedCount(UnitTypes::Terran_Goliath,81)<32){
			this->bom->build(32,UnitTypes::Terran_Goliath,81);
		}
		if (this->myInfo->countUnitNum(UnitTypes::Terran_Goliath,1)>=12 && this->myInfo->countUnitNum(UnitTypes::Terran_Factory,1)<=6){
			if (this->bom->getPlannedCount(UnitTypes::Terran_Factory)<6)
				this->bom->build(6,UnitTypes::Terran_Factory,75);
		}
		if(this->upgradeMng->getPlannedLevel(UpgradeTypes::Charon_Boosters)<1)
			this->bom->upgrade(1,UpgradeTypes::Charon_Boosters,72);
		if (this->bom->getPlannedCount(UnitTypes::Terran_Armory)<2){
			this->bom->build(2,UnitTypes::Terran_Armory,72);
		}
		if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons)<1){
			if(this->upgradeMng->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons)<1)
				this->bom->upgrade(1,UpgradeTypes::Terran_Vehicle_Weapons,71);
		}			
		else{
			if(this->upgradeMng->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Weapons)<3)
				this->bom->upgrade(3,UpgradeTypes::Terran_Vehicle_Weapons,70);
		}
		if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Plating)<1){
			if(this->upgradeMng->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Plating)<1)
				this->bom->upgrade(1,UpgradeTypes::Terran_Vehicle_Plating,71);
		}		
		else{
			if(this->upgradeMng->getPlannedLevel(UpgradeTypes::Terran_Vehicle_Plating)<3)
				this->bom->upgrade(3,UpgradeTypes::Terran_Vehicle_Plating,70);
		}
	}

	//for against protoss arbiter
	if ((enemyInfo->EnemyhasBuilt(UnitTypes::Protoss_Arbiter,1) || enemyInfo->EnemyhasBuilt(UnitTypes::Protoss_Arbiter_Tribunal,2)) &&
		  this->myInfo->countUnitNum(UnitTypes::Terran_Command_Center,1)>=2 &&
			myInfo->countUnitNum(UnitTypes::Terran_Science_Vessel,2) < 2)
	{
		if (Broodwar->getFrameCount()%240 == 9)
		{
			Broodwar->printf("Arbiter!");
		}

		if(this->upgradeMng->getPlannedLevel(UpgradeTypes::Charon_Boosters)<1)
			this->bom->upgrade(1,UpgradeTypes::Charon_Boosters,72);
		if (this->bom->getPlannedCount(UnitTypes::Terran_Goliath,71)<15)
		{
			this->bom->build(15,UnitTypes::Terran_Goliath,71);
		}

		if (myInfo->countUnitNum(UnitTypes::Terran_Starport,1) < 1)
		{
			this->bom->build(1,UnitTypes::Terran_Starport,110);
		}
		else
		{
			this->bom->build(1,UnitTypes::Terran_Science_Facility,106);
			this->bom->build(1,UnitTypes::Terran_Control_Tower,108);
		}
	
		if (this->enemyInfo->countUnitNum(UnitTypes::Protoss_Arbiter) > 3)
		{
			this->bom->build(this->enemyInfo->countUnitNum(UnitTypes::Protoss_Arbiter),UnitTypes::Terran_Science_Vessel,104);
		}
		else
		{
			this->bom->build(2,UnitTypes::Terran_Science_Vessel,104);
		}

		if (!this->bom->plannedTech(TechTypes::EMP_Shockwave) && !Broodwar->self()->hasResearched(TechTypes::EMP_Shockwave))
		{
			this->bom->research(TechTypes::EMP_Shockwave,72);
		}

		if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::Titan_Reactor) < 1)
		{
			this->bom->upgrade(1,UpgradeTypes::Titan_Reactor,68);
		}
	}

	//for against protoss dark templar
	if (enemyInfo->EnemyhasBuilt(UnitTypes::Protoss_Dark_Templar,1) ||
		  (enemyInfo->EnemyhasBuilt(UnitTypes::Protoss_Templar_Archives,2) && Broodwar->getFrameCount() < 24*60*8))
	{
		if (Broodwar->getFrameCount()%(24*30) == 9)
		{
			Broodwar->printf("Dark Templar!");
		}
		
		if (Broodwar->self()->supplyUsed()/2 > 100)
		{
			if (myInfo->countUnitNum(UnitTypes::Terran_Science_Vessel,2) < 2)
			{
				this->bom->build(1,UnitTypes::Terran_Starport,110);
				this->bom->build(1,UnitTypes::Terran_Control_Tower,108);
				this->bom->build(1,UnitTypes::Terran_Science_Facility,106);
				this->bom->build(2,UnitTypes::Terran_Science_Vessel,104);
			}
		}
		else
		{
			this->bom->build(1,UnitTypes::Terran_Academy,102);
			this->bom->build(this->myInfo->countUnitNum(UnitTypes::Terran_Command_Center,1),UnitTypes::Terran_Comsat_Station,100);
		}
	}

	//for against enemy air unit
	if (this->enemyInfo->EnemyhasBuilt(UnitTypes::Protoss_Stargate,2) || 
		  this->enemyInfo->EnemyhasBuilt(UnitTypes::Terran_Starport,2) || 
		  this->enemyInfo->EnemyhasBuilt(UnitTypes::Zerg_Spire,2))
	{
		if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Armory) > 0)
		{
			if (this->upgradeMng->getPlannedLevel(UpgradeTypes::Charon_Boosters) < 1)
			{
				this->bom->upgrade(1,UpgradeTypes::Charon_Boosters,72);
			}
			if (this->bom->getPlannedCount(UnitTypes::Terran_Goliath,71) < 8)
			{
				this->bom->build(8,UnitTypes::Terran_Goliath,71);
			}
		}
		else
		{
			if (this->bom->getPlannedCount(UnitTypes::Terran_Armory,71) < 1)
			{
				this->bom->build(1,UnitTypes::Terran_Armory,71);
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
	for each (Unit* cc in SelectAll(UnitTypes::Terran_Command_Center))
	{
		int r = cc->getTilePosition() == Broodwar->self()->getStartLocation() ? 32*25 : 32*20;
		for each (Unit* u in Broodwar->getUnitsInRadius(cc->getPosition(),r))
		{
			if (u->getPlayer() != Broodwar->enemy())
			{
				continue;
			}

			UnitType type = u->getType();
			if (type == UnitTypes::Protoss_Scarab || type == UnitTypes::Terran_Vulture_Spider_Mine || type == UnitTypes::Terran_Nuclear_Missile || type == UnitTypes::Protoss_Interceptor)
			{
				continue;
			}

			if (type.canAttack() || type == UnitTypes::Protoss_Carrier || type == UnitTypes::Protoss_Reaver || type == UnitTypes::Terran_Bunker || type == UnitTypes::Protoss_Shuttle)
			{
				enemyInSight.insert(u);	
			}
		}
	}

	//for each (Unit* u in Broodwar->self()->getUnits())
	//{
	//	if (!u->getType().isBuilding() && !u->isCompleted())
	//	{
	//		continue;
	//	}

	//	//unit far from base
	//	if (u->getTilePosition().getDistance(Broodwar->self()->getStartLocation()) > this->baseSightRangeLimitation)
	//	{
	//		continue;
	//	}
	//	
	//	for each (Unit* e in Broodwar->getUnitsInRadius(u->getPosition(),u->getType().sightRange()))
	//	{
	//		if (e->getPlayer() != Broodwar->enemy())
	//		{
	//			continue;
	//		}

	//		if (e->getType() == UnitTypes::Protoss_Scarab || e->getType() == UnitTypes::Terran_Vulture_Spider_Mine)
	//		{
	//			continue;
	//		}
	//			
	//		if (e->isLifted())
	//		{
	//			continue;
	//		}

	//		if (e->getType().canAttack() || e->getType() == UnitTypes::Protoss_Carrier || e->getType() == UnitTypes::Protoss_Reaver)
	//		{
	//			this->enemyInSight.insert(e);	
	//		}
	//	}	
	//}

	// cancel the dying building
	
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
	if (this->enemyInSight.size()>0)
	{
		this->mUnderAttack = true;
		return this->mUnderAttack;
	}
	else
	{
		this->mUnderAttack = false;
		return this->mUnderAttack;
	}
}

void MentalClass::onUnitDestroy(BWAPI::Unit* u)
{
	this->enemyInSight.erase(u);
}

void MentalClass::onUnitMorph(BWAPI::Unit* u)
{
	this->enemyInSight.erase(u);	
}

void MentalClass::onUnitEvade(BWAPI::Unit* u)
{
	this->enemyInSight.erase(u);
}
void MentalClass::destroy()
{
	if (theMentalManager) delete theMentalManager;
}

void MentalClass::upDateSightRange()
{
	this->baseSightRangeLimitation = 30 + Broodwar->self()->supplyUsed()/4;
}

void MentalClass::attackTimingCheck()
{
	//for zerg timing
	if (Broodwar->enemy()->getRace() == Races::Zerg)
	{
		//Broodwar->drawTextScreen(460,30,"\x11 ME: %d | ENEMY: %d",(int)this->myInfo->myFightingValue(),(int)this->enemyInfo->enemyFightingValue());
		//calculate timing
		if (this->STflag==ZrushZergling && SelectAll()(Marine)(isCompleted).not(isLoaded).size()>=10 && Broodwar->getFrameCount()<(24*60*7+24*30))
			this->goAttack = true;
		else
		{
			if (SelectAll()(Marine)(isCompleted).not(isLoaded).size() < 4 || Broodwar->getFrameCount()>(24*60*7+24*30))
				this->goAttack = false;
		}

		if (this->enemyInfo->allenemyFighter.size()>5 || Broodwar->getFrameCount() > 24*60*6)
		{
			int detectorCount = SelectAll()(isCompleted)(Comsat_Station)(Energy,">=",50).size() + SelectAll()(isCompleted)(Science_Vessel).size();
			if (this->goAttack)
			{
				if ((!SelectAllEnemy().not(isDetected)(isAttacking).empty() && detectorCount < 1)
					  ||
					  this->myInfo->myFightingValue().first < this->enemyInfo->enemyFightingValue().second) 
				{
					this->goAttack = false;
				}
			}
			else if (detectorCount > 0)
			{
				if (this->myInfo->myFightingValue().first > this->enemyInfo->enemyFightingValue().first * 2.4 &&
					  this->myInfo->myFightingValue().first > this->enemyInfo->enemyFightingValue().second &&
					  this->myInfo->getMyArmyNum() >= 30)
				{
					this->goAttack = true;
				}
				else if (this->myInfo->getMyArmyNum() >= 50 || Broodwar->self()->supplyUsed()/2 >= 180 || Broodwar->getFrameCount() > 24*60*50)
				{
					this->goAttack = true;
				}
			}
		}
	}
	//for protoss timng
	else if (Broodwar->enemy()->getRace() == Races::Protoss)
	{
		if (this->STflag == PtechCarrier)
		{
			if (Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode) && SelectAll()(Siege_Tank)(isCompleted).size() > 0)
			{
				this->goAttack = true;
			}
			else
			{
				this->goAttack = false;
			}
		}
		else
		{
			if (this->goAttack)
			{
				if(Broodwar->self()->supplyUsed()/2 < 190 && this->myInfo->myFightingValue().first < 1.2 * this->enemyInfo->enemyFightingValue().first)
					this->goAttack = false;
			}
			else
			{
				if (this->tpTiming->attackTimingMap[1] || this->tpTiming->attackTimingMap[2] || this->tpTiming->attackTimingMap[3])
					this->goAttack = true;
			}
		}
	}
	//for terran timing
	else if (Broodwar->enemy()->getRace() == Races::Terran)
	{
		//for 8bb rush
		int killedMarine = Broodwar->self()->deadUnitCount(UnitTypes::Terran_Marine);
		if (Broodwar->getFrameCount()<=24*60*8 && this->myInfo->countUnitNum(UnitTypes::Terran_Marine,1)>=2 && !marineRushOver)
		{
			//if we lost too many marine and barely kill any enemy
			if (killedMarine>=4 && this->enemyInfo->killedEnemyNum < killedMarine)
			{
				this->goAttack = false;	
				marineRushOver = true;
				//cancel rush bunker
				if (this->bom->getPlannedCount(UnitTypes::Terran_Bunker)>0){
					this->bom->deleteItem(UnitTypes::Terran_Bunker,95, BWTA::getRegion(this->scm->enemyStartLocation->getTilePosition()));
				}
				//delete marine
				//&& this->bom->getPlannedCount(UnitTypes::Terran_Marine)>0
				if (SelectAll()(isCompleted)(Marine).size()>=4)
					this->bom->deleteItem(UnitTypes::Terran_Marine);
			}					
			//if our fight value is larger & enemy doesn't have bunker & enemy number <7 
			//else if (this->myInfo->myFightingValue().first > this->enemyInfo->enemyFightingValue().first &&
			//_T_
			else if (this->myInfo->myFightingValue().first * 2 > this->enemyInfo->enemyFightingValue().first &&
				!this->enemyInfo->EnemyhasBuilt(UnitTypes::Terran_Bunker,1) &&
				SelectAllEnemy()(isCompleted)(canAttack).not(isWorker,isBuilding).size()<7)
			{
					//if we already have bunker
					if (this->myInfo->countUnitNum(UnitTypes::Terran_Bunker,1)>0)
					{
						int mrInBk = SelectAll()(isCompleted)(isLoaded)(Marine).size();
						//if we have marine in bunker, then continue rush
						if (mrInBk > 0){
							this->goAttack = true;
							this->bom->build(2,UnitTypes::Terran_Siege_Tank_Tank_Mode,140);
							if(!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode))
								this->bom->research(TechTypes::Tank_Siege_Mode,135);
						}

						//if bunker is empty, and enemy has no vulture, continue rush
						else if (mrInBk == 0 && !this->enemyInfo->EnemyhasBuilt(UnitTypes::Terran_Vulture,1))
							this->goAttack = true;
						this->bom->build(2,UnitTypes::Terran_Siege_Tank_Tank_Mode,140);
						if(!Broodwar->self()->hasResearched(TechTypes::Tank_Siege_Mode))
							this->bom->research(TechTypes::Tank_Siege_Mode,135);
					}
					//if don't have bunker yet
					else
					{
						//enemy has vulture
						if (this->enemyInfo->EnemyhasBuilt(UnitTypes::Terran_Vulture,1) 
							&& this->myInfo->countUnitNum(UnitTypes::Terran_Marine,1)/this->enemyInfo->CountEunitNum(UnitTypes::Terran_Vulture)<3){
								this->goAttack = false;
								marineRushOver = true;
								//cancel bunker plan
								if (this->bom->getPlannedCount(UnitTypes::Terran_Bunker)>0){
									this->bom->deleteItem(UnitTypes::Terran_Bunker,95, BWTA::getRegion(this->scm->enemyStartLocation->getTilePosition()));
								}
								// delete marine
								if (SelectAll()(isCompleted)(Marine).size()>=4 && this->bom->getPlannedCount(UnitTypes::Terran_Marine)>0)
									this->bom->deleteItem(UnitTypes::Terran_Marine);
						}
						//_T_
						//enemy has medic or firebat
						//else if (this->enemyInfo->EnemyhasBuilt(UnitTypes::Terran_Medic,1) || this->enemyInfo->EnemyhasBuilt(UnitTypes::Terran_Firebat,1))
						//{
						//	this->goAttack = false;
						//	marineRushOver = true;
						//	//cancel bunker plan
						//	if (this->bom->getPlannedCount(UnitTypes::Terran_Bunker)>0){
						//		this->bom->deleteItem(UnitTypes::Terran_Bunker,95, BWTA::getRegion(this->scm->enemyStartLocation->getTilePosition()));
						//	}
						//	// delete marine
						//	if (SelectAll()(isCompleted)(Marine).size()>=4 && this->bom->getPlannedCount(UnitTypes::Terran_Marine)>0)
						//		this->bom->deleteItem(UnitTypes::Terran_Marine);
						//}
						else
							this->goAttack = true;
					}
			}				
			else
			{
				this->goAttack = false;
				marineRushOver = true;
				int see = this->bom->getPlannedCount(UnitTypes::Terran_Bunker,95);
				if (this->bom->getPlannedCount(UnitTypes::Terran_Bunker)>0){
					this->bom->deleteItem(UnitTypes::Terran_Bunker,95, BWTA::getRegion(this->scm->enemyStartLocation->getTilePosition()));
					see = this->bom->getPlannedCount(UnitTypes::Terran_Bunker,95);
				}

				if (SelectAll()(isCompleted)(Marine).size()>=4 && this->bom->getPlannedCount(UnitTypes::Terran_Marine)>0)
					this->bom->deleteItem(UnitTypes::Terran_Marine);
			}				
		}
		//_T_
		else
		{
			if (this->goAttack)
			{
				if (Broodwar->self()->supplyUsed()/2 < 180 &&
						SelectAll()(isCompleted)(Battlecruiser)(HitPoints,">",400).size() < 6 &&
					  this->myInfo->myFightingValue().first < this->enemyInfo->enemyFightingValue().first)
				{
					this->goAttack = false;
				}
			}
			else
			{
				if (Broodwar->getFrameCount() > 24*60*10 &&
					  Broodwar->getFrameCount() < 24*60*14 &&
					  Broodwar->self()->supplyUsed()/2 > 120 &&
						this->myInfo->myFightingValue().first > 1.3 * this->enemyInfo->enemyFightingValue().first)
				{
					this->goAttack = true;
				}
				else if (SelectAll()(isCompleted)(Battlecruiser)(HitPoints,">=",400).size() >= 6)
				{
					this->goAttack = true;
				}
				else if (Broodwar->self()->supplyUsed()/2 >= 180 || Broodwar->getFrameCount() > 24*60*50)
				{
					this->goAttack = true;
				}
			}
		}
		//producing air army
		//else if (SelectAll().not(isCompleted)(Battlecruiser).size()>0 && SelectAll()(isCompleted)(Battlecruiser).size()<7 && SelectAll()(isCompleted)(Siege_Tank).size()<10)
		//	this->goAttack = false;
		//else if (this->enemyInfo->allenemyFighter.size()>5 || Broodwar->getFrameCount()>24*60*8)
		//{
		//	int deadArmy = Broodwar->self()->deadUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode)+Broodwar->self()->deadUnitCount(UnitTypes::Terran_Siege_Tank_Siege_Mode)+Broodwar->self()->deadUnitCount(UnitTypes::Terran_Goliath);
		//	if (this->goAttack==true){				
		//		if (deadArmy>=18 && SelectAll()(isCompleted)(Battlecruiser).size()<6)
		//			this->goAttack = false;
		//		if(this->myInfo->myFightingValue().first<=1.4*this->enemyInfo->enemyFightingValue().second)
		//			this->goAttack = false;
		//		//else if (this->enemyInSight.size()>0)
		//		//	this->goAttack = false;
		//	}
		//	else
		//	{
		//		//if we have advantage in early game
		//		if (this->enemyInfo->killedEnemyNum > 5+(Broodwar->self()->deadUnitCount(UnitTypes::Terran_Marine)
		//			+Broodwar->self()->deadUnitCount(UnitTypes::Terran_SCV)+Broodwar->self()->deadUnitCount(UnitTypes::Terran_Siege_Tank_Tank_Mode)*2)
		//			&& Broodwar->getFrameCount()<=24*60*9){
		//				//check if our rush bunker still there
		//				if (this->scm->enemyStartLocation && 
		//					SelectAll().inRegion(this->scm->enemyStartLocation->getRegion())(isCompleted)(Bunker)(LoadedUnitsCount,">=",1).size()>0 
		//					&& SelectAll()(isCompleted)(Siege_Tank).size()>=1){
		//						this->goAttack = true;

		//				}
		//				//if we killed lots of enemy workers
		//				else if (this->enemyInfo->enemyDeadWorker >=10 && Broodwar->self()->supplyUsed()/2>=60 
		//					&& SelectAll()(isCompleted)(Siege_Tank).size()>=5){
		//						this->goAttack = true;
		//				}
		//				//else 
		//				else if (this->myInfo->myFightingValue().first > 2*this->enemyInfo->enemyFightingValue().first && 
		//					Broodwar->self()->supplyUsed()/2>=100){
		//						this->goAttack = true;
		//				}
		//		}
		//		//when we have advantage
		//		else if (this->myInfo->myFightingValue().first > 2*this->enemyInfo->enemyFightingValue().first 
		//			&& this->myInfo->myDeadArmy*2 < this->enemyInfo->killedEnemyNum
		//			&& Broodwar->self()->supplyUsed()/2>80){
		//				this->goAttack = true;
		//		}
		//		else if ((this->myInfo->myFightingValue().first > 1.4*this->enemyInfo->enemyFightingValue().first
		//			&& Broodwar->self()->supplyUsed()/2>130) && deadArmy<20){
		//				this->goAttack = true;
		//		}				
		//		else if (SelectAll()(isCompleted)(Battlecruiser).size()>=6 || Broodwar->self()->supplyUsed()/2>=180)
		//			this->goAttack = true;
		//	}		
		//}
	}
}

void MentalClass::showDebugInfo()
{
	string   atkTarType = MacroManager::create()->getAttackTarget()->getType();
	Position atkTarPos  = MacroManager::create()->getAttackTarget()->getPosition();
	
	if (!this->goAttack)
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
	if (this->myInfo->myFightingValue().first>this->enemyInfo->enemyFightingValue().second)
	{
		Broodwar->drawTextScreen(433,25,"\x07 My_FV: %d > Enemy_DV: %d",(int)this->myInfo->myFightingValue().first,(int)this->enemyInfo->enemyFightingValue().second);
	}
	else if (this->myInfo->myFightingValue().first==this->enemyInfo->enemyFightingValue().second)
	{
		Broodwar->drawTextScreen(433,25,"\x19 My_FV: %d = Enemy_DV: %d",(int)this->myInfo->myFightingValue().first,(int)this->enemyInfo->enemyFightingValue().second);
	}
	else
	{
		Broodwar->drawTextScreen(433,25,"\x08 My_FV: %d < Enemy_DV: %d",(int)this->myInfo->myFightingValue().first,(int)this->enemyInfo->enemyFightingValue().second);
	}

	// third line
	if (this->myInfo->myFightingValue().second > this->enemyInfo->enemyFightingValue().first)
	{
		Broodwar->drawTextScreen(433,35,"\x07 My_DV: %d > Enemy_FV: %d",(int)this->myInfo->myFightingValue().second,(int)this->enemyInfo->enemyFightingValue().first);
	}
	else if (this->myInfo->myFightingValue().second == this->enemyInfo->enemyFightingValue().first)
	{
		Broodwar->drawTextScreen(433,35,"\x19 My_DV: %d = Enemy_FV: %d",(int)this->myInfo->myFightingValue().second,(int)this->enemyInfo->enemyFightingValue().first);
	}
	else
	{
		Broodwar->drawTextScreen(433,35,"\x08 My_DV: %d < Enemy_FV: %d",(int)this->myInfo->myFightingValue().second,(int)this->enemyInfo->enemyFightingValue().first);
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