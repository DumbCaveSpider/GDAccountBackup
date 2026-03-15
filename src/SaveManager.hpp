#pragma once

#include <arc/prelude.hpp>

using namespace geode::prelude;

using SaveFuture = arc::Future<>;

class SaveManager {
    std::optional<arc::mpsc::Sender<std::shared_ptr<DS_Dictionary>>> saveChannel;
    volatile int accountID = 0;
    arc::Mutex<std::string> argonToken;
    arc::Mutex<std::string> backupURL;

    static SaveFuture createSaveTask(std::string data);

    SaveManager();

public:
    static SaveManager& get() {
        static SaveManager instance;
        return instance;
    }

    void scheduleSave();
    void scheduleSaveDelayed();
};