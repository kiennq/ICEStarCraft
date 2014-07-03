#include <MapInfo.h>

using namespace BWAPI;
using namespace BWTA;
using namespace std;
using namespace ICEStarCraft;

Map::Map()
{
	_Known = false;
	_BaseInfo.clear();
	_TankDropPositions.clear();
}

void Map::makeKnown()
{
	_Known = true;
}

void Map::setName(string name)
{
	_Name = name;
}

void Map::setHash(string hash)
{
	_Hash = hash;
}

void Map::setBaseInfo(map<TilePosition,vector<Position>> baseinfo)
{
	_BaseInfo = baseinfo;
}

void Map::setTankDropPositions(map<TilePosition,TilePosition> TankDropPosition)
{
	_TankDropPositions = TankDropPosition;
}

void Map::setTankSiegePositions(map<TilePosition,Position> TankSiegePosition)
{
	_TankSiegePositions = TankSiegePosition;
}

bool Map::isKnown() const
{
	return _Known;
}

string Map::getName() const
{
	return _Name;
}

string Map::getHash() const
{
	return _Hash;
}

vector<Position> Map::getChokepoints(TilePosition BaseTileLocation)
{
	vector<Position> chokepoints;
	if (_BaseInfo.find(BaseTileLocation) != _BaseInfo.end())
	{
		chokepoints = _BaseInfo[BaseTileLocation];
	}
	return chokepoints;
}

TilePosition Map::getTankDropPosition(TilePosition tp)
{
	if (_TankDropPositions.find(tp) != _TankDropPositions.end())
	{
		return _TankDropPositions[tp];
	}
	return TilePositions::None;
}

map<TilePosition,TilePosition> Map::getTankDropPositions()
{
	return _TankDropPositions;
}

Position Map::getTankSiegePosition(TilePosition tp)
{
	map<TilePosition, Position>::iterator i = _TankSiegePositions.find(tp);
	if (i != _TankSiegePositions.end())
	{
		return i->second;
	}
	return Positions::None;
}

map<TilePosition,BWAPI::Position>& Map::getTankSiegePositions()
{
	return _TankSiegePositions;
}

/************************************************************************/

namespace ICEStarCraft
{
	namespace Maps
	{
		Map Benzene;
		Map Destination;
		Map Heartbreak_Ridge;
		Map Aztec;
		Map Neo_Moon_Glaive;
		Map Tau_Cross;
		Map Andromeda;
		Map Circuit_Breaker;
		Map Electric_Circuit;
		Map Empire_of_the_Sun;
		Map Fighting_Spirit;
		Map Fortress;
		Map Icarus;
		Map Jade;
		Map La_Mancha;
		Map Python;
		Map Roadrunner;

		Map Unknown;

