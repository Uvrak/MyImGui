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

        std::vector<ItemProperty>
            properties;
    };
}