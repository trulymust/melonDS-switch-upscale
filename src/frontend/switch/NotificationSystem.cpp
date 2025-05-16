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

    // Usa MeasureText per calcolare la larghezza e l'altezza del testo
    Gfx::Vector2f textSize = Gfx::MeasureText(Gfx::SystemFontStandard, TextLineHeight, message.c_str());
    float padding = 20.f; // Padding per i bordi
    float avatarSize = (textureId >= 0 && nwidth > 0 && nheight > 0) ? 40.f : 0.f; // Dimensione dell'icona (quadrata)
    float spacing = (avatarSize > 0) ? 10.f : 0.f; // Spazio tra l'icona e il testo
    float rectWidth = textSize.X + padding * 2 + avatarSize + spacing; // Larghezza rettangolo
    float rectHeight = std::max(textSize.Y + padding * 2, avatarSize + padding * 2); // Altezza rettangolo

    Gfx::Vector2f position = {1280.f - rectWidth - 20.f, 40.f}; // Posizionamento in alto a destra
    Gfx::Vector2f size = {rectWidth, rectHeight};

    Gfx::DrawRectangle(position, size, WidgetColorBright, true);

    Gfx::Vector2f textOffset = position + Gfx::Vector2f{
        padding + avatarSize + spacing, // Offset orizzontale accanto all'icona
        (rectHeight - textSize.Y) / 2.f // Centrare verticalmente il testo
    };

    if (textureId >= 0) {
        if (nwidth > 0 && nheight > 0) {
            Gfx::Vector2f avatarPos = position + Gfx::Vector2f{padding, (rectHeight - avatarSize) / 2.f};
            Gfx::Vector2f avatarDrawSize = {avatarSize, avatarSize};

            Gfx::DrawRectangle(
                textureId,
                avatarPos,
                avatarDrawSize,
                {0.f, 0.f}, {static_cast<float>(nwidth), static_cast<float>(nheight)},
                WidgetColorBright
            );
        } else {
            printf("DEBUG: Invalid avatar dimensions: nwidth=%d, nheight=%d\n", nwidth, nheight);
            fflush(stdout);
        }
    }

    if (!message.empty()) {
        Gfx::DrawText(Gfx::SystemFontStandard, textOffset, TextLineHeight, DarkColor,
                      Gfx::align_Left, Gfx::align_Center, message.c_str());
    } else {
        printf("DEBUG: Notification message is empty\n");
        fflush(stdout);
    }
}
