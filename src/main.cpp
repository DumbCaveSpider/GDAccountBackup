#include <Geode/Geode.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/modify/AccountLayer.hpp>
#include <Geode/modify/GameStatsManager.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/async.hpp>
#include <argon/argon.hpp>
#include <matjson.hpp>

#include "BackupPopup.hpp"
#include "helper.hpp"

using namespace geode::prelude;
using namespace geode::utils::web;

class $modify(MyAccountLayer, AccountLayer) {
      void customSetup() {
            log::debug("AccountLayer says hi");
            // menu at the right side
            auto menu = CCMenu::create();
            menu->setPosition({m_mainLayer->getContentSize().width / 2, 20});
            menu->setContentSize({45, 120});
            m_mainLayer->addChild(menu);

            // button to open the popup
            // @geode-ignore(unknown-resource)
            auto sprite = CCSprite::create("saveIcon.png"_spr);
            auto buttonSprite =
                CircleButtonSprite::create(sprite, geode::CircleBaseColor::DarkPurple);
            auto button = CCMenuItemSpriteExtra::create(
                buttonSprite, this, menu_selector(MyAccountLayer::getValidation));
            sprite->setScale(0.5f);
            button->setPosition({0, 25});

            menu->addChild(button);

            AccountLayer::customSetup();
      }

