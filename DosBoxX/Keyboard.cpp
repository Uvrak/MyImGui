#include "Keyboard.h"

#include <algorithm>
#include <string>
#include <utility>

#include "NamedPipeClient.h"
#include "imgui.h"

namespace DosBoxX
{
    namespace
    {
        ImGuiKey keyFromScancode(SDL_Scancode scancode)
        {
            if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z)
                return static_cast<ImGuiKey>(ImGuiKey_A + scancode - SDL_SCANCODE_A);
            if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9)
                return static_cast<ImGuiKey>(ImGuiKey_1 + scancode - SDL_SCANCODE_1);
            if (scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F12)
                return static_cast<ImGuiKey>(ImGuiKey_F1 + scancode - SDL_SCANCODE_F1);
            switch (scancode)
            {
            case SDL_SCANCODE_0: return ImGuiKey_0;
            case SDL_SCANCODE_RETURN: return ImGuiKey_Enter;
            case SDL_SCANCODE_ESCAPE: return ImGuiKey_Escape;
            case SDL_SCANCODE_BACKSPACE: return ImGuiKey_Backspace;
            case SDL_SCANCODE_TAB: return ImGuiKey_Tab;
            case SDL_SCANCODE_SPACE: return ImGuiKey_Space;
            case SDL_SCANCODE_MINUS: return ImGuiKey_Minus;
            case SDL_SCANCODE_EQUALS: return ImGuiKey_Equal;
            case SDL_SCANCODE_LEFTBRACKET: return ImGuiKey_LeftBracket;
            case SDL_SCANCODE_RIGHTBRACKET: return ImGuiKey_RightBracket;
            case SDL_SCANCODE_BACKSLASH: return ImGuiKey_Backslash;
            case SDL_SCANCODE_SEMICOLON: return ImGuiKey_Semicolon;
            case SDL_SCANCODE_APOSTROPHE: return ImGuiKey_Apostrophe;
            case SDL_SCANCODE_COMMA: return ImGuiKey_Comma;
            case SDL_SCANCODE_PERIOD: return ImGuiKey_Period;
            case SDL_SCANCODE_SLASH: return ImGuiKey_Slash;
            case SDL_SCANCODE_CAPSLOCK: return ImGuiKey_CapsLock;
            case SDL_SCANCODE_INSERT: return ImGuiKey_Insert;
            case SDL_SCANCODE_HOME: return ImGuiKey_Home;
            case SDL_SCANCODE_PAGEUP: return ImGuiKey_PageUp;
            case SDL_SCANCODE_DELETE: return ImGuiKey_Delete;
            case SDL_SCANCODE_END: return ImGuiKey_End;
            case SDL_SCANCODE_PAGEDOWN: return ImGuiKey_PageDown;
            case SDL_SCANCODE_UP: return ImGuiKey_UpArrow;
            case SDL_SCANCODE_DOWN: return ImGuiKey_DownArrow;
            case SDL_SCANCODE_LEFT: return ImGuiKey_LeftArrow;
            case SDL_SCANCODE_RIGHT: return ImGuiKey_RightArrow;
            case SDL_SCANCODE_LCTRL: return ImGuiKey_LeftCtrl;
            case SDL_SCANCODE_RCTRL: return ImGuiKey_RightCtrl;
            case SDL_SCANCODE_LSHIFT: return ImGuiKey_LeftShift;
            case SDL_SCANCODE_RSHIFT: return ImGuiKey_RightShift;
            case SDL_SCANCODE_LALT: return ImGuiKey_LeftAlt;
            case SDL_SCANCODE_RALT: return ImGuiKey_RightAlt;
            default: return ImGuiKey_None;
            }
        }
    }

    void Keyboard::recordBackgroundEvent(const SDL_KeyboardEvent& event)
    {
        if (event.repeat) return;
        const ImGuiKey key = keyFromScancode(event.scancode);
        if (key != ImGuiKey_None)
            m_backgroundEvents.push_back({key, event.type == SDL_EVENT_KEY_DOWN});
    }

    void Keyboard::update(
        NamedPipeClient& namedPipeClient,
        const DosBoxKeyCommandResolver&
        commandResolver,
        bool enabled,
        bool background
    )
    {
        if (background != m_background)
        {
            for (const auto& [key, command] : m_downCommands)
                namedPipeClient.send("KEYUP:" + command);
            m_downCommands.clear();
            m_background = background;
        }
        if (!enabled)
        {
            for (const auto& [key, command] : m_downCommands)
                namedPipeClient.send("KEYUP:" + command);
            m_downCommands.clear();
            m_backgroundEvents.clear();
            return;
        }
        struct KeyMapping
        {
            ImGuiKey key;
            const char* command;
        };

        const auto processKeys =
            [
                &namedPipeClient,
                &commandResolver,
                this,
                enabled
            ](
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

                    const int keyId = static_cast<int>(mapping.key);
                    const auto down = m_downCommands.find(keyId);
                    if (down != m_downCommands.end() &&
                        (!enabled || ImGui::IsKeyReleased(mapping.key)))
                    {
                        namedPipeClient.send("KEYUP:" + down->second);
                        m_downCommands.erase(down);
                    }

                    if (!enabled || !ImGui::IsKeyPressed(mapping.key, false))
                        continue;

                    std::string resolvedCommand =
                        mapping.command;

                    if (commandResolver)
                    {
                        resolvedCommand =
                            commandResolver(
                                mapping.key,
                                mapping.command
                            );
                    }

                    if (resolvedCommand.empty())
                    {
                        continue;
                    }

                    if (m_downCommands.find(keyId) == m_downCommands.end())
                    {
                        std::string command =
                            "KEYDOWN:";

                        command +=
                            resolvedCommand;

                        if (namedPipeClient.send(command))
                            m_downCommands.emplace(keyId, resolvedCommand);
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
            { ImGuiKey_RightArrow, "RIGHT" },
            { ImGuiKey_CapsLock, "CAPSLOCK" }
        };

        if (background)
        {
            const std::pair<const KeyMapping*, size_t> groups[] = {
                {letterKeys, std::size(letterKeys)},
                {digitKeys, std::size(digitKeys)},
                {functionKeys, std::size(functionKeys)},
                {navigationKeys, std::size(navigationKeys)},
                {symbolKeys, std::size(symbolKeys)},
                {modifierKeys, std::size(modifierKeys)},
                {basicKeys, std::size(basicKeys)},
                {specialKeys, std::size(specialKeys)}
            };
            for (const auto& event : m_backgroundEvents)
            {
                for (const auto& [mappings, count] : groups)
                {
                    const auto* match = std::find_if(mappings, mappings + count,
                        [&](const KeyMapping& mapping) { return mapping.key == event.key; });
                    if (match == mappings + count) continue;
                    const int keyId = static_cast<int>(event.key);
                    const auto down = m_downCommands.find(keyId);
                    if (!event.down)
                    {
                        if (down != m_downCommands.end())
                        {
                            namedPipeClient.send("KEYUP:" + down->second);
                            m_downCommands.erase(down);
                        }
                    }
                    else if (down == m_downCommands.end())
                    {
                        std::string command = commandResolver
                            ? commandResolver(event.key, match->command)
                            : match->command;
                        if (!command.empty() && namedPipeClient.send("KEYDOWN:" + command))
                            m_downCommands.emplace(keyId, std::move(command));
                    }
                    break;
                }
            }
            m_backgroundEvents.clear();
            return;
        }

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
        m_backgroundEvents.clear();
    }
}
