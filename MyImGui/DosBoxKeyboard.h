#pragma once

#include "DosBoxController.h"

namespace MyImGui
{
    class NamedPipeClient;

    class DosBoxKeyboard
    {
    public:
        void update(
            NamedPipeClient& NamedPipeClient
        );

    };
}