      void getValidation(CCObject* sender) {
            // hide button and show the loading spinner
            if (auto button = static_cast<CCMenuItemSpriteExtra*>(sender)) {
                  button->setEnabled(false);
                  button->setOpacity(0);
                  auto spinner = LoadingSpinner::create(45.f);
                  spinner->setAnchorPoint({0.f, 0.f});
                  spinner->setTag(1000);
                  button->addChild(spinner);
            }
            // if the accountId = -1 or 0, the user is not logged in
            if (auto acct = GJAccountManager::get()) {
                  if (acct->m_accountID == -1 || acct->m_accountID == 0) {
                        log::info("User is not logged in, cannot open backup popup");
                        Notification::create("You must be logged in into your account!",
                                             NotificationIcon::Error)
                            ->show();
                        // re-enable the button and remove the spinner
                        if (auto button = static_cast<CCMenuItemSpriteExtra*>(sender)) {
                              button->setEnabled(true);
                              button->setOpacity(255);
                              if (auto spinner = button->getChildByTag(1000)) {
                                    spinner->removeFromParent();
                              }
                        }
                        return;
                  }
            }

            // get the authentication token
            auto buttonSender = sender;

            auto accountData = argon::getGameAccountData();
            if (!accountData.valid()) {
                  Notification::create("You must be logged in to Geometry Dash to authenticate",
                                       NotificationIcon::Error)
                      ->show();
                  if (auto button = static_cast<CCMenuItemSpriteExtra*>(buttonSender)) {
                        button->setEnabled(true);
                        button->setOpacity(255);
                        if (auto spinner = button->getChildByTag(1000)) {
                              spinner->removeFromParent();
                        }
                  }
                  return;
            }

            async::spawn(
                  argon::startAuth(std::move(accountData)),
                  [buttonSender](Result<std::string> result) {
                  if (!result) {
                        log::warn("Argon authentication failed: {}", result.unwrapErr());
                        argon::clearToken();
                        Notification::create(result.unwrapErr(),
                                             NotificationIcon::Error)
                            ->show();
                        // restore UI
                        if (auto button =
                                static_cast<CCMenuItemSpriteExtra*>(buttonSender)) {
                              button->setEnabled(true);
                              button->setOpacity(255);
                              if (auto spinner = button->getChildByTag(1000)) {
                                    spinner->removeFromParent();
                              }
                        }
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
                  static async::TaskHolder<web::WebResponse> authListener;

                  matjson::Value body = matjson::makeObject(
                      {{"accountId", accountId}, {"argonToken", token}});
                  std::string backupUrl =
                      Mod::get()->getSettingValue<std::string>("backup-url");

                  auto authReq = createBackupRequest()
                                     .timeout(std::chrono::seconds(10))
                                     .header("Content-Type", "application/json")
                                     .bodyJSON(body)
                                     .post(backupUrl + "/auth");

                  authListener.spawn(std::move(authReq), [buttonSender](web::WebResponse resp) {
                        if (resp.ok()) {
                              auto strResult = resp.string();
                              if (strResult) {
                                    const std::string& str = strResult.unwrap();
                                    auto parsed = matjson::Value::parse(str);
                                    if (parsed) {
                                          auto obj = parsed.unwrap();
                                          if (auto authStatus = obj.asInt()) {
                                                int status = authStatus.unwrap();
                                                if (status == 1) {
                                                      log::info("Authentication successful");
                                                      // open the backup popup
                                                      auto popup = BackupPopup::create();
                                                      if (popup)
                                                            popup->show();
                                                      // restore UI in any case
                                                      if (auto button =
                                                              static_cast<CCMenuItemSpriteExtra*>(buttonSender)) {
                                                            button->setEnabled(true);
                                                            button->setOpacity(255);
                                                            if (auto spinner = button->getChildByTag(1000)) {
                                                                  spinner->removeFromParent();
                                                            }
                                                      }
                                                } else {
                                                      log::warn("Authentication failed: response was not 1");
                                                      Notification::create("Authentication failed",
                                                                           NotificationIcon::Error)
                                                          ->show();
                                                      // re-enable the button and remove the spinner
                                                      if (auto button =
                                                              static_cast<CCMenuItemSpriteExtra*>(buttonSender)) {
                                                            button->setEnabled(true);
                                                            button->setOpacity(255);
                                                            if (auto spinner = button->getChildByTag(1000)) {
                                                                  spinner->removeFromParent();
                                                            }
                                                      }
                                                }
                                          } else {
                                                log::warn("Authentication failed: could not parse response as integer");
                                                Notification::create("Authentication failed: invalid response",
                                                                     NotificationIcon::Error)
                                                    ->show();
                                                // re-enable the button and remove the spinner
                                                if (auto button =
                                                        static_cast<CCMenuItemSpriteExtra*>(buttonSender)) {
                                                      button->setEnabled(true);
                                                      button->setOpacity(255);
                                                      if (auto spinner = button->getChildByTag(1000)) {
                                                            spinner->removeFromParent();
                                                      }
                                                }
                                          }
                                    } else {
                                          log::warn("Authentication failed: could not parse response JSON");
                                          Notification::create("Authentication failed: invalid response",
                                                               NotificationIcon::Error)
                                              ->show();
                                          // re-enable the button and remove the spinner
                                          if (auto button =
                                                  static_cast<CCMenuItemSpriteExtra*>(buttonSender)) {
                                                button->setEnabled(true);
                                                button->setOpacity(255);
                                                if (auto spinner = button->getChildByTag(1000)) {
                                                      spinner->removeFromParent();
                                                }
                                          }
                                    }
                              } else {
                                    log::warn("Authentication failed: could not get response body");
                                    Notification::create("Authentication failed: invalid response",
                                                         NotificationIcon::Error)
                                        ->show();
                                    // re-enable the button and remove the spinner
                                    if (auto button =
                                            static_cast<CCMenuItemSpriteExtra*>(buttonSender)) {
                                          button->setEnabled(true);
                                          button->setOpacity(255);
                                          if (auto spinner = button->getChildByTag(1000)) {
                                                spinner->removeFromParent();
                                          }
                                    }
                              }
                        } else {
                              log::info("Authentication failed with status: {}",
                                        resp.code());
                              Notification::create(
                                  fmt::format("Authentication failed with status: {}",
                                              resp.code()),
                                  NotificationIcon::Error)
                                  ->show();
                              // re-enable the button and remove the spinner
                              if (auto button =
                                      static_cast<CCMenuItemSpriteExtra*>(buttonSender)) {
                                    button->setEnabled(true);
                                    button->setOpacity(255);
                                    if (auto spinner = button->getChildByTag(1000)) {
                                          spinner->removeFromParent();
                                    }
                              }
                        }
                  });
            }
            );
      }
};

static void startAutoBackup() {
      bool autoBackup = Mod::get()->getSettingValue<bool>("auto-backup");

      if (!autoBackup) {
            log::warn("Auto-backup is disabled, skipping backup");
            return;
      }

      // make sure the player is on a level
      if (!PlayLayer::get())
            return;
      // check if this is the first time setup
      if (!Mod::get()->getSavedValue<bool>("hasRead2")) {
            BackupPopup::showNotice();
            Mod::get()->setSavedValue<bool>("hasRead2", true);
      }

      log::info("starting auto-backup");
      Notification::create("Auto-backup in progress", NotificationIcon::Loading)
          ->show();
      // backup only the account data
      std::string token = Mod::get()->getSavedValue<std::string>("argonToken");
      int accountId = 0;
      if (auto acct = GJAccountManager::get()) {
            accountId = acct->m_accountID;
      }
      std::string saveData;
      if (auto gm = GameManager::sharedState()) {
            saveData = gm->getCompressedSaveString();
      }

      std::string backupUrl = Mod::get()->getSettingValue<std::string>("backup-url");

      // save account data only
      matjson::Value bodySave = matjson::makeObject({{"accountId", accountId},
                                                     {"saveData", saveData},
                                                     {"argonToken", token}});

      auto reqSave = createBackupRequest()
                         .timeout(std::chrono::seconds(30))
                         .bodyJSON(bodySave)
                         .post(backupUrl + "/save");
      static async::TaskHolder<web::WebResponse> saveListener;
      saveListener.spawn(std::move(reqSave), [](web::WebResponse resp) {
            if (resp.ok()) {
                  log::info("Auto-backup successful");
                  Notification::create("Auto-backup completed successfully!",
                                       NotificationIcon::Success)
                      ->show();
            } else {
                  log::warn("Auto-backup failed with status: {}", resp.code());
                  Notification::create(
                      fmt::format("Auto-backup failed with status: {}", resp.code()),
                      NotificationIcon::Error)
                      ->show();
            }
      });
}

class $modify(MyGameStatsManager, GameStatsManager) {
      void completedLevel(GJGameLevel* p0) {
            GameStatsManager::completedLevel(p0);
            startAutoBackup();
      }
};