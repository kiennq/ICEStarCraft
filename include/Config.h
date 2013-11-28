#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include <string>
#include <json/json.h>

namespace ICEStarCraft
{
  // Read parameters from outside file (read/....)
  class Config
  {
  public:
    // singleton, lazy implementation
    static Config& instance();

    // To get a property from file, corresponding function should be added here
    // TODO: Add other functions here

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