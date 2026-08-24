#include "replay_popup_view.hpp"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <unordered_set>

// DISCLAIMER: I REALLY HATE UIs

using namespace geode::prelude;

namespace showcase {

namespace {
constexpr int kUpvoteTag = 10'000;
constexpr int kDownvoteTag = 20'000;

CCLabelBMFont* label(std::string const& value, char const* font, float scale) {
  auto result = CCLabelBMFont::create(value.c_str(), font);
  result->setScale(scale);
  return result;
}

CCNode* container(CCSize size) {
  auto result = CCNode::create();
  result->setContentSize(size);
  result->setAnchorPoint({.5f, .5f});
  result->ignoreAnchorPointForPosition(false);
  return result;
}

void addPanelBackground(CCNode* parent, ccColor3B color, GLubyte opacity = 220) {
  auto background = CCScale9Sprite::create("square02b_001.png");
  background->setColor(color);
  background->setOpacity(opacity);
  background->setContentSize(parent->getContentSize());
  background->setID("panel-background"_spr);
  parent->addChildAtPosition(background, Anchor::Center, {}, -2);
}

CCScale9Sprite* compactPanel(CCSize size, ccColor3B color) {
  constexpr float scale = .4f;
  auto panel = CCScale9Sprite::create("square02b_001.png");
  panel->setContentSize(size / scale);
  panel->setScale(scale);
  panel->setColor(color);
  return panel;
}

void tintButtonBackground(ButtonSprite* button, ccColor3B color) {
  if (button->m_BGSprite) {
    button->m_BGSprite->setColor(color);
  } else if (button->m_subBGSprite) {
    button->m_subBGSprite->setColor(color);
  }
}

CCNode* tabFace(std::string const& title, float width, bool pressed, ButtonSprite** buttonOut) {
  auto face = container({width, 24.f});
  constexpr float scale = .6f;
  auto button = ButtonSprite::create(title.c_str(), static_cast<int>(width / scale), true,
    "bigFont.fnt", "GJ_button_04.png", 40.f, .57f);
  auto nativeSize = button->getContentSize();
  auto fittedScale = std::min(width / nativeSize.width, 22.f / nativeSize.height);
  button->setScale(std::min(scale, fittedScale));
  face->setContentHeight(40.f * button->getScale());
  tintButtonBackground(button, pressed ? ccColor3B{100, 62, 42} : ccColor3B{140, 80, 48});
  face->addChildAtPosition(button, Anchor::Center);
  *buttonOut = button;
  return face;
}

void configureLocalMenu(CCMenu* menu, CCSize size, CCPoint position) {
  menu->ignoreAnchorPointForPosition(false);
  menu->setAnchorPoint({.5f, .5f});
  menu->setContentSize(size);
  menu->setPosition(position);
}

void setButtonScale(CCMenuItemSpriteExtra* button, float scale) {
  button->setScale(scale);
  button->m_baseScale = scale;
}

CCNode* controlIcon(int index) {
  std::string sprite;
  switch (index) {
    case 0:
      sprite = "ghost_badge.png"_spr;
      break;
    case 1:
      sprite = "inputs_badge.png"_spr;
      break;
    case 2:
      sprite = "play_badge.png"_spr;
      break;
    default:
      return CCNode::create();
  }

  auto icon = CCSprite::create(sprite.c_str());
  icon->setScale(.2f);
  return icon;
}

CCNode* controlToggleSprite(int index, bool active) {
  auto result =
    CircleButtonSprite::create(controlIcon(index), CircleBaseColor::Green, CircleBaseSize::Small);
  if (!active) result->setColor({110, 110, 110});
  return result;
}

uint64_t hashText(std::string const& value) {
  uint64_t hash = 1469598103934665603ull;
  for (auto byte : value) {
    hash ^= static_cast<uint8_t>(byte);
    hash *= 1099511628211ull;
  }
  return hash;
}

std::string replayCode(std::string const& levelHash, std::string const& replayID,
  std::unordered_set<std::string>& used) {
  constexpr char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
  auto value = hashText(levelHash + ":" + replayID);
  std::string code(6, 'A');
  do {
    auto candidate = value;
    for (auto& character : code) {
      character = alphabet[candidate & 31];
      candidate >>= 5;
    }
    value += 0x9e3779b97f4a7c15ull;
  } while (!used.insert(code).second);
  return code;
}

std::string ageLabel(std::string const& createdAt) {
  if (createdAt.size() < 19) return "new";
  std::tm parsed{};
  std::istringstream input(createdAt.substr(0, 19));
  input >> std::get_time(&parsed, "%Y-%m-%dT%H:%M:%S");
  if (input.fail()) return "new";
#ifdef _WIN32
  auto created = _mkgmtime(&parsed);
#else
  auto created = timegm(&parsed);
#endif
  auto seconds = std::max<std::time_t>(0, std::time(nullptr) - created);
  if (seconds < 3600) return fmt::format("{}m", std::max<std::time_t>(1, seconds / 60));
  if (seconds < 86400) return fmt::format("{}h", seconds / 3600);
  if (seconds < 86400 * 7) return fmt::format("{}d", seconds / 86400);
  if (seconds < 86400 * 35) return fmt::format("{}w", seconds / (86400 * 7));
  return fmt::format("{}mo", seconds / (86400 * 30));
}

CCNode* voteSprite(int64_t votes, bool upvote) {
  constexpr float buttonScale = .575f;
  auto result = container({66.f * buttonScale, 23.f});
  auto button =
    ButtonSprite::create("", 66 - 16, true, "bigFont.fnt", "GJ_button_04.png", 30.f, .47f);
  button->setScale(buttonScale);
  tintButtonBackground(button, upvote ? ccColor3B{210, 145, 20} : ccColor3B{190, 72, 55});
  result->addChildAtPosition(button, Anchor::Center);

  auto icon = CCSprite::createWithSpriteFrameName(
    upvote ? "GJ_likesIcon_001.png" : "GJ_dislikesIcon_001.png");
  icon->setScale(.45f);
  icon->setPosition({10.f, upvote ? 11.5f : 13.5f});
  result->addChild(icon);
  auto count = label(fmt::format("{}", votes), "bigFont.fnt", .27f);
  count->setAnchorPoint({1.0f, 0.5f});
  count->setPosition({34.f, 11.5f});
  result->addChild(count);
  return result;
}

CCNode* markerStrip(int userCoins, bool pressed) {
  auto markers = container({32.f, 26.f});
  if (userCoins < 0) return markers;
  for (int index = 0; index < 3; ++index) {
    auto marker = CCSprite::createWithSpriteFrameName("usercoin_small01_001.png");
    auto active = index < std::clamp(userCoins, 0, 3);
    auto color = active ? ccColor3B{255, 255, 255} : ccColor3B{165, 165, 165};
    if (pressed) {
      color.r = static_cast<GLubyte>(color.r * .65f);
      color.g = static_cast<GLubyte>(color.g * .65f);
      color.b = static_cast<GLubyte>(color.b * .65f);
    }
    marker->setColor(color);
    marker->setOpacity(255);
    marker->setScale(.82f);
    marker->setPosition({8.f + index * 7.5f, 13.f});
    markers->addChild(marker);
  }
  return markers;
}
}

ReplayPopupView* ReplayPopupView::create(CCSize size, ReplayPopupActions actions) {
  auto result = new ReplayPopupView;
  if (result->init(size, std::move(actions))) {
    result->autorelease();
    return result;
  }
  delete result;
  return nullptr;
}

bool ReplayPopupView::init(CCSize size, ReplayPopupActions actions) {
  if (!CCNode::init()) return false;
  m_actions = std::move(actions);
  setContentSize(size);
  setAnchorPoint({.5f, .5f});
  ignoreAnchorPointForPosition(false);
  setID("replay-popup-view"_spr);

  m_tabs = CCMenu::create();
  configureLocalMenu(m_tabs, {size.width, 30.f}, {size.width / 2.f, size.height - 5.f});
  m_tabs->setID("view-tabs"_spr);
  constexpr float tabGap = 2.f;
  auto tabsWidth = size.width;
  auto tabWidth = (tabsWidth - tabGap * (views.size() - 1)) / views.size();
  auto firstTabX = (size.width - tabsWidth) / 2.f + tabWidth / 2.f;
  for (size_t index = 0; index < views.size(); ++index) {
    auto title = std::string{views[index]};
    title[0] = static_cast<char>(std::toupper(title[0]));
    ButtonSprite* face = nullptr;
    ButtonSprite* pressedFace = nullptr;
    auto tab = CCMenuItemSprite::create(tabFace(title, tabWidth, false, &face),
      tabFace(title, tabWidth, true, &pressedFace), this,
      menu_selector(ReplayPopupView::onSelectView));
    tab->setTag(static_cast<int>(index));
    tab->setID(fmt::format("view-tab-{}"_spr, index));
    tab->setPosition({firstTabX + index * (tabWidth + tabGap), 15.f});
    m_tabs->addChild(tab);
    m_tabButtons.push_back(tab);
    m_tabFaces.push_back(face);
    m_tabPressedFaces.push_back(pressedFace);
  }
  addChild(m_tabs);

  constexpr float bodyY = 78.f;
  constexpr float bodyHeight = 140.f;
  constexpr float controlsWidth = 128.f;
  constexpr float panelGap = 7.f;
  auto replayWidth = size.width - controlsWidth - panelGap;

  auto controls = buildControls({controlsWidth, bodyHeight});
  controls->setPosition({controlsWidth / 2.f, bodyY});
  addChild(controls);

  auto replayPanel = buildReplayPanel({replayWidth, bodyHeight});
  replayPanel->setPosition({controlsWidth + panelGap + replayWidth / 2.f, bodyY});
  addChild(replayPanel);
  return true;
}

CCNode* ReplayPopupView::buildControls(CCSize size) {
  auto panel = container(size);
  panel->setID("controls-panel"_spr);
  addPanelBackground(panel, {70, 38, 22});

  auto tools = CCMenu::create();
  tools->setID("controls-menu"_spr);
  configureLocalMenu(tools, {112.f, 22.f}, {size.width / 2.f, size.height - 32.f});
  for (int index = 0; index < 3; ++index) {
    auto button = CCMenuItemToggler::create(controlToggleSprite(index, false),
      controlToggleSprite(index, true), this, nullptr);
    button->toggle(index == 2);
    button->setEnabled(false);
    button->setScale(.72f);
    button->setTag(index);
    button->setID(fmt::format("control-toggle-{}"_spr, index));
    button->setPosition({20.f + index * 36.f, 21.f});
    tools->addChild(button);
  }
  panel->addChild(tools);

  m_offset = label("+0.5s", "bigFont.fnt", .30f);
  m_offset->setPosition({28.f, 100.f});
  m_offset->setVisible(false);
  panel->addChild(m_offset);

  auto playMenu = CCMenu::create();
  playMenu->setID("play-menu"_spr);
  configureLocalMenu(playMenu, {52.f, 32.f}, {93.f, 19.f});
  playMenu->setAnchorPoint({0.0f, 0.0f});
  playMenu->setPosition({8.f, 8.f});
  ButtonSprite* playFace = nullptr;
  auto playButton = CCMenuItemSpriteExtra::create(
    tabFace("PLAY", 50, false, &playFace), this, menu_selector(ReplayPopupView::onPlay));
  playButton->setID("play-button"_spr);
  playMenu->setContentSize(playButton->getContentSize());
  playButton->setPosition({playButton->getContentWidth() / 2, playButton->getContentHeight() / 2});
  playMenu->addChild(playButton);
  panel->addChild(playMenu);

  auto recordMenu = CCMenu::create();
  recordMenu->setID("record-menu"_spr);
  configureLocalMenu(recordMenu, {52.f, 32.f}, {93.f, 19.f});
  recordMenu->setAnchorPoint({1.0f, 0.0f});
  recordMenu->setPosition({128.f - 8.f, 8.f});
  ButtonSprite* recordFace = nullptr;
  auto recordButton = CCMenuItemSpriteExtra::create(
    tabFace("RECORD", 50.f, false, &recordFace), this, menu_selector(ReplayPopupView::onRecord));
  recordButton->setID("record-button"_spr);
  recordMenu->setContentSize(recordButton->getContentSize());
  recordButton->setPosition(
    {recordButton->getContentWidth() / 2, recordButton->getContentHeight() / 2});
  recordMenu->addChild(recordButton);
  panel->addChild(recordMenu);
  return panel;
}

CCNode* ReplayPopupView::buildReplayPanel(CCSize size) {
  m_replayPanel = container(size);
  m_replayPanel->setID("replay-panel"_spr);
  addPanelBackground(m_replayPanel, {70, 38, 22});

  m_status = label("Finding verified replays...", "bigFont.fnt", .42f);
  m_status->setID("status-label"_spr);
  m_status->setPosition({size.width / 2.f, size.height / 2.f});
  m_replayPanel->addChild(m_status);
  return m_replayPanel;
}

CCNode* ReplayPopupView::buildReplayRow(std::string const& levelHash, ReplaySummary const& replay,
  size_t index, std::unordered_set<std::string>& usedCodes) {
  auto row = container({m_replayPanel->getContentWidth() - 14.f, 27.f});
  row->setID(fmt::format("replay-row-{}"_spr, index));

  auto menu = CCMenu::create();
  menu->setID(fmt::format("replay-select-menu-{}"_spr, index));
  configureLocalMenu(menu, row->getContentSize() - CCSize{6.f, 2.f},
    {row->getContentWidth() / 2.f, row->getContentHeight() / 2.f});

  auto faceWidth = menu->getContentWidth();
  auto centerY = menu->getContentHeight() / 2.f;
  auto codeText = replayCode(levelHash, replay.id, usedCodes);
  auto makeRowFace = [&](bool pressed) {
    auto face = container(row->getContentSize() - CCSize{6.f, 2.f});
    auto border = compactPanel(
      face->getContentSize(), index == m_selected ? ccColor3B{255, 205, 0} : ccColor3B{38, 19, 10});
    face->addChildAtPosition(border, Anchor::Center);
    auto fillColor = index == m_selected ? ccColor3B{145, 78, 24} : ccColor3B{91, 49, 25};
    if (pressed) {
      fillColor.r = static_cast<GLubyte>(fillColor.r * .65f);
      fillColor.g = static_cast<GLubyte>(fillColor.g * .65f);
      fillColor.b = static_cast<GLubyte>(fillColor.b * .65f);
    }
    auto fill = compactPanel(face->getContentSize() - CCSize{2.f, 2.f}, fillColor);
    face->addChildAtPosition(fill, Anchor::Center);

    auto rankText = label(fmt::format("{}", index + 1), "goldFont.fnt", .34f);
    if (pressed) rankText->setColor({165, 135, 0});
    rankText->setPosition({10.f, centerY});
    face->addChild(rankText);
    auto code = label(codeText, "bigFont.fnt", .31f);
    code->setColor(pressed ? ccColor3B{124, 124, 124} : ccColor3B{190, 190, 190});
    code->setAnchorPoint({0.f, .5f});
    code->setPosition({20.f, centerY});
    face->addChild(code);
    auto markers = markerStrip(replay.userCoins, pressed);
    markers->setPosition({103.f, centerY});
    face->addChild(markers);
    auto age = label(ageLabel(replay.createdAt), "bigFont.fnt", .25f);
    if (pressed) age->setColor({165, 165, 165});
    age->setAnchorPoint({1.f, .5f});
    age->setPosition({face->getContentWidth() - 6.f, centerY});
    face->addChild(age);
    return face;
  };

  auto face = makeRowFace(false);
  face->setPosition({row->getContentWidth() / 2.f, row->getContentHeight() / 2.f});
  row->addChild(face);

  auto pressed = container(face->getContentSize());
  auto shade = compactPanel(face->getContentSize(), {0, 0, 0});
  shade->setOpacity(95);
  pressed->addChildAtPosition(shade, Anchor::Center);
  auto selector = CCMenuItemSprite::create(container(face->getContentSize()), pressed, this,
    menu_selector(ReplayPopupView::onSelectReplay));
  selector->setTag(static_cast<int>(index));
  selector->setID(fmt::format("replay-select-button-{}"_spr, index));
  selector->setPosition({faceWidth / 2.f, centerY});
  menu->addChild(selector);

  auto voteMenu = CCMenu::create();
  voteMenu->setID(fmt::format("replay-vote-menu-{}"_spr, index));
  configureLocalMenu(voteMenu, row->getContentSize() - CCSize{6.f, 2.f},
    {row->getContentWidth() / 2.f, row->getContentHeight() / 2.f});
  voteMenu->setTouchPriority(menu->getTouchPriority() - 1);

  auto upvote = CCMenuItemSprite::create(voteSprite(replay.upvotes, true),
    voteSprite(replay.upvotes, true), this, menu_selector(ReplayPopupView::onVote));
  upvote->setTag(kUpvoteTag + static_cast<int>(index));
  upvote->setID(fmt::format("replay-upvote-button-{}"_spr, index));
  upvote->setPosition({faceWidth - 84.f, centerY});
  voteMenu->addChild(upvote);

  auto downvote = CCMenuItemSprite::create(voteSprite(replay.downvotes, false),
    voteSprite(replay.downvotes, false), this, menu_selector(ReplayPopupView::onVote));
  downvote->setTag(kDownvoteTag + static_cast<int>(index));
  downvote->setID(fmt::format("replay-downvote-button-{}"_spr, index));
  downvote->setPosition({faceWidth - 44.f, centerY});
  voteMenu->addChild(downvote);

  row->addChild(menu, 3);
  row->addChild(voteMenu, 2);
  return row;
}

void ReplayPopupView::setActiveView(std::string const& view) {
  auto selected = std::find(views.begin(), views.end(), view);
  if (selected == views.end()) return;
  auto index = static_cast<size_t>(selected - views.begin());
  for (size_t tab = 0; tab < m_tabButtons.size(); ++tab) {
    auto color = tab == index ? ccColor3B{165, 108, 68} : ccColor3B{140, 80, 48};
    tintButtonBackground(m_tabFaces[tab], color);
    tintButtonBackground(m_tabPressedFaces[tab], {100, 62, 42});
  }
}

void ReplayPopupView::showStatus(std::string const& message) {
  if (m_list) m_list->setVisible(false);
  m_status->setString(message.c_str());
  m_status->setVisible(true);
}

void ReplayPopupView::showReplays(
  std::string const& levelHash, std::vector<ReplaySummary> replays, size_t selected) {
  m_replays = std::move(replays);
  if (m_replays.empty()) {
    m_selected = 0;
    showStatus("No verified replays yet");
    return;
  }
  m_selected = std::min(selected, m_replays.size() - 1);
  if (m_list) m_list->removeFromParent();

  m_list = container(m_replayPanel->getContentSize() - CCSize{10.f, 10.f});
  m_list->setID("replay-list"_spr);
  std::unordered_set<std::string> usedCodes;
  for (size_t index = 0; index < std::min<size_t>(m_replays.size(), 4); ++index) {
    auto row = buildReplayRow(levelHash, m_replays[index], index, usedCodes);
    row->setPosition(
      {m_list->getContentWidth() / 2.f, m_list->getContentHeight() - 15.f - index * 28.f});
    m_list->addChild(row);
  }
  m_list->setPosition(
    {m_replayPanel->getContentWidth() / 2.f, m_replayPanel->getContentHeight() / 2.f});
  m_replayPanel->addChild(m_list);
  m_status->setVisible(false);
}

std::vector<ReplaySummary> const& ReplayPopupView::replays() const {
  return m_replays;
}

size_t ReplayPopupView::selected() const {
  return m_selected;
}

void ReplayPopupView::onSelectView(CCObject* sender) {
  auto index = static_cast<CCNode*>(sender)->getTag();
  if (index < 0 || static_cast<size_t>(index) >= views.size()) return;
  if (m_actions.selectView) m_actions.selectView(views[index]);
}

void ReplayPopupView::onSelectReplay(CCObject* sender) {
  auto index = static_cast<CCNode*>(sender)->getTag();
  if (index < 0 || static_cast<size_t>(index) >= m_replays.size()) return;
  if (m_actions.selectReplay) m_actions.selectReplay(static_cast<size_t>(index));
}

void ReplayPopupView::onVote(CCObject* sender) {
  auto tag = static_cast<CCNode*>(sender)->getTag();
  auto vote = tag >= kDownvoteTag ? -1 : 1;
  auto index = tag - (vote > 0 ? kUpvoteTag : kDownvoteTag);
  if (index < 0 || static_cast<size_t>(index) >= m_replays.size()) return;
  if (m_actions.vote) m_actions.vote(static_cast<size_t>(index), vote);
}

void ReplayPopupView::onPlay(CCObject*) {
  if (m_actions.play) m_actions.play();
}

void ReplayPopupView::onRecord(CCObject*) {
  Notification::create("Coming soon...")->show();
}

void ReplayPopupView::onInfo(CCObject*) {
  if (m_actions.info) m_actions.info();
}

}
