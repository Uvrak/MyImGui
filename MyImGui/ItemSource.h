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

        virtual std::vector<std::string>
            classNames() const
        {
            return {};
        }

        virtual int characterLevel(
            int characterIndex
        ) const
        {
            return 0;
        }

        virtual int characterClassId(
            int characterIndex
        ) const
        {
            return 0;
        }

        virtual std::string selectedItemName() const
        {
            return {};
        }

        virtual int selectedCharacterIndex() const
        {
            return -1;
        }

        virtual int selectedCharacterLevel() const
        {
            return 0;
        }

        virtual int selectedCharacterAccuracy() const
        {
            return 0;
        }

        virtual int selectedCharacterAccuracyBonus() const
        {
            return 0;
        }

        virtual int selectedCharacterClassId() const
        {
            return 0;
        }

        virtual int selectedMaterialId() const
        {
            return 0;
        }

        virtual int selectedMaterialHitBonus() const
        {
            return 0;
        }

        virtual int selectedMaterialDamageBonus() const
        {
            return 0;
        }
    };
}