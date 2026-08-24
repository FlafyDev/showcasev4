#include "recorder.hpp"

#include <algorithm>
#include <utility>

namespace showcase {

RecorderManager& RecorderManager::get() {
  static RecorderManager instance;
  return instance;
}

void RecorderManager::start(uint32_t seed) {
  m_replay = Replay("Showcase", 4);
  m_replay.author = "Showcase";
  m_replay.duration = 0.f;
  m_replay.gameVersion = GEODE_COMP_GD_VERSION;
  m_replay.framerate = 240.0;
  m_replay.seed = static_cast<int>(seed);
  m_replay.ldm = false;
  m_replay.platformer = false;
  m_recording = true;
}

void RecorderManager::clear() {
  m_recording = false;
  m_replay = Replay();
}

bool RecorderManager::recording() const {
  return m_recording;
}

void RecorderManager::input(uint32_t tick, bool playerOne, bool down) {
  if (!m_recording) return;

  if (!m_replay.inputs.empty() && m_replay.inputs.back().frame == tick &&
      m_replay.inputs.back().player2 == !playerOne) {
    m_replay.inputs.back().down = down;
    return;
  }

  m_replay.inputs.emplace_back(tick, 1, !playerOne, down);
}

geode::Result<std::vector<uint8_t>> RecorderManager::finish(uint32_t terminalTick) {
  if (!m_recording) return geode::Err("Replay recorder is not active");
  m_recording = false;

  std::stable_sort(
    m_replay.inputs.begin(), m_replay.inputs.end(), [](auto const& left, auto const& right) {
      if (left.frame != right.frame) return left.frame < right.frame;
      return !left.player2 && right.player2;
    });

  // after sorting, remove duplicate entries (same frame & player) so it keeps only the last entry
  auto output = std::exchange(m_replay.inputs, {});
  for (auto const& input : output) {
    if (!m_replay.inputs.empty() && m_replay.inputs.back().frame == input.frame &&
        m_replay.inputs.back().player2 == input.player2) {
      m_replay.inputs.back().down = input.down;
    } else {
      m_replay.inputs.push_back(input);
    }
  }

  m_replay.duration = static_cast<float>(terminalTick) / 240.f;

  auto exported = m_replay.exportData();
  if (exported.isErr()) return geode::Err(exported.unwrapErr());

  return geode::Ok(std::move(exported).unwrap());
}

}
