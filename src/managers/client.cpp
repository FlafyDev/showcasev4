#include "client.hpp"

#include "auth.hpp"

#include <Geode/utils/base64.hpp>

#include <chrono>

using namespace geode::prelude;

namespace showcase {

Client& Client::get() {
  static Client instance;
  return instance;
}

web::WebRequest Client::request() const {
  auto result = web::WebRequest().timeout(std::chrono::seconds(30));

  if (auto account = AuthManager::get().account()) {
    result.header("Authorization", "Bearer " + account->token);
    result.header("X-Argon-Account-ID", fmt::format("{}", account->accountID));
  }

  return result;
}

Result<> Client::expectSuccess(web::WebResponse const& response) {
  if (response.ok()) return Ok();
  return Err(fmt::format("Server request failed ({})", response.code()));
}

Result<bool> Client::parseObserve(web::WebResponse const& response) {
  if (auto status = expectSuccess(response); status.isErr()) return Err(status.unwrapErr());

  auto json = response.json();
  if (json.isErr()) return Err("Invalid server response");

  auto value = json.unwrap().get("needs_content");
  if (value.isErr()) return Err("Invalid server response");

  auto needsContent = value.unwrap().asBool();
  if (needsContent.isErr()) return Err("Invalid server response");

  return Ok(needsContent.unwrap());
}

Result<std::vector<ReplaySummary>> Client::parseReplayArray(matjson::Value const& json) {
  if (!json.isArray()) return Err("Invalid server response");

  std::vector<ReplaySummary> replays;
  for (auto const& value : json.asArray().unwrap()) {
    auto id = value.get("id");
    auto score = value.get("score");
    auto upvotes = value.get("upvotes");
    auto downvotes = value.get("downvotes");
    auto userCoins = value.get("user_coins");
    auto createdAt = value.get("created_at");
    if (id.isErr() || score.isErr() || upvotes.isErr() || downvotes.isErr() || userCoins.isErr() ||
        createdAt.isErr()) {
      return Err("Invalid replay list entry");
    }

    auto parsedID = id.unwrap().asString();
    auto parsedScore = score.unwrap().asInt();
    auto parsedUpvotes = upvotes.unwrap().asInt();
    auto parsedDownvotes = downvotes.unwrap().asInt();
    auto parsedCreatedAt = createdAt.unwrap().asString();
    if (parsedID.isErr() || parsedID.unwrap().empty() || parsedScore.isErr() ||
        parsedUpvotes.isErr() || parsedUpvotes.unwrap() < 0 || parsedDownvotes.isErr() ||
        parsedDownvotes.unwrap() < 0 || parsedCreatedAt.isErr() ||
        parsedCreatedAt.unwrap().empty()) {
      return Err("Invalid replay list entry");
    }

    int64_t parsedCoins = -1;
    if (!userCoins.unwrap().isNull()) {
      auto coins = userCoins.unwrap().asInt();
      if (coins.isErr() || coins.unwrap() < 0 || coins.unwrap() > 3) {
        return Err("Invalid replay list entry");
      }
      parsedCoins = coins.unwrap();
    }

    auto visibleScore = parsedScore.unwrap();

    replays.push_back({
      parsedID.unwrap(),
      visibleScore,
      parsedUpvotes.unwrap(),
      parsedDownvotes.unwrap(),
      static_cast<int>(parsedCoins),
      parsedCreatedAt.unwrap(),
    });
  }

  return Ok(std::move(replays));
}

Result<ReplayViews> Client::parseReplayList(web::WebResponse const& response) {
  if (auto status = expectSuccess(response); status.isErr()) return Err(status.unwrapErr());

  auto json = response.json();
  if (json.isErr() || !json.unwrap().isObject()) return Err("Invalid server response");

  auto parseView = [&json](char const* name) -> Result<std::vector<ReplaySummary>> {
    auto value = json.unwrap().get(name);
    if (value.isErr()) return Err("Invalid server response");
    return parseReplayArray(value.unwrap());
  };

  auto recommended = parseView("recommended");
  auto newReplays = parseView("new");
  auto top = parseView("top");
  auto different = parseView("different");
  auto mine = parseView("mine");
  if (recommended.isErr() || newReplays.isErr() || top.isErr() || different.isErr() ||
      mine.isErr()) {
    return Err("Invalid server response");
  }

  return Ok(ReplayViews{
    std::move(recommended).unwrap(),
    std::move(newReplays).unwrap(),
    std::move(top).unwrap(),
    std::move(different).unwrap(),
    std::move(mine).unwrap(),
  });
}

arc::Future<Result<bool>> Client::observe(LevelIdentity level) {
  auto response = co_await request()
                    .bodyJSON(matjson::makeObject({
                      {"level_id", level.id},
                      {"revision", level.revision},
                      {"content_hash", level.hash},
                    }))
                    .post(std::string(origin) + "/v4/levels/observe");
  co_return parseObserve(response);
}

arc::Future<Result<>> Client::upload(LevelIdentity level) {
  auto response = co_await request()
                    .bodyString(level.encoded)
                    .put(std::string(origin) + "/v4/levels/" + level.hash + "/content");
  co_return expectSuccess(response);
}

arc::Future<Result<ReplayViews>> Client::listReplays(std::string hash) {
  auto response = co_await request().get(std::string(origin) + "/v4/levels/" + hash + "/replays");
  co_return parseReplayList(response);
}

arc::Future<Result<ByteVector>> Client::replayData(std::string hash, std::string id) {
  auto response = co_await request()
                    .header("Cache-Control", "no-cache")
                    .get(std::string(origin) + "/v4/levels/" + hash + "/replays/" + id + "/data");
  if (auto status = expectSuccess(response); status.isErr()) co_return Err(status.unwrapErr());
  co_return Ok(response.data());
}

arc::Future<Result<>> Client::submit(std::string hash, ByteVector replay) {
  auto response =
    co_await request()
      .bodyJSON(matjson::makeObject({
        {"content_hash", hash},
        {"replay_base64", utils::base64::encode(replay, utils::base64::Base64Variant::Normal)},
      }))
      .post(std::string(origin) + "/v4/submissions");
  co_return expectSuccess(response);
}

arc::Future<Result<>> Client::vote(std::string hash, std::string replayID, int value) {
  auto response =
    co_await request()
      .bodyJSON(matjson::makeObject({{"vote", value}}))
      .put(std::string(origin) + "/v4/levels/" + hash + "/replays/" + replayID + "/vote");
  co_return expectSuccess(response);
}

arc::Future<Result<>> Client::report(Report report) {
  std::vector<matjson::Value> segments;
  segments.reserve(report.segments.size());

  for (auto const& segment : report.segments) {
    segments.push_back(matjson::makeObject({
      {"sequence", segment.sequence},
      {"mode", segmentModeName(segment.mode)},
      {"start_tick", segment.startTick},
      {"end_tick", segment.endTick},
      {"outcome", segmentOutcomeName(segment.outcome)},
    }));
  }

  auto response = co_await request()
                    .bodyJSON(matjson::makeObject({
                      {"session_id", report.session.sessionID},
                      {"level_id", report.session.levelID},
                      {"revision", report.session.revision},
                      {"content_hash", report.session.contentHash},
                      {"ended", report.ended},
                      {"segments", matjson::Value(std::move(segments))},
                    }))
                    .post(std::string(origin) + "/v4/report/sessions");
  co_return expectSuccess(response);
}

}
