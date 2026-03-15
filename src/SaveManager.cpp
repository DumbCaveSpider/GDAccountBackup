#include "SaveManager.hpp"

#include "BackupPopup.hpp"
#include "helper.hpp"

SaveFuture SaveManager::createSaveTask(std::string data, const bool isData, std::function<void(web::WebResponse)> callback) {
    queueInMainThread([] {
        if (!Mod::get()->getSavedValue<bool>("hasRead2")) {
            BackupPopup::showNotice();
            Mod::get()->setSavedValue<bool>("hasRead2", true);
        }
    });

    SaveManager& instance = get();
    *co_await instance.backupNotification->shouldShow.lock() = true;
    *co_await instance.backupNotification->status.lock() = BackupStatus::InProgress;
    int accountID = *co_await instance.accountID.lock();
    std::string token = *co_await instance.argonToken.lock();
    std::string dataKey = isData ? "saveData" : "levelData";
    web::WebRequest req = createBackupRequest()
        .timeout(std::chrono::seconds(30))
        .bodyJSON(matjson::makeObject({
            {"accountId", accountID},
            {dataKey, data},
            {"argonToken", token}
        }))
        .onProgress([&instance](const web::WebProgress& progress) {
            *instance.backupNotification->progress.blockingLock() = progress.uploadProgress().value_or(0.0f);
        });

    const auto backupUrl = *co_await instance.backupURL.lock();
    auto resp = co_await req.post(backupUrl + "/save");
    if (resp.ok()) *co_await instance.backupNotification->status.lock() = BackupStatus::Completed;
    else *co_await instance.backupNotification->status.lock() = BackupStatus::Failed;

    queueInMainThread([resp, callback] {
        if (callback) {
            callback(resp);
        }
    });
    co_return;
}

SaveManager::SaveManager() {
    auto [dataTx, dataRx] = arc::mpsc::channel<SaveItem>(1);
    auto [levelTx, levelRx] = arc::mpsc::channel<SaveItem>(1);
    saveDataChannel = dataTx;
    saveLevelsChannel = levelTx;
    backupNotification = BackupNotification::create();

    arc::spawn([dataRx = std::move(dataRx), levelRx = std::move(levelRx)]() mutable -> arc::Future<> {
        bool pollData = true;
        bool pollLevels = true;
        while (true) {
            SaveItem item;

            co_await arc::select(
                arc::selectee(dataRx.recv(), [&](auto res) { if (res) item = std::move(*res); else { pollData = false; } }, pollData),
                arc::selectee(levelRx.recv(), [&](auto res) { if (res) item = std::move(*res); else { pollLevels = false; } }, pollLevels)
            );

            if (!pollData && !pollLevels) break;

            if (!item.dictionary) continue;
            const auto dict = std::move(item.dictionary);
            const std::string saveData = ZipUtils::compressString(dict->saveRootSubDictToString(), false, 11);
            co_await createSaveTask(saveData, item.data, std::move(item.callback));
        }
    });
}

void SaveManager::scheduleSaveData(std::function<void(web::WebResponse)> callback) {
    *accountID.blockingLock() = GJAccountManager::get()->m_accountID;
    *argonToken.blockingLock() = Mod::get()->getSavedValue<std::string>("argonToken");
    *backupURL.blockingLock() = Mod::get()->getSettingValue<std::string>("backup-url");

    GameManager* gm = GameManager::sharedState();
    const auto dict = std::make_shared<DS_Dictionary>();
    gm->encodeDataTo(&*dict);

    if (saveDataChannel) {
        (void) saveDataChannel->trySend({dict, std::move(callback), true});
    }
}

void SaveManager::scheduleSaveDataDelayed(std::function<void(web::WebResponse)> callback) {
    arc::spawn([this, callback]() mutable -> arc::Future<> {
        co_await arc::sleepFor(asp::Duration::fromSecs(1));
        queueInMainThread([this, callback] {
            scheduleSaveData(std::move(callback));
        });
    });
}

void SaveManager::scheduleSaveLevels(std::function<void(web::WebResponse)> callback) {
    *accountID.blockingLock() = GJAccountManager::get()->m_accountID;
    *argonToken.blockingLock() = Mod::get()->getSavedValue<std::string>("argonToken");
    *backupURL.blockingLock() = Mod::get()->getSettingValue<std::string>("backup-url");

    LocalLevelManager* llm = LocalLevelManager::sharedState();
    const auto dict = std::make_shared<DS_Dictionary>();
    llm->encodeDataTo(&*dict);

    if (saveLevelsChannel) {
        (void) saveLevelsChannel->trySend({dict, std::move(callback), false});
    }
}