#ifndef VAULT_HEIST_ANIMATION_CONTROLLER_H
#define VAULT_HEIST_ANIMATION_CONTROLLER_H

#include <algorithm>

class AnimationController {
public:
    void play() { playing = true; elapsed = 0.0; }
    void pause() { playing = false; }
    void toggle() { playing = !playing; elapsed = 0.0; }
    void stop() { playing = false; index = 0; elapsed = 0.0; }
    void setSpeed(float value) { speed = std::max(0.25f, std::min(4.0f, value)); }
    bool isPlaying() const { return playing; }
    int currentIndex() const { return index; }

    void seek(int target, int eventCount) {
        playing = false;
        index = eventCount <= 0 ? -1 : std::max(0, std::min(eventCount - 1, target));
        elapsed = 0.0;
    }

    void step(int direction, int eventCount) {
        playing = false;
        if (eventCount <= 0) { index = -1; return; }
        index = std::max(0, std::min(eventCount - 1, index + direction));
        elapsed = 0.0;
    }

    void update(double deltaSeconds, int eventCount) {
        if (!playing || eventCount <= 0) return;
        elapsed += deltaSeconds * speed;
        if (elapsed >= 0.45) {
            elapsed = 0.0;
            if (++index >= eventCount) { index = eventCount - 1; playing = false; }
        }
    }

private:
    int index = -1;
    float speed = 1.0f;
    double elapsed = 0.0;
    bool playing = false;
};

#endif
