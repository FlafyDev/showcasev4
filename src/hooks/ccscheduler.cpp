#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>

#include "managers/session.hpp"

using namespace geode::prelude;

namespace showcase {

namespace {
constexpr int FASTFORWARD_UPDATE_MULTIPLIER = 40;
}

class $modify(ShowcaseReplayScheduler, CCScheduler) {
  void update(float dt) {
    if (auto* session = ReplaySession::get(); session && session->seeker().fastForwarding()) {
      for (int step = 0; step < FASTFORWARD_UPDATE_MULTIPLIER; ++step) {
        if (auto* session = ReplaySession::get(); !session || !session->seeker().fastForwarding()) break;
        CCScheduler::update(dt);
      }
      return;
    }

    CCScheduler::update(dt);
  }
};

}