		void initialize()
		{
			Map _map;
			vector<Position> _chokepoints;
			map<TilePosition,vector<Position>> _baseinfo;
			map<TilePosition,TilePosition> _tankdropposition;
			map<TilePosition,Position> _tanksiegeposition;


			/************************************************************************/
			/* Benzene                                                              */
			/************************************************************************/

			_map = Map();
			_map.makeKnown();
			_map.setName("Benzene");
			_map.setHash("af618ea3ed8a8926ca7b17619eebcb9126f0d8b1");
			_baseinfo.clear();

			// for start location at (7,96)
			_chokepoints.clear();
			_chokepoints.push_back(Position(658,2496));
			_chokepoints.push_back(Position(380,2000));
			_chokepoints.push_back(Position(448,1732));
			_baseinfo.insert(make_pair(TilePosition(7,96),_chokepoints));

			// for start location at (117,13)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3443,1072));
			_chokepoints.push_back(Position(3640,1872));
			_chokepoints.push_back(Position(3063,1747));
			_baseinfo.insert(make_pair(TilePosition(117,13),_chokepoints));

			_map.setBaseInfo(_baseinfo);

			_tankdropposition.clear();
			_tankdropposition.insert(make_pair(TilePosition(113,40),TilePosition(124,47)));
			_tankdropposition.insert(make_pair(TilePosition(11,70),TilePosition(3,63)));
			_map.setTankDropPositions(_tankdropposition);

			Benzene = _map;

			/************************************************************************/
			/* Destination                                                          */
			/************************************************************************/

			_map = Map();
			_map.makeKnown();
			_map.setName("Destination");
			_map.setHash("4e24f217d2fe4dbfa6799bc57f74d8dc939d425b");
			_baseinfo.clear();

			// for start location at (31,7)
			_chokepoints.clear();
			_chokepoints.push_back(Position(1762,406));
			_chokepoints.push_back(Position(1620,923));
			_chokepoints.push_back(Position(1573,2105));
			_baseinfo.insert(make_pair(TilePosition(31,7),_chokepoints));

			// for start location at (64,118)
			_chokepoints.clear();
			_chokepoints.push_back(Position(1331,3681));
			_chokepoints.push_back(Position(1272,3265));
			_chokepoints.push_back(Position(1573,2105));
			_baseinfo.insert(make_pair(TilePosition(64,118),_chokepoints));

			_map.setBaseInfo(_baseinfo);
			Destination = _map;


			/************************************************************************/
			/* Heartbreak Ridge                                                     */
			/************************************************************************/

			_map = Map();
			_map.makeKnown();
			_map.setName("Heartbreak Ridge");
			_map.setHash("6f8da3c3cc8d08d9cf882700efa049280aedca8c");
			_baseinfo.clear();

			// for start location at (7,37)
			_chokepoints.clear();
			_chokepoints.push_back(Position(472,1829));
			_chokepoints.push_back(Position(708,2236));
			_chokepoints.push_back(Position(1624,2464));
			_baseinfo.insert(make_pair(TilePosition(7,37),_chokepoints));

			// for start location at (117,56)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3608,1273));
			_chokepoints.push_back(Position(3352,868));
			_chokepoints.push_back(Position(2925,1388));
			_baseinfo.insert(make_pair(TilePosition(117,56),_chokepoints));

			_map.setBaseInfo(_baseinfo);

			_tankdropposition.clear();
			_tankdropposition.insert(make_pair(TilePosition(111,28),TilePosition(124,27)));
			_tankdropposition.insert(make_pair(TilePosition(13,66),TilePosition(3,68)));
			_map.setTankDropPositions(_tankdropposition);

			Heartbreak_Ridge = _map;


			/************************************************************************/
			/* Aztec                                                                */
			/************************************************************************/

			_map = Map();
			_map.makeKnown();
			_map.setName("Aztec");
			_map.setHash("ba2fc0ed637e4ec91cc70424335b3c13e131b75a");
			_baseinfo.clear();

			// for start location at (68,6)
			_chokepoints.clear();
			_chokepoints.push_back(Position(2861,223));
			_chokepoints.push_back(Position(3160,468));
			_chokepoints.push_back(Position(2719,906));
			_baseinfo.insert(make_pair(TilePosition(68,6),_chokepoints));

			// for start location at (117,100)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3528,3744));
			_chokepoints.push_back(Position(3094,3552));
			_chokepoints.push_back(Position(2797,3007));
			_baseinfo.insert(make_pair(TilePosition(117,100),_chokepoints));

			// for start location at (7,83)
			_chokepoints.clear();
			_chokepoints.push_back(Position(290,2166));
			_chokepoints.push_back(Position(528,1860));
			_chokepoints.push_back(Position(1090,1910));
			_baseinfo.insert(make_pair(TilePosition(7,83),_chokepoints));

			_map.setBaseInfo(_baseinfo);
			Aztec = _map;


			/************************************************************************/
			/* Neo Moon Glaive                                                      */
			/************************************************************************/

			_map = Map();
			_map.makeKnown();
			_map.setName("Neo Moon Glaive");
			_map.setHash("c8386b87051f6773f6b2681b0e8318244aa086a6");
			_baseinfo.clear();

			// for start location at (67,6)
			_chokepoints.clear();
			_chokepoints.push_back(Position(1612,540));
			_chokepoints.push_back(Position(1276,772));
			_chokepoints.push_back(Position(1516,1256));
			_baseinfo.insert(make_pair(TilePosition(67,6),_chokepoints));

			// for start location at (117,96)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3548,2444));
			_chokepoints.push_back(Position(3483,2061));
			_chokepoints.push_back(Position(3052,1884));
			_baseinfo.insert(make_pair(TilePosition(117,96),_chokepoints));

			// for start location at (7,90)
			_chokepoints.clear();
			_chokepoints.push_back(Position(786,3262));
			_chokepoints.push_back(Position(1216,3348));
			_chokepoints.push_back(Position(1564,3100));
			_baseinfo.insert(make_pair(TilePosition(7,90),_chokepoints));

			_map.setBaseInfo(_baseinfo);
			Neo_Moon_Glaive = _map;


			/************************************************************************/
			/* Tau Cross                                                            */
			/************************************************************************/

			_map = Map();
			_map.makeKnown();
			_map.setName("Tau Cross");
			_map.setHash("9bfc271360fa5bab3707a29e1326b84d0ff58911");
			_baseinfo.clear();

			// for start location at (7,44)
			_chokepoints.clear();
			_chokepoints.push_back(Position(402,836));
			_chokepoints.push_back(Position(924,608));
			_chokepoints.push_back(Position(1428,532));
			_baseinfo.insert(make_pair(TilePosition(7,44),_chokepoints));

			// for start location at (117,9)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3747,1145));
			_chokepoints.push_back(Position(3452,1424));
			_chokepoints.push_back(Position(2468,2313));
			_baseinfo.insert(make_pair(TilePosition(117,9),_chokepoints));
			
			// for start location at (93,118)
			_chokepoints.clear();
			_chokepoints.push_back(Position(2152,3676));
			_chokepoints.push_back(Position(1660,3408));
			_chokepoints.push_back(Position(1700,2388));
			_baseinfo.insert(make_pair(TilePosition(93,118),_chokepoints));

			_map.setBaseInfo(_baseinfo);

			_tankdropposition.clear();
			_tankdropposition.insert(make_pair(TilePosition(103,37),TilePosition(91,33)));
			_tankdropposition.insert(make_pair(TilePosition(14,13),TilePosition(9,4)));
			_tankdropposition.insert(make_pair(TilePosition(51,113),TilePosition(39,116)));
			_map.setTankDropPositions(_tankdropposition);

			_tanksiegeposition.clear();
			_tanksiegeposition.insert(make_pair(TilePosition(117,9),Position(3100,1400)));
			_map.setTankSiegePositions(_tanksiegeposition);

			Tau_Cross = _map;


			/************************************************************************/
			/* Andromeda                                                            */
			/************************************************************************/

			_map = Map();
			_map.makeKnown();
			_map.setName("Andromeda");
			_map.setHash("1e983eb6bcfa02ef7d75bd572cb59ad3aab49285");
			_baseinfo.clear();

			// for start location at (7,6)
			_chokepoints.clear();
			_chokepoints.push_back(Position(591,910));
			_chokepoints.push_back(Position(828,884));
			_chokepoints.push_back(Position(1180,1616));
			_baseinfo.insert(make_pair(TilePosition(7,6),_chokepoints));

			// for start location at (117,7)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3509,919));
			_chokepoints.push_back(Position(3268,868));
			_chokepoints.push_back(Position(2416,1344));
			_baseinfo.insert(make_pair(TilePosition(117,7),_chokepoints));

			// for start location at (117,119)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3526,3150));
			_chokepoints.push_back(Position(3196,3145));
			_chokepoints.push_back(Position(2904,2504));
			_baseinfo.insert(make_pair(TilePosition(117,119),_chokepoints));

			// for start location at (7,118)
			_chokepoints.clear();
			_chokepoints.push_back(Position(563,3152));
			_chokepoints.push_back(Position(900,3141));
			_chokepoints.push_back(Position(1176,2496));
			_baseinfo.insert(make_pair(TilePosition(7,118),_chokepoints));

			_map.setBaseInfo(_baseinfo);

			_tankdropposition.clear();
			_tankdropposition.insert(make_pair(TilePosition(21,21),TilePosition(35,24)));
			_tankdropposition.insert(make_pair(TilePosition(103,21),TilePosition(92,24)));
			_tankdropposition.insert(make_pair(TilePosition(103,105),TilePosition(92,102)));
			_tankdropposition.insert(make_pair(TilePosition(21,105),TilePosition(35,102)));
			_map.setTankDropPositions(_tankdropposition);

			Andromeda = _map;


			/************************************************************************/
			/* Circuit Breaker                                                      */
			/************************************************************************/

			_map = Map();
			_map.makeKnown();
			_map.setName("Circuit Breaker");
			_map.setHash("450a792de0e544b51af5de578061cb8a2f020f32");
			_baseinfo.clear();

			// for start location at (7,9)
			_chokepoints.clear();
			_chokepoints.push_back(Position(243,944));
			_chokepoints.push_back(Position(600,1116));
			_chokepoints.push_back(Position(863,1307));
			_baseinfo.insert(make_pair(TilePosition(7,9),_chokepoints));

			// for start location at (117,9)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3842,942));
			_chokepoints.push_back(Position(3493,1113));
			_chokepoints.push_back(Position(3125,1356));
			_baseinfo.insert(make_pair(TilePosition(117,9),_chokepoints));

			// for start location at (117,118)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3794,3168));
			_chokepoints.push_back(Position(3492,3004));
			_chokepoints.push_back(Position(2847,2875));
			_baseinfo.insert(make_pair(TilePosition(117,118),_chokepoints));

			// for start location at (7,118)
			_chokepoints.clear();
			_chokepoints.push_back(Position(335,3150));
			_chokepoints.push_back(Position(600,3004));
			_chokepoints.push_back(Position(886,2791));
			_baseinfo.insert(make_pair(TilePosition(7,118),_chokepoints));

			_map.setBaseInfo(_baseinfo);
			Circuit_Breaker = _map;


			/************************************************************************/
			/* Electric Circuit                                                     */
			/************************************************************************/

			_map = Map();
			_map.makeKnown();
			_map.setName("Electric Circuit");
			_map.setHash("9505d618c63a0959f0c0bfe21c253a2ea6e58d26");
			_baseinfo.clear();

			// for start location at (7,7)
			_chokepoints.clear();
			_chokepoints.push_back(Position(916,448));
			_chokepoints.push_back(Position(1386,415));
			_chokepoints.push_back(Position(1912,734));
			_baseinfo.insert(make_pair(TilePosition(7,7),_chokepoints));

			// for start location at (117,7)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3580,968));
			_chokepoints.push_back(Position(3516,1351));
			_chokepoints.push_back(Position(2991,1454));
			_baseinfo.insert(make_pair(TilePosition(117,7),_chokepoints));

			// for start location at (117,119)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3153,3676));
			_chokepoints.push_back(Position(2703,3698));
			_chokepoints.push_back(Position(2176,3356));
			_baseinfo.insert(make_pair(TilePosition(117,119),_chokepoints));

			// for start location at (7,119)
			_chokepoints.clear();
			_chokepoints.push_back(Position(508,3144));
			_chokepoints.push_back(Position(585,2759));
			_chokepoints.push_back(Position(1034,2619));
			_baseinfo.insert(make_pair(TilePosition(7,119),_chokepoints));

			_map.setBaseInfo(_baseinfo);
			Electric_Circuit = _map;


			/************************************************************************/
			/* Empire of the Sun                                                    */
			/************************************************************************/

			_map = Map();
			_map.makeKnown();
			_map.setName("Empire of the Sun");
			_map.setHash("a220d93efdf05a439b83546a579953c63c863ca7");
			_baseinfo.clear();

			// for start location at (7,6)
			_chokepoints.clear();
			_chokepoints.push_back(Position(818,291));
			_chokepoints.push_back(Position(1140,512));
			_chokepoints.push_back(Position(1712,1428));
			_baseinfo.insert(make_pair(TilePosition(7,6),_chokepoints));

			// for start location at (117,6)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3254,290));
			_chokepoints.push_back(Position(2948,492));
			_chokepoints.push_back(Position(2380,1424));
			_baseinfo.insert(make_pair(TilePosition(117,6),_chokepoints));

			// for start location at (117,119)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3262,3811));
			_chokepoints.push_back(Position(2948,3548));
			_chokepoints.push_back(Position(2380,2636));
			_baseinfo.insert(make_pair(TilePosition(117,119),_chokepoints));

			// for start location at (7,119)
			_chokepoints.clear();
			_chokepoints.push_back(Position(816,3811));
			_chokepoints.push_back(Position(1144,3520));
			_chokepoints.push_back(Position(1712,2632));
			_baseinfo.insert(make_pair(TilePosition(7,119),_chokepoints));

			_map.setBaseInfo(_baseinfo);
			Empire_of_the_Sun = _map;


			/************************************************************************/
			/* Fighting Spirit                                                      */
			/************************************************************************/

			_map = Map();
			_map.makeKnown();
			_map.setName("Fighting Spirit");
			_map.setHash("d2f5633cc4bb0fca13cd1250729d5530c82c7451");
			_baseinfo.clear();

			// for start location at (7,6)
			_chokepoints.clear();
			_chokepoints.push_back(Position(354,982));
			_chokepoints.push_back(Position(728,1209));
			_chokepoints.push_back(Position(1084,1344));
			_baseinfo.insert(make_pair(TilePosition(7,6),_chokepoints));

			// for start location at (117,7)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3167,362));
			_chokepoints.push_back(Position(2937,705));
			_chokepoints.push_back(Position(2816,924));
			_baseinfo.insert(make_pair(TilePosition(117,7),_chokepoints));

			// for start location at (117,117)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3693,3135));
			_chokepoints.push_back(Position(3366,2905));
			_chokepoints.push_back(Position(3004,2752));
			_baseinfo.insert(make_pair(TilePosition(117,117),_chokepoints));

			// for start location at (7,116)
			_chokepoints.clear();
			_chokepoints.push_back(Position(904,3776));
			_chokepoints.push_back(Position(1148,3440));
			_chokepoints.push_back(Position(1280,3168));
			_baseinfo.insert(make_pair(TilePosition(7,116),_chokepoints));

			_map.setBaseInfo(_baseinfo);
			Fighting_Spirit = _map;


			/************************************************************************/
			/* Fortress                                                             */
			/************************************************************************/

			_map = Map();
			_map.makeKnown();
			_map.setName("Fortress");
			_map.setHash("83320e505f35c65324e93510ce2eafbaa71c9aa1");
			_baseinfo.clear();

			// for start location at (49,7)
			_chokepoints.clear();
			_chokepoints.push_back(Position(2250,233));
			_chokepoints.push_back(Position(2492,520));
			_chokepoints.push_back(Position(2492,520));
			_baseinfo.insert(make_pair(TilePosition(49,7),_chokepoints));

			// for start location at (117,54)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3690,2228));
			_chokepoints.push_back(Position(3448,2470));
			_chokepoints.push_back(Position(3448,2470));
			_baseinfo.insert(make_pair(TilePosition(117,54),_chokepoints));

			// for start location at (77,119)
			_chokepoints.clear();
			_chokepoints.push_back(Position(1792,3864));
			_chokepoints.push_back(Position(1592,3592));
			_chokepoints.push_back(Position(1592,3592));
			_baseinfo.insert(make_pair(TilePosition(77,119),_chokepoints));

			// for start location at (7,74)
			_chokepoints.clear();
			_chokepoints.push_back(Position(398,1877));
			_chokepoints.push_back(Position(630,1637));
			_chokepoints.push_back(Position(630,1637));
			_baseinfo.insert(make_pair(TilePosition(7,74),_chokepoints));

			_map.setBaseInfo(_baseinfo);
			Fortress = _map;


			/************************************************************************/
			/* Icarus                                                               */
			/************************************************************************/

			_map = Map();
			_map.makeKnown();
			_map.setName("Icarus");
			_map.setHash("0409ca0d7fe0c7f4083a70996a8f28f664d2fe37");
			_baseinfo.clear();

			// for start location at (43,8)
			_chokepoints.clear();
			_chokepoints.push_back(Position(2255,270));
			_chokepoints.push_back(Position(2492,520));
			_chokepoints.push_back(Position(2392,944));
			_baseinfo.insert(make_pair(TilePosition(43,8),_chokepoints));

			// for start location at (116,47)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3827,2224));
			_chokepoints.push_back(Position(3557,2400));
			_chokepoints.push_back(Position(3062,2256));
			_baseinfo.insert(make_pair(TilePosition(116,47),_chokepoints));

			// for start location at (81,118)
			_chokepoints.clear();
			_chokepoints.push_back(Position(1862,3790));
			_chokepoints.push_back(Position(1596,3596));
			_chokepoints.push_back(Position(1688,3152));
			_baseinfo.insert(make_pair(TilePosition(81,118),_chokepoints));

			// for start location at (8,77)
			_chokepoints.clear();
			_chokepoints.push_back(Position(277,1857));
			_chokepoints.push_back(Position(575,1741));
			_chokepoints.push_back(Position(1020,1824));
			_baseinfo.insert(make_pair(TilePosition(8,77),_chokepoints));

			_map.setBaseInfo(_baseinfo);

			_tankdropposition.clear();
			_tankdropposition.insert(make_pair(TilePosition(78,6),TilePosition(92,1)));
			_tankdropposition.insert(make_pair(TilePosition(117,77),TilePosition(122,89)));
			_tankdropposition.insert(make_pair(TilePosition(46,118),TilePosition(35,126)));
			_tankdropposition.insert(make_pair(TilePosition(7,49),TilePosition(5,37)));
			_map.setTankDropPositions(_tankdropposition);

			Icarus = _map;


			/************************************************************************/
			/* Jade                                                                 */
			/************************************************************************/

			_map = Map();
			_map.makeKnown();
			_map.setName("Jade");
			_map.setHash("df21ac8f19f805e1e0d4e9aa9484969528195d9f");
			_baseinfo.clear();

			// for start location at (7,7)
			_chokepoints.clear();
			_chokepoints.push_back(Position(828,492));
			_chokepoints.push_back(Position(1184,540));
			_chokepoints.push_back(Position(669,1203));
			_baseinfo.insert(make_pair(TilePosition(7,7),_chokepoints));

			// for start location at (117,7)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3594,1020));
			_chokepoints.push_back(Position(3490,1273));
			_chokepoints.push_back(Position(3156,1160));
			_baseinfo.insert(make_pair(TilePosition(117,7),_chokepoints));

			// for start location at (117,117)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3228,3628));
			_chokepoints.push_back(Position(2902,3581));
			_chokepoints.push_back(Position(3300,2956));
			_baseinfo.insert(make_pair(TilePosition(117,117),_chokepoints));

			// for start location at (8,117)
			_chokepoints.clear();
			_chokepoints.push_back(Position(562,3038));
			_chokepoints.push_back(Position(674,2809));
			_chokepoints.push_back(Position(948,2924));
			_baseinfo.insert(make_pair(TilePosition(8,117),_chokepoints));

			_map.setBaseInfo(_baseinfo);
			Jade = _map;


			/************************************************************************/
			/* La Mancha                                                            */
			/************************************************************************/

			_map = Map();
			_map.makeKnown();
			_map.setName("La Mancha");
			_map.setHash("e47775e171fe3f67cc2946825f00a6993b5a415e");
			_baseinfo.clear();

			// for start location at (7,6)
			_chokepoints.clear();
			_chokepoints.push_back(Position(406,878));
			_chokepoints.push_back(Position(638,1227));
			_chokepoints.push_back(Position(1604,1660));
			_baseinfo.insert(make_pair(TilePosition(7,6),_chokepoints));

			// for start location at (116,6)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3304,508));
			_chokepoints.push_back(Position(2941,622));
			_chokepoints.push_back(Position(2280,1452));
			_baseinfo.insert(make_pair(TilePosition(116,6),_chokepoints));

			// for start location at (116,117)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3636,3224));
			_chokepoints.push_back(Position(3449,2890));
			_chokepoints.push_back(Position(2628,2236));
			_baseinfo.insert(make_pair(TilePosition(116,117),_chokepoints));

			// for start location at (8,117)
			_chokepoints.clear();
			_chokepoints.push_back(Position(800,3600));
			_chokepoints.push_back(Position(1146,3543));
			_chokepoints.push_back(Position(1640,2476));
			_baseinfo.insert(make_pair(TilePosition(8,117),_chokepoints));

			_map.setBaseInfo(_baseinfo);
			La_Mancha = _map;


			/************************************************************************/
			/* Python                                                               */
			/************************************************************************/

			_map = Map();
			_map.makeKnown();
			_map.setName("Python");
			_map.setHash("de2ada75fbc741cfa261ee467bf6416b10f9e301");
			_baseinfo.clear();

			// for start location at (83,6)
			_chokepoints.clear();
			_chokepoints.push_back(Position(1950,203));
			_chokepoints.push_back(Position(1818,642));
			_chokepoints.push_back(Position(1818,642));
			_baseinfo.insert(make_pair(TilePosition(83,6),_chokepoints));

			// for start location at (117,40)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3806,1899));
			_chokepoints.push_back(Position(3392,2042));
			_chokepoints.push_back(Position(3392,2042));
			_baseinfo.insert(make_pair(TilePosition(117,40),_chokepoints));

			// for start location at (42,119)
			_chokepoints.clear();
			_chokepoints.push_back(Position(2120,3872));
			_chokepoints.push_back(Position(2188,3548));
			_chokepoints.push_back(Position(2188,3548));
			_baseinfo.insert(make_pair(TilePosition(42,119),_chokepoints));

			// for start location at (7,86)
			_chokepoints.clear();
			_chokepoints.push_back(Position(264,2176));
			_chokepoints.push_back(Position(652,2076));
			_chokepoints.push_back(Position(652,2076));
			_baseinfo.insert(make_pair(TilePosition(7,86),_chokepoints));

			_map.setBaseInfo(_baseinfo);
			Python = _map;


			/************************************************************************/
			/* Roadrunner                                                           */
			/************************************************************************/

			_map = Map();
			_map.makeKnown();
			_map.setName("Roadrunner");
			_map.setHash("9a4498a896b28d115129624f1c05322f48188fe0");
			_baseinfo.clear();

			// for start location at (27,6)
			_chokepoints.clear();
			_chokepoints.push_back(Position(1512,448));
			_chokepoints.push_back(Position(1784,424));
			_chokepoints.push_back(Position(1851,805));
			_baseinfo.insert(make_pair(TilePosition(27,6),_chokepoints));

			// for start location at (117,35)
			_chokepoints.clear();
			_chokepoints.push_back(Position(3618,1718));
			_chokepoints.push_back(Position(3528,1964));
			_chokepoints.push_back(Position(3225,1940));
			_baseinfo.insert(make_pair(TilePosition(117,35),_chokepoints));

			// for start location at (98,119)
			_chokepoints.clear();
			_chokepoints.push_back(Position(2589,3659));
			_chokepoints.push_back(Position(2300,3648));
			_chokepoints.push_back(Position(2363,3236));
			_baseinfo.insert(make_pair(TilePosition(98,119),_chokepoints));

			// for start location at (7,90)
			_chokepoints.clear();
			_chokepoints.push_back(Position(500,2369));
			_chokepoints.push_back(Position(465,2151));
			_chokepoints.push_back(Position(985,2225));
			_baseinfo.insert(make_pair(TilePosition(7,90),_chokepoints));

			_map.setBaseInfo(_baseinfo);
			Roadrunner = _map;


			/************************************************************************/
			/* Unknown maps                                                         */
			/************************************************************************/

			Unknown = Map();
		}
	}
}

