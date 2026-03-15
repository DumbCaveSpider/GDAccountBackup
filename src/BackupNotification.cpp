#include "BackupNotification.hpp"

#include <Geode/Geode.hpp>
#include <UIBuilder.hpp>

bool BackupNotification::init() {
    if (!CCNodeRGBA::init()) return false;

    constexpr CCSize SIZE = { 150.0f, 50.0f };

    setContentSize(SIZE);
    setAnchorPoint({ 1.0f, 0.0f });

    background = Build(NineSlice::create("square02b_small.png", { 0.0f, 0.0f, 40.0f, 40.0f }))
        .color({ 0, 0, 0 })
        .opacity(150)
        .contentSize(SIZE)
        .anchorPoint({ 0.0f, 0.0f })
        .id("background")
        .parent(this);

    spinner = Build<LoadingSpinner>::create(20.0f)
        .anchorPoint({0.5f, 0.5f})
        .pos({SIZE.width - SIZE.height / 2.0f, SIZE.height / 2.0f})
        .id("loading-spinner")
        .parent(this);

    label = Build<CCLabelBMFont>::create("Backing up...", "goldFont.fnt")
        .scale(0.55f)
        .anchorPoint({0.5f, 0.5f})
        .pos({(SIZE.width - SIZE.height) / 2.0f + 5.0f, SIZE.height / 2.0f + 10.0f})
        .id("label")
        .parent(this);

    progressBar = Build<ProgressBar>::create(ProgressBarStyle::Solid)
        .anchorPoint({0.5f, 0.5f})
        .pos({(SIZE.width - SIZE.height) / 2.0f + 5.0f, SIZE.height / 2.0f - 10.0f})
        .scale(0.3f)
        .with([](ProgressBar* bar) {
            bar->setFillColor({ 144, 213, 255 });
            bar->showProgressLabel(true);
            bar->setPrecision(1);
            bar->getChildByIndex(0)->setScaleY(2.0f);
            bar->getChildByIndex(0)->runAction(CCFadeTo::create(0.0f, 0));
            bar->getChildByIndex(0)->getChildByIndex(0)->runAction(CCFadeTo::create(0.0f, 0));
            bar->getChildByIndex(1)->setScale(1.3f);
            bar->getChildByIndex(1)->runAction(CCFadeTo::create(0.0f, 0));
        })
        .id("progress-bar")
        .parent(this);

    failedSprite = Build<CCSprite>::createSpriteName("edit_delBtnSmall_001.png")
        .scale(0.7f/0.52f)
        .anchorPoint({0.5f, 0.5f})
        .pos({SIZE.width - SIZE.height / 2.0f, SIZE.height / 2.0f})
        .visible(false)
        .id("failed-icon")
        .parent(this);

    successSprite = Build<CCSprite>::createSpriteName("GJ_completesIcon_001.png")
        .scale(0.7f/0.98f)
        .anchorPoint({0.5f, 0.5f})
        .pos({SIZE.width - SIZE.height / 2.0f, SIZE.height / 2.0f})
        .visible(false)
        .id("success-icon")
        .parent(this);

    setCascadeOpacityEnabled(true);
    setOpacity(0);

    scheduleUpdate();
    const CCSize winSize = CCDirector::sharedDirector()->getWinSize();
    OverlayManager* overlay = OverlayManager::get();
    setPosition({ winSize.width - 10.0f, 10.0f });
    overlay->addChild(this);

    return true;
}

void BackupNotification::update(const float delta) {
    progressBar->updateProgress(*progress.blockingLock());
    const BackupStatus obtainedStatus = *status.blockingLock();
    spinner->setVisible(obtainedStatus == BackupStatus::InProgress);
    failedSprite->setVisible(obtainedStatus == BackupStatus::Failed);
    successSprite->setVisible(obtainedStatus == BackupStatus::Completed);

    if (obtainedStatus != BackupStatus::InProgress) {
        hide();
    }

    {
        arc::MutexGuard<bool> showGuard = shouldShow.blockingLock();
        if (*showGuard) {
            show();
            *showGuard = false;
        }
    }
}

constexpr float FADE_TIME = 0.4f;
constexpr float WAIT_TIME = 1.0f;

void BackupNotification::show() {
    if (shown) return;
    shown = true;

    stopAllActions();
    runAction(CCSequence::create(
        CallFuncExt::create([this] {
            runAction(CCFadeTo::create(FADE_TIME, 255));
            progressBar->getChildByIndex(0)->runAction(CCFadeTo::create(FADE_TIME, 255));
            progressBar->getChildByIndex(0)->getChildByIndex(0)->runAction(CCFadeTo::create(FADE_TIME, 255));
            progressBar->getChildByIndex(1)->runAction(CCFadeTo::create(FADE_TIME, 255));
        }),
        nullptr
    ));
}

void BackupNotification::hide() {
    if (!shown) return;
    shown = false;

    stopAllActions();
    runAction(CCSequence::create(
        CCDelayTime::create(WAIT_TIME),
        CallFuncExt::create([this] {
            runAction(CCEaseExponentialIn::create(CCFadeTo::create(FADE_TIME, 0)));
            progressBar->getChildByIndex(0)->runAction(CCEaseExponentialIn::create(CCFadeTo::create(FADE_TIME, 0)));
            progressBar->getChildByIndex(0)->getChildByIndex(0)->runAction(CCEaseExponentialIn::create(CCFadeTo::create(FADE_TIME, 0)));
            progressBar->getChildByIndex(1)->runAction(CCEaseExponentialIn::create(CCFadeTo::create(FADE_TIME, 0)));
        }),
        nullptr
    ));
}