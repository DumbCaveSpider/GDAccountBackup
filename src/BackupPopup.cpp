#include "BackupPopup.hpp"

#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/general.hpp>
#include <Geode/utils/web.hpp>
#include <argon/argon.hpp>
#include <ctime>
#include <matjson.hpp>
#include <string>

#include "helper.hpp"
#include "Geode/ui/BasedButtonSprite.hpp"
#include "Geode/ui/NineSlice.hpp"
#include "MembershipPopup.hpp"

using namespace geode::prelude;

BackupPopup *BackupPopup::create() {
  auto ret = new BackupPopup();
  if (ret && ret->init()) {
    ret->autorelease();
    return ret;
  }
  delete ret;
  return nullptr;
}

static std::string getResponseFailMessage(web::WebResponse const &response,
                                          std::string const &fallback) {
  auto message = response.string().unwrapOrDefault();
  if (!message.empty())
    return message;
  return fallback;
}

bool BackupPopup::init() {
  if (!Popup::init(380.f, 275.f, "GJ_square02.png"))
    return false;
  auto accountData = argon::getGameAccountData();
  std::string labelText = "Backing up: " + std::string(accountData.username);
  setTitle(labelText);
  m_title->setFntFile("bigFont.fnt");
  m_title->setPositionY(m_title->getPositionY() + 5.f);

  // combined account and local levels size label
  sizeLabel = CCLabelBMFont::create(
      "Account Size: ... MB / Local Levels Size: ... MB", "chatFont.fnt");
  sizeLabel->setAlignment(kCCTextAlignmentCenter);
  sizeLabel->setPosition({m_mainLayer->getContentSize().width / 2,
                          m_mainLayer->getContentSize().height - 55});
  sizeLabel->setScale(0.5f);
  m_mainLayer->addChild(sizeLabel, 2);

  // last saved label
  lastSavedLabel = CCLabelBMFont::create("Last Saved: ...", "chatFont.fnt");
  lastSavedLabel->setAlignment(kCCTextAlignmentCenter);
  lastSavedLabel->setPosition({m_mainLayer->getContentSize().width / 2,
                               m_mainLayer->getContentSize().height - 65});
  lastSavedLabel->setScale(0.5f);
  m_mainLayer->addChild(lastSavedLabel, 2);

  // free space / total size
  freeSpaceLabel = CCLabelBMFont::create("Free: ...", "bigFont.fnt");
  freeSpaceLabel->setAlignment(kCCTextAlignmentLeft);
  freeSpaceLabel->setPosition({m_mainLayer->getContentSize().width / 2, 12.f});
  freeSpaceLabel->setScale(0.3f);
  m_mainLayer->addChild(freeSpaceLabel, 2);

  subscriberLabel = CCLabelBMFont::create("-", "goldFont.fnt");
  subscriberLabel->setAlignment(kCCTextAlignmentCenter);
  subscriberLabel->setPosition(
      {m_title->getPositionX(), m_title->getPositionY() - 25.f});
  subscriberLabel->setScale(0.5f);
  m_mainLayer->addChild(subscriberLabel, 2);

  // background
  auto bg = NineSlice::create("square02_small.png");
  bg->setPosition({m_mainLayer->getContentSize().width / 2,
                   m_mainLayer->getContentSize().height - 53});
  bg->setContentSize({250, 45});
  bg->setOpacity(100);
  m_mainLayer->addChild(bg, 0);

  fetchAndUpdateStatus();

  float verticalSpacing = 36.f;
  const int numButtons = 4;

  auto saveAccountBtn =
      ButtonSprite::create("Save Account Data", 250.f, true, "goldFont.fnt",
                           "GJ_button_01.png", 0.f, 1.f);
  m_saveAccountItem = CCMenuItemSpriteExtra::create(
      saveAccountBtn, this, menu_selector(BackupPopup::onSave));

  auto saveLevelsBtn =
      ButtonSprite::create("Save Local Levels", 250.f, true, "goldFont.fnt",
                           "GJ_button_01.png", 0.f, 1.f);
  m_saveLevelsItem = CCMenuItemSpriteExtra::create(
      saveLevelsBtn, this, menu_selector(BackupPopup::onSaveLocalLevels));

  auto loadBtn =
      ButtonSprite::create("Load Account Data", 250.f, true, "goldFont.fnt",
                           "GJ_button_03.png", 0.f, 1.f);
  m_loadItem = CCMenuItemSpriteExtra::create(
      loadBtn, this, menu_selector(BackupPopup::onLoad));

  auto loadLevelsBtn =
      ButtonSprite::create("Load Local Levels", 250.f, true, "goldFont.fnt",
                           "GJ_button_03.png", 0.f, 1.f);
  m_loadLevelsItem = CCMenuItemSpriteExtra::create(
      loadLevelsBtn, this, menu_selector(BackupPopup::onLoadLocalLevels));

  auto deleteBtn =
      ButtonSprite::create("Delete Backups", 250.f, true, "goldFont.fnt",
                           "GJ_button_06.png", 0.f, 1.f);
  m_deleteItem = CCMenuItemSpriteExtra::create(
      deleteBtn, this, menu_selector(BackupPopup::onDelete));
  float startY = verticalSpacing * ((numButtons - 1)) + 70;

  // membership button popup only for main backup server
  if (Mod::get()->getSettingValue<std::string>("backup-url") ==
      "https://gdbackup.arcticwoof.xyz") {
    auto membershipSpr = CCSprite::create("subIcon.png"_spr);
    auto membershipBtnSpr = EditorButtonSprite::create(
        membershipSpr, EditorBaseColor::Magenta, EditorBaseSize::Normal);
    auto membershipBtn = CCMenuItemSpriteExtra::create(
        membershipBtnSpr, this, menu_selector(BackupPopup::onMembershipPopup));
    membershipBtn->setPosition({30.f, 30.f});
    m_buttonMenu->addChild(membershipBtn);
  }

  m_saveAccountItem->setPosition(
      {m_mainLayer->getContentSize().width / 2, startY});
  m_saveLevelsItem->setPosition({m_mainLayer->getContentSize().width / 2,
                                 startY - 1.0f * verticalSpacing});
  m_loadItem->setPosition({m_mainLayer->getContentSize().width / 2,
                           startY - 2.0f * verticalSpacing});
  m_loadLevelsItem->setPosition({m_mainLayer->getContentSize().width / 2,
                                 startY - 3.0f * verticalSpacing});
  m_deleteItem->setPosition({m_mainLayer->getContentSize().width / 2,
                             startY - 4.0f * verticalSpacing});
  m_buttonMenu->addChild(m_saveAccountItem);
  m_buttonMenu->addChild(m_saveLevelsItem);
  m_buttonMenu->addChild(m_loadItem);
  m_buttonMenu->addChild(m_loadLevelsItem);
  m_buttonMenu->addChild(m_deleteItem);

  auto modSettingsBtnSprite = CircleButtonSprite::createWithSpriteFrameName(
      // @geode-ignore(unknown-resource)
      "geode.loader/settings.png", 1.f, CircleBaseColor::Green,
      CircleBaseSize::Medium);
  modSettingsBtnSprite->setScale(0.75f);

  auto modSettingsButton = CCMenuItemSpriteExtra::create(
      modSettingsBtnSprite, this, menu_selector(BackupPopup::onModSettings));
  modSettingsButton->setPosition({m_mainLayer->getContentSize().width,
                                  m_mainLayer->getContentSize().height});
  m_buttonMenu->addChild(modSettingsButton);
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
  auto showNoticeSpr =
      CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
  auto showNoticeBtn = CCMenuItemSpriteExtra::create(
      showNoticeSpr, this, menu_selector(BackupPopup::onShowNotice));
  showNoticeBtn->setPosition(
      {m_mainLayer->getContentSize().width - 25.f, 25.f});
  m_buttonMenu->addChild(showNoticeBtn);
  return true;
}

