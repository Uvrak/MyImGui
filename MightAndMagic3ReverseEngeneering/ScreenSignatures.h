#pragma once

#include "GameModule.h"

namespace MightAndMagic3
{
    class ScreenSignatures
    {
    public:
        static const ScreenSignature&
            mainMenu();

        static const ScreenSignature&
            loadGame();

        static const ScreenSignature& mainGame();
    };
}
