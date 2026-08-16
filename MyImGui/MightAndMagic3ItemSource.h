#pragma once

#include "ItemSource.h"

#include "DosBoxMemoryReader.h"

namespace MightAndMagic3
{
    class ItemSource final :
        public ItemExplorer::ItemSource
    {
    public:
        ItemSource(
            MyImGui::DosBoxMemoryReader& memoryReader
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
    private:
        std::vector<
            ItemExplorer::Item
        > m_items;

        MyImGui::DosBoxMemoryReader&
            m_memoryReader;

        std::string readString(
            size_t address
        ) const;
        
    };
}