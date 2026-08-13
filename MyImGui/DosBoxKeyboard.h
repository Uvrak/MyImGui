#pragma once

#include "DosBoxController.h"

#include <functional>
#include <string>

#include "imgui.h"

namespace MyImGui
{
    class NamedPipeClient;

    using DosBoxKeyCommandResolver =
        std::function<
        std::string(
            ImGuiKey key,
            const char* defaultCommand
        )
        >;

    class DosBoxKeyboard
    {
    public:
        void update(
            NamedPipeClient& namedPipeClient,
            const DosBoxKeyCommandResolver&
            commandResolver = {}
        );
    };
}
