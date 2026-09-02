#pragma once

#include <Geode/Geode.hpp>

namespace showcase {

class ReplaySession;

class InputVisualizerDriver {
 public:
  explicit InputVisualizerDriver(ReplaySession* session, bool enabled);
  InputVisualizerDriver(InputVisualizerDriver const&) = delete;
  InputVisualizerDriver& operator=(InputVisualizerDriver const&) = delete;

  bool enabled() const {
    return m_enabled;
  };
  void enable() {
    m_enabled = true;
  };
  void disable() {
    m_enabled = false;
  };

 private:
  ReplaySession* m_session;
  bool m_enabled;
};

}
