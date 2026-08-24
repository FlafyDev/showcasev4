#include "level.hpp"

#include <hash.hpp>

namespace showcase {

// TODO: cache last 3 inspected levels
geode::Result<LevelIdentity> getLevelIdentity(GJGameLevel* level) {
  if (!level || level->isPlatformer()) {
    return geode::Err("Showcase supports classic levels only");
  }
  if (level->m_levelString.empty()) {
    return geode::Err("Download the level first");
  }

  // zlib failing means empty result (I THINK)
  auto content = cocos2d::ZipUtils::decompressString(level->m_levelString, false, 0);
  if (content.empty()) {
    return geode::Err("The downloaded level data is invalid");
  }

  auto bytes = std::span(reinterpret_cast<uint8_t const*>(content.data()), content.size());
  return geode::Ok(LevelIdentity{
    static_cast<int>(level->m_levelID),
    level->m_levelRev,
    level->m_levelString,
    calculateHash(bytes),
  });
}

}
