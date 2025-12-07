#include "BackupPopup.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/utils/web.hpp>
#include <argon/argon.hpp>
#include <ctime>
#include <matjson.hpp>
#include <regex>
#include <sstream>

using namespace geode::prelude;

BackupPopup* BackupPopup::create() {
      auto ret = new BackupPopup();
      if (ret && ret->initAnchored(380.f, 275.f, "GJ_square02.png")) {
            ret->autorelease();
            return ret;
      }
      CC_SAFE_DELETE(ret);
      return nullptr;
}

bool BackupPopup::setup() {
      setTitle("Account Alternative Backup");
      int accountId = 0;
      if (auto acct = GJAccountManager::get()) {
            accountId = acct->m_accountID;
      }

      // try to get username (visible outside the if block)
      gd::string name;
      if (auto glm = GameLevelManager::sharedState()) {
            name = glm->tryGetUsername(accountId);
      }

      std::string labelText = "Backing up: " + std::string(name);
      auto label = CCLabelBMFont::create(labelText.c_str(), "bigFont.fnt");
      label->setAlignment(kCCTextAlignmentCenter);
      label->setPosition({m_mainLayer->getContentSize().width / 2,
                          m_mainLayer->getContentSize().height - 35});
      label->setScale(0.5f);
      m_mainLayer->addChild(label, 2);

      // combined account and local levels size label
      sizeLabel = CCLabelBMFont::create("Account & Local Level Data Size: ...", "chatFont.fnt");
      sizeLabel->setAlignment(kCCTextAlignmentCenter);
      sizeLabel->setPosition({m_mainLayer->getContentSize().width / 2,
                              m_mainLayer->getContentSize().height - 50});
      sizeLabel->setScale(0.5f);
      m_mainLayer->addChild(sizeLabel, 2);

      // last saved label
      lastSavedLabel = CCLabelBMFont::create("Last Saved: ...", "chatFont.fnt");
      lastSavedLabel->setAlignment(kCCTextAlignmentCenter);
      lastSavedLabel->setPosition({m_mainLayer->getContentSize().width / 2,
                                   m_mainLayer->getContentSize().height - 60});
      lastSavedLabel->setScale(0.5f);
      m_mainLayer->addChild(lastSavedLabel, 2);
      auto updateLastSaved = [this]() {
            int accountId = 0;
            if (auto acct = GJAccountManager::get()) {
                  accountId = acct->m_accountID;
            }
            std::string token = Mod::get()->getSavedValue<std::string>("argonToken");
            matjson::Value body =
                matjson::makeObject({{"accountId", accountId}, {"argonToken", token}});
            std::string backupUrl =
                Mod::get()->getSettingValue<std::string>("backup-url");
            auto req = geode::utils::web::WebRequest()
                           .timeout(std::chrono::seconds(30))
                           .header("Content-Type", "application/json")
                           .bodyJSON(body)
                           .post(backupUrl + "/lastsaved");
            static geode::EventListener<geode::utils::web::WebTask> lastSavedListener;
            lastSavedListener.bind([this](geode::utils::web::WebTask::Event* e) {
                  if (auto* resp = e->getValue()) {
                        if (resp->ok()) {
                              auto strResult = resp->string();
                              if (strResult) {
                                    lastSavedLabel->setString(
                                        formatLastSavedLabel(strResult.unwrap()).c_str());
                              } else {
                                    lastSavedLabel->setString("Last Saved: N/A");
                              }
                        } else {
                              lastSavedLabel->setString("Last Saved: N/A");
                        }
                  } else {
                        lastSavedLabel->setString("Last Saved: ...");
                  }
            });
            lastSavedListener.setFilter(std::move(req));
      };
      updateLastSaved();

      auto updateSize = [this]() {
            int accountId = 0;
            if (auto acct = GJAccountManager::get()) {
                  accountId = acct->m_accountID;
            }
            std::string token = Mod::get()->getSavedValue<std::string>("argonToken");
            matjson::Value body =
                matjson::makeObject({{"accountId", accountId}, {"argonToken", token}});
            std::string backupUrl =
                Mod::get()->getSettingValue<std::string>("backup-url");
            auto req = geode::utils::web::WebRequest()
                           .timeout(std::chrono::seconds(30))
                           .header("Content-Type", "application/json")
                           .bodyJSON(body)
                           .post(backupUrl + "/check");
            static geode::EventListener<geode::utils::web::WebTask> sizeListener;
            sizeListener.bind([this](geode::utils::web::WebTask::Event* e) {
                  if (auto* resp = e->getValue()) {
                        if (resp->ok()) {
                              auto strResult = resp->string();
                              if (strResult) {
                                    const std::string& str = strResult.unwrap();
                                    // expecting JSON: {"saveData":12345,"levelData":67890}
                                    auto parsed = matjson::Value::parse(str);
                                    if (parsed) {
                                          auto obj = parsed.unwrap();
                                          long long saveBytes = 0;
                                          long long levelBytes = 0;
                                          if (auto s = obj["saveData"].asInt())
                                                saveBytes = s.unwrap();
                                          if (auto l = obj["levelData"].asInt())
                                                levelBytes = l.unwrap();
                                          setCombinedSize(saveBytes, levelBytes);
                                    } else {
                                          setCombinedSizeNA();
                                    }
                              } else {
                                    setCombinedSizeNA();
                              }
                        } else {
                              setCombinedSizeNA();
                        }
                  } else {
                        setCombinedSizeLoading();
                  }
            });
            sizeListener.setFilter(std::move(req));
      };
      updateSize();

      float centerX = m_mainLayer->getContentSize().width / 2;
      float centerY = m_mainLayer->getContentSize().height / 2 - 20.f;
      float verticalSpacing = 36.f;

      const int numButtons = 4;

      auto saveAccountBtn = ButtonSprite::create("Save Account Data", 250.f, true, "goldFont.fnt", "GJ_button_01.png", 0.f, 1.f);
      auto saveAccountItem = CCMenuItemSpriteExtra::create(
          saveAccountBtn, this, menu_selector(BackupPopup::onSave));

      auto saveLevelsBtn = ButtonSprite::create("Save Local Levels", 250.f, true, "goldFont.fnt", "GJ_button_01.png", 0.f, 1.f);
      auto saveLevelsItem = CCMenuItemSpriteExtra::create(
          saveLevelsBtn, this, menu_selector(BackupPopup::onSaveLocalLevels));

      auto loadBtn = ButtonSprite::create("Load Account Data", 250.f, true, "goldFont.fnt", "GJ_button_03.png", 0.f, 1.f);
      auto loadItem = CCMenuItemSpriteExtra::create(
          loadBtn, this, menu_selector(BackupPopup::onLoad));

      auto loadLevelsBtn = ButtonSprite::create("Load Local Levels", 250.f, true, "goldFont.fnt", "GJ_button_03.png", 0.f, 1.f);
      auto loadLevelsItem = CCMenuItemSpriteExtra::create(
          loadLevelsBtn, this, menu_selector(BackupPopup::onLoadLocalLevels));

      auto deleteBtn = ButtonSprite::create("Delete Backups", 250.f, true, "goldFont.fnt", "GJ_button_06.png", 0.f, 1.f);
      auto deleteItem = CCMenuItemSpriteExtra::create(
          deleteBtn, this, menu_selector(BackupPopup::onDelete));
      float startY = verticalSpacing * ((numButtons - 1));
      float offsetStartY = 40.f;

      saveAccountItem->setPosition({0, startY - offsetStartY});
      saveLevelsItem->setPosition({0, startY - 1.0f * verticalSpacing - offsetStartY});
      loadItem->setPosition({0, startY - 2.0f * verticalSpacing - offsetStartY});
      loadLevelsItem->setPosition({0, startY - 3.0f * verticalSpacing - offsetStartY});
      deleteItem->setPosition({0, startY - 4.0f * verticalSpacing - offsetStartY});
      auto menu = CCMenu::create();
      menu->addChild(saveAccountItem);
      menu->addChild(saveLevelsItem);
      menu->addChild(loadItem);
      menu->addChild(loadLevelsItem);
      menu->addChild(deleteItem);
      menu->setPosition({centerX, centerY});
      m_mainLayer->addChild(menu, 5);

      // mod settings button in the top right corner
      auto modMenu = CCMenu::create();
      auto modSettingsBtnSprite = CircleButtonSprite::createWithSpriteFrameName(
          // @geode-ignore(unknown-resource)
          "geode.loader/settings.png", 1.f, CircleBaseColor::Green,
          CircleBaseSize::Medium);
      modSettingsBtnSprite->setScale(0.75f);

      auto modSettingsButton = CCMenuItemSpriteExtra::create(
          modSettingsBtnSprite, this, menu_selector(BackupPopup::onModSettings));
      modSettingsButton->setPosition({m_mainLayer->getContentSize().width,
                                      m_mainLayer->getContentSize().height});
      modMenu->addChild(modSettingsButton);
      modMenu->setPosition({0.f, 0.f});
      m_mainLayer->addChild(modMenu, 5);
      // art pretty things
      auto leftArt = CCSprite::createWithSpriteFrameName("rewardCorner_001.png");
      leftArt->setPosition({0.f, 0.f});
      leftArt->setScale(1.4f);
      leftArt->setAnchorPoint({0.f, 0.f});
      m_mainLayer->addChild(leftArt);

      auto rightArt = CCSprite::createWithSpriteFrameName("rewardCorner_001.png");
      rightArt->setPosition({m_mainLayer->getContentSize().width, 0.f});
      rightArt->setAnchorPoint({1.f, 0.f});
      rightArt->setScale(1.4f);
      rightArt->setFlipX(true);
      m_mainLayer->addChild(rightArt);

      // info button
      auto showNoticeSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
      auto showNoticeBtn = CCMenuItemSpriteExtra::create(showNoticeSpr, this, menu_selector(BackupPopup::onShowNotice));
      showNoticeBtn->setPosition({m_mainLayer->getContentSize().width - 25.f,
                                  25.f});
      modMenu->addChild(showNoticeBtn);

      return true;
}

