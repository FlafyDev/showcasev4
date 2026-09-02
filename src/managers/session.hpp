#pragma once

#include <Geode/Geode.hpp>
#include <gdr/gdr.hpp>

#include "managers/drivers/auto_play_driver.hpp"
#include "managers/drivers/ghost_driver.hpp"
#include "managers/drivers/input_visualizer_driver.hpp"
#include "managers/drivers/seeker_driver.hpp"

namespace showcase {

class ReplaySession {
 public:
  explicit ReplaySession(Replay replay, bool autoPlayEnabled,
    bool inputVisualizerEnabled, bool ghostEnabled);

  void attach(PlayLayer* playLayer) {
    m_playLayer = playLayer;
  };

  static ReplaySession* get();
  static void queue(std::unique_ptr<ReplaySession> session);
  static bool hasQueued();
  static std::unique_ptr<ReplaySession> takeQueued();

  friend class AutoPlayDriver;
  friend class GhostDriver;
  friend class InputVisualizerDriver;
  friend class SeekerDriver;

  AutoPlayDriver& autoPlay() {
    return m_autoPlay;
  }
  AutoPlayDriver const& autoPlay() const {
    return m_autoPlay;
  }

  InputVisualizerDriver& inputVisualizer() {
    return m_inputVisualizer;
  }
  InputVisualizerDriver const& inputVisualizer() const {
    return m_inputVisualizer;
  }

  GhostDriver& ghost() {
    return m_ghost;
  }
  GhostDriver const& ghost() const {
    return m_ghost;
  }

  SeekerDriver& seeker() {
    return m_seeker;
  }
  SeekerDriver const& seeker() const {
    return m_seeker;
  }

 private:
  ReplaySession(ReplaySession&&) = delete;
  ReplaySession& operator=(ReplaySession&&) = delete;

  static std::unique_ptr<ReplaySession> queued_session;
  PlayLayer* m_playLayer = nullptr;
  Replay m_replay;

  AutoPlayDriver m_autoPlay;
  InputVisualizerDriver m_inputVisualizer;
  GhostDriver m_ghost;
  SeekerDriver m_seeker;
};

}
