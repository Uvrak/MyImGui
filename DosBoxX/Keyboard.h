#pragma once

#include "Controller.h"

#include <functional>
#include <string>

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
            commandResolver = {}
        );
    };
}