void BackupPopup::onSave(CCObject* sender) {
      geode::createQuickPopup(
          "Save Account Data",
          "Do you want to <cg>save</c> your account data to the "
          "backup server?\n<cy>This will overwrite your existing backup saved in "
          "the server.</c>",
          "Cancel", "Save", [this, sender](FLAlertLayer*, bool confirmed) {
                if (!confirmed)
                      return;
                this->disableButton(sender);
                this->showLoading("Saving account data...");

                std::string token =
                    Mod::get()->getSavedValue<std::string>("argonToken");
                int accountId = 0;
                if (auto acct = GJAccountManager::get()) {
                      accountId = acct->m_accountID;
                }
                std::string saveData;
                if (auto gm = GameManager::sharedState()) {
                      saveData = gm->getCompressedSaveString();
                }

                std::string backupUrl =
                    Mod::get()->getSettingValue<std::string>("backup-url");

                matjson::Value bodySave = matjson::makeObject({{"accountId", accountId},
                                                               {"saveData", saveData},
                                                               {"argonToken", token}});
                auto reqSave = geode::utils::web::WebRequest()
                                   .timeout(std::chrono::seconds(30))
                                   .bodyJSON(bodySave)
                                   .post(backupUrl + "/save");

                static geode::EventListener<geode::utils::web::WebTask> saveListener;
                saveListener.bind([this, sender, accountId, token,
                                   backupUrl](geode::utils::web::WebTask::Event* e) {
                      if (auto* resp = e->getValue()) {
                            if (resp->ok()) {
                                  Notification::create("Account data saved successfully!",
                                                       NotificationIcon::Success)
                                      ->show();
                                  this->hideLoading();
                                  this->enableButton(sender);
                                  // refresh size
                                  matjson::Value body = matjson::makeObject(
                                      {{"accountId", accountId}, {"argonToken", token}});
                                  auto reqSize = geode::utils::web::WebRequest()
                                                     .timeout(std::chrono::seconds(30))
                                                     .bodyJSON(body)
                                                     .post(backupUrl + "/check");
                                  static geode::EventListener<geode::utils::web::WebTask>
                                      sizeListener2;
                                  sizeListener2.bind([this](geode::utils::web::WebTask::Event* e2) {
                                        if (auto* resp = e2->getValue()) {
                                              if (resp->ok()) {
                                                    auto strResult = resp->string();
                                                    if (strResult) {
                                                          const std::string& str = strResult.unwrap();
                                                          auto parsed = matjson::Value::parse(str);
                                                          if (parsed) {
                                                                auto obj = parsed.unwrap();
                                                                long long saveBytes = 0;
                                                                long long levelBytes = 0;
                                                                if (auto s = obj["saveData"].asInt())
                                                                      saveBytes = s.unwrap();
                                                                if (auto l = obj["levelData"].asInt())
                                                                      levelBytes = l.unwrap();
                                                                setCombinedSize(saveBytes, levelBytes);
                                                          } else {
                                                                setCombinedSizeNA();
                                                          }
                                                    } else {
                                                          setCombinedSizeNA();
                                                    }
                                              } else {
                                                    setCombinedSizeNA();
                                              }
                                        } else {
                                              setCombinedSizeLoading();
                                        }
                                  });
                                  sizeListener2.setFilter(std::move(reqSize));
                                  // refresh last saved
                                  auto reqLast = geode::utils::web::WebRequest()
                                                     .timeout(std::chrono::seconds(30))
                                                     .bodyJSON(body)
                                                     .post(backupUrl + "/lastsaved");
                                  static geode::EventListener<geode::utils::web::WebTask>
                                      lastSavedListener2;
                                  lastSavedListener2.bind(
                                      [this](geode::utils::web::WebTask::Event* e3) {
                                            if (auto* resp = e3->getValue()) {
                                                  if (resp->ok()) {
                                                        auto strResult = resp->string();
                                                        if (strResult) {
                                                              lastSavedLabel->setString(
                                                                  formatLastSavedLabel(strResult.unwrap()).c_str());
                                                        } else {
                                                              lastSavedLabel->setString("Last Saved: N/A");
                                                        }
                                                  } else {
                                                        lastSavedLabel->setString("Last Saved: N/A");
                                                  }
                                            } else {
                                                  lastSavedLabel->setString("Last Saved: ...");
                                            }
                                      });
                                  lastSavedListener2.setFilter(std::move(reqLast));
                            } else {
                                  Notification::create("Account Save failed: " +
                                                           std::to_string(resp->code()),
                                                       NotificationIcon::Error)
                                      ->show();
                                  this->hideLoading();
                                  this->enableButton(sender);
                            }
                      } else if (e->isCancelled()) {
                            Notification::create("Account Save request was cancelled",
                                                 NotificationIcon::Error)
                                ->show();
                            this->hideLoading();
                            this->enableButton(sender);
                      }
                });
                saveListener.setFilter(std::move(reqSave));
          });
      if (!Mod::get()->getSavedValue<bool>("hasRead2")) {
            BackupPopup::showNotice();
            Mod::get()->setSavedValue<bool>("hasRead2", true);
      }
}