void BackupPopup::onMembershipPopup(CCObject *sender) {
  auto popup = MembershipPopup::create();
  if (popup)
    popup->show();
}

void BackupPopup::onSave(CCObject *sender) {
  geode::createQuickPopup(
      "Save Account Data",
      "Do you want to <cg>save</c> your account data to the "
      "backup server?\n<cy>This will overwrite your existing backup saved in "
      "the server.</c>",
      "Cancel", "Save", [this, sender](FLAlertLayer *, bool confirmed) {
        if (!confirmed)
          return;
        auto upopup = UploadActionPopup::create(this, "Saving account data...");
        upopup->show();

        std::string token =
            Mod::get()->getSavedValue<std::string>("argonToken");
        auto accountData = argon::getGameAccountData();
        std::string saveData;
        if (auto gm = GameManager::sharedState()) {
          saveData = gm->getCompressedSaveString();
        }

        std::string backupUrl =
            Mod::get()->getSettingValue<std::string>("backup-url");

        matjson::Value bodySave =
            matjson::makeObject({{"accountId", accountData.accountId},
                                 {"saveData", saveData},
                                 {"argonToken", token}});
        auto reqSave = createBackupRequest()
                           .timeout(std::chrono::seconds(30))
                           .bodyJSON(bodySave)
                           .post(backupUrl + "/save");

        m_listener.spawn(
            std::move(reqSave), [this, sender, upopup, accountData, token,
                                 backupUrl](web::WebResponse resp) {
              if (resp.ok()) {
                upopup->showSuccessMessage("Account data saved successfully!");
                this->fetchAndUpdateStatus();
              } else {
                upopup->showFailMessage(
                    getResponseFailMessage(resp, "Unknown error"));
              }
            });
      });
  if (!Mod::get()->getSavedValue<bool>("hasRead2")) {
    BackupPopup::showNotice();
    Mod::get()->setSavedValue<bool>("hasRead2", true);
  }
}

