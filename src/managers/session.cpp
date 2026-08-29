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

ReplaySession::ReplaySession(std::unique_ptr<const Replay> replay, bool autoPlayEnabled)
  : m_replay(std::move(replay)), m_autoPlay(AutoPlayDriver(this, autoPlayEnabled)) {}

void ReplaySession::queue(std::unique_ptr<ReplaySession> session) {
  queued_session = std::move(session);
}

std::unique_ptr<ReplaySession> ReplaySession::takeQueued() {
  return std::exchange(queued_session, nullptr);
}

}
