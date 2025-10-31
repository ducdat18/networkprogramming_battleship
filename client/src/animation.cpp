#include "animation.h"
#include <algorithm>
#include <ctime>

AnimationManager::AnimationManager() {
}

AnimationManager::~AnimationManager() {
    active_animations.clear();
}

void AnimationManager::addExplosion(int row, int col) {
    // Explosion lasts 1.0 second (reduced from 1.5)
    active_animations.push_back(Animation(ANIM_EXPLOSION, row, col, 1.0));
}

void AnimationManager::addSplash(int row, int col) {
    // Splash lasts 0.8 seconds (reduced from 1.0)
    active_animations.push_back(Animation(ANIM_SPLASH, row, col, 0.8));
}

void AnimationManager::addShipSink(int row, int col) {
    // Ship sink lasts 1.5 seconds (reduced from 2.0)
    active_animations.push_back(Animation(ANIM_SHIP_SINK, row, col, 1.5));
}

void AnimationManager::update() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    double current_time = ts.tv_sec + ts.tv_nsec / 1000000000.0;

    for (auto& anim : active_animations) {
        double elapsed = current_time - anim.start_time;
        anim.progress = elapsed / anim.duration;

        if (anim.progress >= 1.0) {
            anim.progress = 1.0;
            anim.finished = true;
        }
    }

    // Remove finished animations
    cleanup();
}

const Animation* AnimationManager::getAnimationAt(int row, int col) const {
    for (const auto& anim : active_animations) {
        if (anim.row == row && anim.col == col && !anim.finished) {
            return &anim;
        }
    }
    return nullptr;
}

bool AnimationManager::hasAnimationAt(int row, int col) const {
    return getAnimationAt(row, col) != nullptr;
}

void AnimationManager::cleanup() {
    active_animations.erase(
        std::remove_if(active_animations.begin(), active_animations.end(),
            [](const Animation& a) { return a.finished; }),
        active_animations.end()
    );
}

void AnimationManager::clearAll() {
    active_animations.clear();
}
