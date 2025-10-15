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
        auto sprite = CCSprite::create("save.png"_spr);
        sprite->setScale(0.7f);
        auto buttonSprite = CircleButtonSprite::create(sprite, geode::CircleBaseColor::DarkPurple);
        auto button = CCMenuItemSpriteExtra::create(
            buttonSprite,
            this,
            menu_selector(MyAccountLayer::getValidation));
        button->setPosition({menu->getContentSize().width / 2, 25});

        menu->addChild(button);

        AccountLayer::customSetup();
    }

    void getValidation(CCObject *sender)
    {
        static EventListener<WebTask> listener;
        listener.bind([](WebTask::Event *e)
                      {
            if (WebResponse* resp = e->getValue()) {
                if (resp->ok()) {
                    auto jsonResult = resp->json();
                    if (!jsonResult) {
                        log::warn("Failed to parse JSON: {}", jsonResult.unwrapErr());
                        return;
                    }
                    auto json = jsonResult.unwrap();
                    bool valid = false;
                    if (json.isObject() && json.contains("valid")) {
                        valid = json["valid"].asBool().unwrapOr(false);
                    }
                    log::info("Validation response: {}", valid);
                    if (valid) {
                        // open the backup
                        auto popup = BackupPopup::create();
                        if (popup) popup->show();
                    }
                } else {
                    log::info("Validation request failed with status: {}", resp->code());
                    Notification::create(fmt::format("Validation request failed with status: {}", resp->code()), NotificationIcon::Error)->show();
                    return;
                }
            } else if (e->isCancelled()) {
                log::warn("Validation request was cancelled");
                Notification::create("Validation request was cancelled", NotificationIcon::Error)->show();
                return;
            } });

        // start argon auth to get an auth token
        auto res = argon::startAuth(argon::AuthCallback([](Result<std::string> result)
                                                        {
            if (!result) {
                log::warn("Argon authentication failed: {}", result.unwrapErr());
                return;
            }
            auto token = std::move(result).unwrap();
            log::info("Argon authentication succeeded, token: {}", token);

            // fetch compressed save string after auth succeeded
            if (auto gm = GameManager::sharedState()) {
                auto save = gm->getCompressedSaveString(); // get the compressed save string
                clipboard::write(save);
                log::debug("Compressed save string fetched");
            } else {
                log::debug("GameManager::sharedState() returned null");
            }

            // get the current account id (GJAccountManager provides this)
            int accountId = 0;
            if (auto acct = GJAccountManager::get()) {
                accountId = acct->m_accountID;
            }


            std::string url = std::string("https://argon.globed.dev/v1/validation/check?account_id=") +
                std::to_string(accountId) + "&authtoken=" + token;

            auto req = WebRequest()
                .timeout(std::chrono::seconds(10))
                .get(url);

            listener.setFilter(std::move(req)); }));
    }
};