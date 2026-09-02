#include "session.hpp"

#include "auth.hpp"
#include "client.hpp"
#include "managers/drivers/auto_play_driver.hpp"
#include "models/replay.hpp"
#include "utils/random.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>
#include <utility>

using namespace geode::prelude;

namespace showcase {

std::unique_ptr<ReplaySession> ReplaySession::queued_session;

ReplaySession::ReplaySession(Replay replay, bool autoPlayEnabled,
  bool inputVisualizerEnabled, bool ghostEnabled)
  : m_replay(std::move(replay)),
    m_autoPlay(AutoPlayDriver(this, autoPlayEnabled)),
    m_inputVisualizer(InputVisualizerDriver(this, inputVisualizerEnabled)),
    m_ghost(GhostDriver(this, ghostEnabled)),
    m_seeker(SeekerDriver(this, true)) {}

void ReplaySession::queue(std::unique_ptr<ReplaySession> session) {
  queued_session = std::move(session);
}

bool ReplaySession::hasQueued() {
  return queued_session != nullptr;
}

std::unique_ptr<ReplaySession> ReplaySession::takeQueued() {
  return std::exchange(queued_session, nullptr);
}

}
