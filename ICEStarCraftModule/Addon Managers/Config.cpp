#include "Config.h"
#include <iostream>
#include <fstream>

using namespace std;
using namespace ICEStarCraft;
using namespace BWAPI;
using namespace BWTA;

Config& Config::i()
{
  static Config config; 
  return config;
}

Config::Config(const std::string& fn /*= "bwapi-data/read/config.ini"*/)
{
  Json::Reader reader;
  std::filebuf fb;
  if (fb.open(fn.c_str(), std::ios::in))
  {
    std::istream is(&fb);
    if (!reader.parse(is, _root))
    {
      // Failed, just quietly return and use default value
      Broodwar->printf("%s", reader.getFormattedErrorMessages().c_str());
      fb.close();
      return;
    }
    fb.close();
  }
}

Json::Value& Config::operator()()
{
  // Simply return _root, caller has to decide to take which property
  return _root;
}

int Config::TIME_BUILDING_FADE()
{
  _GET_JSON1(_root, "Building Fade Time", 24*30).asInt();
}

int Config::TIME_MOVING_UNIT_FADE()
{
  _GET_JSON1(_root, "Moving Unit Fade Time", 24*10).asInt();
}

int ICEStarCraft::Config::GF_TURRET_BUILD_TIME()
{
  _GET_JSON2(_root, "Game Flow", "Build Turret", 24*60*10).asInt();
}

bool Config::DEBUG_BUILD_ORDER_MANAGER()
{
  _GET_JSON2(_root, "Debug", "Build Order Manager", false).asBool();
}

bool ICEStarCraft::Config::DEBUG_GAME_FLOW()
{
  _GET_JSON2(_root, "Debug", "Game Flow", false).asBool();
}

bool ICEStarCraft::Config::DEBUG_ALL()
{
  _GET_JSON2(_root, "Debug", "All", false).asBool();
}
