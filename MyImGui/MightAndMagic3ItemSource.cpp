#include "pch.h"
#include "MightAndMagic3ItemSource.h"

namespace MightAndMagic3
{
    constexpr int MaxItemId = 100;

    ItemSource::ItemSource(
        MyImGui::DosBoxMemoryReader& memoryReader
    )
        :
        m_memoryReader(
            memoryReader
        )
    {
        ItemExplorer::Item item;

        item.id = 1;
        item.name =
            readString(
                0x265A5
            );

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

    std::string ItemSource::readString(
        size_t address
    ) const
    {
        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        if (address >= memory.size())
        {
            return {};
        }

        std::string result;

        while (address < memory.size())
        {
            const uint8_t value =
                memory[address];

            if (value == 0)
            {
                break;
            }

            result.push_back(
                static_cast<char>(value)
            );

            ++address;
        }

        return result;
    }

    bool ItemSource::refresh()
    {
        if (!m_memoryReader.isOpen())
        {
            if (!m_memoryReader.tryOpen())
            {
                return false;
            }
        }

        if (!m_memoryReader.readSnapshot())
        {
            return false;
        }

        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        constexpr size_t FirstItemNameAddress =
            0x265A5;

        if (FirstItemNameAddress >= memory.size())
        {
            return false;
        }

        m_items.clear();

        size_t address =
            FirstItemNameAddress;

        int itemId = 1;

        while (address < memory.size() &&
            itemId <= MaxItemId)
        {
            const std::string name =
                readString(
                    address
                );

            if (name.empty())
            {
                break;
            }

            ItemExplorer::Item item;

            item.id =
                itemId;

            item.name =
                name;

            m_items.push_back(
                item
            );

            address +=
                name.size() + 1;

            ++itemId;
        }

        return !m_items.empty();
    }
}
