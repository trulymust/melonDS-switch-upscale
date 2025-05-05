#pragma once
#include <string>
#include <time.h> 

struct Notification {
    bool active = false;
    std::string message;
    time_t startTime = 0;
    int durationSeconds = 3;

    void Show(const std::string& msg) {
        message = msg;
        active = true;
        startTime = time(nullptr);
    }

    void Render();
};

extern Notification g_notification;