void BackupPopup::onSaveLocalLevels(CCObject *sender) {
  geode::createQuickPopup(
      "Save Local Levels",
      "Do you want to <cg>save</c> your local levels to the "
      "backup server?\n<cy>This will overwrite your existing backup saved in "
      "the server.</c>",
      "Cancel", "Save", [this, sender](FLAlertLayer *, bool confirmed) {
        if (!confirmed)
          return;
        auto upopup = UploadActionPopup::create(this, "Saving local levels...");
        upopup->show();

        std::string token =
            Mod::get()->getSavedValue<std::string>("argonToken");
        auto accountData = argon::getGameAccountData();
        std::string levelData;
        if (auto gm = GameManager::sharedState()) {
          levelData = LocalLevelManager::get()->getCompressedSaveString();
        }

        std::string backupUrl =
            Mod::get()->getSettingValue<std::string>("backup-url");

        matjson::Value bodyLevel =
            matjson::makeObject({{"accountId", accountData.accountId},
                                 {"levelData", levelData},
                                 {"argonToken", token}});
        auto reqLevel = createBackupRequest()
                            .timeout(std::chrono::seconds(30))
                            .bodyJSON(bodyLevel)
                            .post(backupUrl + "/save");

        m_listener.spawn(
            std::move(reqLevel), [this, sender, upopup, accountData, token,
                                  backupUrl](web::WebResponse resp) {
              if (resp.ok()) {
                upopup->showSuccessMessage("Local Levels saved successfully!");
                this->fetchAndUpdateStatus();
              } else {
                upopup->showFailMessage(
                    getResponseFailMessage(resp, "Unknown error"));
              }
            });
      });
  // check if this is the first time setup
  if (!Mod::get()->getSavedValue<bool>("hasRead2")) {
    BackupPopup::showNotice();
    Mod::get()->setSavedValue<bool>("hasRead2", true);
  }
}

