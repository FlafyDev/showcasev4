#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace showcase {

enum class SegmentMode {
  Normal,
  Practice,
};

enum class SegmentOutcome {
  Death,
  Completion,
  Restart,
  Exit,
};

struct AttemptSegment {
  uint32_t sequence;
  SegmentMode mode;
  uint32_t startTick;
  uint32_t endTick;
  SegmentOutcome outcome;
};

struct ReportSessionIdentity {
  std::string sessionID;
  int accountID;
  int levelID;
  int revision;
  std::string contentHash;
};

struct Report {
  ReportSessionIdentity session;
  std::vector<AttemptSegment> segments;
  bool ended = false;
};

char const* segmentModeName(SegmentMode mode);
char const* segmentOutcomeName(SegmentOutcome outcome);

}
