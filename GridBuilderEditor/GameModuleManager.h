#pragma once

#include "GameModule.h"

#include <cstddef>
#include <string>
#include <vector>

struct RegisteredGameModule
{
    std::string name;

    GameModule* module =
        nullptr;
};

class GameModuleManager
{
public:
    void add(
        const std::string& name,
        GameModule* module
    );

    void setActive(
        size_t index
    );

    GameModule* active();

    const GameModule*
        active() const;

    const std::vector<
        RegisteredGameModule
    >& modules() const;

private:
    std::vector<
        RegisteredGameModule
    > m_modules;

    size_t m_activeIndex =
        0;
};