void BackupPopup::onLoad(CCObject *sender) {
  geode::createQuickPopup(
      "Load Account Data",
      "Do you want to <cg>download</c> your account data from the backup "
      "server?\n<cy>This will merge your current account data.</c>",
      "Cancel", "Load", [this, sender](FLAlertLayer *, bool confirmed) {
        if (!confirmed)
          return;

        auto upopup =
            UploadActionPopup::create(this, "Downloading account data...");
        upopup->show();
        std::string token =
            Mod::get()->getSavedValue<std::string>("argonToken");
        auto accountData = argon::getGameAccountData();
        matjson::Value body = matjson::makeObject(
            {{"accountId", accountData.accountId}, {"argonToken", token}});
        std::string backupUrl =
            Mod::get()->getSettingValue<std::string>("backup-url");
        auto req = createBackupRequest()
                       .timeout(std::chrono::seconds(30))
                       .bodyJSON(body)
                       .post(backupUrl + "/load");
        m_listener.spawn(
            std::move(req), [this, sender, upopup](web::WebResponse resp) {
              if (resp.ok()) {
                if (auto gm = GameManager::sharedState()) {
                  std::string saveData;
                  if (auto result = resp.string()) {
                    saveData = std::string(result.unwrap().c_str());
                  }

                  if (!saveData.empty()) {
                    gd::string saveDataGd = saveData;
                    gm->loadFromCompressedString(saveDataGd);
                    upopup->showSuccessMessage("Backup loaded successfully!");
                  } else {
                    upopup->showFailMessage(
                        "Failed to get backup data from response");
                  }
                }
              } else {
                upopup->showFailMessage(
                    getResponseFailMessage(resp, "Unknown error"));
              }
            });
      });
}

void BackupPopup::onDelete(CCObject *sender) {
  geode::createQuickPopup(
      "Delete Data",
      "Do you want to <cr>permanently delete</c> your account data and local "
      "levels from the backup server?\n<cy>This action cannot be undone.</c>",
      "Cancel", "Delete", [this, sender](FLAlertLayer *, bool confirmed) {
        if (!confirmed)
          return;

        auto upopup = UploadActionPopup::create(this, "Deleting backup...");
        upopup->show();
        std::string token =
            Mod::get()->getSavedValue<std::string>("argonToken");
        auto accountData = argon::getGameAccountData();
        matjson::Value body = matjson::makeObject(
            {{"accountId", accountData.accountId}, {"argonToken", token}});
        std::string backupUrl =
            Mod::get()->getSettingValue<std::string>("backup-url");
        auto req = createBackupRequest()
                       .timeout(std::chrono::seconds(30))
                       .bodyJSON(body)
                       .post(backupUrl + "/delete");
        m_listener.spawn(std::move(req), [this, sender,
                                          upopup](web::WebResponse resp) {
          if (resp.ok()) {
            upopup->showSuccessMessage("Backup deleted successfully!");
            // Update size and last saved labels
            if constexpr (std::is_member_object_pointer_v<
                              decltype(&BackupPopup::sizeLabel)>) {
              if (sizeLabel && lastSavedLabel) {
                auto accountData = argon::getGameAccountData();
                std::string token =
                    Mod::get()->getSavedValue<std::string>("argonToken");
                matjson::Value body =
                    matjson::makeObject({{"accountId", accountData.accountId},
                                         {"argonToken", token}});
                std::string backupUrl =
                    Mod::get()->getSettingValue<std::string>("backup-url");
                // fetch and update both size and last saved once
                this->fetchAndUpdateStatus();
              }
            }
          } else {
            upopup->showFailMessage(
                getResponseFailMessage(resp, "Unknown error"));
          }
        });
      });
}

