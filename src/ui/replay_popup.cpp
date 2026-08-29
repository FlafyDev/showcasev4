#include "replay_popup.hpp"
#include <memory>

#include "managers/auth.hpp"
#include "managers/client.hpp"
#include "managers/session.hpp"
#include "utils/gdr2_decode.hpp"

using namespace geode::prelude;

namespace showcase {

ReplayPopup* ReplayPopup::create(LevelIdentity identity, std::function<void()> play) {
  auto result = new ReplayPopup;
  if (result->init(std::move(identity), std::move(play))) {
    result->autorelease();
    return result;
  }
  delete result;
  return nullptr;
}

bool ReplayPopup::init(LevelIdentity identity, std::function<void()> playLevel) {
  if (!Popup::init(440.f, 224.f)) return false;

  m_identity = std::move(identity);
  m_playLevel = std::move(playLevel);

  setTitle("Showcase", "goldFont.fnt", .7f, 12.f);

  m_view = ReplayPopupView::create({440.f - 50.f, 224.f - 56.f},
    {
      .selectView = [this](std::string view) { selectView(std::move(view)); },
      .selectReplay = [this](size_t index) { selectReplay(index); },
      .vote = [this](size_t index, int vote) { voteReplay(index, vote); },
      .play = [this] { playSelected(); },
      .info =
        [] {
          FLAlertLayer::create(
            "Showcase", "Select a verified replay, vote on it, then press Play.", "OK")
            ->show();
        },
    });
  m_mainLayer->addChildAtPosition(m_view, Anchor::Center, {0.f, -8.f});

  fetchViews();
  selectView("recommended");

  return true;
}

void ReplayPopup::selectView(std::string view) {
  m_activeView = std::move(view);
  m_view->setActiveView(m_activeView);

  if (m_activeView == "mine" && !AuthManager::get().hasAccount()) {
    m_view->showStatus("No account logged in");
    return;
  }

  if (m_replayViews) {
    auto const* replays = &m_replayViews->recommended;
    if (m_activeView == "new")
      replays = &m_replayViews->newReplays;
    else if (m_activeView == "top")
      replays = &m_replayViews->top;
    else if (m_activeView == "different")
      replays = &m_replayViews->different;
    else if (m_activeView == "mine")
      replays = &m_replayViews->mine;
    m_view->showReplays(m_identity.hash, *replays, 0);
    return;
  }

  m_view->showStatus("Finding replays...");
}

void ReplayPopup::fetchViews() {
  if (m_loading) return;

  m_loading = true;
  m_listRequest.spawn("Showcase replay lists", Client::get().listReplays(m_identity.hash),
    [this](Result<ReplayViews> result) {
      m_loading = false;
      if (result.isErr()) {
        Notification::create("Failed to fetch replays", NotificationIcon::Error)->show();
        return;
      }
      m_replayViews = std::move(result).unwrap();
      selectView(m_activeView);
    });
}

void ReplayPopup::selectReplay(size_t index) {
  if (index >= m_view->replays().size()) return;
  m_view->showReplays(m_identity.hash, m_view->replays(), index);
}

void ReplayPopup::voteReplay(size_t index, int vote) {
  if (index >= m_view->replays().size()) return;

  auto replayID = m_view->replays()[index].id;
  m_actionRequest.spawn("Showcase replay vote", Client::get().vote(m_identity.hash, replayID, vote),
    [this](Result<> result) {
      if (result.isErr()) {
        Notification::create("Vote failed", NotificationIcon::Error)->show();
        return;
      }
      fetchViews();
    });
}

void ReplayPopup::playSelected() {
  playReplay(m_view->selected());
}

void ReplayPopup::playReplay(size_t index) {
  if (index >= m_view->replays().size()) return;
  auto replayID = m_view->replays()[index].id;

  m_downloadRequest.spawn("Showcase replay download",
    Client::get().replayData(m_identity.hash, replayID),
    [this, replayID](Result<ByteVector> result) {
      if (result.isErr()) {
        log::warn("Replay {} for level content {} download failed: {}", replayID, m_identity.hash,
          result.unwrapErr());
        Notification::create("Failed to download replay data", NotificationIcon::Error)->show();
        return;
      }

      auto bytes = std::move(result).unwrap();
      auto decoded = gdr2Decode(bytes);
      if (decoded.isErr()) {
        log::warn("Replay {} for level content {} decode failed: {}", replayID, m_identity.hash,
          decoded.unwrapErr());
        Notification::create("Failed to decode replay data", NotificationIcon::Error)->show();
        return;
      }

      auto replay = std::make_unique<const Replay>(std::move(decoded).unwrap());
      ReplaySession::queue(std::make_unique<ReplaySession>(std::move(replay), true));

      auto play = m_playLevel;
      onClose(nullptr);
      play();
    });
}

}
