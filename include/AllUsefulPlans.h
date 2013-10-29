#include <BWAPI.h>
#pragma once;
enum gameStage
{
	unknown,
	early,
	middle,
	late
};

enum enemyRace
{
	eZerg,
	eProtoss,
	eTerran
};

enum myGameState
{
		mAdvantage,
		Balance,
		mDisadvantage
};

enum allEnemyPlan
{
	//Protoss plans
		pBalance,
		pEconomic,
		pAggressive,
		pTech,
	//Zerg plans
		zBalance,
		z5DRush,
		zAggressive,
		zEconomic,
		zTech,
	//Terran plans
		tBalance,
		tAggressive,
		tEconomic,
		tTech,
};
