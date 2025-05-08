#include "NotificationSystem.h"
#include "Style.h"
#include <cstdio>
#include <cstdarg>

Notification g_notification;

void Notification::Show(const char* fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    message = buffer;
    active = true;
    startTime = time(nullptr);
    icon = nullptr;
}

void Notification::ShowWithIcon(const Gfx::PackedQuad* iconPtr, const char* fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    message = buffer;
    icon = iconPtr;
    active = true;
    startTime = time(nullptr);
}

void Notification::Render() {
    if (!active)
        return;

    if (difftime(time(nullptr), startTime) > durationSeconds) {
        active = false;
        return;
    }

    Gfx::Vector2f position = {1280.f - 400.f, 40.f};
    Gfx::Vector2f size = {380.f, 50.f};

    Gfx::DrawRectangle(position, size, WidgetColorBright, true);

    Gfx::Vector2f textOffset = position + Gfx::Vector2f{10.f, 25.f};

    if (icon) {
        Gfx::DrawRectangle(
            icon->AtlasTexture,
            position + Gfx::Vector2f{5.f, 5.f},
            {40.f, 40.f},
            {static_cast<float>(icon->PackX), static_cast<float>(icon->PackY)},
            WidgetColorBright
        );
        textOffset.X += 50.f;
    } else {
        printf("DEBUG: Notification System: icon is null\n");
        fflush(stdout);
    }

    Gfx::DrawText(Gfx::SystemFontStandard, textOffset, TextLineHeight, DarkColor,
                  Gfx::align_Left, Gfx::align_Center, message.c_str());
}