void BackupPopup::onSaveLocalLevels(CCObject* sender) {
      geode::createQuickPopup(
          "Save Local Levels",
          "Do you want to <cg>save</c> your local levels to the "
          "backup server?\n<cy>This will overwrite your existing backup saved in "
          "the server.</c>",
          "Cancel", "Save", [this, sender](FLAlertLayer*, bool confirmed) {
                if (!confirmed)
                      return;
                this->disableButton(sender);
                this->showLoading("Saving local levels...");

                std::string token =
                    Mod::get()->getSavedValue<std::string>("argonToken");
                int accountId = 0;
                if (auto acct = GJAccountManager::get()) {
                      accountId = acct->m_accountID;
                }
                std::string levelData;
                if (auto gm = GameManager::sharedState()) {
                      levelData = LocalLevelManager::get()->getCompressedSaveString();
                }

                std::string backupUrl =
                    Mod::get()->getSettingValue<std::string>("backup-url");

                matjson::Value bodyLevel =
                    matjson::makeObject({{"accountId", accountId},
                                         {"levelData", levelData},
                                         {"argonToken", token}});
                auto reqLevel = geode::utils::web::WebRequest()
                                    .timeout(std::chrono::seconds(30))
                                    .bodyJSON(bodyLevel)
                                    .post(backupUrl + "/save");

                static geode::EventListener<geode::utils::web::WebTask> levelListener;
                levelListener.bind([this, sender, accountId, token,
                                    backupUrl](geode::utils::web::WebTask::Event* e) {
                      if (auto* resp = e->getValue()) {
                            if (resp->ok()) {
                                  Notification::create("Local Levels saved successfully!",
                                                       NotificationIcon::Success)
                                      ->show();
                                  this->hideLoading();
                                  this->enableButton(sender);
                                  // refresh size
                                  matjson::Value body = matjson::makeObject(
                                      {{"accountId", accountId}, {"argonToken", token}});
                                  auto reqSize = geode::utils::web::WebRequest()
                                                     .timeout(std::chrono::seconds(30))
                                                     .bodyJSON(body)
                                                     .post(backupUrl + "/check");
                                  static geode::EventListener<geode::utils::web::WebTask>
                                      sizeListener2;
                                  sizeListener2.bind([this](geode::utils::web::WebTask::Event* e2) {
                                        if (auto* resp = e2->getValue()) {
                                              if (resp->ok()) {
                                                    auto strResult = resp->string();
                                                    if (strResult) {
                                                          const std::string& str = strResult.unwrap();
                                                          auto parsed = matjson::Value::parse(str);
                                                          if (parsed) {
                                                                auto obj = parsed.unwrap();
                                                                long long saveBytes = 0;
                                                                long long levelBytes = 0;
                                                                if (auto s = obj["saveData"].asInt())
                                                                      saveBytes = s.unwrap();
                                                                if (auto l = obj["levelData"].asInt())
                                                                      levelBytes = l.unwrap();
                                                                setCombinedSize(saveBytes, levelBytes);
                                                          } else {
                                                                setCombinedSizeNA();
                                                          }
                                                    } else {
                                                          setCombinedSizeNA();
                                                    }
                                              } else {
                                                    setCombinedSizeNA();
                                              }
                                        } else {
                                              setCombinedSizeLoading();
                                        }
                                  });
                                  sizeListener2.setFilter(std::move(reqSize));
                                  // refresh last saved
                                  auto reqLast = geode::utils::web::WebRequest()
                                                     .timeout(std::chrono::seconds(30))
                                                     .bodyJSON(body)
                                                     .post(backupUrl + "/lastsaved");
                                  static geode::EventListener<geode::utils::web::WebTask>
                                      lastSavedListener2;
                                  lastSavedListener2.bind(
                                      [this](geode::utils::web::WebTask::Event* e3) {
                                            if (auto* resp = e3->getValue()) {
                                                  if (resp->ok()) {
                                                        auto strResult = resp->string();
                                                        if (strResult) {
                                                              lastSavedLabel->setString(
                                                                  formatLastSavedLabel(strResult.unwrap()).c_str());
                                                        } else {
                                                              lastSavedLabel->setString("Last Saved: N/A");
                                                        }
                                                  } else {
                                                        lastSavedLabel->setString("Last Saved: N/A");
                                                  }
                                            } else {
                                                  lastSavedLabel->setString("Last Saved: ...");
                                            }
                                      });
                                  lastSavedListener2.setFilter(std::move(reqLast));
                            } else {
                                  Notification::create("Local Level Save failed: " +
                                                           std::to_string(resp->code()),
                                                       NotificationIcon::Error)
                                      ->show();
                                  this->hideLoading();
                                  this->hideLoading();
                                  this->enableButton(sender);
                            }
                      } else if (e->isCancelled()) {
                            Notification::create("Local Level Save request was cancelled",
                                                 NotificationIcon::Error)
                                ->show();
                            this->hideLoading();
                            this->enableButton(sender);
                      }
                });
                levelListener.setFilter(std::move(reqLevel));
          });
      // check if this is the first time setup
      if (!Mod::get()->getSavedValue<bool>("hasRead2")) {
            BackupPopup::showNotice();
            Mod::get()->setSavedValue<bool>("hasRead2", true);
      }
}

