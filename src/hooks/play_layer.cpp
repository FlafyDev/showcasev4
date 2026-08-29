#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "managers/recorder.hpp"
#include "managers/report.hpp"
#include "managers/session.hpp"
#include "models/level.hpp"
#include "managers/auth.hpp"
#include "managers/client.hpp"
#include "utils/random.hpp"

#include <limits>

using namespace geode::prelude;

namespace showcase {

namespace {
// Haven't actually done any research on whether this is what defines all randomness in a level :p
void seedGame(uint32_t seed) {
  GameToolbox::fast_srand(seed);
}

uint32_t replayTick(GJBaseGameLayer const* layer) {
  return layer->m_gameState.m_currentProgress / 2 + 1;
}

ReplaySession* sessionLayer(GJBaseGameLayer const* layer);

bool isValidLevel(GJBaseGameLayer const* layer) {
  return layer && layer->m_level && layer->m_level->m_levelType == GJLevelType::Saved &&
         !layer->m_level->m_levelNotDownloaded && !layer->m_isPlatformer;
}

bool shouldRecord(GJBaseGameLayer const* layer) {
  auto* session = sessionLayer(layer);
  bool autoPlaying = session && session->autoPlay().enabled();

  return Mod::get()->getSettingValue<bool>("auto-submit") && !autoPlaying &&
         AuthManager::get().hasAccount() && isValidLevel(layer) && !layer->m_isPracticeMode;
}

bool shouldReport(GJBaseGameLayer const* layer) {
  auto* session = sessionLayer(layer);
  bool autoPlaying = session && session->autoPlay().enabled();

  return Mod::get()->getSettingValue<bool>("share-attempts") && !autoPlaying &&
         AuthManager::get().hasAccount() && isValidLevel(layer);
}

arc::Future<Result<>> submitReplay(LevelIdentity identity, std::vector<uint8_t> bytes) {
  auto observed = co_await Client::get().observe(identity);
  if (observed.isErr()) co_return Err(observed.unwrapErr());

  if (observed.unwrap()) {
    auto uploaded = co_await Client::get().upload(identity);
    if (uploaded.isErr()) co_return Err(uploaded.unwrapErr());
  }
  co_return co_await Client::get().submit(identity.hash, std::move(bytes));
}

void submitReplayWrapper(LevelIdentity identity, std::vector<uint8_t> bytes) {
  async::spawn(submitReplay(std::move(identity), std::move(bytes)), [](Result<> result) {
    if (result.isOk()) {
      log::info("Replay submitted for verification");
    } else {
      log::warn("Replay submission failed: {}", result.unwrapErr());
    }
  });
}

}

class $modify(ShowcaseReplayBaseLayer, GJBaseGameLayer) {
  void handleButton(bool down, int button, bool playerOne) {
    if (button == 1 && shouldRecord(this)) {
      RecorderManager::get().input(replayTick(this), playerOne, down);
    }
    GJBaseGameLayer::handleButton(down, button, playerOne);
  }

  void processCommands(float dt, bool halfTick, bool lastTick) {
    GJBaseGameLayer::processCommands(dt, halfTick, lastTick);

    auto* session = sessionLayer(this);
    if (!session) return;

    if (session->autoPlay().enabled()) {
      for (auto const& input : session->autoPlay().inputsAt(replayTick(this))) {
        GJBaseGameLayer::handleButton(input.down, input.button, !input.player2);
      }
    }
  }
};

class $modify(ShowcaseReplayPlayLayer, PlayLayer) {
  struct Fields {
    std::unique_ptr<ReplaySession> m_replaySession;
  };

  bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
    ReportManager::get().terminateSession();
    RecorderManager::get().clear();

    m_fields->m_replaySession = ReplaySession::takeQueued();
    if (auto* session = m_fields->m_replaySession.get()) {
      if (auto& autoPlay = session->autoPlay(); autoPlay.enabled()) {
        seedGame(autoPlay.seed());
      }
    }

    if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

    if (auto* session = m_fields->m_replaySession.get()) {
      session->attach(this);
    }

    return true;
  }

  // Kind of broken way to turn on practice mode on start, but IDK other way
  void onEnterTransitionDidFinish() {
    PlayLayer::onEnterTransitionDidFinish();

    if (auto* session = m_fields->m_replaySession.get();
      session && session->autoPlay().enabled() && !m_isPracticeMode) {
      togglePracticeMode(true);
    }
  }

  void togglePracticeMode(bool enabled) {
    if (enabled) {
      // Cuz no recording on practice mode
      RecorderManager::get().clear();
    } else {
      if (auto* session = m_fields->m_replaySession.get()) {
        session->autoPlay().disable();
      }
    }

    PlayLayer::togglePracticeMode(enabled);
  }

  void destroyPlayer(PlayerObject* player, GameObject* object) {
    if (!m_anticheatSpike || object != m_anticheatSpike) {
      ReportManager::get().closeSegment(replayTick(this), SegmentOutcome::Death);
    }

    PlayLayer::destroyPlayer(player, object);
  }

  void resetLevel() {
    ReportManager::get().closeSegment(replayTick(this), SegmentOutcome::Restart);
    PlayLayer::resetLevel();
    ReportManager::get().startSegment(replayTick(this), m_isPracticeMode);

    // `shouldRecord` checks !practice_mode, so it restarts the recording only when starting from
    // the beginning.
    if (shouldRecord(this)) {
      // For now, I don't want to apply a seed (when recording) without proper acknowledgement from
      // the user that a seed is being set. It would also break recordings when restarting the
      // level, so that needs consideration as well...
      RecorderManager::get().start(generateRandomUInt32() & std::numeric_limits<int32_t>::max());
    }

    if (!ReportManager::get().sessionActive() && shouldReport(this)) {
      auto identity = getLevelIdentity(m_level);
      if (identity.isOk()) {
        ReportManager::get().startSession(identity.unwrap(), replayTick(this), m_isPracticeMode);
      } else {
        log::warn("Level report was not started: {}", identity.unwrapErr());
      }
    }
  }

  void levelComplete() {
    ReportManager::get().closeSegment(replayTick(this), SegmentOutcome::Completion);

    if (RecorderManager::get().recording()) {
      auto identity = getLevelIdentity(m_level);
      if (identity.isOk()) {
        auto replay = RecorderManager::get().finish(replayTick(this));
        if (replay.isOk()) {
          submitReplayWrapper(std::move(identity).unwrap(), std::move(replay).unwrap());
        } else {
          log::warn("Completed replay was not submitted: {}", replay.unwrapErr());
        }
      } else {
        log::warn("Completed replay was not submitted: {}", identity.unwrapErr());
      }
    }
    PlayLayer::levelComplete();
  }

  void onQuit() {
    ReportManager::get().endSession(replayTick(this));
    m_fields->m_replaySession.reset();
    RecorderManager::get().clear();

    PlayLayer::onQuit();
  }
};

namespace {

ReplaySession* sessionLayer(GJBaseGameLayer const* layer) {
  auto* playLayer = typeinfo_cast<PlayLayer*>(layer);
  if (!playLayer) {
    return nullptr;
  }

  return modify_cast<ShowcaseReplayPlayLayer*>(playLayer)->m_fields->m_replaySession.get();
}

}

}
