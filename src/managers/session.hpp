#pragma once

#include <Geode/Geode.hpp>
#include <gdr/gdr.hpp>

#include "managers/drivers/auto_play_driver.hpp"

namespace showcase {

class ReplaySession {
 public:
  explicit ReplaySession(std::unique_ptr<const Replay> replay, bool autoPlayEnabled);

  void attach(PlayLayer* playLayer) {
    m_playLayer = playLayer;
  };

  static void queue(std::unique_ptr<ReplaySession> session);
  static std::unique_ptr<ReplaySession> takeQueued();

  friend class AutoPlayDriver;

  AutoPlayDriver& autoPlay() {
    return m_autoPlay;
  }
  AutoPlayDriver const& autoPlay() const {
    return m_autoPlay;
  }

 private:
  ReplaySession(ReplaySession&&) = delete;
  ReplaySession& operator=(ReplaySession&&) = delete;

  static std::unique_ptr<ReplaySession> queued_session;
  PlayLayer* m_playLayer = nullptr;
  std::unique_ptr<const Replay> m_replay;

  AutoPlayDriver m_autoPlay;
  // GhostDriver m_ghost;
  // InputVisualizationDriver m_inputs;
  // ProgressBarDriver m_progressBar;
};

}