void BackupPopup::onLoadLocalLevels(CCObject *sender) {
  geode::createQuickPopup(
      "Load Local Levels",
      "Do you want to <cg>download</c> your local levels from the backup "
      "server?\n<cy>This will merge your current local levels.</c>",
      "Cancel", "Load", [this, sender](FLAlertLayer *, bool confirmed) {
        if (!confirmed)
          return;

        auto upopup =
            UploadActionPopup::create(this, "Loading local levels...");
        upopup->show();
        std::string token =
            Mod::get()->getSavedValue<std::string>("argonToken");
        auto accountData = argon::getGameAccountData();
        matjson::Value body = matjson::makeObject(
            {{"accountId", accountData.accountId}, {"argonToken", token}});
        std::string backupUrl =
            Mod::get()->getSettingValue<std::string>("backup-url");
        auto req = createBackupRequest()
                       .timeout(std::chrono::seconds(30))
                       .bodyJSON(body)
                       .post(backupUrl + "/loadlevel");
        m_listener.spawn(std::move(req), [this, sender,
                                          upopup](web::WebResponse resp) {
          if (resp.ok()) {
            std::string levelData;
            if (auto result = resp.string()) {
              levelData = std::string(result.unwrap().c_str());
            }

            if (!levelData.empty()) {
              // load into LocalLevelManager
              if (auto llm = LocalLevelManager::get()) {
                gd::string levelDataGd = levelData;
                llm->loadFromCompressedString(levelDataGd);
                upopup->showSuccessMessage("Local Levels loaded successfully!");
              } else {
                upopup->showFailMessage("LocalLevelManager not available");
              }
            } else {
              upopup->showFailMessage(
                  "Failed to get local levels data from response");
            }
          } else {
            upopup->showFailMessage(
                getResponseFailMessage(resp, "Unknown error"));
          }
        });
      });
}

void BackupPopup::onShowNotice(CCObject *sender) {
  BackupPopup::showNotice(); // for the main.cpp ig
}

void BackupPopup::onClosePopup(UploadActionPopup *popup) {
  popup->closePopup();
  m_listener.cancel();
  log::debug("Closed popup, cancelled any ongoing requests");
}

void BackupPopup::showNotice() {
  geode::MDPopup::create(
      "PLEASE READ",
      "### By clicking <cg>OK</cg>, you agree to the following terms:\n\n"
      "<cy>1.</c> Your account data will be stored on a <ca>third-party "
      "server</c> "
      "from <cg>ArcticWoof Services</c>.\n\n"
      "<cy>2.</c> The server is <cc>not affiliated</c> with the <co>RobTop's "
      "official "
      "servers</c>.\n\n"
      "<cy>3.</c> You can delete your account data from the backup server by "
      "clicking the <cr>DELETE</c> button at any <cg>time</c>.\n\n"
      "<cy>4.</c> Your data will be used solely for backup purposes and will "
      "not be shared with anywhere else.\n\n"
      "<cy>5.</c> You accept that there's a possibility that this may break "
      "your account if used incorrectly. Ensure to contact the mod developer "
      "if this happened.\n\n"
      "<cy>6.</c> Your backup will be <cr>automatically deleted after 60 days "
      "(2 months)</c> from your last successful save. "
      "Be sure to <cg>save your data regularly</c>.\n\n"
      "<cy>7.</c> If you are using <cf>ArcticWoof's Backup Server</c>, the "
      "maximum storage is <cg>**32MB**</c> for both <ca>account data and local "
      "levels combined</c>. Unless you have the subscription which the max is "
      "<cg>512MB</c>.\n\n"
      "### The following data will be sent to the backup server:\n\n"
      "<cl>- Your Geometry Dash Account ID</c>\n\n"
      "<cl>- Your Argon Token</c>\n\n"
      "<cl>- Your Account Save Data (Compressed)</c>\n\n"
      "## <cr>Use this backup as an alternative option, not as the main "
      "backup solution for your account! Please use the <cg>main backup</cg> "
      "in the game.</c>\n\n"
      "<cr>If you wish to opt out of this service, simply close the popup "
      "and uninstall the mod. You can delete your data from the server at "
      "any time by clicking the <cr>DELETE</c> button at any "
      "<cg>time</c>.</c>\n\n"
      "If you need support, join <cf>ArcticWoof's Discord Server</c> located "
      "at the Account Backup mod page on Geode.\n\n"
      "<cc>You can read this notice by clicking the info button at the Account "
      "Backup Popup! You can read the terms at "
      "<cg>https://arcticwoof.xyz/privacy</cg></c>",
      "OK")
      ->show();
}

