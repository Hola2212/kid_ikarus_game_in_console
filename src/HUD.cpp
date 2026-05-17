#include "HUD.h"
#include <thread>
#include <chrono>

HUDData    HUD::snapshot_{};
std::mutex HUD::snapshotMutex_{};

void HUD::threadFunc(GameState* gs) {
    while (gs->running.load()) {
        HUDData data;
        {
            std::lock_guard<std::mutex> sl(gs->pitMutex);
            data.lives  = gs->pit.lives;
            data.hp     = gs->pit.hp;
            data.hearts = gs->pit.hearts;
        }
        data.phase  = gs->phase.load();
        data.status = gs->status.load();
        data.score  = gs->score.load();
        {
            std::lock_guard<std::mutex> sl(snapshotMutex_);
            snapshot_ = data;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(HUD_THREAD_MS));
    }
}

HUDData HUD::getSnapshot() {
    std::lock_guard<std::mutex> sl(snapshotMutex_);
    return snapshot_;
}