void BackupPopup::onLoad(CCObject* sender) {
      geode::createQuickPopup(
          "Load Data",
          "Do you want to <cg>download</c> your account data from the backup "
          "server?\n<cy>This will merge your current account data.</c>",
          "Cancel", "Load", [this, sender](FLAlertLayer*, bool confirmed) {
                if (!confirmed)
                      return;

                this->disableButton(sender);
                this->showLoading("Downloading account data...");
                std::string token =
                    Mod::get()->getSavedValue<std::string>("argonToken");
                int accountId = 0;
                if (auto acct = GJAccountManager::get()) {
                      accountId = acct->m_accountID;
                }
                matjson::Value body = matjson::makeObject(
                    {{"accountId", accountId}, {"argonToken", token}});
                std::string backupUrl =
                    Mod::get()->getSettingValue<std::string>("backup-url");
                auto req = geode::utils::web::WebRequest()
                               .timeout(std::chrono::seconds(30))
                               .bodyJSON(body)
                               .post(backupUrl + "/load");
                static geode::EventListener<geode::utils::web::WebTask> listener;
                listener.bind([this, sender](geode::utils::web::WebTask::Event* e) {
                      if (auto* resp = e->getValue()) {
                            if (resp->ok()) {
                                  if (auto gm = GameManager::sharedState()) {
                                        auto result = resp->string();
                                        if (result) {
                                              gd::string saveStr = result.unwrap();
                                              gm->loadFromCompressedString(saveStr);
                                              Notification::create("Backup loaded successfully!",
                                                                   NotificationIcon::Success)
                                                  ->show();
                                              this->hideLoading();
                                              this->enableButton(sender);
                                        } else {
                                              Notification::create(
                                                  "Failed to get backup data from response",
                                                  NotificationIcon::Error)
                                                  ->show();
                                              this->enableButton(sender);
                                        }
                                  }
                            } else {
                                  Notification::create("Load failed: " +
                                                           std::to_string(resp->code()),
                                                       NotificationIcon::Error)
                                      ->show();
                                  this->hideLoading();
                                  this->enableButton(sender);
                            }
                      } else if (e->isCancelled()) {
                            Notification::create("Load request was cancelled",
                                                 NotificationIcon::Error)
                                ->show();
                            this->hideLoading();
                            this->enableButton(sender);
                      }
                });
                listener.setFilter(std::move(req));
          });
}

