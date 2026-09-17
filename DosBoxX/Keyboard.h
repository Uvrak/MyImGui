#pragma once

#include "Controller.h"

#include <functional>
#include <string>
#include <unordered_map>

#include "imgui.h"

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
        void update(
            NamedPipeClient& namedPipeClient,
            const DosBoxKeyCommandResolver&
            commandResolver = {},
            bool enabled = true
        );
    private:
        std::unordered_map<int, std::string> m_downCommands;
    };
}
