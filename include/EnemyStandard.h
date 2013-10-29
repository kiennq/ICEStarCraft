#include <BWAPI.h>
#include <BWTA.h>
#include "AllUsefulPlans.h"
#pragma once;
class ProtossStandard{
public:
	ProtossStandard();
	void P_Economic(int cf);
	void P_Aggressive(int cf);
	void P_Tech(int cf);
	void P_Balance(int cf);
	int currentframe;
	int simuframe;
	int basepopulation;
	int baseBG;
	int bglimitation;
	int baseNexus;
	int totleMoney;
	int pupulationGrow;
	int bgGrow;
	int nexusGrow;
	int moneyGrow;
	gameStage currentStage;


};