#pragma once
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

    float alpha = 1.f;

    bool shown = false;

    bool init() override;

    void update(float delta) override;

public:
    volatile float progress = 0.0f;
    volatile BackupStatus status = BackupStatus::InProgress;
    volatile bool shouldShow = false;

    CREATE_FUNC(BackupNotification);

    void show();
    void hide();
};