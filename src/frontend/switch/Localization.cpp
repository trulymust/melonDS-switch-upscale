#include "Localization.h"

#include <string.h>

#include "PlatformConfig.h"

namespace Localization
{

struct TranslationEntry
{
    const char* English;
    const char* SimplifiedChinese;
};

static const TranslationEntry TranslationTable[] =
{
    {"Select", "选择"},
    {"Toggle", "切换"},
    {"Choose", "选择"},
    {"Cancel", "取消"},
    {"Edit", "编辑"},
    {"Remap", "重映射"},
    {"Back", "返回"},
    {"Start", "开始"},
    {"Unpause", "继续"},

    {"Continue", "继续"},
    {"Open lid", "打开上屏盖"},
    {"Close lid", "合上上屏盖"},
    {"Reset", "重置"},
    {"RetroAchievements List", "RetroAchievements 列表"},
    {"Cheats", "金手指"},
    {"Display settings", "显示设置"},
    {"Input settings", "输入设置"},
    {"Close", "关闭游戏"},
    {"Browse", "浏览"},
    {"Boot firmware", "启动固件"},
    {"Emulation settings", "模拟设置"},
    {"Exit", "退出"},
    {"Last played...", "最近游玩..."},
    {"Savestates", "即时存档"},
    {"Save state", "保存状态"},
    {"Load state", "读取状态"},
    {"Undo\nload", "撤销\n读取"},
    {"Failed to create savestate", "创建即时存档失败"},
    {"Couldn't load savefile", "无法读取存档"},

    {"Language", "语言"},
    {"General", "通用"},
    {"Console mode", "主机模式"},
    {"Boot directly (Skip bios)", "直接启动（跳过 BIOS）"},
    {"Switch CPU clock", "Switch CPU 频率"},
    {"JIT recompiler", "JIT 重编译器"},
    {"Enable JIT recompiler", "启用 JIT 重编译器"},
    {"Maximum block size", "最大代码块大小"},
    {"Enable JIT Branch Optimisations", "启用 JIT 分支优化"},
    {"Enable JIT Literal Optimisations", "启用 JIT 常量优化"},
    {"Enable JIT Fast Memory", "启用 JIT 快速内存"},
    {"RetroAchievements", "RetroAchievements"},
    {"RetroAchievements Username", "RetroAchievements 用户名"},
    {"RetroAchievements Password", "RetroAchievements 密码"},
    {"Login", "登录"},
    {"Hardcore Mode", "硬核模式"},
    {"Disable RA notifications", "关闭 RA 通知"},

    {"Presentation settings", "显示设置"},
    {"Framerate", "帧率"},
    {"Limit framerate", "限制帧率"},
    {"GUI", "界面"},
    {"Global rotation", "全局旋转"},
    {"Show performance metrics", "显示性能指标"},
    {"Screens", "屏幕"},
    {"Rotation", "旋转"},
    {"Sizing", "尺寸"},
    {"Gap", "间距"},
    {"Layout", "布局"},
    {"Aspect ratio top", "上屏宽高比"},
    {"Aspect ratio bottom", "下屏宽高比"},
    {"Swap screens", "交换屏幕"},
    {"Integer scaling", "整数缩放"},
    {"Filtering", "滤镜"},
    {"3D internal resolution", "3D 内部分辨率"},

    {"This title does not have any achievements.", "这个游戏没有成就。"},
    {"Please check the RetroAchievements website for more information.", "请到 RetroAchievements 网站查看更多信息。"},

    {"Action Replay", "Action Replay"},
    {"Enable cheats", "启用金手指"},
    {"No game is loaded.", "尚未载入游戏。"},
    {"Start a game before editing cheats.", "先启动游戏才能编辑金手指。"},
    {"No cheat codes found.", "没有找到金手指代码。"},
    {"Place a matching .mch file next to the ROM.", "请把匹配的 .mch 文件放在 ROM 旁边。"},
    {"No codes in this category.", "这个分类里没有代码。"},
    {"Failed to save cheat file.", "保存金手指文件失败。"},

    {"Touchscreen", "触摸屏"},
    {"Cursor mode", "光标模式"},
    {"Click mode", "点击模式"},
    {"Left handed mode", "左手模式"},
    {"Joycon", "Joy-Con"},
    {"Hold to fastforward (ZL)", "按住 ZL 快进"},
    {"Buttons Remapping", "按键重映射"},
    {"Hotkeys Remapping", "快捷键重映射"},
    {"Reset to default", "恢复默认"},
    {"Save this configuration", "保存当前配置"},
    {"Load old configuration", "读取旧配置"},
    {"Waiting for input...", "等待输入..."},
};

LanguageId CurrentLanguage()
{
    if (Config::Language == Language_SimplifiedChinese)
        return Language_SimplifiedChinese;

    return Language_English;
}

const char* Text(const char* text)
{
    if (CurrentLanguage() != Language_SimplifiedChinese)
        return text;

    for (const TranslationEntry& entry : TranslationTable)
    {
        if (strcmp(entry.English, text) == 0)
            return entry.SimplifiedChinese;
    }

    return text;
}

const char* Options(OptionList list)
{
    switch (list)
    {
    case OptionList::Language:
        return CurrentLanguage() == Language_SimplifiedChinese
            ? "English\0简体中文\0"
            : "English\0Simplified Chinese\0";
    case OptionList::ConsoleMode:
        return CurrentLanguage() == Language_SimplifiedChinese
            ? "DS\0DSi（实验）\0"
            : "DS\0DSi (experimental)\0";
    case OptionList::SwitchCpuClock:
        return "1020 MHz\0" "1224 MHz\0" "1581 MHz\0" "1785 MHz\0" "918 Mhz\0" "816 Mhz\0" "714 Mhz\0";
    case OptionList::Rotation:
        return "0°\090°\000180°\000270°\0";
    case OptionList::ScreenSizing:
        return CurrentLanguage() == Language_SimplifiedChinese
            ? "均等\0突出上屏\0突出下屏\0自动\0仅上屏\0仅下屏\0"
            : "Even\0Emphasise top\0Emphasise bottom\0Auto\0Top only\0Bottom only\0";
    case OptionList::ScreenGap:
        return "0\0001\08\00016\00032\00064\0090\000128\0";
    case OptionList::ScreenLayout:
        return CurrentLanguage() == Language_SimplifiedChinese
            ? "自然\0纵向\0横向\0混合\0"
            : "Natural\0Vertical\0Horizontal\0Hybrid\0";
    case OptionList::AspectRatio:
        return CurrentLanguage() == Language_SimplifiedChinese
            ? "4:3（原生）\00016:9\0"
            : "4:3 (native)\00016:9\0";
    case OptionList::Filtering:
        return CurrentLanguage() == Language_SimplifiedChinese
            ? "最近邻\0线性\0锐化\0"
            : "Nearest\0Linear\0Sharp\0";
    case OptionList::InternalResolution:
        return "1x\0" "2x\0" "4x\0";
    case OptionList::CursorMode:
        return CurrentLanguage() == Language_SimplifiedChinese
            ? "鼠标模式\0偏移模式\0体感控制\0"
            : "Mouse mode\0Offset mode\0Motion controls!\0";
    case OptionList::ClickMode:
        return CurrentLanguage() == Language_SimplifiedChinese
            ? "按住\0切换\0"
            : "Hold\0Toggle\0";
    }

    return "";
}

}
