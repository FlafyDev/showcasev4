#include "seeker_driver.hpp"

#include "managers/session.hpp"

#include <algorithm>
#include <limits>

using namespace geode::prelude;

namespace showcase {

namespace {

bool nodeVisible(CCNode* node) {
  for (; node; node = node->getParent()) {
    if (!node->isVisible()) return false;
  }
  return true;
}

}

class ProgressBarTouchLayer : public CCLayer {
 public:
  static ProgressBarTouchLayer* create(
    SeekerDriver* driver, PauseLayer* pauseLayer, PlayLayer* playLayer) {
    auto* layer = new ProgressBarTouchLayer();
    if (layer && layer->init()) {
      layer->m_driver = driver;
      layer->m_pauseLayer = pauseLayer;
      layer->m_playLayer = playLayer;
      layer->autorelease();

      return layer;
    }

    CC_SAFE_DELETE(layer);
    return nullptr;
  }

  bool init() override {
    if (!CCLayer::init()) return false;

    setTouchEnabled(true);
    setTouchMode(kCCTouchesOneByOne);
    setTouchPriority(-1000);

    m_line = CCLayerColor::create({255, 255, 255, 255});
    m_line->setAnchorPoint({.5f, 0.f});
    m_line->setVisible(false);
    addChild(m_line);

    m_label = CCLabelBMFont::create("", "bigFont.fnt");
    m_label->setScale(.35f);
    m_label->setVisible(false);
    addChild(m_label);
    scheduleUpdate();
    return true;
  }

  void raiseProgressBar() {
    raiseNode(m_playLayer->m_progressBar, m_barHome, std::numeric_limits<int>::max() - 2);
    raiseNode(
      m_playLayer->m_percentageLabel, m_percentageHome, std::numeric_limits<int>::max() - 1);
  }

  bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
    if (!percentAt(touch->getLocation())) return false;
    m_touchActive = true;
    m_valid = true;
    updateIndicator(touch->getLocation());
    return true;
  }

  void ccTouchMoved(CCTouch* touch, CCEvent*) override {
    if (!m_valid) return;
    if (!percentAt(touch->getLocation())) {
      m_valid = false;
      hideIndicator();
      return;
    }
    updateIndicator(touch->getLocation());
  }

  void ccTouchEnded(CCTouch* touch, CCEvent*) override {
    auto percent = m_valid ? percentAt(touch->getLocation()) : std::nullopt;
    m_touchActive = false;
    m_valid = false;
    hideIndicator();
    if (!percent) return;

    m_driver->startSeek(*percent);
    setTouchEnabled(false);
    Ref<PauseLayer> pauseLayer = m_pauseLayer;
    Loader::get()->queueInMainThread([pauseLayer] { pauseLayer->onResume(nullptr); });
  }

  void ccTouchCancelled(CCTouch*, CCEvent*) override {
    m_touchActive = false;
    m_valid = false;
    hideIndicator();
  }

  void update(float) override {
    if (m_touchActive) return;
    auto mouse = geode::cocos::getMousePos();
    if (percentAt(mouse)) {
      updateIndicator(mouse);
    } else {
      hideIndicator();
    }
  }

  void onExit() override {
    restoreNode(m_barHome);
    restoreNode(m_percentageHome);
    CCLayer::onExit();
  }

 private:
  struct NodeHome {
    CCNode* node = nullptr;
    CCNode* parent = nullptr;
    CCPoint position;
    int zOrder = 0;
  };

  void raiseNode(CCNode* node, NodeHome& home, int zOrder) {
    if (!node) return;
    home = {node, node->getParent(), node->getPosition(), node->getZOrder()};
    auto worldPosition = home.parent->convertToWorldSpace(home.position);
    node->retain();
    node->removeFromParentAndCleanup(false);
    addChild(node, zOrder);
    node->setPosition(convertToNodeSpace(worldPosition));
    node->release();
  }

  void restoreNode(NodeHome& home) {
    if (!home.node) return;
    home.node->retain();
    home.node->removeFromParentAndCleanup(false);
    home.parent->addChild(home.node, home.zOrder);
    home.node->setPosition(home.position);
    home.node->release();
    home = {};
  }

  std::optional<float> percentAt(CCPoint world) const {
    auto* bar = m_playLayer->m_progressBar;
    if (!bar || !nodeVisible(bar)) return std::nullopt;

    auto local = bar->convertToNodeSpace(world);
    auto size = bar->getContentSize();
    if (local.x < 0.f || local.x > size.width || local.y < -8.f || local.y > size.height + 8.f) {
      return std::nullopt;
    }
    return 100.f * local.x / size.width;
  }

