#include "report.hpp"

#include "auth.hpp"
#include "client.hpp"
#include "utils/random.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>

using namespace geode::prelude;

namespace showcase {

namespace {
constexpr size_t MAX_SEGMENTS_PER_SESSION = 65'536;
}

ReportManager& ReportManager::get() {
  static ReportManager instance;
  return instance;
}

bool ReportManager::sessionActive() const {
  return m_currentSession.has_value();
}

void ReportManager::startSession(LevelIdentity const& level, uint32_t tick, bool practice) {
  terminateSession();

  auto account = AuthManager::get().account();
  if (!account) return;

  m_currentSession = CurrentSession{{generateUUID(), account->accountID, level.id, level.revision, level.hash}};
  startSegment(tick, practice);
}

void ReportManager::startSegment(uint32_t tick, bool practice) {
  if (!m_currentSession) return;

  m_currentSession->segment = OpenSegment{
    practice ? SegmentMode::Practice : SegmentMode::Normal,
    tick,
  };
}

void ReportManager::closeSegment(uint32_t endTick, SegmentOutcome outcome) {
  if (!m_currentSession || !m_currentSession->segment) return;

  auto segment = std::move(*m_currentSession->segment);
  m_currentSession->segment.reset();
  endTick = std::max(endTick, segment.startTick);

  if (m_currentSession->segments.size() >= MAX_SEGMENTS_PER_SESSION) return;
  m_currentSession->segments.push_back(
    {m_currentSession->nextSequence++, segment.mode, segment.startTick, endTick, outcome});
}

void ReportManager::endSession(uint32_t tick) {
  if (!m_currentSession) return;

  closeSegment(tick, SegmentOutcome::Exit);
  auto session = std::move(*m_currentSession);
  m_currentSession.reset();

  auto account = AuthManager::get().account();
  if (!account || account->accountID != session.identity.accountID) return;

  async::spawn(
    Client::get().report({std::move(session.identity), std::move(session.segments), true}),
    [](Result<> result) {
      if (result.isErr()) {
        log::warn("Session upload failed: {}", result.unwrapErr());
      }
    });
}

void ReportManager::terminateSession() {
  m_currentSession.reset();
}

}
