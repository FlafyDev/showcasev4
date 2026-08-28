#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

#include "models/level.hpp"
#include "models/replay.hpp"
#include "models/report.hpp"

#include <string>
#include <vector>

namespace showcase {

class Client {
 public:
  static Client& get();
  static constexpr std::string_view origin = SHOWCASE_API_ORIGIN;

  // Endpoints
  arc::Future<geode::Result<bool>> observe(LevelIdentity level);
  arc::Future<geode::Result<>> upload(LevelIdentity level);
  arc::Future<geode::Result<ReplayViews>> listReplays(std::string hash);
  arc::Future<geode::Result<geode::ByteVector>> replayData(std::string hash, std::string id);
  arc::Future<geode::Result<>> submit(std::string hash, geode::ByteVector replay);
  arc::Future<geode::Result<>> vote(std::string hash, std::string replayID, int value);
  arc::Future<geode::Result<>> report(Report report);
  arc::Future<geode::Result<>> crashReport(std::string revision, std::string log);

 private:
  Client() = default;
  Client(Client const&) = delete;
  Client& operator=(Client const&) = delete;

  geode::utils::web::WebRequest request() const;
  static geode::Result<> expectSuccess(geode::utils::web::WebResponse const& response);
  static geode::Result<bool> parseObserve(geode::utils::web::WebResponse const& response);
  static geode::Result<ReplayViews> parseReplayList(geode::utils::web::WebResponse const& response);
  static geode::Result<std::vector<ReplaySummary>> parseReplayArray(matjson::Value const& json);
};

}
