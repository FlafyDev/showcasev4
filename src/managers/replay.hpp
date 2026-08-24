#pragma once

#include <Geode/Geode.hpp>
#include <gdr/gdr.hpp>

#include <cstdint>
#include <optional>
#include <span>

namespace showcase {

using Replay = gdr::Replay<>;

class ReplayManager {
 public:
  static ReplayManager& get();

  geode::Result<Replay> decode(std::span<uint8_t const> bytes) const;
  void queue(Replay replay);
  bool queued() const;
  bool playing() const;
  uint32_t seed() const;
  void begin();
  void stop();
  std::span<Replay::InputType const> inputsAt(uint32_t frame) const;

 private:
  ReplayManager() = default;
  ReplayManager(ReplayManager const&) = delete;
  ReplayManager& operator=(ReplayManager const&) = delete;

  std::optional<Replay> m_pending;
  std::optional<Replay> m_active;
};

}
