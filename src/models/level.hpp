#pragma once

#include <Geode/Geode.hpp>

#include <string>

namespace showcase {

struct LevelIdentity {
  int id;
  int revision;
  std::string encoded;
  std::string hash;
};

geode::Result<LevelIdentity> getLevelIdentity(GJGameLevel* level);

}
