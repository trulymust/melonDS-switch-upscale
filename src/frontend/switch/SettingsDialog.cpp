#include "SettingsDialog.h"

#include "Style.h"
#include "KeyExplanations.h"
#include "BackButton.h"
#include "main.h"

#include <switch.h>

#include "GPU.h"
#include "PlatformConfig.h"
#include "InputConfig.h"
#include "ARCodeFile.h"
#include "Localization.h"
#include "../FrontendUtil.h"

#include <string.h>

#include "RetroAchievements.h"
#include "NotificationSystem.h"
#include "ErrorDialog.h"

namespace {
    static u64 PlatformKeysHeld = 0;
    static u64 PlatformKeysDown = 0;
    static u64 PreviousKeys = 0;
    static int AppliedUpscaleFactor = -1;

    static const char* Tr(const char* text)
    {
        return Localization::Text(text);
    }
}

static PadState pad;

namespace SettingsDialog
{
const char* SettingsPrefix = "settingsdialog_entries";

const char* ComboboxElementPrefix = "settings_combobox";

void DoSlider(BoxGui::Frame& parent, BoxGui::Skewer& skewer, const char* name, int& value, int low, int high, bool first = false)
{
    BoxGui::Frame settingFrame{parent, skewer.Spit({parent.Area.Size.X, UIRowHeight}, Gfx::align_Right),
        {5.f, 5.f}, {5.f, 5.f}};

    bool selected = BoxGui::InputElement(settingFrame, BoxGui::MakeUniqueName(SettingsPrefix, BoxGui::MakeUniqueName(name, 0)));
    if (selected && BoxGui::LeftPressed())
    {
        value--;
        if (value < low)
            value = low;
    }
    if (selected && BoxGui::RightPressed())
    {
        value++;
        if (value > high)
            value = high;
    }

    // a bit wasteful
    Gfx::DrawRectangle(settingFrame.Area.Position - Gfx::Vector2f{0.f, 5.f}, settingFrame.Area.Size + Gfx::Vector2f{0.f, 5.f*2.f}, WidgetColorBright, true);

    if (selected)
        Gfx::DrawRectangle(settingFrame.Area.Position, settingFrame.Area.Size, WidgetColorVibrant);

    BoxGui::Skewer settingSkewer{settingFrame, settingFrame.Area.Size.Y/2.f, BoxGui::direction_Horizontal};

    settingSkewer.AlignLeft(15.f);
    BoxGui::Frame nameFrame{settingFrame, settingSkewer.Spit({settingFrame.Area.Size.X*0.8f, TextLineHeight})};
    Gfx::DrawText(Gfx::SystemFontStandard, nameFrame.Area.Position, TextLineHeight, DarkColor, "%s", name);

    settingSkewer.AlignRight(20.f);
    BoxGui::Frame valueFrame{settingFrame, settingSkewer.Spit({30.f, TextLineHeight})};
    Gfx::DrawText(Gfx::SystemFontStandard, valueFrame.Area.Position, TextLineHeight, DarkColor, "%d", value);

    settingSkewer.Advance(15.f);

    BoxGui::Frame bar{settingFrame, settingSkewer.Spit({150.f, 5.f})};
    Gfx::DrawRectangle(bar.Area.Position, bar.Area.Size, DarkColor);

    Gfx::DrawRectangle(bar.Area.Position + Gfx::Vector2f({(float)(value - low) / (high - low + 1) * bar.Area.Size.X, bar.Area.Size.Y/2.f-TextLineHeight/2.f}), {10.f, TextLineHeight}, DarkColor);

    if (!first)
    {
        Gfx::DrawRectangle(settingFrame.Area.Position + Gfx::Vector2f{10.f, -(5.f + 1.f)},
            {settingFrame.Area.Size.X - 2*10.f, 2.f},
            SeparatorColor);
    }
}

void DoCheckbox(BoxGui::Frame& parent, BoxGui::Skewer& skewer, const char* name, bool& value, bool first = false)
{
    BoxGui::Frame settingFrame{parent, skewer.Spit({parent.Area.Size.X, UIRowHeight}, Gfx::align_Right),
        {5.f, 5.f}, {5.f, 5.f}};

    bool selected = BoxGui::InputElement(settingFrame, BoxGui::MakeUniqueName(SettingsPrefix, BoxGui::MakeUniqueName(name, 0)));
    if (selected && BoxGui::ConfirmPressed())
    {
        value ^= true;
    }
    if (selected)
    {
        KeyExplanation::Explain(KeyExplanation::button_A, Tr("Toggle"));
    }

    // a bit wasteful
    Gfx::DrawRectangle(settingFrame.Area.Position - Gfx::Vector2f{0.f, 5.f}, settingFrame.Area.Size + Gfx::Vector2f{0.f, 5.f*2.f}, WidgetColorBright, true);

    if (selected)
        Gfx::DrawRectangle(settingFrame.Area.Position, settingFrame.Area.Size, WidgetColorVibrant);

    BoxGui::Skewer settingSkewer{settingFrame, settingFrame.Area.Size.Y/2.f, BoxGui::direction_Horizontal};

    settingSkewer.AlignLeft(15.f);
    BoxGui::Frame nameFrame{settingFrame, settingSkewer.Spit({settingFrame.Area.Size.X*0.8f, TextLineHeight})};
    Gfx::DrawText(Gfx::SystemFontStandard, nameFrame.Area.Position, TextLineHeight, DarkColor, "%s", name);

    settingSkewer.AlignRight(25.f);
    BoxGui::Frame checkMarkFrame{settingFrame, settingSkewer.Spit({TextLineHeight, TextLineHeight})};

    Gfx::DrawText(Gfx::SystemFontNintendoExt,
        checkMarkFrame.Area.Position, TextLineHeight,
        DarkColor,
        value ? GFX_NINTENDOFONT_CHECKMARK : GFX_NINTENDOFONT_CROSS);

    if (!first)
    {
        Gfx::DrawRectangle(settingFrame.Area.Position + Gfx::Vector2f{10.f, -(5.f + 1.f)},
            {settingFrame.Area.Size.X - 2*10.f, 2.f},
            SeparatorColor);
    }
}

bool DoCheckboxWithId(BoxGui::Frame& parent, BoxGui::Skewer& skewer, const char* name, bool& value, int uniqueId, bool first = false)
{
    BoxGui::Frame settingFrame{parent, skewer.Spit({parent.Area.Size.X, UIRowHeight}, Gfx::align_Right),
        {5.f, 5.f}, {5.f, 5.f}};

    bool selected = BoxGui::InputElement(settingFrame, BoxGui::MakeUniqueName("settingsdialog_checkbox", uniqueId));
    bool changed = false;
    if (selected && BoxGui::ConfirmPressed())
    {
        value ^= true;
        changed = true;
    }
    if (selected)
    {
        KeyExplanation::Explain(KeyExplanation::button_A, Tr("Toggle"));
    }

    Gfx::DrawRectangle(settingFrame.Area.Position - Gfx::Vector2f{0.f, 5.f}, settingFrame.Area.Size + Gfx::Vector2f{0.f, 5.f*2.f}, WidgetColorBright, true);

    if (selected)
        Gfx::DrawRectangle(settingFrame.Area.Position, settingFrame.Area.Size, WidgetColorVibrant);

    BoxGui::Skewer settingSkewer{settingFrame, settingFrame.Area.Size.Y/2.f, BoxGui::direction_Horizontal};

    settingSkewer.AlignLeft(15.f);
    BoxGui::Frame nameFrame{settingFrame, settingSkewer.Spit({settingFrame.Area.Size.X*0.8f, TextLineHeight})};
    Gfx::DrawText(Gfx::SystemFontStandard, nameFrame.Area.Position, TextLineHeight, DarkColor, "%s", name);

    settingSkewer.AlignRight(25.f);
    BoxGui::Frame checkMarkFrame{settingFrame, settingSkewer.Spit({TextLineHeight, TextLineHeight})};

    Gfx::DrawText(Gfx::SystemFontNintendoExt,
        checkMarkFrame.Area.Position, TextLineHeight,
        DarkColor,
        value ? GFX_NINTENDOFONT_CHECKMARK : GFX_NINTENDOFONT_CROSS);

    if (!first)
    {
        Gfx::DrawRectangle(settingFrame.Area.Position + Gfx::Vector2f{10.f, -(5.f + 1.f)},
            {settingFrame.Area.Size.X - 2*10.f, 2.f},
            SeparatorColor);
    }

    return changed;
}

void DoCombobox(BoxGui::Frame& parent, BoxGui::Skewer& skewer, const char* name, const char* options, int& selectedOption, bool first = false)
{
    BoxGui::Frame settingFrame{parent, skewer.Spit({parent.Area.Size.X, UIRowHeight}, Gfx::align_Right),
        {5.f, 5.f}, {5.f, 5.f}};

    bool selected = BoxGui::InputElement(settingFrame, BoxGui::MakeUniqueName(SettingsPrefix, BoxGui::MakeUniqueName(name, 0)));
    if (selected && BoxGui::ConfirmPressed())
    {
        struct Dialog
        {
            int OriginalValue;
            int& SelectedOption;
            const char* Name, *Options;
            double StartTimestamp;
            double EndTimestamp = -INFINITY;

            bool operator()(BoxGui::Frame& rootFrame)
            {
                Gfx::Color color = DarkColor;
                // fade in
                color.A = (float)std::min((Gfx::AnimationTimestamp - StartTimestamp) * 5.0, 0.8);
                Gfx::DrawRectangle(rootFrame.Area.Position, rootFrame.Area.Size, color);

                Gfx::Vector2f Size = rootFrame.Area.Size * 0.9f;
                Size.X = std::min(Size.X, 720.f);
                BoxGui::Frame dialogFrame{rootFrame, rootFrame.Area.CenteredChild(Size)};

                Gfx::DrawRectangle(dialogFrame.Area.Position, dialogFrame.Area.Size, WidgetColorBright, true);

                BoxGui::Skewer optionsSkewer{dialogFrame, 0.f, BoxGui::direction_Vertical};
                optionsSkewer.AlignLeft(30.f);
                BoxGui::Frame titleFrame{dialogFrame, optionsSkewer.Spit({dialogFrame.Area.Size.X, TextLineHeight * 2.f}, Gfx::align_Right), {5.f, 5.f}, {5.f, 5.f}};
                Gfx::DrawText(Gfx::SystemFontStandard, titleFrame.Area.Position + Gfx::Vector2f{15.f, 0.f}, TextLineHeight * 2.f, DarkColor,
                    Gfx::align_Left, Gfx::align_Left, Name);
                optionsSkewer.Advance(10.f);

                const char* curOption = Options;
                int i = 0;
                while (true)
                {
                    BoxGui::Frame optionFrame{dialogFrame,
                        optionsSkewer.Spit({dialogFrame.Area.Size.X, UIRowHeight}, Gfx::align_Right), {5.f, 5.f}, {5.f, 5.f}};

                    bool selected = BoxGui::InputElement(optionFrame, BoxGui::MakeUniqueName(ComboboxElementPrefix, i));
                    
                    if (selected && BoxGui::ConfirmPressed() && EndTimestamp < 0.0)
                    {
                        SelectedOption = i;
                        if (SelectedOption != OriginalValue)
                            EndTimestamp = Gfx::AnimationTimestamp;
                        else
                            EndTimestamp = 0.f;
                    } 
                    if (selected)
                        Gfx::DrawRectangle(optionFrame.Area.Position, optionFrame.Area.Size, WidgetColorVibrant);

                    BoxGui::Skewer optionSkewer{optionFrame, optionFrame.Area.Size.Y/2.f, BoxGui::direction_Horizontal};
                    if (SelectedOption == i)
                    {
                        optionSkewer.AlignLeft(20.f);
                        Gfx::DrawText(Gfx::SystemFontNintendoExt, optionSkewer.CurrentPosition(), TextLineHeight, DarkColor,
                            Gfx::align_Left, Gfx::align_Center, GFX_NINTENDOFONT_CHECKMARK);

                        KeyExplanation::Explain(KeyExplanation::button_A, Tr("Choose"));
                    }
                    optionSkewer.AlignLeft(50.f);

                    Gfx::DrawText(Gfx::SystemFontStandard, optionSkewer.CurrentPosition(), TextLineHeight, DarkColor,
                        Gfx::align_Left, Gfx::align_Center, curOption);

                    if (i > 0)
                    {
                        Gfx::DrawRectangle(optionFrame.Area.Position + Gfx::Vector2f{10.f, -(5.f + 1.f)},
                            {optionFrame.Area.Size.X - 2*10.f, 2.f},
                            SeparatorColor);
                    }

                    i++;
                    curOption += strlen(curOption) + 1; // skip the \0
                    if (*curOption == '\0')
                        break;
                }

                if (SelectedOption >= i)
                    SelectedOption = 0;

                KeyExplanation::Explain(KeyExplanation::button_B, Tr("Cancel"));

                const double fadeoutLength = 0.25;
                if (BoxGui::CancelPressed())
                {
                    EndTimestamp = 0.f;
                }

                KeyExplanation::DoGui(rootFrame);

                // don't close the dialog immediately
                // instead wait a moment so the user can reflect on their choice :D
                return EndTimestamp < 0.0 || Gfx::AnimationTimestamp - EndTimestamp < fadeoutLength;
            }
        };

        BoxGui::OpenModalDialog(Dialog{selectedOption, selectedOption, name, options, Gfx::AnimationTimestamp});
        BoxGui::ForceSelecton(BoxGui::MakeUniqueName(ComboboxElementPrefix, selectedOption), true, 1);
    }
    if (selected)
    {
        KeyExplanation::Explain(KeyExplanation::button_A, Tr("Choose"));
    }

    Gfx::DrawRectangle(settingFrame.Area.Position - Gfx::Vector2f{0.f, 5.f}, settingFrame.Area.Size + Gfx::Vector2f{0.f, 5.f*2.f}, WidgetColorBright, true);
    if (selected)
        Gfx::DrawRectangle(settingFrame.Area.Position, settingFrame.Area.Size, WidgetColorVibrant);

    BoxGui::Skewer settingSkewer{settingFrame, settingFrame.Area.Size.Y/2.f, BoxGui::direction_Horizontal};

    settingSkewer.AlignLeft(20.f);
    Gfx::DrawText(Gfx::SystemFontStandard, settingSkewer.CurrentPosition(), TextLineHeight, DarkColor,
        Gfx::align_Left, Gfx::align_Center, name);

    const char* selectedOptionName = options;
    for (u32 i = 0; i < selectedOption; i++)
    {
        selectedOptionName += strlen(selectedOptionName) + 1;
        if (*selectedOptionName == '\0')
        {
            selectedOptionName = "Error!!!";
            break;
        }
    }
    settingSkewer.AlignRight(20.f);
    Gfx::DrawText(Gfx::SystemFontStandard, settingSkewer.CurrentPosition(), TextLineHeight, DarkColor,
        Gfx::align_Right, Gfx::align_Center, selectedOptionName);

    if (!first)
    {
        Gfx::DrawRectangle(settingFrame.Area.Position + Gfx::Vector2f{10.f, -(5.f + 1.f)},
            {settingFrame.Area.Size.X - 2*10.f, 2.f},
            SeparatorColor);
    }
}

void SectionHeader(BoxGui::Frame& parent, BoxGui::Skewer& skewer, const char* name)
{
    const float height = TextLineHeight * 2.f;
    skewer.Advance(height);
    BoxGui::Frame nameFrame{parent, skewer.Spit({parent.Area.Size.X * 0.6f, height}, Gfx::align_Right), {5.f, 0.f}, {5.f, 15.f}};

    Gfx::DrawRectangle(nameFrame.Area.Position - Gfx::Vector2f{0.f, 15.f}, nameFrame.Area.Size + Gfx::Vector2f{0.f, 15.f*2.f}, WidgetColorBright, true);
    Gfx::DrawText(Gfx::SystemFontStandard, nameFrame.Area.Position + Gfx::Vector2f{10.f, 0.f}, height, DarkColor, "%s", name);
}

void DoTextField(BoxGui::Frame& parent, BoxGui::Skewer& skewer, const char* label, char* buffer, size_t bufferSize, bool first = false)
{
    BoxGui::Frame settingFrame{parent, skewer.Spit({parent.Area.Size.X, UIRowHeight}, Gfx::align_Right),
        {5.f, 5.f}, {5.f, 5.f}};

    bool selected = BoxGui::InputElement(settingFrame, BoxGui::MakeUniqueName(SettingsPrefix, BoxGui::MakeUniqueName(label, 0)));
    if (selected && BoxGui::ConfirmPressed())
    {
        SwkbdConfig kbd;
        Result rc = swkbdCreate(&kbd, 0);
        if (R_SUCCEEDED(rc)) {
            swkbdConfigMakePresetDefault(&kbd);
            swkbdConfigSetInitialText(&kbd, buffer);
            swkbdConfigSetTextCheckCallback(&kbd, NULL);
    
            char out[bufferSize];
            rc = swkbdShow(&kbd, out, bufferSize);
            if (R_SUCCEEDED(rc)) {
                strncpy(buffer, out, bufferSize - 1);
                buffer[bufferSize - 1] = '\0';
            }
            swkbdClose(&kbd);
        }
    }

    if (selected)
    {
        KeyExplanation::Explain(KeyExplanation::button_A, Tr("Edit"));
    }

    Gfx::DrawRectangle(settingFrame.Area.Position - Gfx::Vector2f{0.f, 5.f}, settingFrame.Area.Size + Gfx::Vector2f{0.f, 5.f*2.f}, WidgetColorBright, true);
    if (selected)
        Gfx::DrawRectangle(settingFrame.Area.Position, settingFrame.Area.Size, WidgetColorVibrant);

    BoxGui::Skewer settingSkewer{settingFrame, settingFrame.Area.Size.Y/2.f, BoxGui::direction_Horizontal};

    settingSkewer.AlignLeft(20.f);
    Gfx::DrawText(Gfx::SystemFontStandard, settingSkewer.CurrentPosition(), TextLineHeight, DarkColor,
        Gfx::align_Left, Gfx::align_Center, label);

    settingSkewer.AlignRight(20.f);
    Gfx::DrawText(Gfx::SystemFontStandard, settingSkewer.CurrentPosition(), TextLineHeight, DarkColor,
        Gfx::align_Right, Gfx::align_Center, buffer);

    if (!first)
    {
        Gfx::DrawRectangle(settingFrame.Area.Position + Gfx::Vector2f{10.f, -(5.f + 1.f)},
            {settingFrame.Area.Size.X - 2*10.f, 2.f},
            SeparatorColor);
    }
}

void DoLabel(BoxGui::Frame& parent, BoxGui::Skewer& skewer, const char* text, bool first = false)
{
    BoxGui::Frame settingFrame{
        parent,
        skewer.Spit({parent.Area.Size.X, UIRowHeight}, Gfx::align_Right),
        {5.f, 5.f},
        {5.f, 5.f}
    };

    bool selected = BoxGui::InputElement(settingFrame, BoxGui::MakeUniqueName(SettingsPrefix, BoxGui::MakeUniqueName(text, 0)));

    Gfx::DrawRectangle(settingFrame.Area.Position - Gfx::Vector2f{0.f, 5.f},
                       settingFrame.Area.Size + Gfx::Vector2f{0.f, 5.f * 2.f},
                       WidgetColorBright, true);

    if (selected)
        Gfx::DrawRectangle(settingFrame.Area.Position, settingFrame.Area.Size, WidgetColorVibrant);
               

    BoxGui::Skewer settingSkewer{settingFrame, settingFrame.Area.Size.Y / 2.f, BoxGui::direction_Horizontal};

    settingSkewer.AlignLeft(20.f);
    Gfx::DrawText(Gfx::SystemFontStandard,
                  settingSkewer.CurrentPosition(),
                  TextLineHeight,
                  DarkColor,
                  Gfx::align_Left,
                  Gfx::align_Center,
                  text);

    if (!first)
    {
        Gfx::DrawRectangle(settingFrame.Area.Position + Gfx::Vector2f{10.f, -(5.f + 1.f)},
                           {settingFrame.Area.Size.X - 2 * 10.f, 2.f},
                           SeparatorColor);
    }
}

const char* ButtonToString(u64 buttons)
{
    static std::string result;
    result.clear();
    bool first = true;

    if (buttons & HidNpadButton_A) {
        if (!first) result += " + ";
        result += "A";
        first = false;
    }
    if (buttons & HidNpadButton_B) {
        if (!first) result += " + ";
        result += "B";
        first = false;
    }
    if (buttons & HidNpadButton_X) {
        if (!first) result += " + ";
        result += "X";
        first = false;
    }
    if (buttons & HidNpadButton_Y) {
        if (!first) result += " + ";
        result += "Y";
        first = false;
    }
    if (buttons & HidNpadButton_StickL) {
        if (!first) result += " + ";
        result += "L Stick Button";
        first = false;
    }
    if (buttons & HidNpadButton_StickR) {
        if (!first) result += " + ";
        result += "R Stick Button";
        first = false;
    }
    if (buttons & HidNpadButton_L) {
        if (!first) result += " + ";
        result += "L";
        first = false;
    }
    if (buttons & HidNpadButton_R) {
        if (!first) result += " + ";
        result += "R";
        first = false;
    }
    if (buttons & HidNpadButton_ZL) {
        if (!first) result += " + ";
        result += "ZL";
        first = false;
    }
    if (buttons & HidNpadButton_ZR) {
        if (!first) result += " + ";
        result += "ZR";
        first = false;
    }
    if (buttons & HidNpadButton_Plus) {
        if (!first) result += " AND ";
        result += "+";
        first = false;
    }
    if (buttons & HidNpadButton_Minus) {
        if (!first) result += " + ";
        result += "-";
        first = false;
    }
    if (buttons & HidNpadButton_Up) {
        if (!first) result += " + ";
        result += "D-Pad UP";
        first = false;
    }
    if (buttons & HidNpadButton_Down) {
        if (!first) result += " + ";
        result += "D-Pad Down";
        first = false;
    }
    if (buttons & HidNpadButton_Right) {
        if (!first) result += " + ";
        result += "D-Pad Right";
        first = false;
    }
    if (buttons & HidNpadButton_Left) {
        if (!first) result += " + ";
        result += "D-Pad Left";
        first = false;
    }
    if (buttons & HidNpadButton_StickLLeft) {
        if (!first) result += " + ";
        result += "L Stick Left";
        first = false;
    }
    if (buttons & HidNpadButton_StickLUp) {
        if (!first) result += " + ";
        result += "L Stick Up";
        first = false;
    }
    if (buttons & HidNpadButton_StickLRight) {
        if (!first) result += " + ";
        result += "L Stick Right";
        first = false;
    }
    if (buttons & HidNpadButton_StickLDown) {
        if (!first) result += " + ";
        result += "L Stick Down";
        first = false;
    }
    if (buttons & HidNpadButton_StickRLeft) {
        if (!first) result += " + ";
        result += "R Stick Left";
        first = false;
    }
    if (buttons & HidNpadButton_StickRUp) {
        if (!first) result += " + ";
        result += "R Stick Up";
        first = false;
    }
    if (buttons & HidNpadButton_StickRRight) {
        if (!first) result += " + ";
        result += "R Stick Right";
        first = false;
    }
    if (buttons & HidNpadButton_StickRDown) {
        if (!first) result += " + ";
        result += "R Stick Down";
        first = false;
    }
    if (buttons & HidNpadButton_LeftSL) {
        if (!first) result += " + ";
        result += "Left SL";
        first = false;
    }
    if (buttons & HidNpadButton_LeftSR) {
        if (!first) result += " + ";
        result += "Left SR";
        first = false;
    }
    if (buttons & HidNpadButton_RightSL) {
        if (!first) result += " + ";
        result += "Right SL";
        first = false;
    }
    if (buttons & HidNpadButton_RightSR) {
        if (!first) result += " + ";
        result += "Right SR";
        first = false;
    }

    if (result.empty()) {
        return "•͡˘㇁•͡˘";
    }

    return result.c_str();
}


void DoInputButton(BoxGui::Frame& parent, BoxGui::Skewer& skewer, const char* name, u64& mappedKey, bool first = false)
{
    BoxGui::Frame settingFrame{parent, skewer.Spit({parent.Area.Size.X, UIRowHeight}, Gfx::align_Right),
        {5.f, 5.f}, {5.f, 5.f}};

    bool selected = BoxGui::InputElement(settingFrame, BoxGui::MakeUniqueName(SettingsPrefix, BoxGui::MakeUniqueName(name, 0)));
    if (selected && BoxGui::ConfirmPressed())
    {
        struct Dialog
        {
            const char* Name;
            u64& MappedKey;
            double StartTimestamp;
            double EndTimestamp = -INFINITY;
            bool inputCaptured = false;

            bool operator()(BoxGui::Frame& rootFrame)
            {
                Gfx::Color color = DarkColor;
                color.A = (float)std::min((Gfx::AnimationTimestamp - StartTimestamp) * 5.0, 0.8);
                Gfx::DrawRectangle(rootFrame.Area.Position, rootFrame.Area.Size, color);

                Gfx::Vector2f Size = rootFrame.Area.Size * 0.9f;
                Size.X = std::min(Size.X, 720.f);
                BoxGui::Frame dialogFrame{rootFrame, rootFrame.Area.CenteredChild(Size)};
                Gfx::DrawRectangle(dialogFrame.Area.Position, dialogFrame.Area.Size, WidgetColorBright, true);

                BoxGui::Skewer dialogSkewer{dialogFrame, 0.f, BoxGui::direction_Vertical};
                dialogSkewer.AlignLeft(30.f);

                BoxGui::Frame titleFrame{dialogFrame, dialogSkewer.Spit({dialogFrame.Area.Size.X, TextLineHeight * 2.f}, Gfx::align_Right), {5.f, 5.f}, {5.f, 5.f}};
                Gfx::DrawText(Gfx::SystemFontStandard, titleFrame.Area.Position + Gfx::Vector2f{15.f, 0.f}, TextLineHeight * 2.f, DarkColor,
                    Gfx::align_Left, Gfx::align_Left, Name);

                dialogSkewer.Advance(10.f);

                BoxGui::Frame msgFrame{dialogFrame, dialogSkewer.Spit({dialogFrame.Area.Size.X, TextLineHeight * 2.f}, Gfx::align_Right), {5.f, 5.f}, {5.f, 5.f}};
                Gfx::DrawText(Gfx::SystemFontStandard, msgFrame.Area.Position + Gfx::Vector2f{15.f, 0.f}, TextLineHeight * 1.5f, DarkColor,
                    Gfx::align_Left, Gfx::align_Left, Tr("Waiting for input..."));

                const double elapsedInput = Gfx::AnimationTimestamp - StartTimestamp;

                if (!inputCaptured && elapsedInput > 0.3)
                {
                    static PadState pad;
                    static bool padInitialized = false;
                    if (!padInitialized) {
                        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
                        padInitializeAny(&pad);
                        padInitialized = true;
                    }
                    padUpdate(&pad);
                    u64 keys = padGetButtonsDown(&pad);
                    if (keys != 0) {
                        MappedKey = keys;
                        inputCaptured = true;
                        EndTimestamp = Gfx::AnimationTimestamp;
                    }
                }

                const double elapsed = Gfx::AnimationTimestamp - StartTimestamp;
                if (elapsed > 10.0 || BoxGui::CancelPressed())
                    EndTimestamp = 0.f;

                KeyExplanation::Explain(KeyExplanation::button_B, Tr("Cancel"));
                KeyExplanation::DoGui(rootFrame);

                const double fadeoutLength = 0.25;
                return EndTimestamp < 0.0 || Gfx::AnimationTimestamp - EndTimestamp < fadeoutLength;
            }
        };

        BoxGui::OpenModalDialog(Dialog{name, mappedKey, Gfx::AnimationTimestamp});
    }

    if (selected)
    {
        KeyExplanation::Explain(KeyExplanation::button_A, Tr("Remap"));
    }

    Gfx::DrawRectangle(settingFrame.Area.Position - Gfx::Vector2f{0.f, 5.f}, settingFrame.Area.Size + Gfx::Vector2f{0.f, 5.f*2.f}, WidgetColorBright, true);
    if (selected)
        Gfx::DrawRectangle(settingFrame.Area.Position, settingFrame.Area.Size, WidgetColorVibrant);

    BoxGui::Skewer settingSkewer{settingFrame, settingFrame.Area.Size.Y / 2.f, BoxGui::direction_Horizontal};

    settingSkewer.AlignLeft(20.f);
    Gfx::DrawText(Gfx::SystemFontStandard, settingSkewer.CurrentPosition(), TextLineHeight, DarkColor,
        Gfx::align_Left, Gfx::align_Center, name);

    const char* keyName = ButtonToString(mappedKey);
    settingSkewer.AlignRight(20.f);
    Gfx::DrawText(Gfx::SystemFontStandard, settingSkewer.CurrentPosition(), TextLineHeight, DarkColor,
        Gfx::align_Right, Gfx::align_Center, keyName);

    if (!first)
    {
        Gfx::DrawRectangle(settingFrame.Area.Position + Gfx::Vector2f{10.f, -(5.f + 1.f)},
            {settingFrame.Area.Size.X - 2*10.f, 2.f},
            SeparatorColor);
    }
}

void ShowImage(BoxGui::Frame& parent, BoxGui::Skewer& skewer, int textureId, int nwidth, int nheight, float imageSize = 64.f)
{
    if (textureId < 0 || nwidth <= 0 || nheight <= 0)
        return;

    BoxGui::Frame settingFrame{
        parent,
        skewer.Spit({parent.Area.Size.X, imageSize + 10.f}, Gfx::align_Right),
        {5.f, 5.f},
        {5.f, 5.f}
    };

    BoxGui::Skewer settingSkewer{settingFrame, settingFrame.Area.Size.Y / 2.f, BoxGui::direction_Horizontal};
    settingSkewer.AlignLeft(20.f);

    Gfx::Vector2f avatarPos = settingSkewer.CurrentPosition();
    Gfx::Vector2f avatarDrawSize = {imageSize, imageSize};

    Gfx::DrawRectangle(textureId, avatarPos, avatarDrawSize,
                       {0.f, 0.f},
                       {static_cast<float>(nwidth), static_cast<float>(nheight)},
                       WidgetColorBright);
}

void DoGui(BoxGui::Frame& parent)
{
    BoxGui::Frame settingsFrame{parent,
        {{0.f, BackButtonHeight}, {parent.Area.Size.X, parent.Area.Size.Y - BackButtonHeight}},
        {0.f, 0.f}, {0.f, 0.f},
        BoxGui::direction_Vertical, BoxGui::MakeUniqueName(SettingsPrefix, -1), false, true};

    BoxGui::Skewer settingsSkewer{settingsFrame, 0.f, BoxGui::direction_Vertical};

    Gfx::PushScissor(settingsFrame.Area.Position.X, settingsFrame.Area.Position.Y, settingsFrame.Area.Size.X, settingsFrame.Area.Size.Y);

    const char* title = "error";
    switch (CurrentUiScreen)
    {
    case uiScreen_EmulationSettings:
        title = Tr("Emulation settings");
        {
            SectionHeader(settingsFrame, settingsSkewer, Tr("General"));
            DoCombobox(settingsFrame, settingsSkewer, Tr("Console mode"), Localization::Options(Localization::OptionList::ConsoleMode), Config::ConsoleType, true);
            if (Config::ConsoleType == 0)
            {
                bool bootDirectly = Config::DirectBoot;
                DoCheckbox(settingsFrame, settingsSkewer, Tr("Boot directly (Skip bios)"), bootDirectly);
                Config::DirectBoot = bootDirectly;
            }
            DoCombobox(settingsFrame, settingsSkewer, Tr("Switch CPU clock"), Localization::Options(Localization::OptionList::SwitchCpuClock), Config::SwitchOverclock);
        }
        {
            bool jitEnable = Config::JIT_Enable;
            SectionHeader(settingsFrame, settingsSkewer, Tr("JIT recompiler"));
            bool branchOptimisations = Config::JIT_BranchOptimisations;
            bool literalOptimisations = Config::JIT_LiteralOptimisations;
            bool fastMemory = Config::JIT_FastMemory;

            DoCheckbox(settingsFrame, settingsSkewer, Tr("Enable JIT recompiler"), jitEnable, true);
            if (jitEnable)
            {
                DoSlider(settingsFrame, settingsSkewer, Tr("Maximum block size"), Config::JIT_MaxBlockSize, 1, 32);
                DoCheckbox(settingsFrame, settingsSkewer, Tr("Enable JIT Branch Optimisations"), branchOptimisations);
                DoCheckbox(settingsFrame, settingsSkewer, Tr("Enable JIT Literal Optimisations"), literalOptimisations);
                DoCheckbox(settingsFrame, settingsSkewer, Tr("Enable JIT Fast Memory"), fastMemory);
            }

            Config::JIT_Enable = jitEnable;
            Config::JIT_BranchOptimisations = branchOptimisations;
            Config::JIT_LiteralOptimisations = literalOptimisations;
            Config::JIT_FastMemory = fastMemory;
        }
        {
            bool loginRA = false, hardcore = Config::hardcoreMode, notification = Config::notification;
            int status = 0;
            static char username[64] = {0}, password[64] = {0};

            if (strlen(Config::RetroAchievementsUsername) > 0 && strlen(username) == 0) {
                strncpy(username, Config::RetroAchievementsUsername, sizeof(username) - 1);
                username[sizeof(username) - 1] = '\0';
            }

            SectionHeader(settingsFrame, settingsSkewer, Tr("RetroAchievements"));
            DoTextField(settingsFrame, settingsSkewer, Tr("RetroAchievements Username"), username, sizeof(username));
            DoTextField(settingsFrame, settingsSkewer, Tr("RetroAchievements Password"), password, sizeof(password));
            DoCheckbox(settingsFrame, settingsSkewer, Tr("Login"), loginRA);
            if (loginRA)
                InitRetroAchievements(username, password, false);

            DoCheckbox(settingsFrame, settingsSkewer, Tr("Hardcore Mode"), hardcore);
            Config::hardcoreMode = hardcore;

            DoCheckbox(settingsFrame, settingsSkewer, Tr("Disable RA notifications"), notification);
            Config::notification = notification;
        }
        break;
    case uiScreen_DisplaySettings:
        title = Tr("Presentation settings");
        {
            SectionHeader(settingsFrame, settingsSkewer, Tr("Framerate"));
            bool limitFramerate = Config::LimitFramerate;
            DoCheckbox(settingsFrame, settingsSkewer, Tr("Limit framerate"), limitFramerate, true);
            Config::LimitFramerate = limitFramerate;
        }
        {
            SectionHeader(settingsFrame, settingsSkewer, Tr("GUI"));

            if (Config::Language < 0 || Config::Language >= Localization::Language_Max)
                Config::Language = Localization::Language_English;
            DoCombobox(settingsFrame, settingsSkewer, Tr("Language"), Localization::Options(Localization::OptionList::Language), Config::Language, true);
            if (Config::Language < 0 || Config::Language >= Localization::Language_Max)
                Config::Language = Localization::Language_English;
            DoCombobox(settingsFrame, settingsSkewer, Tr("Global rotation"), Localization::Options(Localization::OptionList::Rotation), Config::GlobalRotation);
            bool showPerformanceMetrics = Config::ShowPerformanceMetrics;
            DoCheckbox(settingsFrame, settingsSkewer, Tr("Show performance metrics"), showPerformanceMetrics);
            Config::ShowPerformanceMetrics = showPerformanceMetrics;
        }
        {
            SectionHeader(settingsFrame, settingsSkewer, Tr("Screens"));

            DoCombobox(settingsFrame, settingsSkewer, Tr("Rotation"), Localization::Options(Localization::OptionList::Rotation), Config::ScreenRotation, true);
            DoCombobox(settingsFrame, settingsSkewer, Tr("Sizing"), Localization::Options(Localization::OptionList::ScreenSizing), Config::ScreenSizing);
            DoCombobox(settingsFrame, settingsSkewer, Tr("Gap"), Localization::Options(Localization::OptionList::ScreenGap), Config::ScreenGap);
            DoCombobox(settingsFrame, settingsSkewer, Tr("Layout"), Localization::Options(Localization::OptionList::ScreenLayout), Config::ScreenLayout);
            DoCombobox(settingsFrame, settingsSkewer, Tr("Aspect ratio top"), Localization::Options(Localization::OptionList::AspectRatio), Config::ScreenAspectTop);
            DoCombobox(settingsFrame, settingsSkewer, Tr("Aspect ratio bottom"), Localization::Options(Localization::OptionList::AspectRatio), Config::ScreenAspectBot);
            bool screenSwap = Config::ScreenSwap;
            DoCheckbox(settingsFrame, settingsSkewer, Tr("Swap screens"), screenSwap);
            Config::ScreenSwap = screenSwap;
            bool integerScaling = Config::IntegerScaling;
            DoCheckbox(settingsFrame, settingsSkewer, Tr("Integer scaling"), integerScaling);
            Config::IntegerScaling = integerScaling;
            if (Config::Filtering < 0)
                Config::Filtering = 0;
            if (Config::Filtering > 2)
                Config::Filtering = 2;
            DoCombobox(settingsFrame, settingsSkewer, Tr("Filtering"), Localization::Options(Localization::OptionList::Filtering), Config::Filtering);
            if (Config::Filtering < 0)
                Config::Filtering = 0;
            if (Config::Filtering > 2)
                Config::Filtering = 2;
            Config::ClampInternalResolutionOption();
            DoCombobox(settingsFrame, settingsSkewer, Tr("3D internal resolution"), Localization::Options(Localization::OptionList::InternalResolution), Config::upscaleFactor);
            Config::ClampInternalResolutionOption();
            if (Config::upscaleFactor != AppliedUpscaleFactor && Emulation::State != Emulation::emuState_Nothing)
            {
                GPU::RenderSettings renderSettings{true, Config::InternalResolutionScale(), false};
                GPU::SetRenderSettings(0, renderSettings);
                AppliedUpscaleFactor = Config::upscaleFactor;
            }
        }
        Emulation::UpdateScreenLayout();
        break;
    case uiScreen_RetroAchievements:
        title = Tr("RetroAchievements List");
        {    
            if (g_loadAchievements) {
                g_achievements = achievements_list();   
            }
            if (g_achievements.empty()) {
                DoLabel(settingsFrame, settingsSkewer, Tr("This title does not have any achievements."));
                DoLabel(settingsFrame, settingsSkewer, Tr("Please check the RetroAchievements website for more information."));
            } else {
                for (const auto& ach : g_achievements) {
                    SectionHeader(settingsFrame, settingsSkewer, ach.title.c_str());
                    DoLabel(settingsFrame, settingsSkewer, ach.description.c_str());
                    DoLabel(settingsFrame, settingsSkewer, ach.progress.c_str());
                    if (ach.textureId >= 0)
                        ShowImage(settingsFrame, settingsSkewer, ach.textureId, ach.width, ach.height);
                }
                
            }
        }
        break;
    case uiScreen_Cheats:
        title = Tr("Cheats");
        {
            SectionHeader(settingsFrame, settingsSkewer, Tr("Action Replay"));

            bool cheatsEnabled = Config::EnableCheats != 0;
            bool oldCheatsEnabled = cheatsEnabled;
            DoCheckbox(settingsFrame, settingsSkewer, Tr("Enable cheats"), cheatsEnabled, true);
            if (cheatsEnabled != oldCheatsEnabled)
            {
                Config::EnableCheats = cheatsEnabled ? 1 : 0;
                Frontend::EnableCheats(cheatsEnabled);
            }

            ARCodeFile* cheatFile = Frontend::GetCheatFile();
            if (!cheatFile)
            {
                DoLabel(settingsFrame, settingsSkewer, Tr("No game is loaded."));
                DoLabel(settingsFrame, settingsSkewer, Tr("Start a game before editing cheats."));
                break;
            }

            if (cheatFile->Categories.empty())
            {
                DoLabel(settingsFrame, settingsSkewer, Tr("No cheat codes found."));
                DoLabel(settingsFrame, settingsSkewer, Tr("Place a matching .mch file next to the ROM."));
                break;
            }

            int uniqueId = 0;
            for (ARCodeCatList::iterator catIt = cheatFile->Categories.begin(); catIt != cheatFile->Categories.end(); catIt++)
            {
                ARCodeCat& cat = *catIt;
                SectionHeader(settingsFrame, settingsSkewer, cat.Name);

                if (cat.Codes.empty())
                {
                    DoLabel(settingsFrame, settingsSkewer, Tr("No codes in this category."));
                    continue;
                }

                for (ARCodeList::iterator codeIt = cat.Codes.begin(); codeIt != cat.Codes.end(); codeIt++)
                {
                    ARCode& code = *codeIt;
                    bool enabled = code.Enabled;
                    if (DoCheckboxWithId(settingsFrame, settingsSkewer, code.Name, enabled, uniqueId++))
                    {
                        code.Enabled = enabled;
                        if (!Frontend::SaveCheats())
                            ErrorDialog::Open(Tr("Failed to save cheat file."));
                    }
                }
            }
        }
        break;
    case uiScreen_InputSettings:
        title = Tr("Input settings");

        padUpdate(&pad);
        {
            SectionHeader(settingsFrame, settingsSkewer, Tr("Touchscreen"));
            
            DoCombobox(settingsFrame, settingsSkewer, Tr("Cursor mode"), Localization::Options(Localization::OptionList::CursorMode), Config::TouchscreenMode, true);
            DoCombobox(settingsFrame, settingsSkewer, Tr("Click mode"), Localization::Options(Localization::OptionList::ClickMode), Config::TouchscreenClickMode);
            bool leftHanded = Config::LeftHandedMode;
            DoCheckbox(settingsFrame, settingsSkewer, Tr("Left handed mode"), leftHanded);
            Config::LeftHandedMode = leftHanded;
        }
        {
            SectionHeader(settingsFrame, settingsSkewer, Tr("Joycon"));
            bool fastforward = Config::FastForward;
            DoCheckbox(settingsFrame, settingsSkewer, Tr("Hold to fastforward (ZL)"), fastforward);
            Config::FastForward = fastforward;
        }
        {
            static bool defaultMapping = false;

            static bool saveMapping = false;
            static bool loadMapping = false;

            SectionHeader(settingsFrame, settingsSkewer, Tr("Buttons Remapping"));

            DoInputButton(settingsFrame, settingsSkewer, "A: ", InputConfig::ButtonA);
            DoInputButton(settingsFrame, settingsSkewer, "B: ", InputConfig::ButtonB);
            DoInputButton(settingsFrame, settingsSkewer, "X: ", InputConfig::ButtonX);
            DoInputButton(settingsFrame, settingsSkewer, "Y: ", InputConfig::ButtonY);
            DoInputButton(settingsFrame, settingsSkewer, "Left Stick Button: ", InputConfig::ButtonStickL);
            DoInputButton(settingsFrame, settingsSkewer, "Right Stick Button: ", InputConfig::ButtonStickR);
            DoInputButton(settingsFrame, settingsSkewer, "L: ", InputConfig::ButtonL);
            DoInputButton(settingsFrame, settingsSkewer, "R: ", InputConfig::ButtonR);
            DoInputButton(settingsFrame, settingsSkewer, "ZL: ", InputConfig::ButtonZL);
            DoInputButton(settingsFrame, settingsSkewer, "ZR: ", InputConfig::ButtonZR);
            DoInputButton(settingsFrame, settingsSkewer, "Start: ", InputConfig::ButtonStart);
            DoInputButton(settingsFrame, settingsSkewer, "Select: ", InputConfig::ButtonSelect);
            DoInputButton(settingsFrame, settingsSkewer, "Up: ", InputConfig::ButtonUp);
            DoInputButton(settingsFrame, settingsSkewer, "Down: ", InputConfig::ButtonDown);
            DoInputButton(settingsFrame, settingsSkewer, "Left: ", InputConfig::ButtonLeft);
            DoInputButton(settingsFrame, settingsSkewer, "Right: ", InputConfig::ButtonRight);

            DoInputButton(settingsFrame, settingsSkewer, "Left Stick Up: ", InputConfig::ButtonStickLUp);
            DoInputButton(settingsFrame, settingsSkewer, "Left Stick Right: ", InputConfig::ButtonStickLRight);
            DoInputButton(settingsFrame, settingsSkewer, "Left Stick Down: ", InputConfig::ButtonStickLDown);
            DoInputButton(settingsFrame, settingsSkewer, "Left Stick Left: ", InputConfig::ButtonStickLLeft);

            DoInputButton(settingsFrame, settingsSkewer, "Right Stick Up: ", InputConfig::ButtonStickRUp);
            DoInputButton(settingsFrame, settingsSkewer, "Right Stick Right: ", InputConfig::ButtonStickRRight);
            DoInputButton(settingsFrame, settingsSkewer, "Right Stick Down: ", InputConfig::ButtonStickRDown);
            DoInputButton(settingsFrame, settingsSkewer, "Right Stick Left: ", InputConfig::ButtonStickRLeft);

            DoInputButton(settingsFrame, settingsSkewer, "Left SL: ", InputConfig::ButtonLeftSL);
            DoInputButton(settingsFrame, settingsSkewer, "Left SR ", InputConfig::ButtonLeftSR);
            DoInputButton(settingsFrame, settingsSkewer, "Right SL: ", InputConfig::ButtonRightSL);
            DoInputButton(settingsFrame, settingsSkewer, "Right SR: ", InputConfig::ButtonRightSR);

            SectionHeader(settingsFrame, settingsSkewer, Tr("Hotkeys Remapping"));

            DoInputButton(settingsFrame, settingsSkewer, "Pause ", InputConfig::Pause);
            DoInputButton(settingsFrame, settingsSkewer, "Simulate Mic Noise: ", InputConfig::MicNoise);
            DoInputButton(settingsFrame, settingsSkewer, "Change Main Screen: ", InputConfig::changeScreen);
            DoInputButton(settingsFrame, settingsSkewer, "FastForward: ", InputConfig::fastForward);
            DoInputButton(settingsFrame, settingsSkewer, "QuickSave: ", InputConfig::QuickSave);
            DoInputButton(settingsFrame, settingsSkewer, "QuickLoad: ", InputConfig::QuickLoad);
            
            DoCheckbox(settingsFrame, settingsSkewer, Tr("Reset to default"), defaultMapping);
            DoCheckbox(settingsFrame, settingsSkewer, Tr("Save this configuration"), saveMapping);
            DoCheckbox(settingsFrame, settingsSkewer, Tr("Load old configuration"), loadMapping);

            if (defaultMapping) {
                InputConfig::ResetToDefault();
                defaultMapping = false;
            }

            if (loadMapping) {
                InputConfig::loadMappingFromFile("sdmc:/switch/melonDS/input.cfg");
                loadMapping = false;
            }

            if (saveMapping) {
                InputConfig::saveMappingToFile("sdmc:/switch/melonDS/input.cfg");
                saveMapping = false;
            }    

        }
        break;
    }
    Gfx::PopScissor();

    BackButton::DoGui(parent, title);

    KeyExplanation::Explain(KeyExplanation::button_B, Tr("Back"));
    if (BoxGui::CancelPressed())
        BackButton::GoBack();
}

}
