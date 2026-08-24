#pragma once

#include <Geode/Geode.hpp>
#include <array>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

#include "models/replay.hpp"

namespace showcase {

struct ReplayPopupActions {
  std::function<void(std::string)> selectView;
  std::function<void(size_t)> selectReplay;
  std::function<void(size_t, int)> vote;
  std::function<void()> play;
  std::function<void()> info;
};

class ReplayPopupView final : public cocos2d::CCNode {
 public:
  static ReplayPopupView* create(cocos2d::CCSize size, ReplayPopupActions actions);

  void setActiveView(std::string const& view);
  void showStatus(std::string const& message);
  void showReplays(
    std::string const& levelHash, std::vector<ReplaySummary> replays, size_t selected);
  std::vector<ReplaySummary> const& replays() const;
  size_t selected() const;

 private:
  static constexpr std::array<char const*, 5> views{
    "recommended", "top", "new", "different", "mine"};

  ReplayPopupActions m_actions;
  cocos2d::CCMenu* m_tabs = nullptr;
  std::vector<cocos2d::CCMenuItemSprite*> m_tabButtons;
  std::vector<ButtonSprite*> m_tabFaces;
  std::vector<ButtonSprite*> m_tabPressedFaces;
  cocos2d::CCNode* m_replayPanel = nullptr;
  cocos2d::CCNode* m_list = nullptr;
  cocos2d::CCLabelBMFont* m_status = nullptr;
  cocos2d::CCLabelBMFont* m_offset = nullptr;
  std::vector<ReplaySummary> m_replays;
  size_t m_selected = 0;

  bool init(cocos2d::CCSize size, ReplayPopupActions actions);
  cocos2d::CCNode* buildControls(cocos2d::CCSize size);
  cocos2d::CCNode* buildReplayPanel(cocos2d::CCSize size);
  cocos2d::CCNode* buildReplayRow(std::string const& levelHash, ReplaySummary const& replay,
    size_t index, std::unordered_set<std::string>& usedCodes);

  void onSelectView(cocos2d::CCObject* sender);
  void onSelectReplay(cocos2d::CCObject* sender);
  void onVote(cocos2d::CCObject* sender);
  void onPlay(cocos2d::CCObject* sender);
  void onRecord(cocos2d::CCObject* sender);
  void onInfo(cocos2d::CCObject* sender);
};

}
