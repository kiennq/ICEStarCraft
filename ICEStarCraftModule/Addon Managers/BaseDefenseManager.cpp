#include "BaseDefenseManager.h"

using namespace std;
using namespace BWAPI;
using namespace BWTA;

BaseDefenseManager* theBaseDefenseManager = NULL;

BaseDefenseManager* BaseDefenseManager::create() 
{
	if (theBaseDefenseManager) return theBaseDefenseManager;
	else return theBaseDefenseManager = new BaseDefenseManager();
}

void BaseDefenseManager::destroy()
{
	if (theBaseDefenseManager)
	{
		delete theBaseDefenseManager;
		theBaseDefenseManager = NULL;
	}
}

BaseDefenseManager::BaseDefenseManager()
{
	arbitrator = NULL;
	tanks.clear();
	requestedUnits.clear();
}

BaseDefenseManager::~BaseDefenseManager()
{

}

void BaseDefenseManager::onOffer(set<Unit*> units)
{
	for each (Unit* u in units)
	{
		map<Unit*,BaseClass*>::iterator i = requestedUnits.find(u);
		if (i == requestedUnits.end())
		{
			arbitrator->decline(this, u, 0);
			continue;
		}

		BaseClass* b = i->second;
		if (!b->protectors.empty())
		{
			if (b->protectors.find(u) == b->protectors.end())
			{
				arbitrator->decline(this, u, 0);
			}
			continue;
		}

		arbitrator->accept(this, u);
		b->protectors.insert(u);
		tanks.insert(u);
	}
}

void BaseDefenseManager::onRevoke(BWAPI::Unit* unit, double bid)
{
	for each (BaseClass* b in baseManager->getBaseSet())
	{
		if (b->protectors.find(unit) != b->protectors.end())
		{
			b->protectors.erase(unit);
		}
	}

	tanks.erase(unit);
}

void BaseDefenseManager::onUnitDestroy(BWAPI::Unit* unit)
{
	if (!unit)
	{
		return;
	}

	for each (BaseClass* b in baseManager->getBaseSet())
	{
		if (b->protectors.find(unit) != b->protectors.end())
		{
			b->protectors.erase(unit);
		}
	}

	tanks.erase(unit);
}

void BaseDefenseManager::update()
{
	if (Broodwar->enemy()->getRace() != Races::Terran || Broodwar->getFrameCount()%(24*5) != 48)
	{
		return;
	}

	if (SelectAll()(isCompleted)(Siege_Tank).size() >= 10 && tanks.size() < baseManager->getBaseSet().size())
	{
		for each (BaseClass* b in baseManager->getBaseSet())
		{
			if (!b->protectors.empty())
			{
				continue;
			}

			for each (Unit* u in SelectAll()(isCompleted)(Siege_Tank)(HitPoints,">=",120))
			{
        if (u->getTilePosition().getDistance(Broodwar->self()->getStartLocation()) > 30)
        {
          continue;
        }

				if (tanks.find(u) != tanks.end())
				{
					continue;
				}

				arbitrator->setBid(this, u, 480);
				requestedUnits.insert(make_pair(u, b));
				break;
			}
		}
	}
	
  for each (BaseClass* b in baseManager->getBaseSet())
  {
    for each (Unit* u in b->protectors)
    {
      MicroUnitControl::tankAttack(u,b->getBaseLocation()->getPosition(),32*3);
    }
  }
}