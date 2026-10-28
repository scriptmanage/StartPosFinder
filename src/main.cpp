#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>

using namespace geode::prelude;

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
        if (s_isFilteringStartPos && s_targetOriginalLevelID > 0 && levels) {
            auto verifiedLevels = cocos2d::CCArray::create();
            auto otherLevels = cocos2d::CCArray::create();

            for (auto obj : CCArrayExt<GJGameLevel*>(levels)) {
                if (!obj) continue;

                if (obj->m_originalLevel.value() == s_targetOriginalLevelID) {
                    verifiedLevels->addObject(obj);
                } else {
                    otherLevels->addObject(obj);
                }
            }

            auto sortedLevels = cocos2d::CCArray::create();
            for (auto obj : CCArrayExt<GJGameLevel*>(verifiedLevels)) {
                sortedLevels->addObject(obj);
            }
            for (auto obj : CCArrayExt<GJGameLevel*>(otherLevels)) {
                sortedLevels->addObject(obj);
            }

            LevelBrowserLayer::setupLevelBrowser(sortedLevels);
            return;
        }

        LevelBrowserLayer::setupLevelBrowser(levels);
    }

    void destructor() {
        s_isFilteringStartPos = false;
        s_targetOriginalLevelID = 0;
        LevelBrowserLayer::~LevelBrowserLayer();
    }
};

class $modify(MyLevelCell, LevelCell) {
    void loadFromLevel(GJGameLevel* level) {
        LevelCell::loadFromLevel(level);

        if (!s_isFilteringStartPos || s_targetOriginalLevelID <= 0 || !level || !m_mainLayer) {
            return;
        }

        if (level->m_originalLevel.value() == s_targetOriginalLevelID) {
            if (auto existingBadge = m_mainLayer->getChildByID("verified-copy-badge"_spr)) {
                existingBadge->removeFromParent();
            }

            CCSprite* badge = CCSprite::create("startpos_verified.png"_spr);
            if (!badge) {
                badge = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
            }

            if (!badge) {
                return;
            }

            badge->setID("verified-copy-badge"_spr);
            badge->setScale(0.35f);

            CCNode* titleNode = m_mainLayer->getChildByID("level-name-label");
            if (!titleNode) {
                titleNode = m_mainLayer->getChildByID("level-name");
            }
            if (!titleNode) {
                for (auto child : CCArrayExt<CCNode*>(m_mainLayer->getChildren())) {
                    if (auto bmFont = typeinfo_cast<CCLabelBMFont*>(child)) {
                        if (bmFont->getPositionY() > 60.f && bmFont->getPositionX() < 100.f) {
                            titleNode = bmFont;
                            break;
                        }
                    }
                }
            }

            if (titleNode) {
                float titleRight = titleNode->getPositionX() + (titleNode->getContentSize().width * titleNode->getScale());
                float titleY = titleNode->getPositionY();

                if (titleRight + 16.f < 275.f) {
                    badge->setPosition(ccp(titleRight + 10.f, titleY));
                } else {
                    if (auto creatorNode = m_mainLayer->getChildByID("creator-name")) {
                        float creatorRight = creatorNode->getPositionX() + (creatorNode->getContentSize().width * creatorNode->getScale());
                        badge->setPosition(ccp(creatorRight + 10.f, creatorNode->getPositionY()));
                    } else {
                        badge->setPosition(ccp(38.f, 62.f));
                    }
                }
            } else {
                badge->setPosition(ccp(38.f, 62.f));
            }

            m_mainLayer->addChild(badge, 15);
        }
    }
};
