#include "SaveManager.hpp"

#include "BackupPopup.hpp"
#include "helper.hpp"

SaveFuture SaveManager::createSaveTask(std::string data) {
    queueInMainThread([] {
        if (!Mod::get()->getSavedValue<bool>("hasRead2")) {
            BackupPopup::showNotice();
            Mod::get()->setSavedValue<bool>("hasRead2", true);
        }
        Notification::create("Auto-backup in progress", NotificationIcon::Loading)->show();
    });

    SaveManager& instance = get();
    std::string token = *co_await instance.argonToken.lock();
    web::WebRequest req = createBackupRequest()
        .timeout(std::chrono::seconds(30))
        .bodyJSON(matjson::makeObject({
            {"accountId", instance.accountID},
            {"saveData", data},
            {"argonToken", token}
        }));

    const auto backupUrl = *co_await instance.backupURL.lock();
    auto resp = co_await req.post(backupUrl + "/save");
    queueInMainThread([resp] {
        if (resp.ok()) {
            log::info("Auto-backup successful");
            Notification::create("Auto-backup completed successfully!",NotificationIcon::Success)->show();
        } else {
            log::warn("Auto-backup failed with status: {}", resp.code());
            Notification::create(
            fmt::format("Auto-backup failed with status: {}", resp.code()),NotificationIcon::Error)->show();
        }
    });
    co_return;
}

SaveManager::SaveManager() {
    auto [tx, rx] = arc::mpsc::channel<std::shared_ptr<DS_Dictionary>>(1);
    saveChannel = tx;

    arc::spawn([rx = std::move(rx)]() mutable -> arc::Future<> {
        while (auto rdict = co_await rx.recv()) {
            const auto dict = std::move(*rdict);
            const std::string saveData = ZipUtils::compressString(dict->saveRootSubDictToString(), false, 11);
            co_await createSaveTask(saveData);
        }
    });
}

void SaveManager::scheduleSave() {
    accountID = GJAccountManager::get()->m_accountID;
    *argonToken.blockingLock() = Mod::get()->getSavedValue<std::string>("argonToken");
    *backupURL.blockingLock() = Mod::get()->getSettingValue<std::string>("backup-url");

    GameManager* gm = GameManager::sharedState();
    const auto dict = std::make_shared<DS_Dictionary>();
    gm->encodeDataTo(&*dict);

    if (saveChannel) {
        (void) saveChannel->trySend(dict);
    }
}

void SaveManager::scheduleSaveDelayed() {
    arc::spawn([this]() mutable -> arc::Future<> {
        co_await arc::sleepFor(asp::Duration::fromSecs(1));
        queueInMainThread([this] {
            scheduleSave();
        });
    });
}