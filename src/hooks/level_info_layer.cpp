#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>

#include "models/level.hpp"
#include "managers/client.hpp"
#include "ui/replay_popup.hpp"

using namespace geode::prelude;

namespace showcase {

class $modify(ShowcaseLevelInfoLayer, LevelInfoLayer) {
  struct Fields {
    async::TaskHolder<Result<ReplayViews>> replayListRequest;
  };

  bool init(GJGameLevel* level, bool challenge) {
    if (!LevelInfoLayer::init(level, challenge)) return false;
    if (!level || level->isPlatformer()) return true;

    auto playMenu = m_playBtnMenu;
    if (!playMenu) return true;

    auto icon = CircleButtonSprite::createWithSprite("clapper.png"_spr);
    icon->setScale(.5f);
    auto button = CCMenuItemSpriteExtra::create(
      icon, this, menu_selector(ShowcaseLevelInfoLayer::openShowcase));
    button->setID("replay-button"_spr);
    button->setPosition({27.f, 27.f});
    button->setZOrder(-2);
    button->setVisible(false);

    auto overlay = CCMenu::create();
    overlay->setID("replay-menu"_spr);
    overlay->setTouchPriority(playMenu->getTouchPriority() - 1);
    overlay->setPosition(playMenu->getPosition());
    overlay->setContentSize(playMenu->getContentSize());
    overlay->setAnchorPoint(playMenu->getAnchorPoint());
    overlay->setZOrder(playMenu->getZOrder() + 1);
    overlay->addChild(button);
    addChild(overlay);

    // BUG: Doesn't show showcase icon when level is not downloaded yet.
    // It's not so easy to fix because the level content hash is needed.
    auto identity = getLevelIdentity(m_level);
    if (identity.isOk()) {
      m_fields->replayListRequest.spawn("Showcase level contains replays",
        Client::get().listReplays(identity.unwrap().hash),
        [this, button](Result<ReplayViews> replays) {
          if (isRunning() && replays.isOk() && !replays.unwrap().recommended.empty()) {
            button->setVisible(true);
          }
        });
    }

    return true;
  }

  void openShowcase(CCObject*) {
    auto identity = getLevelIdentity(m_level);
    if (identity.isErr()) {
      FLAlertLayer::create("Showcase", identity.unwrapErr(), "OK")->show();
      return;
    }
    Ref<LevelInfoLayer> layer = this;
    ReplayPopup::create(std::move(identity).unwrap(), [layer] {
      if (layer->isRunning()) {
        layer->onPlay(nullptr);
      }
    })->show();
  }
};

}
