#pragma once

#include <arc/prelude.hpp>

#include "BackupNotification.hpp"

using namespace geode::prelude;

using SaveFuture = arc::Future<>;

struct SaveItem {
    std::shared_ptr<DS_Dictionary> dictionary;
    std::function<void(web::WebResponse)> callback;
    bool data;
};

class SaveManager {
    std::optional<arc::mpsc::Sender<SaveItem>> saveDataChannel;
    std::optional<arc::mpsc::Sender<SaveItem>> saveLevelsChannel;
    volatile int accountID = 0;
    arc::Mutex<std::string> argonToken;
    arc::Mutex<std::string> backupURL;
    Ref<BackupNotification> backupNotification = nullptr;

    static SaveFuture createSaveTask(std::string data, bool isData, std::function<void(web::WebResponse)> callback);

    SaveManager();

public:
    static SaveManager& get() {
        static SaveManager instance;
        return instance;
    }

    void scheduleSaveData(std::function<void(web::WebResponse)> callback = nullptr);
    void scheduleSaveDataDelayed(std::function<void(web::WebResponse)> callback = nullptr);

    void scheduleSaveLevels(std::function<void(web::WebResponse)> callback = nullptr);
};