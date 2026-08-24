#pragma once

#include "replay.hpp"

namespace showcase {

class RecorderManager {
 public:
  static RecorderManager& get();

  void start(uint32_t seed);
  void clear();
  bool recording() const;
  void input(uint32_t tick, bool playerOne, bool down);
  geode::Result<std::vector<uint8_t>> finish(uint32_t terminalTick);

 private:
  RecorderManager() = default;
  RecorderManager(RecorderManager const&) = delete;
  RecorderManager& operator=(RecorderManager const&) = delete;

  bool m_recording = false;
  Replay m_replay;
};

}
