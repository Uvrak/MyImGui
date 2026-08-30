#include "GameModuleManager.h"

void GameModuleManager::add(
    const std::string& name,
    GameModule* module
)
{
    if (!module)
    {
        return;
    }

    m_modules.push_back(
        {
            name,
            module
        }
    );
}

void GameModuleManager::setActive(
    size_t index
)
{
    if (index >=
        m_modules.size())
    {
        return;
    }

    m_activeIndex =
        index;
}

GameModule*
GameModuleManager::active()
{
    if (m_modules.empty() ||
        m_activeIndex >=
        m_modules.size())
    {
        return nullptr;
    }

    return
        m_modules[m_activeIndex].module;
}

const GameModule*
GameModuleManager::active() const
{
    if (m_modules.empty() ||
        m_activeIndex >=
        m_modules.size())
    {
        return nullptr;
    }

    return
        m_modules[m_activeIndex].module;
}

const std::vector<
    RegisteredGameModule
>& GameModuleManager::modules() const
{
    return m_modules;
}