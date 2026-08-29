#pragma once

#include <Geode/Geode.hpp>
#include <models/replay.hpp>

#include <cstdint>

namespace showcase {

geode::Result<Replay> gdr2Decode(std::span<uint8_t const> bytes);

}
