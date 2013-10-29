#include "EnemyStandard.h"
#include <time.h>
ProtossStandard::ProtossStandard()
{
	this->currentframe = 0;
	this->simuframe = 0;
	this->basepopulation = 0;
	this->baseBG = 0;
	this->baseNexus = 0;
	this->totleMoney = 0;
	this->pupulationGrow = 0;
	this->bgGrow = 0;
	this->nexusGrow = 0;
	this->moneyGrow = 0;
	this->bglimitation = 0;
	this->currentStage = unknown;

}

void ProtossStandard::P_Economic(int cf)
{
	this->currentframe = cf;
	//if time between 4:50--5:10 
	if (this->currentframe >=6960 && this->currentframe <= 7440){
		this->basepopulation = 40;
		this->baseBG = 3;
		this->baseNexus = 2;
		this->moneyGrow = 38;
		this->bglimitation = 4;
	}
	//if time between 9:50--10:10 
	if (this->currentframe >=14160 && this->currentframe <= 14640){
		this->basepopulation = 125;
		this->baseBG = 9;
		this->baseNexus = 3;
		this->moneyGrow = 56;
		this->bglimitation = 10;
	}
	//if time >15:00 
	if (this->currentframe >21600){
		this->basepopulation = 200;
		this->baseBG = 12;
		this->baseNexus = 4;
		this->moneyGrow = 78;
		this->bglimitation = 20;
	}
	//if time between 5:10 and 9:50
	else if (this->currentframe > 7440 && this->currentframe < 14160){
		this->simuframe = this->currentframe - 7440;
		this->basepopulation = 40;
		this->baseBG = 3;
		this->baseNexus = 2;
		this->moneyGrow = 38;
		this->bglimitation = 10;
		this->currentStage = early;
		if (this->currentframe>=9360){
			time_t t;
			srand((unsigned)time(&t)); // time seed	
			this->baseNexus = 2+rand()%2;
			if (this->baseNexus==3){
				this->moneyGrow =45;
			}
		}
	}
	// if time between 10:10 and 15:00 
	else if (this->currentframe > 14640 && this->currentframe < 21600){
		this->simuframe = this->currentframe - 14640;
		this->basepopulation = 125;
		this->baseBG = 9;
		this->baseNexus = 3;
		this->moneyGrow = 56;
		this->bglimitation = 15;
		this->currentStage = middle;
	}

}


void ProtossStandard::P_Tech(int cf)
{
	this->currentframe = cf;
	//if time between 4:50--5:10 
	if (this->currentframe >=6960 && this->currentframe <= 7440){
		this->basepopulation = 35;
		this->baseBG = 2;
		this->baseNexus = 1;
		this->moneyGrow = 21;
		this->bglimitation = 2;
	}
	//if time between 9:50--10:10 
	if (this->currentframe >=14160 && this->currentframe <= 14640){
		this->basepopulation = 115;
		this->baseBG = 7;
		this->baseNexus = 3;
		this->moneyGrow = 56;
		this->bglimitation = 10;
	}
	//if time >12:00 
	if (this->currentframe >17280){
		this->basepopulation = 185;
		this->baseBG = 9;
		this->baseNexus = 4;
		this->moneyGrow = 78;
		this->bglimitation = 15;
	}
	//if time between 5:10 and 9:50
	else if (this->currentframe > 7440 && this->currentframe < 14160){
		this->simuframe = this->currentframe - 7440;
		this->basepopulation = 35;
		this->baseBG = 2;
		this->baseNexus = 2;
		this->moneyGrow = 38;
		this->bglimitation = 8;
		this->currentStage = early;
		//if time>6:30
		if (this->currentframe>=10080){
			time_t t;
			srand((unsigned)time(&t)); // time seed	
			this->baseNexus = 2+rand()%2;
			if (this->baseNexus==3){
				this->moneyGrow =42;
			}
		}
	}
	// if time between 10:10 and 12:00 
	else if (this->currentframe > 14640 && this->currentframe < 17280){
		this->simuframe = this->currentframe - 14640;
		this->basepopulation = 115;
		this->baseBG = 7;
		this->baseNexus = 3;
		this->moneyGrow = 56;
		this->bglimitation = 15;
		this->currentStage = middle;
	}
}

