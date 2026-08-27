#pragma once

#include "ItemSource.h"

#include "MemoryReader.h"

namespace MightAndMagic3
{
    class ItemSource final :
        public ItemExplorer::ItemSource
    {
    public:
        ItemSource(
            DosBoxMemoryTools::MemoryReader& memoryReader
        );

        const char* gameName()
            const override;

        const std::vector<
            ItemExplorer::Item
        >& items() const override;

        const ItemExplorer::Item*
            findById(
                int id
            ) const override;

        bool refresh();

        int selectedItemId() const;

        std::string selectedItemName() const override;

        int selectedCharacterLevel() const override;

        int selectedCharacterAccuracy() const override;

        int selectedCharacterAccuracyBonus() const override;

        int selectedCharacterClassId() const override;

        int selectedMaterialId() const;

        std::string materialName(
            int materialId
        ) const;

    private:
        std::vector<
            ItemExplorer::Item
        > m_items;

        DosBoxMemoryTools::MemoryReader&
            m_memoryReader;

        std::string readString(
            size_t address
        ) const;
    };
}