#pragma once

#include "WorldMode.h"

#include <memory>
#include <functional>
#include <utility>

namespace ow3d
{
    class TileWorld;
    class IWorld;
    class Renderer;

    class WorldManager
    {
    public:
        using TileWorldInitializer = std::function<void(TileWorld&)>;
        using WorldFactory = std::function<std::unique_ptr<IWorld>()>;

        void setTileWorldFactory(WorldFactory factory)
        {
            m_tileWorldFactory = std::move(factory);
        }

        void setTileWorldInitializer(TileWorldInitializer initializer)
        {
            m_tileWorldInitializer = std::move(initializer);
        }

        WorldManager();
        ~WorldManager();

        void setMode(WorldMode mode);
        WorldMode mode() const;

        void update(float deltaTime);
        void render(Renderer& renderer);

    private:
        TileWorldInitializer m_tileWorldInitializer;
        WorldFactory m_tileWorldFactory;
        WorldMode m_mode = WorldMode::Normal;
        std::unique_ptr<IWorld> m_world;
    };
}