void BackupPopup::setCombinedSize(long long saveBytes, long long levelBytes) {
  if (!sizeLabel)
    return;
  double saveMB = saveBytes / (1024.0 * 1024.0);
  double levelMB = levelBytes / (1024.0 * 1024.0);
  sizeLabel->setString(
      fmt::format("Account Size: {:.2f} MB / Local Levels Size: {:.2f} MB",
                  saveMB, levelMB)
          .c_str());
}

void BackupPopup::setCombinedSizeNA() {
  if (!sizeLabel)
    return;
  sizeLabel->setString("Account Size: N/A / Local Levels Size: N/A");
}

void BackupPopup::setCombinedSizeLoading() {
  if (!sizeLabel)
    return;
  sizeLabel->setString("Account Size: ... MB / Local Levels Size: ... MB");
}

void BackupPopup::onModSettings(CCObject *) { openSettingsPopup(getMod()); }

void BackupPopup::setLastSavedFromCheckResponse(const std::string &jsonStr) {
  if (!lastSavedLabel)
    return;
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
      lastSavedLabel->setString(
          fmt::format("Last Saved: {}", lsr.unwrap()).c_str());
    } else {
      auto tsRes = numFromString<long long>(lastSaved);
      if (!tsRes) {
        lastSavedLabel->setString("Last Saved: N/A");
      } else {
        long long tsVal = tsRes.unwrap();
        auto friendly =
            GameToolbox::timestampToHumanReadable(static_cast<time_t>(tsVal));
        lastSavedLabel->setString(
            fmt::format("Last Saved: {}", friendly).c_str());
      }
    }
    return;
  }
  if (auto rel = obj["lastSavedRelative"].asString()) {
    lastSavedLabel->setString(
        fmt::format("Last Saved: {}", rel.unwrap()).c_str());
    return;
  }
  lastSavedLabel->setString("Last Saved: N/A");
}

