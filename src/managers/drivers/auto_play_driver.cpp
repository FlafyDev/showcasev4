#include "managers/drivers/auto_play_driver.hpp"
#include "managers/session.hpp"

#include <algorithm>
#include <vector>

namespace showcase {

AutoPlayDriver::AutoPlayDriver(ReplaySession* session, bool enabled)
  : m_session(session), m_enabled(enabled) {}

uint32_t AutoPlayDriver::seed() const {
  return m_session->m_replay.seed;
}

std::span<Replay::InputType const> AutoPlayDriver::inputsAt(uint32_t frame) const {
  auto& replay = m_session->m_replay;

  auto first = std::lower_bound(replay.inputs.begin(), replay.inputs.end(), frame,
    [](auto const& input, uint32_t target) { return input.frame < target; });
  auto last = first;
  while (last != replay.inputs.end() && last->frame == frame) ++last;

  return {first, static_cast<size_t>(last - first)};
}

}
