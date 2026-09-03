#pragma once

#include <Geode/Geode.hpp>

namespace showcase {

class ReplaySession;

class SeekerDriver {
 public:
  explicit SeekerDriver(ReplaySession* session, bool enabled);
  ~SeekerDriver();
  SeekerDriver(SeekerDriver const&) = delete;
  SeekerDriver& operator=(SeekerDriver const&) = delete;

  void onPlayLayerPause(PauseLayer* pauseLayer);
  void startSeek(float targetPercent);
  void captureTarget();
  void endSeek();
  void terminateSeek();
  bool fastForwarding() const;
  bool seeking() const;
  bool targetCaptured() const;
  bool reachedSeekingTarget() const;

  bool enabled() const {
    return m_enabled;
  }

 private:
  ReplaySession* m_session;
  bool m_enabled;
  friend class ProgressBarTouchLayer;

  cocos2d::CCNode* m_overlay = nullptr;
  bool m_seeking = false;
  bool m_targetCaptured = false;
  float m_targetPercent = 0.f;
};

}
