#include <SupplyManager.h>
#include <UnitGroupManager.h>

SupplyManager* theSupplyManager = NULL;

SupplyManager* SupplyManager::create()
{
	if (theSupplyManager) return theSupplyManager;
	else return theSupplyManager = new SupplyManager();
}


void SupplyManager::destroy()
{
	if (theSupplyManager) delete theSupplyManager;
}

SupplyManager::SupplyManager()
{
	this->buildManager      = NULL;
	this->buildOrderManager = NULL;
	this->lastFrameCheck    = 0;
	this->seedPosition      = BWAPI::TilePositions::None;
}

void SupplyManager::setBuildManager(BuildManager* buildManager)
{
	this->buildManager = buildManager;
}
void SupplyManager::setBuildOrderManager(BuildOrderManager* buildOrderManager)
{
	this->buildOrderManager = buildOrderManager;
}
void SupplyManager::update()
{
	if (BWAPI::Broodwar->getFrameCount()>lastFrameCheck+25)
	{
		lastFrameCheck=BWAPI::Broodwar->getFrameCount();
		double productionCapacity       = 0; // original one is int
		lastFrameCheck               = BWAPI::Broodwar->getFrameCount();
		std::set<BWAPI::Unit*> units = BWAPI::Broodwar->self()->getUnits();
		int supplyBuildTime = BWAPI::Broodwar->self()->getRace().getSupplyProvider().buildTime();
		int time = BWAPI::Broodwar->getFrameCount() + supplyBuildTime*2;
		for(std::set<BuildOrderManager::MetaUnit*>::iterator i = this->buildOrderManager->MetaUnitPointers.begin(); i != this->buildOrderManager->MetaUnitPointers.end(); i++)
		{
			std::set<BWAPI::UnitType> m=this->buildOrderManager->unitsCanMake(*i,time);
			double max=0;   // original one is int
			for(std::set<BWAPI::UnitType>::iterator j=m.begin();j!=m.end();j++)
			{
				double s=j->supplyRequired();  // original one is int
				if (j->isTwoUnitsInOneEgg())
					s*= 1.8;//1.35
				if (j->buildTime()<supplyBuildTime && (*i)->getType().getRace()!=BWAPI::Races::Zerg)
					s*= 1.8;//1.2
				if (s > max)
					max=s;
			}
			productionCapacity += max;
		}
		if (getPlannedSupply() <= BWAPI::Broodwar->self()->supplyUsed() + (int)productionCapacity)
		{
			if (TerrainManager::create()->bsPos != TilePositions::None && SelectAll()(Supply_Depot)(isCompleted).size() == 1)
			{
				this->buildOrderManager->buildAdditional(1,BWAPI::Broodwar->self()->getRace().getSupplyProvider(),1000,TerrainManager::create()->bsPos);
			}
			else
			{
				this->buildOrderManager->buildAdditional(1,BWAPI::Broodwar->self()->getRace().getSupplyProvider(),1000,seedPosition);
			}
		}
	}
}

std::string SupplyManager::getName() const
{
	return "Supply Manager";
}

int SupplyManager::getPlannedSupply() const
{
	int plannedSupply=0;
	//planned supply depends on the the amount of planned supply providers times the amount of supply they provide.
	plannedSupply+=buildOrderManager->getPlannedCount(BWAPI::UnitTypes::Terran_Supply_Depot)*BWAPI::UnitTypes::Terran_Supply_Depot.supplyProvided();
//	plannedSupply+=buildOrderManager->getPlannedCount(BWAPI::UnitTypes::Terran_Command_Center)*BWAPI::UnitTypes::Terran_Command_Center.supplyProvided();
	//plannedSupply+=buildOrderManager->getPlannedCount(BWAPI::UnitTypes::Protoss_Pylon)*BWAPI::UnitTypes::Protoss_Pylon.supplyProvided();
	//plannedSupply+=buildOrderManager->getPlannedCount(BWAPI::UnitTypes::Zerg_Overlord)*BWAPI::UnitTypes::Zerg_Overlord.supplyProvided();

	plannedSupply+=SelectAll()(Command_Center)(isCompleted).size()*BWAPI::UnitTypes::Terran_Command_Center.supplyProvided();
	//plannedSupply+=SelectAll()(Supply_Depot)(isCompleted||isConstructing).size()*BWAPI::UnitTypes::Terran_Supply_Depot.supplyProvided();
	//plannedSupply+=SelectAll(BWAPI::UnitTypes::Protoss_Nexus).size()*BWAPI::UnitTypes::Protoss_Nexus.supplyProvided();
	//plannedSupply+=SelectAll(BWAPI::UnitTypes::Zerg_Hatchery).size()*BWAPI::UnitTypes::Zerg_Hatchery.supplyProvided();
	//plannedSupply+=SelectAll(BWAPI::UnitTypes::Zerg_Lair).size()*BWAPI::UnitTypes::Zerg_Lair.supplyProvided();
	//plannedSupply+=SelectAll(BWAPI::UnitTypes::Zerg_Hive).size()*BWAPI::UnitTypes::Zerg_Hive.supplyProvided();
	return plannedSupply;
}
int SupplyManager::getSupplyTime(int supplyCount) const
{
	if (getPlannedSupply()<supplyCount)
		return -1; //not planning to make this much supply

	if (BWAPI::Broodwar->self()->supplyTotal()>=supplyCount)
		return BWAPI::Broodwar->getFrameCount(); //already have this much supply

	int supply=BWAPI::Broodwar->self()->supplyTotal();
	int time = BWAPI::Broodwar->getFrameCount();
	std::set<BWAPI::Unit*> units = SelectAll()(-isCompleted);
	std::map<int, int> supplyAdditions;
	for(std::set<BWAPI::Unit*>::iterator i = units.begin(); i != units.end(); i++)
	{
		if ((*i)->getType().supplyProvided()>0)
		{
			supplyAdditions[time+(*i)->getRemainingBuildTime()]+=(*i)->getType().supplyProvided();
		}
	}
	for(std::map<int,int>::iterator i=supplyAdditions.begin();i!=supplyAdditions.end();i++)
	{
		time=i->second;
		supply+=i->first;
		if (supply>=supplyCount)
			return time;
	}
	return -1;
}

void SupplyManager::setSeedPosition(BWAPI::TilePosition p)
{
	this->seedPosition = p;
}
