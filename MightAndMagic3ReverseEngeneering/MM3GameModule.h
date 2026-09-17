#pragma once

#include "../GridBuilderCore/GameModule.h"
#include "MapDetector.h"
#include "../DosBoxMemoryTools/MemoryReader.h"
#include "MM3Launcher.h"
#include "StateReader.h"

namespace MightAndMagic3
{
    class MM3GameModule final : public GameModule
    {
    public:
        MM3GameModule(DosBoxX::Controller& controller,
                      DosBoxX::NamedPipeClient& pipe,
                      std::optional<std::size_t> mapIdAddress);
        void start() override;
        void update() override;
        std::optional<std::string> currentMapKey() const override;
        std::optional<std::string> currentMapName() const override;
        std::optional<GamePosition> currentPosition() const override;

    private:
        MM3Launcher m_launcher;
        DosBoxMemoryTools::MemoryReader m_memoryReader;
        MapDetector m_mapDetector;
        StateReader m_stateReader;
        GameState m_state;
    };
}
