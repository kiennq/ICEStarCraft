#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include "InformationManager.h"
#include "UnitGroup.h"
using namespace BWAPI;
using namespace BWTA;
using namespace std;


class MyInfoManager;
class EnemyInfoManager;


class TPTiming
{
public:
	static TPTiming* create();
	static void destroy();
	void CheckTiming();
	map<int,bool> attackTimingMap;
protected:
	TPTiming();
private:

	MyInfoManager* mInfo;
	EnemyInfoManager* eInfo;
};