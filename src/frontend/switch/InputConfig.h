#pragma once
#include <switch.h>
#include <unordered_map>
#include <functional>

namespace InputConfig {

    inline u64 ButtonA = HidNpadButton_A;
    inline u64 ButtonB = HidNpadButton_B;
    inline u64 ButtonX = HidNpadButton_X;
    inline u64 ButtonY = HidNpadButton_Y;
    inline u64 ButtonStickL= HidNpadButton_StickL;
    inline u64 ButtonStickR = HidNpadButton_StickR;
    inline u64 ButtonL = HidNpadButton_L;
    inline u64 ButtonR = HidNpadButton_R;
    inline u64 ButtonZL = HidNpadButton_ZL;
    inline u64 ButtonZR = HidNpadButton_ZR;
    inline u64 ButtonStart = HidNpadButton_Plus;
    inline u64 ButtonSelect = HidNpadButton_Minus;
    inline u64 ButtonUp = HidNpadButton_Up;
    inline u64 ButtonDown = HidNpadButton_Down;
    inline u64 ButtonLeft = HidNpadButton_Left;
    inline u64 ButtonRight = HidNpadButton_Right;


    inline void ResetToDefault() {
        ButtonA = HidNpadButton_A;
        ButtonB = HidNpadButton_B;
        ButtonX = HidNpadButton_X;
        ButtonY = HidNpadButton_Y;
        ButtonStart = HidNpadButton_Plus;
        ButtonSelect = HidNpadButton_Minus;
        ButtonStickL= HidNpadButton_StickL;
        ButtonStickR = HidNpadButton_StickR;
        ButtonL = HidNpadButton_L;
        ButtonR = HidNpadButton_R;
        ButtonZL = HidNpadButton_ZL;
        ButtonZR = HidNpadButton_ZR;
        ButtonUp = HidNpadButton_Up;
        ButtonDown = HidNpadButton_Down;
        ButtonLeft = HidNpadButton_Left;
        ButtonRight = HidNpadButton_Right;
    }


    enum LogicalButton {
        LogicalA        = 1 << 0,
        LogicalB        = 1 << 1,
        LogicalX        = 1 << 2,
        LogicalY        = 1 << 3,
        LogicalStart    = 1 << 10,
        LogicalSelect   = 1 << 11,
        LogicalL        = 1 << 6,
        LogicalR        = 1 << 7,
        LogicalUp       = 1 << 13,
        LogicalDown     = 1 << 15,
        LogicalLeft     = 1 << 12,
        LogicalRight    = 1 << 14,
        LogicalZL       = 1 << 8,
        LogicalZR       = 1 << 9,
        LogicalStickL   = 1 << 4,
        LogicalStickR   = 1 << 5,
    };

    inline u64 GetLogicalKeysDown(u64 physicalKeysDown) {
        u64 logical = 0;
        if (physicalKeysDown & ButtonA) logical |= LogicalA;
        if (physicalKeysDown & ButtonB) logical |= LogicalB;
        if (physicalKeysDown & ButtonX) logical |= LogicalX;
        if (physicalKeysDown & ButtonY) logical |= LogicalY;
        if (physicalKeysDown & ButtonStart) logical |= LogicalStart;
        if (physicalKeysDown & ButtonSelect) logical |= LogicalSelect;
        if (physicalKeysDown & ButtonL) logical |= LogicalL;
        if (physicalKeysDown & ButtonR) logical |= LogicalR;
        if (physicalKeysDown & ButtonUp) logical |= LogicalUp;
        if (physicalKeysDown & ButtonDown) logical |= LogicalDown;
        if (physicalKeysDown & ButtonLeft) logical |= LogicalLeft;
        if (physicalKeysDown & ButtonRight) logical |= LogicalRight;
        return logical;
    }

    using LogicalAction = LogicalButton;

    u64 getPhysicalKeysFromLogical(u64 logicalKeys);


    extern std::unordered_map<LogicalAction, std::function<void()>> actionMap;
    void setupInputActions();
}