void BackupPopup::onDelete(CCObject* sender) {
      geode::createQuickPopup(
          "Delete Data",
          "Do you want to <cr>permanently delete</c> your account data and local "
          "levels from the backup server?\n<cy>This action cannot be undone.</c>",
          "Cancel", "Delete", [this, sender](FLAlertLayer*, bool confirmed) {
                if (!confirmed)
                      return;

                this->disableButton(sender);
                this->showLoading("Deleting backup...");
                std::string token =
                    Mod::get()->getSavedValue<std::string>("argonToken");
                int accountId = 0;
                if (auto acct = GJAccountManager::get()) {
                      accountId = acct->m_accountID;
                }
                matjson::Value body = matjson::makeObject(
                    {{"accountId", accountId}, {"argonToken", token}});
                std::string backupUrl =
                    Mod::get()->getSettingValue<std::string>("backup-url");
                auto req = geode::utils::web::WebRequest()
                               .timeout(std::chrono::seconds(30))
                               .bodyJSON(body)
                               .post(backupUrl + "/delete");
                static geode::EventListener<geode::utils::web::WebTask> listener;
                listener.bind([this, sender](geode::utils::web::WebTask::Event* e) {
                      if (auto* resp = e->getValue()) {
                            if (resp->ok()) {
                                  Notification::create("Backup deleted successfully!",
                                                       NotificationIcon::Success)
                                      ->show();
                                  this->hideLoading();
                                  this->enableButton(sender);
                                  // Update size and last saved labels
                                  if constexpr (std::is_member_object_pointer_v<
                                                    decltype(&BackupPopup::sizeLabel)>) {
                                        if (sizeLabel && lastSavedLabel) {
                                              int accountId = 0;
                                              if (auto acct = GJAccountManager::get()) {
                                                    accountId = acct->m_accountID;
                                              }
                                              std::string token =
                                                  Mod::get()->getSavedValue<std::string>("argonToken");
                                              matjson::Value body = matjson::makeObject(
                                                  {{"accountId", accountId}, {"argonToken", token}});
                                              std::string backupUrl =
                                                  Mod::get()->getSettingValue<std::string>("backup-url");
                                              // Update size
                                              auto reqSize = geode::utils::web::WebRequest()
                                                                 .timeout(std::chrono::seconds(30))
                                                                 .bodyJSON(body)
                                                                 .post(backupUrl + "/check");
                                              static geode::EventListener<geode::utils::web::WebTask>
                                                  sizeListener;
                                              sizeListener.bind([this](
                                                                    geode::utils::web::WebTask::Event* e) {
                                                    if (auto* resp = e->getValue()) {
                                                          if (resp->ok()) {
                                                                auto strResult = resp->string();
                                                                if (strResult) {
                                                                      const std::string& str = strResult.unwrap();
                                                                      auto parsed = matjson::Value::parse(str);
                                                                      if (parsed) {
                                                                            auto obj = parsed.unwrap();
                                                                            long long saveBytes = 0;
                                                                            long long levelBytes = 0;
                                                                            if (auto s = obj["saveData"].asInt())
                                                                                  saveBytes = s.unwrap();
                                                                            if (auto l = obj["levelData"].asInt())
                                                                                  levelBytes = l.unwrap();
                                                                            setCombinedSize(saveBytes, levelBytes);
                                                                      } else {
                                                                            setCombinedSizeNA();
                                                                      }
                                                                } else {
                                                                      setCombinedSizeNA();
                                                                }
                                                          } else {
                                                                setCombinedSizeNA();
                                                          }
                                                    } else {
                                                          setCombinedSizeLoading();
                                                    }
                                              });
                                              sizeListener.setFilter(std::move(reqSize));
                                              // Update last saved
                                              auto reqLast = geode::utils::web::WebRequest()
                                                                 .timeout(std::chrono::seconds(30))
                                                                 .bodyJSON(body)
                                                                 .post(backupUrl + "/lastsaved");
                                              static geode::EventListener<geode::utils::web::WebTask>
                                                  lastSavedListener;
                                              lastSavedListener.bind(
                                                  [this](geode::utils::web::WebTask::Event* e) {
                                                        if (auto* resp = e->getValue()) {
                                                              if (resp->ok()) {
                                                                    auto strResult = resp->string();
                                                                    if (strResult) {
                                                                          lastSavedLabel->setString(
                                                                              formatLastSavedLabel(strResult.unwrap()).c_str());
                                                                    } else {
                                                                          lastSavedLabel->setString("Last Saved: N/A");
                                                                    }
                                                              } else {
                                                                    lastSavedLabel->setString("Last Saved: N/A");
                                                              }
                                                        } else {
                                                              lastSavedLabel->setString("Last Saved: ...");
                                                        }
                                                  });
                                              lastSavedListener.setFilter(std::move(reqLast));
                                        }
                                  }
                            } else {
                                  Notification::create("Delete failed: " +
                                                           std::to_string(resp->code()),
                                                       NotificationIcon::Error)
                                      ->show();
                                  this->hideLoading();
                                  this->enableButton(sender);
                            }
                      } else if (e->isCancelled()) {
                            Notification::create("Delete request was cancelled",
                                                 NotificationIcon::Error)
                                ->show();
                            this->hideLoading();
                            this->enableButton(sender);
                      }
                });
                listener.setFilter(std::move(req));
          });
}

