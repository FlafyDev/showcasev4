#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

#include "models/level.hpp"
#include "ui/replay_popup_view.hpp"

#include <functional>
#include <optional>

namespace showcase {

class ReplayPopup final : public geode::Popup {
 public:
  static ReplayPopup* create(LevelIdentity identity, std::function<void()> play);

 protected:
  bool init(LevelIdentity identity, std::function<void()> play);

 private:
  LevelIdentity m_identity;
  ReplayPopupView* m_view = nullptr;
  std::optional<ReplayViews> m_replayViews;
  std::string m_activeView = "recommended";
  std::function<void()> m_playLevel;
  geode::async::TaskHolder<geode::Result<ReplayViews>> m_listRequest;
  geode::async::TaskHolder<geode::Result<>> m_actionRequest;
  geode::async::TaskHolder<geode::Result<geode::ByteVector>> m_downloadRequest;
  bool m_loading = false;

  void selectView(std::string view);
  void fetchViews();
  void selectReplay(size_t index);
  void voteReplay(size_t index, int vote);
  void playSelected();
  void playReplay(size_t index);
};

}
