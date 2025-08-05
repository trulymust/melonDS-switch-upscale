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

        actionMap[LogicalStickL] = []() {};

        actionMap[LogicalStickR] = []() {};

        actionMap[LogicalL] = []() {};

        actionMap[LogicalR] = []() {};

        actionMap[LogicalZL] = []() {};

        actionMap[LogicalZR] = []() {};

        actionMap[LogicalStart] = []() {};

        actionMap[LogicalSelect] = []() {};

        actionMap[LogicalUp] = []() {};

        actionMap[LogicalDown] = []() {};

        actionMap[LogicalLeft] = []() {};

        actionMap[LogicalRight] = []() {};

        actionMap[LogicalStickLLeft] = []() {};

        actionMap[LogicalStickLUp] = []() {};

        actionMap[LogicalStickLRight] = []() {};

        actionMap[LogicalStickLDown] = []() {};

        actionMap[LogicalStickRLeft] = []() {};

        actionMap[LogicalStickRUp] = []() {};

        actionMap[LogicalStickRRight] = []() {};

        actionMap[LogicalStickRDown] = []() {};

        actionMap[LogicalLeftSL] = []() {};

        actionMap[LogicalLeftSR] = []() {};

        actionMap[LogicalRightSL] = []() {};

        actionMap[LogicalRightSL] = []() {};
    }


    u64 getPhysicalKeysFromLogical(u64 logicalKeys) {
        u64 physical = 0;
        if (logicalKeys & LogicalA) physical |= ButtonA;
        if (logicalKeys & LogicalB) physical |= ButtonB;
        if (logicalKeys & LogicalX) physical |= ButtonX;
        if (logicalKeys & LogicalY) physical |= ButtonY;

        if (logicalKeys & LogicalStickL) physical |= ButtonStickL;
        if (logicalKeys & LogicalStickR) physical |= ButtonStickR;

        if (logicalKeys & LogicalL) physical |= ButtonL;
        if (logicalKeys & LogicalR) physical |= ButtonR;
        if (logicalKeys & LogicalZL) physical |= ButtonZL;
        if (logicalKeys & LogicalZR) physical |= ButtonZR;

        if (logicalKeys & LogicalStart) physical |= ButtonStart;
        if (logicalKeys & LogicalSelect) physical |= ButtonSelect;
        
        if (logicalKeys & LogicalUp) physical |= ButtonUp;
        if (logicalKeys & LogicalDown) physical |= ButtonDown;
        if (logicalKeys & LogicalLeft) physical |= ButtonLeft;
        if (logicalKeys & LogicalRight) physical |= ButtonRight;

        if (logicalKeys & LogicalStickLLeft) physical |= ButtonStickLLeft;
        if (logicalKeys & LogicalStickLUp) physical |= ButtonStickLUp;
        if (logicalKeys & LogicalStickLRight) physical |= ButtonStickLRight;
        if (logicalKeys & LogicalStickLDown) physical |= ButtonStickLDown;

        if (logicalKeys & LogicalStickRLeft) physical |= ButtonStickRLeft;
        if (logicalKeys & LogicalStickRUp) physical |= ButtonStickRUp;
        if (logicalKeys & LogicalStickRRight) physical |= ButtonStickRRight;
        if (logicalKeys & LogicalStickRDown) physical |= ButtonStickRDown;

        if (logicalKeys & LogicalLeftSL) physical |= ButtonLeftSL;
        if (logicalKeys & LogicalLeftSR) physical |= ButtonLeftSR;

        if (logicalKeys & LogicalRightSL) physical |= ButtonRightSL;
        if (logicalKeys & LogicalRightSR) physical |= ButtonRightSR;

        return physical;
    }
}
