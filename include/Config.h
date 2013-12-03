#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include <string>
#include <json/json.h>

#define _GET_JSON1(r, n1, v) return (r).get((n1),(v))
#define _GET_JSON2(r, n1, n2, v) return (r)[(n1)].get((n2),(v)) 
#define _GET_JSON3(r, n1, n2, n3, v) return (r)[(n1)][(n2)].get((n3),(v))
#define _GET_JSON4(r, n1, n2, n3, n4, v) return (r)[(n1)][(n2)][(n3)].get((n4),(v))
#define _GET_JSON5(r, n1, n2, n3, n4, n5, v) return (r)[(n1)][(n2)][(n3)][(n4)].get((n5),(v)) 

namespace ICEStarCraft
{
  // Read parameters from outside file (read/....)
  class Config
  {
  public:
    // singleton, lazy implementation
    static Config& i();

    // To get a property from file, corresponding function should be added here
    // TODO: Add other functions here
    int TIME_BUILDING_FADE();
    int TIME_MOVING_UNIT_FADE();

    //Gameflow parameter
    int GF_TURRET_BUILD_TIME();

    // Debug flag
    bool DEBUG_BUILD_ORDER_MANAGER();
    bool DEBUG_GAME_FLOW();

    // General function for getting other value
    Json::Value& operator()();

  private:
    // Need to implemen this one
    Config(const std::string& fn = "bwapi-data/read/config.json");
    // Be sure copy and assign operator not public available
    Config(const Config&);
    void operator=(const Config&);

    Json::Value _root;
  };

}