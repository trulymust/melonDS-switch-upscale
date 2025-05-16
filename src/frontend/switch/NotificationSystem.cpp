#include "NotificationSystem.h"
#include "Style.h"
#include "PlatformConfig.h"
#include <cmath>
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

void DestroyNotification() {
    if (g_notification.textureId != -1) {
        Gfx::TextureDelete(g_notification.textureId);
        g_notification.textureId = -1;
    }
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
        DestroyNotification();
        return;
    }
    Gfx::Vector2f textSize = Gfx::MeasureText(Gfx::SystemFontStandard, TextLineHeight, message.c_str());
    float padding = 20.f;
    float avatarSize = (textureId >= 0 && nwidth > 0 && nheight > 0) ? 40.f : 0.f;
    float spacing = (avatarSize > 0) ? 10.f : 0.f;
    float rectWidth = textSize.X + padding * 2 + avatarSize + spacing;
    float rectHeight = std::max(textSize.Y + padding * 2, avatarSize + padding * 2);

    Gfx::Vector2f position;
    Gfx::Vector2f size = {rectWidth, rectHeight};
    switch (Config::GlobalRotation) {
        case 1:
            position = {20.f, 720.f - rectHeight - 20.f};
            std::swap(size.X, size.Y);
            break;
        case 2:
            position = {20.f, 720.f - rectHeight - 20.f};
            break;
        case 3:
            position = {720.f - rectWidth - 20.f, 20.f};
            std::swap(size.X, size.Y);
            break;
        default:
            position = {1280.f - rectWidth - 20.f, 20.f};
            break;
    }

    Gfx::DrawRectangle(position, size, WidgetColorBright, true);
    Gfx::Vector2f textOffset = position + Gfx::Vector2f{
        padding + avatarSize + spacing,
        (rectHeight - textSize.Y) / 2.f
    };

    Gfx::Vector2f textCenter = textOffset + Gfx::Vector2f{textSize.X / 2.f, textSize.Y / 2.f};
    float angle = 0.f;

    Gfx::Vector2f rotatedTextOffset = textOffset;
    if (angle != 0.f) {
        float x = textOffset.X - position.X - rectWidth / 2.f;
        float y = textOffset.Y - position.Y - rectHeight / 2.f;

        float rotatedX = x * cos(angle * M_PI / 180.f) - y * sin(angle * M_PI / 180.f);
        float rotatedY = x * sin(angle * M_PI / 180.f) + y * cos(angle * M_PI / 180.f);

        rotatedTextOffset.X = rotatedX + position.X + rectWidth / 2.f;
        rotatedTextOffset.Y = rotatedY + position.Y + rectHeight / 2.f;
    }
    if (textureId >= 0 && nwidth > 0 && nheight > 0) {
        Gfx::Vector2f avatarPos = position + Gfx::Vector2f{padding, (rectHeight - avatarSize) / 2.f};
        Gfx::Vector2f avatarDrawSize = {avatarSize, avatarSize};

        Gfx::DrawRectangle(
            textureId,
            avatarPos,
            avatarDrawSize,
            {0.f, 0.f}, {static_cast<float>(nwidth), static_cast<float>(nheight)},
            WidgetColorBright
        );
    }
    if (!message.empty()) {
        Gfx::DrawText(Gfx::SystemFontStandard, rotatedTextOffset, TextLineHeight, DarkColor,
                      Gfx::align_Left, Gfx::align_Center, message.c_str());
    }
}
