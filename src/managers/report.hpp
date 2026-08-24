#pragma once

#include "models/level.hpp"
#include "models/report.hpp"

#include <cstdint>
#include <optional>

namespace showcase {

class ReportManager {
 public:
  static ReportManager& get();

  void startSession(LevelIdentity const& level, uint32_t tick, bool practice);
  void endSession(uint32_t tick);
  void terminateSession();

  void startSegment(uint32_t tick, bool practice);
  void closeSegment(uint32_t tick, SegmentOutcome outcome);

  bool sessionActive() const;

 private:
  struct OpenSegment {
    SegmentMode mode;
    uint32_t startTick;
  };

  struct CurrentSession {
    ReportSessionIdentity identity;
    uint32_t nextSequence = 0;
    std::vector<AttemptSegment> segments;
    std::optional<OpenSegment> segment;
  };

  ReportManager() = default;
  ReportManager(ReportManager const&) = delete;
  ReportManager& operator=(ReportManager const&) = delete;

  std::optional<CurrentSession> m_currentSession;
};

}
