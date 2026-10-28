#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>

using namespace geode::prelude;

static int s_currentTargetOriginalLevelID = 0;

class $modify(MyLevelInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) {
            return false;
        }

        if (!level || level->m_levelName.empty() || level->m_levelID.value() <= 0) {
            return true;
        }

        auto leftMenu = this->getChildByID("left-side-menu");
        if (!leftMenu) {
            leftMenu = this->getChildByID("other-menu");
        }

        auto buttonSprite = CircleButtonSprite::createWithSpriteFrameName(
            "startpos_search.png"_spr,
            1.0f,
            CircleBaseColor::Green,
            CircleBaseSize::Medium
        );
        if (!buttonSprite) {
            buttonSprite = CircleButtonSprite::createWithSpriteFrameName(
                "edit_findBtn_001.png",
                1.0f,
                CircleBaseColor::Green,
                CircleBaseSize::Medium
            );
        }
        if (!buttonSprite) {
            return true;
        }
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
        if (!m_level || m_level->m_levelID.value() <= 0) {
            return;
        }

        int targetID = m_level->m_levelID.value();
        if (m_level->m_originalLevel.value() > 0 && m_level->m_originalLevel.value() != m_level->m_levelID.value()) {
            targetID = m_level->m_originalLevel.value();
        }
        s_currentTargetOriginalLevelID = targetID;

        std::string rawName = m_level->m_levelName;
        std::string cleanName = rawName;

        auto toLower = [](std::string s) {
            for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return s;
        };

        std::string lower = toLower(cleanName);
        for (const std::string& suffix : {" startpos", " start pos", " copy", " start", " pos"}) {
            if (lower.length() > suffix.length() && lower.ends_with(suffix)) {
                cleanName = cleanName.substr(0, cleanName.length() - suffix.length());
                lower = toLower(cleanName);
            }
        }
        while (!cleanName.empty() && cleanName.back() == ' ') {
            cleanName.pop_back();
        }
        if (cleanName.empty()) {
            cleanName = rawName;
        }

        std::string query = cleanName + " startpos";
        if (query.length() > 20) {
            size_t maxLen = 20 > 9 ? 20 - 9 : 0;
            std::string trimmed = cleanName.substr(0, maxLen);
            while (!trimmed.empty() && (static_cast<unsigned char>(trimmed.back()) & 0xC0) == 0x80) {
                trimmed.pop_back();
            }
            if (!trimmed.empty() && (static_cast<unsigned char>(trimmed.back()) & 0x80) != 0) {
                trimmed.pop_back();
            }
            while (!trimmed.empty() && trimmed.back() == ' ') {
                trimmed.pop_back();
            }
            query = trimmed + " startpos";
        }

        auto searchObj = GJSearchObject::create(SearchType::Search, query);
        auto scene = LevelBrowserLayer::scene(searchObj);
        CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
    }
};

class $modify(MyLevelBrowserLayer, LevelBrowserLayer) {
    struct Fields {
        int m_targetOriginalLevelID = 0;
    };

    bool init(GJSearchObject* searchObj) {
        if (!LevelBrowserLayer::init(searchObj)) {
            return false;
        }
        m_fields->m_targetOriginalLevelID = s_currentTargetOriginalLevelID;
        return true;
    }

    void onBack(CCObject* sender) {
        if (m_fields->m_targetOriginalLevelID > 0 || s_currentTargetOriginalLevelID > 0) {
            s_currentTargetOriginalLevelID = 0;
            CCDirector::sharedDirector()->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
            return;
        }
        LevelBrowserLayer::onBack(sender);
    }

    void setupLevelBrowser(cocos2d::CCArray* levels) {
        int targetID = m_fields->m_targetOriginalLevelID;
        if (targetID <= 0) {
            targetID = s_currentTargetOriginalLevelID;
        }

        if (targetID > 0 && levels) {
            auto verifiedLevels = cocos2d::CCArray::create();
            auto otherLevels = cocos2d::CCArray::create();

            for (auto rawObj : CCArrayExt<CCObject*>(levels)) {
                if (!rawObj) continue;

                if (auto level = typeinfo_cast<GJGameLevel*>(rawObj)) {
                    if (level->m_originalLevel.value() == targetID || level->m_levelID.value() == targetID) {
                        verifiedLevels->addObject(level);
                        continue;
                    }
                }
                otherLevels->addObject(rawObj);
            }

            auto sortedLevels = cocos2d::CCArray::createWithCapacity(levels->count());
            for (auto obj : CCArrayExt<CCObject*>(verifiedLevels)) {
                sortedLevels->addObject(obj);
            }
            for (auto obj : CCArrayExt<CCObject*>(otherLevels)) {
                sortedLevels->addObject(obj);
            }

            LevelBrowserLayer::setupLevelBrowser(sortedLevels);
            return;
        }

        LevelBrowserLayer::setupLevelBrowser(levels);
    }
};

class $modify(MyLevelCell, LevelCell) {
    void loadFromLevel(GJGameLevel* level) {
        LevelCell::loadFromLevel(level);

        if (m_mainLayer) {
            if (auto existingBadge = m_mainLayer->getChildByID("verified-copy-badge"_spr)) {
                existingBadge->removeFromParent();
            }
        }

        int targetID = s_currentTargetOriginalLevelID;
        if (targetID <= 0 || !level || !m_mainLayer) {
            return;
        }

        bool isVerified = (level->m_originalLevel.value() == targetID || level->m_levelID.value() == targetID);
        if (!isVerified) {
            return;
        }

        CCSprite* badge = CCSprite::createWithSpriteFrameName("startpos_verified.png"_spr);
        if (!badge) {
            badge = CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
        }
        if (!badge) {
            return;
        }

        badge->setID("verified-copy-badge"_spr);
        badge->setScale(0.32f);

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
            float titleWidth = titleNode->getScaledContentSize().width;
            float titleRight = titleNode->getPositionX() + titleWidth * (1.f - titleNode->getAnchorPoint().x);
            float titleY = titleNode->getPositionY();

            if (titleRight + 20.f < 280.f) {
                badge->setPosition(ccp(titleRight + 8.f, titleY - 1.f));
            } else {
                CCNode* creatorNode = m_mainLayer->getChildByIDRecursive("creator-name");
                if (!creatorNode) {
                    creatorNode = m_mainLayer->getChildByID("creator-name");
                }

                if (creatorNode) {
                    float creatorWidth = creatorNode->getScaledContentSize().width;
                    float creatorRight = creatorNode->getPositionX() + creatorWidth * (1.f - creatorNode->getAnchorPoint().x);
                    badge->setPosition(ccp(creatorRight + 12.f, creatorNode->getPositionY()));
                } else {
                    badge->setPosition(ccp(titleNode->getPositionX() + 15.f, titleY + 12.f));
                }
            }
        } else {
            badge->setPosition(ccp(55.f, 62.f));
        }

        m_mainLayer->addChild(badge, 15);
    }
};
