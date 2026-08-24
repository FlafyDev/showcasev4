#include "report.hpp"

#include <hash.hpp>

namespace showcase {

char const* segmentModeName(SegmentMode mode) {
  switch (mode) {
    case SegmentMode::Normal:
      return "normal";
    case SegmentMode::Practice:
      return "practice";
  }
  return "normal";
}

char const* segmentOutcomeName(SegmentOutcome outcome) {
  switch (outcome) {
    case SegmentOutcome::Death:
      return "death";
    case SegmentOutcome::Completion:
      return "completion";
    case SegmentOutcome::Restart:
      return "restart";
    case SegmentOutcome::Exit:
      return "exit";
  }
  return "exit";
}

}
