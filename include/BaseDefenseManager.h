#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include <Arbitrator.h>
#include "Base.h"
#include "BaseManager.h"
#include "Common.h"
#include "UnitGroupManager.h"

class BaseClass;

class BaseDefenseManager : public Arbitrator::Controller<BWAPI::Unit*,double>
{
public:

	static BaseDefenseManager* create();
	static void destroy();

	void setArbitrator(Arbitrator::Arbitrator<BWAPI::Unit*,double>* arbitrator) {this->arbitrator = arbitrator;}

	std::string getName() const {return "BaseDefenseManager";}

	void update();
	void onUnitDestroy(BWAPI::Unit*);
	void onOffer(std::set<BWAPI::Unit*> units);
	void onRevoke(BWAPI::Unit* unit, double bid);

protected:

	BaseDefenseManager();
	~BaseDefenseManager();

private:

	Arbitrator::Arbitrator<BWAPI::Unit*,double>* arbitrator;

	UnitGroup tanks;
	std::map<BWAPI::Unit*,BaseClass*> requestedUnits;
};