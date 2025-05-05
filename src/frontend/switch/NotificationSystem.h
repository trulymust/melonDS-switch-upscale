#pragma once
#include <string>

struct Notification {
    std::string message;
    float timeRemaining = 0.f;
    bool active = false;

    void Show(const std::string& msg, float duration = 3.0f) {
        message = msg;
        timeRemaining = duration;
        active = true;
    }

    void Update(float deltaTime) {
        if (!active) return;
        timeRemaining -= deltaTime;
        if (timeRemaining <= 0.f) {
            active = false;
        }
    }

    void Render();
};

extern Notification g_notification;