void ProtossStandard::P_Aggressive(int cf)
{
	this->currentframe = cf;
	//if time between 4:50--5:10 
	if (this->currentframe >=6960 && this->currentframe <= 7440){
		this->basepopulation = 35;
		this->baseBG = 2;
		this->baseNexus = 1;
		this->moneyGrow = 21;
		this->bglimitation = 4;
	}
	//if time between 9:50--10:10 
	if (this->currentframe >=14160 && this->currentframe <= 14640){
		this->basepopulation = 115;
		this->baseBG = 7;
		this->baseNexus = 3;
		this->moneyGrow = 56;
		this->bglimitation = 10;
	}
	//if time >12:00 
	if (this->currentframe >17280){
		this->basepopulation = 185;
		this->baseBG = 9;
		this->baseNexus = 4;
		this->moneyGrow = 78;
		this->bglimitation = 15;
	}
	//if time between 5:10 and 9:50
	else if (this->currentframe > 7440 && this->currentframe < 14160){
		this->simuframe = this->currentframe - 7440;
		this->basepopulation = 35;
		this->baseBG = 2;
		this->baseNexus = 2;
		this->moneyGrow = 38;
		this->bglimitation = 8;
		this->currentStage = early;
		//if time>6:30
		if (this->currentframe>=10080){
			time_t t;
			srand((unsigned)time(&t)); // time seed	
			this->baseNexus = 2+rand()%2;
			if (this->baseNexus==3){
				this->moneyGrow =42;
				this->baseBG=3;
				this->basepopulation=50;

			}
			else{
				this->baseBG=4;
				this->basepopulation=60;
			}
		}
	}
	// if time between 10:10 and 12:00 
	else if (this->currentframe > 14640 && this->currentframe < 17280){
		this->simuframe = this->currentframe - 14640;
		this->basepopulation = 115;
		this->baseBG = 7;
		this->baseNexus = 3;
		this->moneyGrow = 56;
		this->bglimitation = 15;
		this->currentStage = middle;
	}
}

void ProtossStandard::P_Balance(int cf)
{
	this->currentframe = cf;
	//if time between 4:50--5:10 
	if (this->currentframe >=6960 && this->currentframe <= 7440){
		this->basepopulation = 35;
		this->baseBG = 2;
		this->baseNexus = 1;
		this->moneyGrow = 21;
		this->bglimitation = 2;
	}
	//if time between 9:50--10:10 
	if (this->currentframe >=14160 && this->currentframe <= 14640){
		this->basepopulation = 115;
		this->baseBG = 7;
		this->baseNexus = 3;
		this->moneyGrow = 56;
		this->bglimitation = 10;
	}
	//if time >12:00 
	if (this->currentframe >17280){
		this->basepopulation = 185;
		this->baseBG = 9;
		this->baseNexus = 4;
		this->moneyGrow = 78;
		this->bglimitation = 15;
	}
	//if time between 5:10 and 9:50
	else if (this->currentframe > 7440 && this->currentframe < 14160){
		this->simuframe = this->currentframe - 7440;
		this->basepopulation = 35;
		this->baseBG = 2;
		this->baseNexus = 2;
		this->moneyGrow = 38;
		this->bglimitation = 8;
		this->currentStage = early;
		//if time>6:30
		if (this->currentframe>=10080){
			time_t t;
			srand((unsigned)time(&t)); // time seed	
			this->baseNexus = 2+rand()%2;
			if (this->baseNexus==3){
				this->moneyGrow =42;
			}
		}
	}
	// if time between 10:10 and 12:00 
	else if (this->currentframe > 14640 && this->currentframe < 17280){
		this->simuframe = this->currentframe - 14640;
		this->basepopulation = 115;
		this->baseBG = 7;
		this->baseNexus = 3;
		this->moneyGrow = 56;
		this->bglimitation = 15;
		this->currentStage = middle;
	}

}