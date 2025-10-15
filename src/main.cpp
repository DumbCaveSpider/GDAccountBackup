#include <Geode/Geode.hpp>
#include <argon/argon.hpp>
#include <Geode/modify/AccountLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include "BackupPopup.hpp"
#include <Geode/utils/web.hpp>
#include <matjson.hpp>

using namespace geode::prelude;
using namespace geode::utils::web;

class $modify(MyAccountLayer, AccountLayer)
{
    void customSetup()
    {
        log::debug("AccountLayer says hi");
        // menu at the right side
        auto menu = CCMenu::create();
        menu->setPosition({450, 20});
        menu->setContentSize({45, 120});
        m_mainLayer->addChild(menu);

        // button to open the popup
        // @geode-ignore(unknown-resource)
        auto sprite = CCSprite::create("saveIcon.png"_spr);
        auto buttonSprite = CircleButtonSprite::create(sprite, geode::CircleBaseColor::DarkPurple);
        auto button = CCMenuItemSpriteExtra::create(
            buttonSprite,
            this,
            menu_selector(MyAccountLayer::getValidation));
        sprite->setScale(0.5f);
        button->setPosition({menu->getContentSize().width / 2, 25});

        menu->addChild(button);

        AccountLayer::customSetup();
    }

    void getValidation(CCObject *sender)
    {
        // if the accountId = -1 or 0, the user is not logged in
        if (auto acct = GJAccountManager::get())
        {
            if (acct->m_accountID == -1 || acct->m_accountID == 0)
            {
                log::info("User is not logged in, cannot open backup popup");
                Notification::create("You must be logged in into your account!", NotificationIcon::Error)->show();
                return;
            }
        }

        // get the authentication token
        auto res = argon::startAuth([](Result<std::string> result)
        {
            if (!result) {
                log::warn("Argon authentication failed: {}", result.unwrapErr());
                Notification::create("Authentication failed", NotificationIcon::Error)->show();
                return;
            }

            auto token = std::move(result).unwrap();
            log::info("Argon authentication succeeded, token received");
            Mod::get()->setSavedValue("argonToken", token);

            // get account id
            int accountId = 0;
            if (auto acct = GJAccountManager::get()) {
                accountId = acct->m_accountID;
            }

            // Now validate with /auth endpoint
            static EventListener<WebTask> authListener;
            authListener.bind([](WebTask::Event *e)
            {
                if (WebResponse* resp = e->getValue()) {
                    if (resp->ok()) {
                        log::info("Authentication successful");
                        // open the backup popup
                        auto popup = BackupPopup::create();
                        if (popup) popup->show();
                    } else {
                        log::info("Authentication failed with status: {}", resp->code());
                        Notification::create(fmt::format("Authentication failed with status: {}", resp->code()), NotificationIcon::Error)->show();
                    }
                } else if (e->isCancelled()) {
                    log::warn("Authentication request was cancelled");
                    Notification::create("Authentication request was cancelled", NotificationIcon::Error)->show();
                }
            });

            matjson::Value body = matjson::makeObject({{"accountId", accountId}, {"argonToken", token}});
            std::string backupUrl = Mod::get()->getSettingValue<std::string>("backup-url");

            auto authReq = WebRequest()
                .timeout(std::chrono::seconds(10))
                .header("Content-Type", "application/json")
                .bodyJSON(body)
                .post(backupUrl + "/auth");

            authListener.setFilter(std::move(authReq));
        }, [](argon::AuthProgress progress) {
            // Log authentication progress
            log::info("Auth progress: {}", argon::authProgressToString(progress));
        });

        if (!res) {
            log::warn("Failed to start auth attempt: {}", res.unwrapErr());
            Notification::create("Failed to start authentication", NotificationIcon::Error)->show();
        }
    }
};