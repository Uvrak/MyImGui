#pragma once

#include <vector>
#include <string>

#include "Item.h"

namespace ItemExplorer
{
    class ItemSource
    {
    public:
        virtual ~ItemSource() = default;

        virtual const char* gameName() const = 0;

        virtual const std::vector<Item>&
            items() const = 0;

        virtual const Item* findById(
            int id
        ) const = 0;

        virtual std::string selectedItemName() const
        {
            return {};
        }
    };
}