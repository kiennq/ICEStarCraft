#include "Config.h"

using namespace std;
using namespace ICEStarCraft;
using namespace BWAPI;
using namespace BWTA;

Config& Config::instance()
{
  static Config config; 
  return config;
}

Config::Config(const std::string& fn /*= "bwapi-data/read/config.ini"*/)
{
  Json::Reader reader;
  if (!reader.parse(fn, _root))
  {
    // Failed, just quietly return and use default value
    return;
  }
}

Json::Value& Config::operator()()
{
  // Simply return _root, caller has to decide to take which property
  return _root;
}
