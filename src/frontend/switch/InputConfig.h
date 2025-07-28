#pragma once
#include <switch.h>

namespace InputConfig {
    //TODO: Other keys here
    inline u64 ButtonA = HidNpadButton_A;
    inline u64 ButtonB = HidNpadButton_B;
    inline u64 ButtonX = HidNpadButton_X;
    inline u64 ButtonY = HidNpadButton_Y;

    //TODO: Other keys
    inline void ResetToDefault() {
        ButtonA = HidNpadButton_A;
        ButtonB = HidNpadButton_B;
        ButtonX = HidNpadButton_X;
        ButtonY = HidNpadButton_Y;
    }

    enum LogicalButton {
        LogicalA = 1 << 0,
        LogicalB = 1 << 1,
        LogicalX = 1 << 2,
        LogicalY = 1 << 3,
    };

    inline u64 GetLogicalKeysDown(u64 physicalKeysDown) {
        u64 logical = 0;
        if (physicalKeysDown & ButtonA) logical |= LogicalA;
        if (physicalKeysDown & ButtonB) logical |= LogicalB;
        if (physicalKeysDown & ButtonX) logical |= LogicalX;
        if (physicalKeysDown & ButtonY) logical |= LogicalY;
        return logical;
    }
}