void BackupPopup::onLoadLocalLevels(CCObject* sender) {
      geode::createQuickPopup(
          "Load Local Levels",
          "Do you want to <cg>download</c> your local levels from the backup "
          "server?\n<cy>This will merge your current local levels.</c>",
          "Cancel", "Load", [this, sender](FLAlertLayer*, bool confirmed) {
                if (!confirmed)
                      return;

                this->disableButton(sender);
                this->showLoading("Loading local levels...");
                std::string token =
                    Mod::get()->getSavedValue<std::string>("argonToken");
                int accountId = 0;
                if (auto acct = GJAccountManager::get()) {
                      accountId = acct->m_accountID;
                }
                matjson::Value body = matjson::makeObject(
                    {{"accountId", accountId}, {"argonToken", token}});
                std::string backupUrl =
                    Mod::get()->getSettingValue<std::string>("backup-url");
                auto req = geode::utils::web::WebRequest()
                               .timeout(std::chrono::seconds(30))
                               .bodyJSON(body)
                               .post(backupUrl + "/loadlevel");
                static geode::EventListener<geode::utils::web::WebTask> listener;
                listener.bind([this, sender](geode::utils::web::WebTask::Event* e) {
                      if (auto* resp = e->getValue()) {
                            if (resp->ok()) {
                                  auto result = resp->string();
                                  if (result) {
                                        // load into LocalLevelManager
                                        if (auto llm = LocalLevelManager::get()) {
                                              gd::string levelsStr = result.unwrap();
                                              llm->loadFromCompressedString(levelsStr);
                                              Notification::create("Local Levels loaded successfully!",
                                                                   NotificationIcon::Success)
                                                  ->show();
                                              this->hideLoading();
                                              this->enableButton(sender);
                                        } else {
                                              Notification::create("LocalLevelManager not available",
                                                                   NotificationIcon::Error)
                                                  ->show();
                                              this->hideLoading();
                                              this->enableButton(sender);
                                        }
                                  } else {
                                        Notification::create(
                                            "Failed to get local levels data from response",
                                            NotificationIcon::Error)
                                            ->show();
                                        this->hideLoading();
                                        this->enableButton(sender);
                                  }
                            } else {
                                  Notification::create("Load local levels failed: " +
                                                           std::to_string(resp->code()),
                                                       NotificationIcon::Error)
                                      ->show();
                                  this->hideLoading();
                                  this->enableButton(sender);
                            }
                      } else if (e->isCancelled()) {
                            Notification::create(
                                "Load local levels request was cancelled or timed out",
                                NotificationIcon::Error)
                                ->show();
                            this->hideLoading();
                            this->enableButton(sender);
                      }
                });
                listener.setFilter(std::move(req));
          });
}

