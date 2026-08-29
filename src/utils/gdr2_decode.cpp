#include "gdr2_decode.hpp"

#include <cstdint>

namespace showcase {

geode::Result<Replay> gdr2Decode(std::span<uint8_t const> bytes) {
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

}
