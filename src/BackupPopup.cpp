#include <Geode/Geode.hpp>
#include <argon/argon.hpp>
#include <matjson.hpp>
#include <Geode/utils/web.hpp>
#include "BackupPopup.hpp"
#include <Geode/binding/GameManager.hpp>

using namespace geode::prelude;

BackupPopup *BackupPopup::create()
{
    auto ret = new BackupPopup();
    if (ret && ret->initAnchored(380.f, 275.f, "GJ_square02.png"))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool BackupPopup::setup()
{
    setTitle("Account Alternative Backup");
    int accountId = 0;
    if (auto acct = GJAccountManager::get())
    {
        accountId = acct->m_accountID;
    }

    // try to get username (visible outside the if block)
    gd::string name;
    if (auto glm = GameLevelManager::sharedState())
    {
        name = glm->tryGetUsername(accountId);
    }

    std::string labelText = "Backing up: " + std::string(name);
    auto label = CCLabelBMFont::create(labelText.c_str(), "bigFont.fnt");
    label->setAlignment(kCCTextAlignmentCenter);
    label->setPosition({m_mainLayer->getContentSize().width / 2, m_mainLayer->getContentSize().height - 50});
    label->setScale(0.7f);
    m_mainLayer->addChild(label);

    // account size
    auto sizeLabel = CCLabelBMFont::create("Account size: ...", "chatFont.fnt");
    sizeLabel->setAlignment(kCCTextAlignmentCenter);
    sizeLabel->setPosition({m_mainLayer->getContentSize().width / 2, m_mainLayer->getContentSize().height - 70});
    sizeLabel->setScale(0.5f);
    m_mainLayer->addChild(sizeLabel);

    // last saved label
    auto lastSavedLabel = CCLabelBMFont::create("Last Saved: ...", "chatFont.fnt");
    lastSavedLabel->setAlignment(kCCTextAlignmentCenter);
    lastSavedLabel->setPosition({m_mainLayer->getContentSize().width / 2, m_mainLayer->getContentSize().height - 90});
    lastSavedLabel->setScale(0.45f);
    m_mainLayer->addChild(lastSavedLabel);
    {
        std::string token = Mod::get()->getSavedValue<std::string>("argonToken");
        matjson::Value body = matjson::makeObject({{"accountId", accountId}, {"argonToken", token}});
        std::string backupUrl = Mod::get()->getSettingValue<std::string>("backup-url");
        auto req = geode::utils::web::WebRequest()
            .timeout(std::chrono::seconds(10))
            .header("Content-Type", "application/json")
            .bodyJSON(body)
            .post(backupUrl + "/lastsaved");
        static geode::EventListener<geode::utils::web::WebTask> lastSavedListener;
        lastSavedListener.bind([lastSavedLabel](geode::utils::web::WebTask::Event *e) {
            if (auto *resp = e->getValue()) {
                if (resp->ok()) {
                    auto strResult = resp->string();
                    if (strResult) {
                        lastSavedLabel->setString(fmt::format("Last Saved: {}", strResult.unwrap()).c_str());
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
    }

    std::string token = Mod::get()->getSavedValue<std::string>("argonToken");
    matjson::Value body = matjson::makeObject({{"accountId", accountId}, {"argonToken", token}});
    std::string backupUrl = Mod::get()->getSettingValue<std::string>("backup-url");
    auto req = geode::utils::web::WebRequest()
                   .timeout(std::chrono::seconds(10))
                   .header("Content-Type", "application/json")
                   .bodyJSON(body)
                   .post(backupUrl + "/check");
    static geode::EventListener<geode::utils::web::WebTask> sizeListener;
    sizeListener.bind([sizeLabel](geode::utils::web::WebTask::Event *e)
                      {
        if (auto *resp = e->getValue()) {
            if (resp->ok()) {
                auto strResult = resp->string();
                if (strResult) {
                    const std::string &str = strResult.unwrap();
                    char* endptr = nullptr;
                    double sizeBytes = std::strtod(str.c_str(), &endptr); // byte into megabytes
                    if (endptr != str.c_str() && *endptr == '\0') {
                        double sizeMB = sizeBytes / (1024.0 * 1024.0);
                        sizeLabel->setString(fmt::format("Compressed Save data size: {:.2f} MB", sizeMB).c_str());
                    } else {
                        sizeLabel->setString("Compressed Save data size: N/A");
                    }
                } else {
                    sizeLabel->setString("Compressed Save data size: N/A");
                }
            } else {
                sizeLabel->setString("Compressed Save data size: N/A");
            }
        } else {
            sizeLabel->setString("Compressed Save data size: ...");
        } });
    sizeListener.setFilter(std::move(req));

    // create three vertically centered buttons: Save, Load, Delete
    float centerX = m_mainLayer->getContentSize().width / 2;
    float centerY = m_mainLayer->getContentSize().height / 2 - 15.f;
    float verticalSpacing = 45.f;

    auto saveBtn = ButtonSprite::create("Save", 180.f);
    auto saveItem = CCMenuItemSpriteExtra::create(saveBtn, this, menu_selector(BackupPopup::onSave));
    saveItem->setPosition({0, verticalSpacing});

    auto loadBtn = ButtonSprite::create("Load", 180.f);
    auto loadItem = CCMenuItemSpriteExtra::create(loadBtn, this, menu_selector(BackupPopup::onLoad));
    loadItem->setPosition({0, 0});

    auto deleteBtn = ButtonSprite::create("Delete", 180.f);
    auto deleteItem = CCMenuItemSpriteExtra::create(deleteBtn, this, menu_selector(BackupPopup::onDelete));
    deleteItem->setPosition({0, -verticalSpacing});

    auto menu = CCMenu::create();
    menu->addChild(saveItem);
    menu->addChild(loadItem);
    menu->addChild(deleteItem);
    menu->setPosition({centerX, centerY});
    m_mainLayer->addChild(menu);

    return true;
}

void BackupPopup::onSave(CCObject *)
{
    geode::createQuickPopup(
        "Save Data",
        "Do you want to <cg>save</c> your account data to the backup server?\n<cy>This will overwrite your existing backup saved in the server.</c>",
        "Cancel", "Save",
        [](FLAlertLayer *, bool confirmed)
        {
            if (!confirmed)
                return;
            std::string token = Mod::get()->getSavedValue<std::string>("argonToken");
            int accountId = 0;
            if (auto acct = GJAccountManager::get())
            {
                accountId = acct->m_accountID;
            }
            std::string saveData;
            if (auto gm = GameManager::sharedState())
            {
                saveData = gm->getCompressedSaveString();
            }
            matjson::Value body = matjson::makeObject({{"accountId", accountId},
                                                       {"saveData", saveData},
                                                       {"argonToken", token}});
            std::string backupUrl = Mod::get()->getSettingValue<std::string>("backup-url");
            auto req = geode::utils::web::WebRequest()
                           .timeout(std::chrono::seconds(10))
                           .header("Content-Type", "application/json")
                           .bodyJSON(body)
                           .post(backupUrl + "/save");
            static geode::EventListener<geode::utils::web::WebTask> listener;
            listener.bind([](geode::utils::web::WebTask::Event *e)
                          {
                if (auto* resp = e->getValue()) {
                    if (resp->ok()) {
                        Notification::create("Backup saved successfully!", NotificationIcon::Success)->show();
                    } else {
                        Notification::create("Save failed: " + std::to_string(resp->code()), NotificationIcon::Error)->show();
                    }
                } else if (e->isCancelled()) {
                    Notification::create("Save request was cancelled", NotificationIcon::Error)->show();
                } });
            listener.setFilter(std::move(req));
        });
}

void BackupPopup::onLoad(CCObject *)
{
    geode::createQuickPopup(
        "Load Data",
        "Do you want to <cg>download</c> your account data from the backup server?\n<cy>This will overwrite your current account data.</c>",
        "Cancel", "Load",
        [](FLAlertLayer *, bool confirmed)
        {
            if (!confirmed)
                return;
            std::string token = Mod::get()->getSavedValue<std::string>("argonToken");
            int accountId = 0;
            if (auto acct = GJAccountManager::get())
            {
                accountId = acct->m_accountID;
            }
            matjson::Value body = matjson::makeObject({{"accountId", accountId},
                                                       {"argonToken", token}});
            std::string backupUrl = Mod::get()->getSettingValue<std::string>("backup-url");
            auto req = geode::utils::web::WebRequest()
                           .timeout(std::chrono::seconds(10))
                           .header("Content-Type", "application/json")
                           .bodyJSON(body)
                           .post(backupUrl + "/load");
            static geode::EventListener<geode::utils::web::WebTask> listener;
            listener.bind([](geode::utils::web::WebTask::Event *e)
                          {
                if (auto* resp = e->getValue()) {
                    if (resp->ok()) {
                        if (auto gm = GameManager::sharedState()) {
                            auto result = resp->string();
                            if (result) {
                                gm->loadFromCompressedString(result.unwrap());
                                Notification::create("Backup loaded successfully!", NotificationIcon::Success)->show();
                            } else {
                                Notification::create("Failed to get backup data from response", NotificationIcon::Error)->show();
                            }
                        }
                    } else {
                        Notification::create("Load failed: " + std::to_string(resp->code()), NotificationIcon::Error)->show();
                    }
                } else if (e->isCancelled()) {
                    Notification::create("Load request was cancelled", NotificationIcon::Error)->show();
                } });
            listener.setFilter(std::move(req));
        });
}

void BackupPopup::onDelete(CCObject *)
{
    geode::createQuickPopup(
        "Delete Data",
        "Do you want to <cg>permanently delete</c> your backup from the backup server?\n<cy>This action cannot be undone.</c>",
        "Cancel", "Delete",
        [](FLAlertLayer *, bool confirmed)
        {
            if (!confirmed)
                return;
            std::string token = Mod::get()->getSavedValue<std::string>("argonToken");
            int accountId = 0;
            if (auto acct = GJAccountManager::get())
            {
                accountId = acct->m_accountID;
            }
            matjson::Value body = matjson::makeObject({{"accountId", accountId},
                                                       {"argonToken", token}});
            std::string backupUrl = Mod::get()->getSettingValue<std::string>("backup-url");
            auto req = geode::utils::web::WebRequest()
                           .timeout(std::chrono::seconds(10))
                           .header("Content-Type", "application/json")
                           .bodyJSON(body)
                           .post(backupUrl + "/delete");
            static geode::EventListener<geode::utils::web::WebTask> listener;
            listener.bind([](geode::utils::web::WebTask::Event *e)
                          {
                if (auto* resp = e->getValue()) {
                    if (resp->ok()) {
                        Notification::create("Backup deleted successfully!", NotificationIcon::Success)->show();
                    } else {
                        Notification::create("Delete failed: " + std::to_string(resp->code()), NotificationIcon::Error)->show();
                    }
                } else if (e->isCancelled()) {
                    Notification::create("Delete request was cancelled", NotificationIcon::Error)->show();
                } });
            listener.setFilter(std::move(req));
        });
}
