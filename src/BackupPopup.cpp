#include "BackupPopup.hpp"

#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/utils/general.hpp>
#include <Geode/utils/web.hpp>
#include <argon/argon.hpp>
#include <ctime>
#include <matjson.hpp>
#include <sstream>
#include <string>

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

      // try to get username
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
      sizeLabel = CCLabelBMFont::create("Account Size: ... MB / Local Levels Size: ... MB", "chatFont.fnt");
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

      // free space / total size
      freeSpaceLabel = CCLabelBMFont::create("Free: ...", "bigFont.fnt");
      freeSpaceLabel->setAlignment(kCCTextAlignmentLeft);
      freeSpaceLabel->setPosition({m_mainLayer->getContentSize().width / 2, 15.f});
      freeSpaceLabel->setScale(0.3f);
      m_mainLayer->addChild(freeSpaceLabel, 2);
      fetchAndUpdateStatus();

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
                auto reqSave = web::WebRequest()
                                   .timeout(std::chrono::seconds(30))
                                   .bodyJSON(bodySave)
                                   .post(backupUrl + "/save");

                static geode::EventListener<web::WebTask> saveListener;
                saveListener.bind([this, sender, accountId, token,
                                   backupUrl](web::WebTask::Event* e) {
                      if (auto* resp = e->getValue()) {
                            if (resp->ok()) {
                                  Notification::create("Account data saved successfully!",
                                                       NotificationIcon::Success)
                                      ->show();
                                  this->hideLoading();
                                  this->enableButton(sender);
                                  this->fetchAndUpdateStatus();
                            } else {
                                  auto errorBody = resp->string();
                                  std::string errorMsg = errorBody ? parseResponseError(errorBody.unwrap()) : "Unknown error";
                                  Notification::create("Account Save failed: " + errorMsg,
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
                auto reqLevel = web::WebRequest()
                                    .timeout(std::chrono::seconds(30))
                                    .bodyJSON(bodyLevel)
                                    .post(backupUrl + "/save");

                static geode::EventListener<web::WebTask> levelListener;
                levelListener.bind([this, sender, accountId, token,
                                    backupUrl](web::WebTask::Event* e) {
                      if (auto* resp = e->getValue()) {
                            if (resp->ok()) {
                                  Notification::create("Local Levels saved successfully!",
                                                       NotificationIcon::Success)
                                      ->show();
                                  this->hideLoading();
                                  this->enableButton(sender);
                                  this->fetchAndUpdateStatus();
                            } else {
                                  auto errorBody = resp->string();
                                  std::string errorMsg = errorBody ? parseResponseError(errorBody.unwrap()) : "Unknown error";
                                  Notification::create("Local Level Save failed: " + errorMsg,
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
          "Load Account Data",
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
                auto req = web::WebRequest()
                               .timeout(std::chrono::seconds(30))
                               .bodyJSON(body)
                               .post(backupUrl + "/load");
                static geode::EventListener<web::WebTask> listener;
                listener.bind([this, sender](web::WebTask::Event* e) {
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
                                  auto errorBody = resp->string();
                                  std::string errorMsg = errorBody ? parseResponseError(errorBody.unwrap()) : "Unknown error";
                                  Notification::create("Load failed: " + errorMsg,
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
                auto req = web::WebRequest()
                               .timeout(std::chrono::seconds(30))
                               .bodyJSON(body)
                               .post(backupUrl + "/delete");
                static geode::EventListener<web::WebTask> listener;
                listener.bind([this, sender](web::WebTask::Event* e) {
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
                                              // fetch and update both size and last saved once
                                              this->fetchAndUpdateStatus();
                                        }
                                  }
                            } else {
                                  auto errorBody = resp->string();
                                  std::string errorMsg = errorBody ? parseResponseError(errorBody.unwrap()) : "Unknown error";
                                  Notification::create("Delete failed: " + errorMsg,
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
                auto req = web::WebRequest()
                               .timeout(std::chrono::seconds(30))
                               .bodyJSON(body)
                               .post(backupUrl + "/loadlevel");
                static geode::EventListener<web::WebTask> listener;
                listener.bind([this, sender](web::WebTask::Event* e) {
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
                                  auto errorBody = resp->string();
                                  std::string errorMsg = errorBody ? parseResponseError(errorBody.unwrap()) : "Unknown error";
                                  Notification::create("Load local levels failed: " + errorMsg,
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
          "<cy>7.</c> If you are using <cf>ArcticWoof's Backup Server</c>, the maximum storage is <cg>**32MB**</c> for both <ca>account data and local levels combined</c>.\n\n"
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
      sizeLabel->setString(fmt::format("Account Size: {:.2f} MB / Local Levels Size: {:.2f} MB", saveMB, levelMB).c_str());
}

void BackupPopup::setCombinedSizeNA() {
      if (!sizeLabel) return;
      sizeLabel->setString("Account Size: N/A / Local Levels Size: N/A");
}

void BackupPopup::setCombinedSizeLoading() {
      if (!sizeLabel) return;
      sizeLabel->setString("Account Size: ... MB / Local Levels Size: ... MB");
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

void BackupPopup::setLastSavedFromCheckResponse(const std::string& jsonStr) {
      if (!lastSavedLabel) return;
      if (jsonStr.empty()) {
            lastSavedLabel->setString("Last Saved: N/A");
            return;
      }
      auto parsed = matjson::Value::parse(jsonStr);
      if (!parsed) {
            lastSavedLabel->setString("Last Saved: N/A");
            return;
      }
      auto obj = parsed.unwrap();
      if (auto ls = obj["lastSaved"].asString()) {
            auto lastSaved = ls.unwrap();
            if (auto lsr = obj["lastSavedRelative"].asString()) {
                  lastSavedLabel->setString(fmt::format("Last Saved: {}", lsr.unwrap()).c_str());
            } else {
                  auto tsRes = numFromString<long long>(lastSaved);
                  if (!tsRes) {
                        lastSavedLabel->setString("Last Saved: N/A");
                  } else {
                        long long tsVal = tsRes.unwrap();
                        auto friendly = GameToolbox::timestampToHumanReadable(static_cast<time_t>(tsVal));
                        lastSavedLabel->setString(fmt::format("Last Saved: {}", friendly).c_str());
                  }
            }
            return;
      }
      if (auto rel = obj["lastSavedRelative"].asString()) {
            lastSavedLabel->setString(fmt::format("Last Saved: {}", rel.unwrap()).c_str());
            return;
      }
      lastSavedLabel->setString("Last Saved: N/A");
}

void BackupPopup::fetchAndUpdateStatus() {
      int accountId = 0;
      if (auto acct = GJAccountManager::get()) {
            accountId = acct->m_accountID;
      }
      std::string token = Mod::get()->getSavedValue<std::string>("argonToken");
      matjson::Value body = matjson::makeObject({{"accountId", accountId}, {"argonToken", token}});
      std::string backupUrl = Mod::get()->getSettingValue<std::string>("backup-url");
      auto req = web::WebRequest()
                     .timeout(std::chrono::seconds(30))
                     .header("Content-Type", "application/json")
                     .bodyJSON(body)
                     .post(backupUrl + "/check");
      static geode::EventListener<web::WebTask> statusListener;
      statusListener.bind([this](web::WebTask::Event* e) {
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
                                    int freePercentage = 0;
                                    long long totalSize = 0;
                                    long long maxDataSize = 0;
                                    if (auto s = obj["saveData"].asInt()) saveBytes = s.unwrap();
                                    if (auto l = obj["levelData"].asInt()) levelBytes = l.unwrap();
                                    if (auto fsp = obj["freeSpacePercentage"].asInt()) freePercentage = fsp.unwrap();
                                    if (auto ts = obj["totalSize"].asInt()) totalSize = ts.unwrap();
                                    if (auto mds = obj["maxDataSize"].asInt()) maxDataSize = mds.unwrap();
                                    setCombinedSize(saveBytes, levelBytes);
                                    setFreeSpaceAndTotal(freePercentage, totalSize, maxDataSize);
                                    setLastSavedFromCheckResponse(str);
                              } else {
                                    setCombinedSizeNA();
                                    setFreeSpaceNA();
                                    if (lastSavedLabel) lastSavedLabel->setString("Last Saved: N/A");
                              }
                        } else {
                              setCombinedSizeNA();
                              setFreeSpaceNA();
                              if (lastSavedLabel) lastSavedLabel->setString("Last Saved: N/A");
                        }
                  } else {
                        setCombinedSizeNA();
                        setFreeSpaceNA();
                        if (lastSavedLabel) lastSavedLabel->setString("Last Saved: N/A");
                  }
            } else {
                  setCombinedSizeLoading();
                  freeSpaceLabel->setString("Free: ...");
                  if (lastSavedLabel) lastSavedLabel->setString("Last Saved: ...");
            }
      });
      statusListener.setFilter(std::move(req));
}

void BackupPopup::setFreeSpaceAndTotal(int freePercentage, long long totalSize, long long maxDataSize) {
      if (!freeSpaceLabel) return;
      double totalMB = totalSize / (1024.0 * 1024.0);
      double maxMB = maxDataSize / (1024.0 * 1024.0);
      freeSpaceLabel->setString(fmt::format("Free: {}% - Total: {:.2f} MB / Max: {:.2f} MB", freePercentage, totalMB, maxMB).c_str());
}

void BackupPopup::setFreeSpaceNA() {
      if (!freeSpaceLabel) return;
      freeSpaceLabel->setString("Free: N/A");
}

std::string BackupPopup::parseResponseError(const std::string& responseBody) {
      if (responseBody.empty()) {
            return "Unknown error";
      }

      auto parsed = matjson::Value::parse(responseBody);
      if (!parsed) {
            return "Unknown error";
      }

      auto obj = parsed.unwrap();
      if (auto err = obj["error"].asString()) {
            return err.unwrap();
      }

      return "Unknown error";
}