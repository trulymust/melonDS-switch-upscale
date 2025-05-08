#pragma once
#include <string>
#include <ctime>
#include <cstdarg>
#include "Gfx.h"

struct Notification {
    bool active = false;
    std::string message;
    time_t startTime = 0;
    int durationSeconds = 3;
    const Gfx::PackedQuad* icon = nullptr;

    void Show(const char* fmt, ...);
    void ShowWithIcon(const Gfx::PackedQuad* icon, const char* fmt, ...);

    void Render();
};

extern Notification g_notification;
    