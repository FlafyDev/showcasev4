#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <optional>

#include "managers/client.hpp"
#include "managers/session.hpp"
#include "models/level.hpp"
#include "ui/replay_popup.hpp"
#include "utils/gdr2_decode.hpp"

using namespace geode::prelude;

namespace showcase {

class $modify(ShowcaseLevelInfoLayer, LevelInfoLayer) {
  struct Fields {
    async::TaskHolder<Result<ReplayViews>> m_replayListRequest;
    async::TaskHolder<Result<ByteVector>> m_replayDownloadRequest;
    std::optional<Replay> m_recommendedReplay;
  };

  bool init(GJGameLevel* level, bool challenge) {
    if (!LevelInfoLayer::init(level, challenge)) return false;
    if (!level || level->isPlatformer()) return true;

    auto playMenu = m_playBtnMenu;
    if (!playMenu) return true;

    auto icon = CircleButtonSprite::createWithSprite("clapper.png"_spr, 0.9f);
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
      auto hash = identity.unwrap().hash;
      m_fields->m_replayListRequest.spawn("Showcase level contains replays",
        Client::get().listReplays(hash), [this, button, hash](Result<ReplayViews> replays) {
          if (!isRunning() || replays.isErr() || replays.unwrap().recommended.empty()) return;

          button->setVisible(true);

          auto replayID = replays.unwrap().recommended.front().id;
          m_fields->m_replayDownloadRequest.spawn("Showcase preload recommended replay",
            Client::get().replayData(hash, std::move(replayID)),
            [this](Result<ByteVector> replayData) {
              if (!isRunning() || replayData.isErr()) return;

              auto replay = gdr2Decode(replayData.unwrap());
              if (replay.isOk()) {
                m_fields->m_recommendedReplay.emplace(std::move(replay).unwrap());
              }
            });
        });
    }

    return true;
  }

  void onPlay(CCObject* sender) {
    if (!ReplaySession::hasQueued() && m_fields->m_recommendedReplay.has_value()) {
      ReplaySession::queue(std::make_unique<ReplaySession>(std::move(m_fields->m_recommendedReplay.value()), false, false, false));
      m_fields->m_recommendedReplay.reset();
    }

    LevelInfoLayer::onPlay(sender);
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