void BackupPopup::fetchAndUpdateStatus() {
  auto accountData = argon::getGameAccountData();
  std::string token = Mod::get()->getSavedValue<std::string>("argonToken");
  matjson::Value body = matjson::makeObject(
      {{"accountId", accountData.accountId}, {"argonToken", token}});
  std::string backupUrl =
      Mod::get()->getSettingValue<std::string>("backup-url");
  auto req = createBackupRequest()
                 .timeout(std::chrono::seconds(30))
                 .header("Content-Type", "application/json")
                 .bodyJSON(body)
                 .post(backupUrl + "/check");
  m_listener.spawn(std::move(req), [this](web::WebResponse resp) {
    if (resp.ok()) {
      std::string str;
      if (auto strResult = resp.string()) {
        str = std::string(strResult.unwrap().c_str());
        auto parsed = matjson::Value::parse(str);
        if (parsed) {
          auto obj = parsed.unwrap();
          long long saveBytes = 0;
          long long levelBytes = 0;
          int freePercentage = 0;
          long long totalSize = 0;
          long long maxDataSize = 0;
          if (auto s = obj["saveData"].asInt())
            saveBytes = s.unwrap();
          if (auto l = obj["levelData"].asInt())
            levelBytes = l.unwrap();
          if (auto fsp = obj["freeSpacePercentage"].asInt())
            freePercentage = fsp.unwrap();
          if (auto ts = obj["totalSize"].asInt())
            totalSize = ts.unwrap();
          if (auto mds = obj["maxDataSize"].asInt())
            maxDataSize = mds.unwrap();

          // disable delete/load buttons if no backup exists
          bool hasBackup = saveBytes > 0 || levelBytes > 0;
          if (!hasBackup) {
            m_deleteItem->setEnabled(hasBackup);
            m_loadItem->setEnabled(saveBytes > 0);
            m_loadLevelsItem->setEnabled(levelBytes > 0);

            m_deleteItem->setNormalImage(ButtonSprite::create(
                "Delete Backups", 250.f, true, "goldFont.fnt",
                "GJ_button_04.png", 0.f, 1.f));
            m_loadItem->setNormalImage(ButtonSprite::create(
                "Load Account Data", 250.f, true, "goldFont.fnt",
                "GJ_button_04.png", 0.f, 1.f));
            m_loadLevelsItem->setNormalImage(ButtonSprite::create(
                "Load Local Levels", 250.f, true, "goldFont.fnt",
                "GJ_button_04.png", 0.f, 1.f));
          } else {
            m_deleteItem->setEnabled(true);
            m_loadItem->setEnabled(true);
            m_loadLevelsItem->setEnabled(true);

            m_deleteItem->setNormalImage(ButtonSprite::create(
                "Delete Backups", 250.f, true, "goldFont.fnt",
                "GJ_button_06.png", 0.f, 1.f));
            m_loadItem->setNormalImage(ButtonSprite::create(
                "Load Account Data", 250.f, true, "goldFont.fnt",
                "GJ_button_03.png", 0.f, 1.f));
            m_loadLevelsItem->setNormalImage(ButtonSprite::create(
                "Load Local Levels", 250.f, true, "goldFont.fnt",
                "GJ_button_03.png", 0.f, 1.f));
          }

          // subscriber is a boolean from /check
          bool isSubscriber = false;
          if (auto sb = obj["subscriber"].asBool()) {
            isSubscriber = sb.unwrap();
          }
          if (isSubscriber) {
            subscriberLabel->setString("Account Backup Extra Subscriber!");
          } else {
            subscriberLabel->setString("Free Account");
          }
          setCombinedSize(saveBytes, levelBytes);
          setFreeSpaceAndTotal(freePercentage, totalSize, maxDataSize);
          setLastSavedFromCheckResponse(str);
        } else {
          setCombinedSizeNA();
          setFreeSpaceNA();
          if (lastSavedLabel)
            lastSavedLabel->setString("Last Saved: N/A");
        }
      } else {
        setCombinedSizeNA();
        setFreeSpaceNA();
        if (lastSavedLabel)
          lastSavedLabel->setString("Last Saved: N/A");
      }
    } else {
      setCombinedSizeNA();
      setFreeSpaceNA();
      if (lastSavedLabel)
        lastSavedLabel->setString("Last Saved: N/A");
    }
  });
}

// public ts
void BackupPopup::refreshStatus() { this->fetchAndUpdateStatus(); }

void BackupPopup::setFreeSpaceAndTotal(int freePercentage, long long totalSize,
                                       long long maxDataSize) {
  if (!freeSpaceLabel)
    return;
  double totalMB = totalSize / (1024.0 * 1024.0);
  double maxMB = maxDataSize / (1024.0 * 1024.0);
  freeSpaceLabel->setString(
      fmt::format("Free: {}% - Total: {:.2f} MB / Max: {:.2f} MB",
                  freePercentage, totalMB, maxMB)
          .c_str());
}

void BackupPopup::setFreeSpaceNA() {
  if (!freeSpaceLabel)
    return;
  freeSpaceLabel->setString("Free: N/A");
}