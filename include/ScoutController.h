#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <map>
#include <list>
#include "Helper.h"
#include "Vector2.h"


#ifndef _SCOUT_DEBUG
#define _SCOUT_DEBUG
#endif

#ifndef _LOG_TO_FILE
//#define _LOG_TO_FILE
#endif

#ifndef _HUMAN_PLAY
//#define _HUMAN_PLAY
#endif

// Need design for multithreading

namespace ICEStarCraft {

  class ScoutController
	{
	public:

		static ScoutController* create();
		static void destroy();

		/** Main loop callback function */
		void onFrame(); /** This method should be designed to be called in main thread, need relay messages */
		void onUnitDestroy(BWAPI::Unit *u);
		void onUnitDiscover(BWAPI::Unit *u);
		void onSendText(const std::string& text);
    void detectUnseenUnitInsideBunker();
    std::list<BWAPI::Position>& getBorder(BWTA::Region* r);

    void addToScoutSet(BWAPI::Unit *u);
    void removeFromScoutSet(BWAPI::Unit *u);

		template<size_t N> void setParams(const double (&array)[N]);

		/** For testing with turn around method */
		Vector2 rotateAroundBuilding(BWAPI::Unit* scout);

		/**
		* Potential field value emited by unit u at position p
		* Potential field should be in the needle shape
		* Potential flow needed
		*/
		Vector2 calculatePVal(BWAPI::Unit* scout);

		Vector2 unitPVal(BWAPI::Unit* u, BWAPI::Unit* s);
		Vector2 regionPVal (BWTA::Region* r, BWAPI::Unit* s);
		Vector2 borderPVal (BWTA::Region* r, BWAPI::Unit* s);

		Vector2 vortexPotential(const BWAPI::Position& s, const BWAPI::Position& p);
		Vector2 sourcePotential(const BWAPI::Position& s, const BWAPI::Position& p);
		Vector2 obsVortexPotential(const BWAPI::Position& s, const BWAPI::Position& p, const BWAPI::Position& c, double a2);
		Vector2 obsSourcePotential(const BWAPI::Position& s, const BWAPI::Position& p, const BWAPI::Position& c, double a2);
		Vector2 needlePotentialVal(const BWAPI::Position& s, const BWAPI::Position& p, const BWAPI::Position& target, double bias);

    /* <unit , < position , reverse> >  */
    std::map<BWAPI::Unit*, std::pair<BWAPI::Position, int> > _scouts;
    // For unseen bunker, assume only one type of unit inside
    std::map<BWAPI::Unit*, BWAPI::UnitType> _enBunker;
		std::set<BWTA::BaseLocation*> _enBaseLoacation;
		std::set<BWAPI::Unit*> _enBuildings;
		std::map<BWAPI::UnitType,std::set<BWAPI::Unit*>> _enUnits;
    std::map<BWTA::Region*, std::list<BWAPI::Position>> _border;
#ifdef _LOG_TO_FILE
		int _enDiscoveredBuildings;
#endif

	protected:
		ScoutController();
		virtual ~ScoutController();

	private:
		int _paralength;
		double* _p;
    // Custom unit info
    static struct _UnitInfo {
      BWAPI::Position pos;
      int iFrame;
      // when stucked, click on predefined mineral
      bool fStuck;
      BWAPI::Unit* mineral;
      // unit will fade after a certain time
      int ctFadeThres;

      _UnitInfo(const BWAPI::Position& pos, int iFrame, bool fStuck, BWAPI::Unit* mineral)
        : pos(pos)
        , iFrame(iFrame)
        , fStuck(fStuck)
        , mineral(mineral) {}

      _UnitInfo(const BWAPI::Position& pos, int iFrame, int ctFadeThres)
        : pos(pos)
        , iFrame(iFrame)
        , ctFadeThres(ctFadeThres) {} 

      _UnitInfo(){}
    };
    std::map<BWAPI::Unit*, _UnitInfo> _scoutLastPositions;
    // Store the position of enemy units
    std::map<BWAPI::Unit*, _UnitInfo> _eUnitPos;

#ifdef _SCOUT_DEBUG
		bool show_object_r;
		bool show_unit_p;
		bool show_region_p;
		bool show_border_p;
		bool show_all_p;

    BWAPI::Position _screenPos;
		std::map<BWAPI::Unit*, std::list<Vector2>> _unit_p;
		std::map<BWAPI::Unit*, std::list<Vector2>> _region_p;
		std::map<BWAPI::Unit*, std::list<Vector2>> _border_p;
		std::map<BWAPI::Unit*, std::list<Vector2>> _all_p;
		std::map<BWAPI::Unit*, std::list<std::pair<BWAPI::Position, int>>> _obj;
#endif // _SCOUT_DEBUG

    void _registerEnUnitPosition();
    // Fade unit instantly
    void _fadeUnit(BWAPI::Unit* u);
    void _fadeUnits();

	};

  

	template<size_t N>
	void ICEStarCraft::ScoutController::setParams( const double (&array)[N] )
	{
		_paralength = N;
		_p = new double[_paralength];
		for (int i = 0; i < N; i++) _p[i] = array[i];
	}

}