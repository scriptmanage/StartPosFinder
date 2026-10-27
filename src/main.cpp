#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>

using namespace geode::prelude;

class $modify(MyLevelInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) {
            return false;
        }

        if (!level || level->m_levelName.empty()) {
            return true;
        }

        auto leftMenu = this->getChildByID("left-side-menu");
        if (!leftMenu) {
            leftMenu = this->getChildByID("other-menu");
        }

        auto buttonSprite = CircleButtonSprite::createWithSpriteFrameName(
            "gj_findBtn_001.png",
            0.85f,
            CircleBaseColor::Green,
            CircleBaseSize::Medium
        );
        buttonSprite->setScale(0.75f);

        auto btn = CCMenuItemSpriteExtra::create(
            buttonSprite,
            this,
            menu_selector(MyLevelInfoLayer::onFindStartPos)
        );
        btn->setID("find-startpos-btn"_spr);

        if (leftMenu) {
            leftMenu->addChild(btn);
            leftMenu->updateLayout();
        } else {
            auto menu = CCMenu::create();
            menu->setPosition(ccp(40.f, CCDirector::sharedDirector()->getWinSize().height / 2.f));
            menu->addChild(btn);
            menu->setID("startpos-menu"_spr);
            this->addChild(menu);
        }

        return true;
    }

    void onFindStartPos(CCObject* sender) {
        if (!m_level) {
            return;
        }

        std::string levelName = m_level->m_levelName;
        std::string target = levelName + " startpos";
        std::string query;

        if (target.length() <= 20) {
            query = target;
        } else {
            query = target.substr(0, 20);
            while (!query.empty() && query.back() == ' ') {
                query.pop_back();
            }
        }

        auto searchObj = GJSearchObject::create(SearchType::Search, query);
        auto scene = LevelBrowserLayer::scene(searchObj);
        CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
    }
};