  void updateIndicator(CCPoint world) {
    auto percent = percentAt(world);
    if (!percent) return;

    auto* bar = m_playLayer->m_progressBar;
    auto x = bar->getContentSize().width * *percent / 100.f;
    auto bottom = convertToNodeSpace(bar->convertToWorldSpace({x, 0.f}));
    auto top = convertToNodeSpace(bar->convertToWorldSpace({x, bar->getContentSize().height}));
    m_line->setContentSize({1.f, std::max(1.f, top.y - bottom.y + 6.f)});
    m_line->setPosition({bottom.x, bottom.y - 3.f});
    m_line->setVisible(true);
    m_label->setString(fmt::format("{:.1f}%", *percent).c_str());
    m_label->setPosition({bottom.x, bottom.y - 11.f});
    m_label->setVisible(true);
  }

  void hideIndicator() {
    m_line->setVisible(false);
    m_label->setVisible(false);
  }

  SeekerDriver* m_driver = nullptr;
  PauseLayer* m_pauseLayer = nullptr;
  PlayLayer* m_playLayer = nullptr;
  NodeHome m_barHome;
  NodeHome m_percentageHome;
  CCLayerColor* m_line = nullptr;
  CCLabelBMFont* m_label = nullptr;
  bool m_touchActive = false;
  bool m_valid = false;
};

SeekerDriver::SeekerDriver(ReplaySession* session, bool enabled)
  : m_session(session), m_enabled(enabled) {}

SeekerDriver::~SeekerDriver() {
  terminateSeek();
}

void SeekerDriver::onPlayLayerPause(PauseLayer* pauseLayer) {
  if (!m_enabled) return;
  if (auto* touchLayer = ProgressBarTouchLayer::create(this, pauseLayer, m_session->m_playLayer)) {
    touchLayer->setID("progress-seek-touch-layer"_spr);
    pauseLayer->addChild(touchLayer, std::numeric_limits<int>::max());
    touchLayer->raiseProgressBar();
  }
}

bool SeekerDriver::reachedSeekingTarget() const {
  return m_seeking && !m_targetCaptured &&
         m_session->m_playLayer->getCurrentPercent() >= m_targetPercent;
}

bool SeekerDriver::seeking() const {
  return m_seeking;
}

bool SeekerDriver::targetCaptured() const {
  return m_targetCaptured;
}

void SeekerDriver::terminateSeek() {
  if (!m_seeking) return;

  if (m_overlay) {
    m_overlay->removeFromParent();
    m_overlay = nullptr;
  }
  m_seeking = false;
  m_targetCaptured = false;
}

bool SeekerDriver::fastForwarding() const {
  return m_seeking && !PlayLayer::get()->m_isPaused;
}

void SeekerDriver::startSeek(float targetPercent) {
  terminateSeek();
  m_session->autoPlay().enable();

  auto* playLayer = m_session->m_playLayer;

  if (!playLayer->m_isPracticeMode) playLayer->togglePracticeMode(true);
  playLayer->removeAllCheckpoints();
  playLayer->resetLevel();

  m_overlay = CCLayerColor::create({20, 20, 22, 190});
  m_overlay->setID("seek-overlay"_spr);
  m_overlay->setContentSize(CCDirector::sharedDirector()->getWinSize());
  playLayer->addChild(m_overlay, playLayer->m_uiLayer->getZOrder() - 1);

  m_targetPercent = std::clamp(targetPercent, 0.0f, 100.00f);
  m_seeking = true;
}

void SeekerDriver::captureTarget() {
  if (!m_seeking || m_targetCaptured) return;

  auto* playLayer = m_session->m_playLayer;

  if (auto* checkpoint = playLayer->createCheckpoint()) {
    playLayer->storeCheckpoint(checkpoint);
    playLayer->m_currentCheckpoint = checkpoint;
    m_targetCaptured = true;
  }
}

void SeekerDriver::endSeek() {
  if (!m_seeking || !m_targetCaptured) return;

  auto* playLayer = m_session->m_playLayer;

  if (m_overlay) {
    m_overlay->removeFromParent();
    m_overlay = nullptr;
  }
  m_seeking = false;
  m_targetCaptured = false;
  m_session->autoPlay().disable();
  playLayer->resetLevel();
  // playLayer->GJBaseGameLayer::handleButton(false, 1, true);
  // playLayer->GJBaseGameLayer::handleButton(false, 1, false);
}

}
