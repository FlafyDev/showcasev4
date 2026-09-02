#pragma once

#include <Geode/Geode.hpp>

namespace showcase {

class ReplaySession;

class GhostDriver {
 public:
  explicit GhostDriver(ReplaySession* session, bool enabled);
  GhostDriver(GhostDriver const&) = delete;
  GhostDriver& operator=(GhostDriver const&) = delete;

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
