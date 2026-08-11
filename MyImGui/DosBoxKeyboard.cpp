#include "pch.h"
#include "DosBoxKeyboard.h"

#include <string>

#include "ExternalWindow.h"
#include "imgui.h"

namespace MyImGui
{
    void DosBoxKeyboard::update(
        ExternalWindow& externalWindow
    )
    {
        struct KeyMapping
        {
            ImGuiKey key;
            const char* command;
        };

        const auto processKeys =
            [&externalWindow](
                const KeyMapping* mappings,
                size_t count
                )
            {
                for (size_t index = 0;
                    index < count;
                    ++index)
                {
                    const KeyMapping& mapping =
                        mappings[index];

                    if (ImGui::IsKeyPressed(
                        mapping.key,
                        false
                    ))
                    {
                        std::string command =
                            "KEYDOWN:";

                        command +=
                            mapping.command;

                        externalWindow.sendIpcCommand(
                            command
                        );
                    }

                    if (ImGui::IsKeyReleased(
                        mapping.key
                    ))
                    {
                        std::string command =
                            "KEYUP:";

                        command +=
                            mapping.command;

                        externalWindow.sendIpcCommand(
                            command
                        );
                    }
                }
            };

        static const KeyMapping letterKeys[] =
        {
            { ImGuiKey_A, "A" },
            { ImGuiKey_B, "B" },
            { ImGuiKey_C, "C" },
            { ImGuiKey_D, "D" },
            { ImGuiKey_E, "E" },
            { ImGuiKey_F, "F" },
            { ImGuiKey_G, "G" },
            { ImGuiKey_H, "H" },
            { ImGuiKey_I, "I" },
            { ImGuiKey_J, "J" },
            { ImGuiKey_K, "K" },
            { ImGuiKey_L, "L" },
            { ImGuiKey_M, "M" },
            { ImGuiKey_N, "N" },
            { ImGuiKey_O, "O" },
            { ImGuiKey_P, "P" },
            { ImGuiKey_Q, "Q" },
            { ImGuiKey_R, "R" },
            { ImGuiKey_S, "S" },
            { ImGuiKey_T, "T" },
            { ImGuiKey_U, "U" },
            { ImGuiKey_V, "V" },
            { ImGuiKey_W, "W" },
            { ImGuiKey_X, "X" },
            { ImGuiKey_Y, "Y" },
            { ImGuiKey_Z, "Z" }
        };

        static const KeyMapping digitKeys[] =
        {
            { ImGuiKey_0, "0" },
            { ImGuiKey_1, "1" },
            { ImGuiKey_2, "2" },
            { ImGuiKey_3, "3" },
            { ImGuiKey_4, "4" },
            { ImGuiKey_5, "5" },
            { ImGuiKey_6, "6" },
            { ImGuiKey_7, "7" },
            { ImGuiKey_8, "8" },
            { ImGuiKey_9, "9" }
        };

        static const KeyMapping functionKeys[] =
        {
            { ImGuiKey_F1,  "F1" },
            { ImGuiKey_F2,  "F2" },
            { ImGuiKey_F3,  "F3" },
            { ImGuiKey_F4,  "F4" },
            { ImGuiKey_F5,  "F5" },
            { ImGuiKey_F6,  "F6" },
            { ImGuiKey_F7,  "F7" },
            { ImGuiKey_F8,  "F8" },
            { ImGuiKey_F9,  "F9" },
            { ImGuiKey_F10, "F10" },
            { ImGuiKey_F11, "F11" },
            { ImGuiKey_F12, "F12" }
        };

        static const KeyMapping navigationKeys[] =
        {
            { ImGuiKey_Home,     "HOME" },
            { ImGuiKey_End,      "END" },
            { ImGuiKey_Insert,   "INSERT" },
            { ImGuiKey_Delete,   "DELETE" },
            { ImGuiKey_PageUp,   "PAGEUP" },
            { ImGuiKey_PageDown, "PAGEDOWN" }
        };

        static const KeyMapping symbolKeys[] =
        {
            { ImGuiKey_Minus,        "MINUS" },
            { ImGuiKey_Equal,        "EQUALS" },
            { ImGuiKey_LeftBracket,  "LEFTBRACKET" },
            { ImGuiKey_RightBracket, "RIGHTBRACKET" },
            { ImGuiKey_Backslash,    "BACKSLASH" },
            { ImGuiKey_Semicolon,    "SEMICOLON" },
            { ImGuiKey_Apostrophe,   "QUOTE" },
            { ImGuiKey_Comma,        "COMMA" },
            { ImGuiKey_Period,       "PERIOD" },
            { ImGuiKey_Slash,        "SLASH" }
        };

        static const KeyMapping modifierKeys[] =
        {
            { ImGuiKey_LeftShift,  "SHIFT" },
            { ImGuiKey_RightShift, "SHIFT" },
            { ImGuiKey_LeftCtrl,   "CTRL" },
            { ImGuiKey_RightCtrl,  "CTRL" },
            { ImGuiKey_LeftAlt,    "ALT" },
            { ImGuiKey_RightAlt,   "ALTGR" }
        };

        static const KeyMapping basicKeys[] =
        {
            { ImGuiKey_Enter, "ENTER" },
            { ImGuiKey_Space, "SPACE" }
        };

        static const KeyMapping specialKeys[] =
        {
            { ImGuiKey_Backspace,  "BACKSPACE" },
            { ImGuiKey_Tab,        "TAB" },
            { ImGuiKey_Escape,     "ESC" },
            { ImGuiKey_UpArrow,    "UP" },
            { ImGuiKey_DownArrow,  "DOWN" },
            { ImGuiKey_LeftArrow,  "LEFT" },
            { ImGuiKey_RightArrow, "RIGHT" }
        };

        processKeys(
            letterKeys,
            std::size(letterKeys)
        );

        processKeys(
            digitKeys,
            std::size(digitKeys)
        );

        processKeys(
            functionKeys,
            std::size(functionKeys)
        );

        processKeys(
            navigationKeys,
            std::size(navigationKeys)
        );

        processKeys(
            symbolKeys,
            std::size(symbolKeys)
        );

        processKeys(
            modifierKeys,
            std::size(modifierKeys)
        );

        processKeys(
            basicKeys,
            std::size(basicKeys)
        );

        processKeys(
            specialKeys,
            std::size(specialKeys)
        );
    }
}