/************************************************************************/

MapInfo::MapInfo()
{
	using namespace Maps;

	initialize();

	_MapInfo.insert(make_pair(Benzene.getHash(),           Benzene));
	_MapInfo.insert(make_pair(Destination.getHash(),       Destination));
	_MapInfo.insert(make_pair(Heartbreak_Ridge.getHash(),  Heartbreak_Ridge));
	_MapInfo.insert(make_pair(Aztec.getHash(),             Aztec));
	_MapInfo.insert(make_pair(Neo_Moon_Glaive.getHash(),   Neo_Moon_Glaive));
	_MapInfo.insert(make_pair(Tau_Cross.getHash(),         Tau_Cross));
	_MapInfo.insert(make_pair(Andromeda.getHash(),         Andromeda));
	_MapInfo.insert(make_pair(Circuit_Breaker.getHash(),   Circuit_Breaker));
	_MapInfo.insert(make_pair(Electric_Circuit.getHash(),  Electric_Circuit));
	_MapInfo.insert(make_pair(Empire_of_the_Sun.getHash(), Empire_of_the_Sun));
	_MapInfo.insert(make_pair(Fighting_Spirit.getHash(),   Fighting_Spirit));
	_MapInfo.insert(make_pair(Fortress.getHash(),          Fortress));
	_MapInfo.insert(make_pair(Icarus.getHash(),            Icarus));
	_MapInfo.insert(make_pair(Jade.getHash(),              Jade));
	_MapInfo.insert(make_pair(La_Mancha.getHash(),         La_Mancha));
	_MapInfo.insert(make_pair(Python.getHash(),            Python));
	_MapInfo.insert(make_pair(Roadrunner.getHash(),        Roadrunner));
	_MapInfo.insert(make_pair(Unknown.getHash(),           Unknown));
}

Map MapInfo::getMapInfo()
{
	if (_MapInfo.find(Broodwar->mapHash()) != _MapInfo.end())
	{
		return _MapInfo[Broodwar->mapHash()];
	}
	return Maps::Unknown;
}
