#include "MM3GameModule.h"

namespace MightAndMagic3
{
    MM3GameModule::MM3GameModule(DosBoxX::Controller& controller,
                                 DosBoxX::NamedPipeClient& pipe,
                                 std::optional<std::size_t> mapIdAddress)
        : m_launcher(controller, pipe),
          m_mapDetector(m_memoryReader, mapIdAddress),
          m_stateReader(m_memoryReader) {}

    void MM3GameModule::start() { m_launcher.start(); }

    void MM3GameModule::update()
    {
        m_launcher.update();
        // Map detection must also work when the launcher is still waiting for
        // DOSBox (or MM3 was started before its launch sequence completed).
        m_mapDetector.update();
        if (m_mapDetector.mapId()) m_stateReader.update(m_state);
    }

    std::optional<std::string> MM3GameModule::currentMapKey() const
    {
        if (const auto id = m_mapDetector.mapId(); id && *id != 0)
            return "mm3/" + std::to_string(*id);
        return std::nullopt;
    }

    std::optional<std::string> MM3GameModule::currentMapName() const
    {
        const auto id = m_mapDetector.mapId();
        if (!id || *id > 66) return std::nullopt;

        const auto& memory = m_memoryReader.memory();
        constexpr std::size_t table = 0x25CAE;
        constexpr std::size_t stringBase = 0x20530;
        const std::size_t pointerAddress = table + 2 * *id;
        if (pointerAddress + 1 >= memory.size()) return std::nullopt;

        const std::size_t offset = static_cast<std::size_t>(memory[pointerAddress]) |
            (static_cast<std::size_t>(memory[pointerAddress + 1]) << 8);
        const std::size_t nameAddress = stringBase + offset;
        if (nameAddress >= memory.size()) return std::nullopt;

        std::string name;
        for (std::size_t i = nameAddress;
             i < memory.size() && i < nameAddress + 64; ++i)
        {
            const auto character = memory[i];
            if (character == 0) return name.empty() ? std::nullopt
                                                     : std::optional<std::string>(name);
            if (character == '*') name += "\xC3\xB6"; // MM3's oe glyph
            else if (character == '@') name += "\xC3\x9F"; // MM3's sharp s glyph
            else if (character >= 32 && character < 127)
                name += static_cast<char>(character);
            else return std::nullopt;
        }
        return std::nullopt;
    }

    std::optional<GamePosition> MM3GameModule::currentPosition() const
    {
        if (!m_mapDetector.mapId()) return std::nullopt;
        const auto& position = m_state.position();
        if (!position.valid) return std::nullopt;

        GamePosition result;
        result.x = position.x;
        // MM3 counts northward from zero; GridBuilder draws north at negative Y.
        result.y = -position.y;
        switch (position.direction)
        {
        case Direction::North: result.direction = GameFacingDirection::North; break;
        case Direction::East: result.direction = GameFacingDirection::East; break;
        case Direction::South: result.direction = GameFacingDirection::South; break;
        case Direction::West: result.direction = GameFacingDirection::West; break;
        default: return std::nullopt;
        }
        return result;
    }
}
