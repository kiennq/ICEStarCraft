#pragma once
#include <BWTA.h>
#include <BWAPI.h>
#include <map>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "UnitGroup.h"

using namespace BWAPI;
using namespace BWTA;
using namespace std;

class UnitController
{
public:
	virtual UnitController ();
	addUnit(UnitGroup group) {this->group.insert(group.begin(),group.end());};
	addUnit(Unit* u) {this->group.insert(u)};
	removeUnit(UnitGroup group) {this->group = this->group - group;};
	removeUnit(Unit* u) {this->group.erase(u);};
protected:
	UnitGroup group;
	virtual ~UnitController();
	UnitType type;
};

class VultureController:UnitController
{
public:
	void initMining(Position* s, Position* e);
	void doMining();
private:
	vector<TilePosition> miningPath;
	
};