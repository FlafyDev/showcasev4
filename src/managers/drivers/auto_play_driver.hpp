#pragma once

#include <Geode/Geode.hpp>
#include <models/replay.hpp>

#include <cstdint>
#include <span>

namespace showcase {

class ReplaySession;

class AutoPlayDriver {
 public:
  explicit AutoPlayDriver(ReplaySession* session, bool enabled);
  AutoPlayDriver(AutoPlayDriver const&) = delete;
  AutoPlayDriver& operator=(AutoPlayDriver const&) = delete;

  uint32_t seed() const;
  std::span<Replay::InputType const> inputsAt(uint32_t frame) const;

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
