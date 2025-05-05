#include "NotificationSystem.h"
#include "Gfx.h"
#include "Style.h"


Notification g_notification;

void Notification::Render() {
    if (!active) return;

    Gfx::Vector2f position = {1280.f - 400.f, 40.f};
    Gfx::Vector2f size = {380.f, 50.f};

    Gfx::DrawRectangle(position, size, WidgetColorBright, true);

    Gfx::DrawRectangle(position, size, WidgetColorVibrant, false);

    Gfx::DrawText(
        Gfx::SystemFontStandard,
        position + Gfx::Vector2f{10.f, 25.f},
        TextLineHeight,
        DarkColor,
        Gfx::align_Left,
        Gfx::align_Center,
        message.c_str()
    );
}
