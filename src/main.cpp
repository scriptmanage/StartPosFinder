#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>

using namespace geode::prelude;

// Храним ID уровня, из которого перешли в поиск StartPos
static int s_targetOriginalLevelID = 0;
static bool s_isFilteringStartPos = false;

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

        // Запоминаем целевой ID для верификации оригинальности копий
        s_targetOriginalLevelID = m_level->m_levelID.value();
        s_isFilteringStartPos = true;

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

class $modify(MyLevelBrowserLayer, LevelBrowserLayer) {
    void setupLevelBrowser(cocos2d::CCArray* levels) {
        // Если поиск вызван нашей кнопкой и задан ID исходного уровня
        if (s_isFilteringStartPos && s_targetOriginalLevelID > 0 && levels) {
            auto filtered = cocos2d::CCArray::create();

            for (auto obj : CCArrayExt<GJGameLevel*>(levels)) {
                if (!obj) continue;

                // Проверяем, что уровень является подлинной копией:
                // m_originalLevel содержит ID уровня, с которого снята копия
                if (obj->m_originalLevel.value() == s_targetOriginalLevelID) {
                    filtered->addObject(obj);
                }
            }

            // Передаем в оригинальный метод только отфильтрованные уровни
            LevelBrowserLayer::setupLevelBrowser(filtered);
            return;
        }

        LevelBrowserLayer::setupLevelBrowser(levels);
    }

    void destructor() {
        // При выходе из браузера сбрасываем флаг фильтрации
        s_isFilteringStartPos = false;
        s_targetOriginalLevelID = 0;
        LevelBrowserLayer::~LevelBrowserLayer();
    }
};