void BackupPopup::disableButton(CCObject* sender) {
      // re-enable the save
      if (auto button = static_cast<CCMenuItemSpriteExtra*>(sender)) {
            button->setEnabled(false);
            // set the buttonsprite back to full opacity, including its children
            if (auto btnSprite =
                    typeinfo_cast<ButtonSprite*>(button->getNormalImage())) {
                  btnSprite->setOpacity(128);
                  // also un-fade all children so labels / icons are normal
                  auto children = btnSprite->getChildren();
                  for (unsigned int i = 0; i < children->count(); ++i) {
                        if (auto child = typeinfo_cast<CCNode*>(children->objectAtIndex(i))) {
                              if (auto rgbaChild = typeinfo_cast<CCNodeRGBA*>(child)) {
                                    rgbaChild->setOpacity(128);
                              }
                        }
                  }
            }
      }
}

void BackupPopup::enableButton(CCObject* sender) {
      // re-enable the save
      if (auto button = static_cast<CCMenuItemSpriteExtra*>(sender)) {
            button->setEnabled(true);
            // set the buttonsprite back to full opacity, including its children
            if (auto btnSprite =
                    typeinfo_cast<ButtonSprite*>(button->getNormalImage())) {
                  btnSprite->setOpacity(255);
                  // also un-fade all children so labels / icons are normal
                  auto children = btnSprite->getChildren();
                  for (unsigned int i = 0; i < children->count(); ++i) {
                        if (auto child = typeinfo_cast<CCNode*>(children->objectAtIndex(i))) {
                              if (auto rgbaChild = typeinfo_cast<CCNodeRGBA*>(child)) {
                                    rgbaChild->setOpacity(255);
                              }
                        }
                  }
            }
      }
}

void BackupPopup::onShowNotice(CCObject* sender) {
      BackupPopup::showNotice();  // for the main.cpp ig
}

void BackupPopup::showNotice() {
      geode::MDPopup::create(
          "PLEASE READ",
          "### By clicking <cg>OK</cg>, you agree to the following terms:\n\n"
          "<cy>1.</c> Your account data will be stored on a <ca>third-party server</c> "
          "from <cg>ArcticWoof Services</c>.\n\n"
          "<cy>2.</c> The server is <cc>not affiliated</c> with the <co>RobTop's official "
          "servers</c>.\n\n"
          "<cy>3.</c> You can delete your account data from the backup server by "
          "clicking the <cr>DELETE</c> button at any <cg>time</c>.\n\n"
          "<cy>4.</c> Your data will be used solely for backup purposes and will "
          "not be shared with anywhere else.\n\n"
          "<cy>5.</c> You accept that there's a possibility that this may break "
          "your account if used incorrectly. Ensure to contact the mod developer "
          "if this happened.\n\n"
          "<cy>6.</c> Your backup will be <cr>automatically deleted after 60 days (2 months)</c> from your last successful save. "
          "Be sure to <cg>save your data regularly</c>.\n\n"
          "### The following data will be sent to the backup server:\n\n"
          "<cl>- Your Geometry Dash Account ID</c>\n\n"
          "<cl>- Your Argon Token</c>\n\n"
          "<cl>- Your Account Save Data (Compressed)</c>\n\n"
          "## <cr>Use this backup as an alternative option, not as the main "
          "backup solution for your account! Please use the <cg>main backup</cg> "
          "in the game.</c>\n\n"
          "<cr>If you wish to opt out of this service, simply close the popup "
          "and uninstall the mod. You can delete your data from the server at "
          "any time by clicking the <cr>DELETE</c> button at any <cg>time</c>.</c>\n\n"
          "If you need support, join <cf>ArcticWoof's Discord Server</c> located at the Account Backup mod page on Geode.\n\n"
          "<cc>You can read this notice by clicking the info button at the Account Backup Popup! You can read the terms at "
          "<cg>https://arcticwoof.xyz/privacy</cg></c>",
          "OK")
          ->show();
}

