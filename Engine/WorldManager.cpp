#include "WorldManager.h"

#include "IWorld.h"
#include "NormalWorld.h"
#include "TileWorld.h"

namespace ow3d
{
    WorldManager::WorldManager()
    {
        setMode(WorldMode::Normal);
    }

    WorldManager::~WorldManager() = default;

    void WorldManager::setMode(WorldMode mode)
    {
        if (m_mode == mode && m_world)
            return;

        m_mode = mode;

        switch (m_mode)
        {
        case WorldMode::Normal:
            m_world = std::make_unique<NormalWorld>();
            break;

        case WorldMode::TileBased:
        {
            if (m_tileWorldFactory)
            {
                m_world = m_tileWorldFactory();
                break;
            }
            auto world = std::make_unique<TileWorld>();
            if (m_tileWorldInitializer)
                m_tileWorldInitializer(*world);
            m_world = std::move(world);
            break;
        }
        }
    }

    WorldMode WorldManager::mode() const
    {
        return m_mode;
    }

    void WorldManager::update(float deltaTime)
    {
        if (m_world)
            m_world->update(deltaTime);
    }

    void WorldManager::render(Renderer& renderer)
    {
        if (m_world)
            m_world->render(renderer);
    }
}
