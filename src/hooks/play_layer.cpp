#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "managers/replay.hpp"
#include "managers/recorder.hpp"
#include "managers/report.hpp"
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

bool isValidLevel(GJBaseGameLayer const* layer) {
  return layer && layer->m_level && layer->m_level->m_levelType == GJLevelType::Saved &&
         !layer->m_level->m_levelNotDownloaded && !layer->m_isPlatformer;
}

bool shouldRecord(GJBaseGameLayer const* layer) {
  return Mod::get()->getSettingValue<bool>("auto-submit") && !ReplayManager::get().playing() &&
         AuthManager::get().hasAccount() && isValidLevel(layer) && !layer->m_isPracticeMode;
}

bool shouldReport(GJBaseGameLayer const* layer) {
  return Mod::get()->getSettingValue<bool>("share-attempts") && !ReplayManager::get().playing() &&
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
    if (button == 1 && shouldRecord(this) && !ReplayManager::get().playing() && !m_isPracticeMode) {
      RecorderManager::get().input(replayTick(this), playerOne, down);
    }
    GJBaseGameLayer::handleButton(down, button, playerOne);
  }

  void processCommands(float dt, bool halfTick, bool lastTick) {
    GJBaseGameLayer::processCommands(dt, halfTick, lastTick);
    for (auto const& input : ReplayManager::get().inputsAt(replayTick(this))) {
      GJBaseGameLayer::handleButton(input.down, input.button, !input.player2);
    }
  }
};

class $modify(ShowcaseReplayPlayLayer, PlayLayer) {
  bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
    ReportManager::get().terminateSession();
    ReplayManager::get().stop();
    RecorderManager::get().clear();

    auto replaying = ReplayManager::get().queued();

    if (replaying) {
      seedGame(ReplayManager::get().seed());
    }

    if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

    if (replaying) {
      ReplayManager::get().begin();
    }

    return true;
  }

  // Kind of broken way to turn on practice mode on start, but IDK other way
  void onEnterTransitionDidFinish() {
    PlayLayer::onEnterTransitionDidFinish();

    if (ReplayManager::get().playing() && !m_isPracticeMode) {
      togglePracticeMode(true);
    }
  }

  void togglePracticeMode(bool enabled) {
    if (enabled) {
      // Cuz no recording on practice mode
      RecorderManager::get().clear();
    } else {
      ReplayManager::get().stop();
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
    ReplayManager::get().stop();
    RecorderManager::get().clear();

    PlayLayer::onQuit();
  }
};

}
