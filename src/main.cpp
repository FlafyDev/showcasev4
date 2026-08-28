#include <Geode/Geode.hpp>

#include "managers/auth.hpp"
#include "managers/crash_log.hpp"

using namespace geode::prelude;

$on_game(Loaded) {
  showcase::CrashLogManager::get().checkLatest();
  showcase::AuthManager::get().authenticate();
}
