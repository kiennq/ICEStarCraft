#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include <string>
#include <map>
#include <vector>

namespace ICEStarCraft
{
	class Map
	{
	public:
		Map();
		
		void makeKnown();
		void setName(std::string);
		void setHash(std::string);
		void setBaseInfo(std::map<BWAPI::TilePosition,std::vector<BWAPI::Position>>);
		void setTankDropPositions(std::map<BWAPI::TilePosition,BWAPI::TilePosition>);
		void setTankSiegePositions(std::map<BWAPI::TilePosition,BWAPI::Position>);
		
		bool operator == (const Map m) const
		{ 
			return m.getHash() == _Hash;
		}

		bool operator != (const Map m) const
		{ 
			return m.getHash() != _Hash;
		}

		bool isKnown() const;
		std::string getName() const;
		std::string getHash() const;
		std::vector<BWAPI::Position> getChokepoints(BWAPI::TilePosition);
		BWAPI::TilePosition getTankDropPosition(BWAPI::TilePosition);
		BWAPI::Position getTankSiegePosition(BWAPI::TilePosition);
		std::map<BWAPI::TilePosition,BWAPI::TilePosition> getTankDropPositions();
		std::map<BWAPI::TilePosition,BWAPI::Position>& getTankSiegePositions();

	private:
		bool _Known;
		std::string _Name;
		std::string _Hash;

		// start locations and useful choke points (first chokepoint, second chokepoint, third chokepoint)
		std::map<BWAPI::TilePosition,std::vector<BWAPI::Position>> _BaseInfo;

		// base locations and positions to drop tanks on high ground 
		std::map<BWAPI::TilePosition,BWAPI::TilePosition> _TankDropPositions;
		std::map<BWAPI::TilePosition,BWAPI::Position> _TankSiegePositions;
	};

	class MapInfo
	{
	public:
		MapInfo();

		Map getMapInfo();

	private:
		std::map<std::string, Map> _MapInfo;
	};

	namespace Maps
	{
		void initialize();

		extern Map Benzene;
		extern Map Destination;
		extern Map Heartbreak_Ridge;
		extern Map Aztec;
		extern Map Neo_Moon_Glaive;
		extern Map Tau_Cross;
		extern Map Andromeda;
		extern Map Circuit_Breaker;
		extern Map Electric_Circuit;
		extern Map Empire_of_the_Sun;
		extern Map Fighting_Spirit;
		extern Map Fortress;
		extern Map Icarus;
		extern Map Jade;
		extern Map La_Mancha;
		extern Map Python;
		extern Map Roadrunner;

		extern Map Unknown;
	}
}