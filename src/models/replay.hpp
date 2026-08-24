#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace showcase {

struct ReplaySummary {
  std::string id;
  int64_t score = 0;
  int64_t upvotes = 0;
  int64_t downvotes = 0;
  int userCoins = -1;
  std::string createdAt;
};

struct ReplayViews {
  std::vector<ReplaySummary> recommended;
  std::vector<ReplaySummary> newReplays;
  std::vector<ReplaySummary> top;
  std::vector<ReplaySummary> different;
  std::vector<ReplaySummary> mine;
};

}
