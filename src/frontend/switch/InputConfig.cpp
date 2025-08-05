#include <stdio.h>
#include <unordered_map>
#include <functional>
#include "InputConfig.h"

namespace InputConfig {

    std::unordered_map<LogicalAction, std::function<void()>> actionMap;

    void setupInputActions() {

        actionMap[LogicalA] = []() { };

        actionMap[LogicalB] = []() {};

        actionMap[LogicalX] = []() {};

        actionMap[LogicalY] = []() {};

        actionMap[LogicalStart] = []() {};

        actionMap[LogicalSelect] = []() {};

        actionMap[LogicalL] = []() {};

        actionMap[LogicalR] = []() {};

        actionMap[LogicalUp] = []() {};

        actionMap[LogicalDown] = []() {};

        actionMap[LogicalLeft] = []() {};

        actionMap[LogicalRight] = []() {};

        actionMap[LogicalZL] = []() {};

        actionMap[LogicalZR] = []() {};

        actionMap[LogicalStickL] = []() {};

        actionMap[LogicalStickR] = []() {};
    }


    u64 getPhysicalKeysFromLogical(u64 logicalKeys) {
        u64 physical = 0;
        if (logicalKeys & LogicalA) physical |= ButtonA;
        if (logicalKeys & LogicalB) physical |= ButtonB;
        if (logicalKeys & LogicalX) physical |= ButtonX;
        if (logicalKeys & LogicalY) physical |= ButtonY;
        if (logicalKeys & LogicalStart) physical |= ButtonStart;
        if (logicalKeys & LogicalSelect) physical |= ButtonSelect;
        if (logicalKeys & LogicalL) physical |= ButtonL;
        if (logicalKeys & LogicalR) physical |= ButtonR;
        if (logicalKeys & LogicalUp) physical |= ButtonUp;
        if (logicalKeys & LogicalDown) physical |= ButtonDown;
        if (logicalKeys & LogicalLeft) physical |= ButtonLeft;
        if (logicalKeys & LogicalRight) physical |= ButtonRight;
        return physical;
    }
}
