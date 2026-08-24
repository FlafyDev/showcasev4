#include "random.hpp"

#include <fmt/format.h>

#include <array>
#include <random>

namespace showcase {

uint32_t generateRandomUInt32() {
  return static_cast<uint32_t>(std::random_device{}());
}

std::string generateUUID() {
  std::random_device random;
  std::array<uint8_t, 16> bytes;
  for (auto& byte : bytes) byte = static_cast<uint8_t>(random());
  bytes[6] = static_cast<uint8_t>((bytes[6] & 0x0f) | 0x40);
  bytes[8] = static_cast<uint8_t>((bytes[8] & 0x3f) | 0x80);
  return fmt::format(
    "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
    bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7], bytes[8],
    bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
}

}
