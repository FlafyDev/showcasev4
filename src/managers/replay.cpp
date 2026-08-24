#include "replay.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace showcase {

ReplayManager& ReplayManager::get() {
  static ReplayManager instance;
  return instance;
}

geode::Result<Replay> ReplayManager::decode(std::span<uint8_t const> bytes) const {
  if (bytes.size() > 2 * 1024 * 1024) return geode::Err("Replay is too large");

  std::vector<uint8_t> data(bytes.begin(), bytes.end());
  auto imported = Replay::importData(data);
  if (imported.isErr()) return geode::Err(imported.unwrapErr());

  auto replay = std::move(imported).unwrap();
  if (replay.getVersion() != 2 || replay.platformer || replay.ldm ||
      (replay.gameVersion != 22 && replay.gameVersion != GEODE_COMP_GD_VERSION) ||
      replay.framerate != 240.0) {
    return geode::Err("Unsupported replay settings");
  }

  if (replay.inputs.size() > 250000) return geode::Err("Replay has too many inputs");

  uint64_t previousFrame = 0;
  for (auto const& input : replay.inputs) {
    if (input.frame > std::numeric_limits<uint32_t>::max() || input.button != 1 ||
        input.frame < previousFrame) {
      return geode::Err("Unsupported replay input");
    }
    previousFrame = input.frame;
  }

  return geode::Ok(std::move(replay));
}

void ReplayManager::queue(Replay replay) {
  m_pending = std::move(replay);
}

bool ReplayManager::queued() const {
  return m_pending.has_value();
}

bool ReplayManager::playing() const {
  return m_active.has_value();
}

uint32_t ReplayManager::seed() const {
  auto const* replay = m_pending ? &*m_pending : m_active ? &*m_active : nullptr;
  return replay ? static_cast<uint32_t>(replay->seed) : 1;
}

void ReplayManager::begin() {
  m_active = std::move(m_pending);
  m_pending.reset();
}

void ReplayManager::stop() {
  m_active.reset();
}

std::span<Replay::InputType const> ReplayManager::inputsAt(uint32_t frame) const {
  if (!m_active) return {};

  auto first = std::lower_bound(m_active->inputs.begin(), m_active->inputs.end(), frame,
    [](auto const& input, uint32_t target) { return input.frame < target; });
  auto last = first;
  while (last != m_active->inputs.end() && last->frame == frame) ++last;

  return {first, static_cast<size_t>(last - first)};
}

}