void BackupPopup::setCombinedSize(long long saveBytes, long long levelBytes) {
      if (!sizeLabel) return;
      double saveMB = saveBytes / (1024.0 * 1024.0);
      double levelMB = levelBytes / (1024.0 * 1024.0);
      sizeLabel->setString(fmt::format("Account and Local Level Data Size: {:.2f} MB / {:.2f} MB", saveMB, levelMB).c_str());
}

void BackupPopup::setCombinedSizeNA() {
      if (!sizeLabel) return;
      sizeLabel->setString("Account and Local Level Data Size: N/A");
}

void BackupPopup::setCombinedSizeLoading() {
      if (!sizeLabel) return;
      sizeLabel->setString("Account and Local Level Data Size: ...");
}

void BackupPopup::onModSettings(CCObject*) { openSettingsPopup(getMod()); }

void BackupPopup::showLoading(const std::string& msg) {
      if (!m_loadingPopup) {
            m_loadingPopup = LoadingPopup::create(msg);
            this->addChild(m_loadingPopup);
      } else {
            m_loadingPopup->setMessage(msg);
      }
}

void BackupPopup::hideLoading() {
      if (m_loadingPopup) {
            m_loadingPopup->hide();
            m_loadingPopup = nullptr;
      }
}

// i just looked up how to parse time in c++ on stackoverflow lol
std::string BackupPopup::formatLastSavedLabel(const std::string& lastSavedRaw) {
      if (lastSavedRaw.empty()) {
            return std::string("Last Saved: N/A");
      }
      // Trim whitespace
      auto s = lastSavedRaw;
      while (!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin());
      while (!s.empty() && std::isspace((unsigned char)s.back())) s.pop_back();

      time_t epoch = 0;
      bool parsed = false;
      // Numeric epoch seconds
      bool allDigits = !s.empty() && std::all_of(s.begin(), s.end(), [](char c) { return std::isdigit((unsigned char)c); });
      if (allDigits) {
            epoch = static_cast<time_t>(std::stoll(s));
            parsed = true;
      } else {
            parsed = false;
      }

      if (!parsed) {
            // Try ISO 8601 like YYYY-MM-DD or YYYY-MM-DDTHH:MM:SS or with space
            std::regex r(R"(^\s*(\d{4})-(\d{2})-(\d{2})(?:[T\s](\d{2}):(\d{2}):(\d{2}))?(?:Z)?\s*$)");
            std::smatch m;
            if (std::regex_search(s, m, r)) {
                  int Y = std::stoi(m[1].str());
                  int Mo = std::stoi(m[2].str());
                  int D = std::stoi(m[3].str());
                  int hh = 0, mm = 0, ss = 0;
                  if (m[4].matched) hh = std::stoi(m[4].str());
                  if (m[5].matched) mm = std::stoi(m[5].str());
                  if (m[6].matched) ss = std::stoi(m[6].str());
                  std::tm tm{};
                  tm.tm_year = Y - 1900;
                  tm.tm_mon = Mo - 1;
                  tm.tm_mday = D;
                  tm.tm_hour = hh;
                  tm.tm_min = mm;
                  tm.tm_sec = ss;
                  tm.tm_isdst = -1;
#if defined(_MSC_VER)
                  epoch = _mkgmtime(&tm);
#elif defined(__unix__)
                  epoch = timegm(&tm);
#else
                  epoch = mktime(&tm);
#endif
                  parsed = true;
            }
      }
      if (!parsed) {
            return fmt::format("Last Saved: {}", lastSavedRaw);
      }
      time_t now = std::time(nullptr);
      double diff = std::difftime(now, epoch);
      long days = static_cast<long>(std::llround(diff / 86400.0));
      std::string ago;
      if (diff >= 0) {
            if (days == 0) {
                  ago = "today";
            } else if (days == 1) {
                  ago = "1 day ago";
            } else {
                  ago = fmt::format("{} days ago", days);
            }
            return fmt::format("Last Saved: {} ({})", lastSavedRaw, ago);
      } else {
            return fmt::format("Last Saved: {} (today)", lastSavedRaw);  // likely future days is just today
      }
}