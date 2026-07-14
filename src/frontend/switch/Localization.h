#ifndef LOCALIZATION_H
#define LOCALIZATION_H

namespace Localization
{

enum LanguageId
{
    Language_English = 0,
    Language_SimplifiedChinese = 1,
    Language_Max
};

enum class OptionList
{
    Language,
    ConsoleMode,
    SwitchCpuClock,
    Rotation,
    ScreenSizing,
    ScreenGap,
    ScreenLayout,
    AspectRatio,
    Filtering,
    InternalResolution,
    CursorMode,
    ClickMode,
};

LanguageId CurrentLanguage();
const char* Text(const char* text);
const char* Options(OptionList list);

}

#endif
