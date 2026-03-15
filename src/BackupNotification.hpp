#pragma once
#include <arc/sync/Mutex.hpp>
#include <Geode/Prelude.hpp>
#include <Geode/cocos/base_nodes/CCNode.h>

using namespace geode::prelude;

enum class BackupStatus {
    InProgress,
    Completed,
    Failed,
};

class BackupNotification : public CCNodeRGBA {
    NineSlice* background = nullptr;
    LoadingSpinner* spinner = nullptr;
    CCLabelBMFont* label = nullptr;
    ProgressBar* progressBar = nullptr;
    CCSprite* failedSprite = nullptr;
    CCSprite* successSprite = nullptr;

    bool shown = false;

    bool init() override;

    void update(float delta) override;

public:
    arc::Mutex<float> progress = arc::Mutex<float>(0.0f);
    arc::Mutex<BackupStatus> status = arc::Mutex<BackupStatus>(BackupStatus::InProgress);
    arc::Mutex<bool> shouldShow = arc::Mutex<bool>(false);

    CREATE_FUNC(BackupNotification);

    void show();
    void hide();
};