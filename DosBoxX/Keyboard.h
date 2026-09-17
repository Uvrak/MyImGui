#pragma once

#include "Controller.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "imgui.h"
#include <SDL3/SDL_events.h>

namespace DosBoxX
{
    class NamedPipeClient;

    using DosBoxKeyCommandResolver =
        std::function<
        std::string(
            ImGuiKey key,
            const char* defaultCommand
        )
        >;

    class Keyboard
    {
    public:
        void recordBackgroundEvent(const SDL_KeyboardEvent& event);
        void update(
            NamedPipeClient& namedPipeClient,
            const DosBoxKeyCommandResolver&
            commandResolver = {},
            bool enabled = true,
            bool background = false
        );
    private:
        struct BackgroundEvent
        {
            ImGuiKey key;
            bool down;
        };
        std::vector<BackgroundEvent> m_backgroundEvents;
        bool m_background = false;
        std::unordered_map<int, std::string> m_downCommands;
    };
}
