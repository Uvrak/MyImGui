#pragma once

#include "ItemSource.h"

namespace MightAndMagic3
{
    class ItemSource final :
        public ItemExplorer::ItemSource
    {
    public:
        ItemSource();

        const char* gameName()
            const override;

        const std::vector<
            ItemExplorer::Item
        >& items() const override;

        const ItemExplorer::Item*
            findById(
                int id
            ) const override;

    private:
        std::vector<
            ItemExplorer::Item
        > m_items;
    };
}