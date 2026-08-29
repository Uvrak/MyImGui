#pragma once

#include <string>
#include <vector>

namespace ItemExplorer
{
    struct ItemProperty
    {
        std::string name;
        std::string value;
    };

    struct Item
    {
        int id = -1;
        std::string name;

        int diceCount = 0;
        int diceSides = 0;

        int materialId = 0;
        std::string material;

        std::vector<ItemProperty>
            properties;
    };
}