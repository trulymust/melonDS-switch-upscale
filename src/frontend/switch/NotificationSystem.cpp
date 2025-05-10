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
}

void Notification::ShowWithIcon(int texId, int width, int height, const char* fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    message = buffer;
    textureId = texId;
    nwidth = width;
    nheight = height;
    active = true;
    startTime = time(nullptr);
}

void Notification::Render() {
    if (!active)
        return;

    if (difftime(time(nullptr), startTime) > durationSeconds) {
        active = false;
        textureId = -1;
        return;
    }

    Gfx::Vector2f position = {1280.f - 400.f, 40.f};
    Gfx::Vector2f size = {380.f, 50.f};

    Gfx::DrawRectangle(position, size, WidgetColorBright, true);

    Gfx::Vector2f textOffset = position + Gfx::Vector2f{10.f, 25.f};

    if (textureId >= 0) {
        Gfx::Vector2f avatarPos = position + Gfx::Vector2f{5.f, 5.f};
        Gfx::Vector2f maxSize = {40.f, 40.f};
    
        float scale = std::min(maxSize.X / nwidth, maxSize.Y / nheight);
        Gfx::Vector2f avatarSize = {nwidth * scale, nheight * scale};
    
        Gfx::DrawRectangle(
            textureId,
            avatarPos,
            avatarSize,
            {0.f, 0.f}, {static_cast<float>(nwidth), static_cast<float>(nheight)},
            WidgetColorBright
        );
    
        textOffset.X += avatarSize.X + 10.f;
    }
    else {
        printf("DEBUG: Notification System: icon is null\n");
        fflush(stdout);
    }

    Gfx::DrawText(Gfx::SystemFontStandard, textOffset, TextLineHeight, DarkColor,
                  Gfx::align_Left, Gfx::align_Center, message.c_str());
}
