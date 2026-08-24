#include <Geode/Geode.hpp>

#include "managers/auth.hpp"

using namespace geode::prelude;

$on_game(Loaded) {
  showcase::AuthManager::get().authenticate();
}
