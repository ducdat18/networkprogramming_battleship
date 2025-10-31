#ifndef ANIMATION_H
#define ANIMATION_H

#include <gtk/gtk.h>
#include <ctime>
#include <vector>

// Animation types
enum AnimationType {
    ANIM_EXPLOSION,
    ANIM_SPLASH,
    ANIM_SHIP_SINK
};

// Single animation instance
struct Animation {
    AnimationType type;
    int row;
    int col;
    double start_time;
    double duration;  // seconds
    double progress;  // 0.0 to 1.0
    bool finished;

    Animation(AnimationType t, int r, int c, double dur)
        : type(t), row(r), col(c), duration(dur), progress(0.0), finished(false) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        start_time = ts.tv_sec + ts.tv_nsec / 1000000000.0;
    }
};

// Animation manager
class AnimationManager {
private:
    std::vector<Animation> active_animations;

public:
    AnimationManager();
    ~AnimationManager();

    // Add new animation
    void addExplosion(int row, int col);
    void addSplash(int row, int col);
    void addShipSink(int row, int col);

    // Update all animations
    void update();

    // Get active animations for a cell
    const Animation* getAnimationAt(int row, int col) const;

    // Check if animation exists at position
    bool hasAnimationAt(int row, int col) const;

    // Clear finished animations
    void cleanup();

    // Clear all
    void clearAll();
};

#endif // ANIMATION_H
