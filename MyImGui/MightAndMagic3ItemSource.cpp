#include "pch.h"
#include "MightAndMagic3ItemSource.h"

namespace MightAndMagic3
{
    ItemSource::ItemSource()
    {
        ItemExplorer::Item item;

        item.id = 1;
        item.name = "Test Item";

        item.properties.push_back(
            {
                "Source",
                "Might and Magic III"
            }
        );

        m_items.push_back(
            item
        );
    }

    const char* ItemSource::gameName()
        const
    {
        return "Might and Magic III";
    }

    const std::vector<
        ItemExplorer::Item
    >& ItemSource::items() const
    {
        return m_items;
    }

    const ItemExplorer::Item*
        ItemSource::findById(
            int id
        ) const
    {
        for (const ItemExplorer::Item& item :
            m_items)
        {
            if (item.id == id)
            {
                return &item;
            }
        }

        return nullptr;
    }
}
