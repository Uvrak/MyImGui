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


            if (itemId >= 1 &&
                itemId <= 33)
            {
                constexpr size_t WeaponDiceCountAddress =
                    0x210DD;

                constexpr size_t WeaponDiceSidesAddress =
                    0x21126;

                const size_t weaponIndex =
                    static_cast<size_t>(
                        itemId - 1
                        );

                const size_t diceCountAddress =
                    WeaponDiceCountAddress +
                    weaponIndex;

                const size_t diceSidesAddress =
                    WeaponDiceSidesAddress +
                    weaponIndex;

                if (diceCountAddress < memory.size() &&
                    diceSidesAddress < memory.size())
                {
                    const uint8_t diceCount =
                        memory[diceCountAddress];

                    const uint8_t diceSides =
                        memory[diceSidesAddress];

                    item.properties.push_back(
                        {
                            "Damage",
                            std::to_string(
                                diceCount
                            ) +
                            "d" +
                            std::to_string(
                                diceSides
                            )
                        }
                    );
                }
            }
            m_items.push_back(
                item
            );

            address +=
                name.size() + 1;

            ++itemId;
        }

        return !m_items.empty();
    }
    
    int ItemSource::selectedItemId() const
    {
        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        constexpr size_t SelectedPartySlotAddress =
            0x2068E;

        constexpr size_t SelectedInventorySlotAddress =
            0x304D0;

        constexpr size_t FirstInventoryItemIdsAddress =
            0x2BFEE;

        constexpr size_t CharacterRecordSize =
            0x12F;

        if (SelectedPartySlotAddress >= memory.size() ||
            SelectedInventorySlotAddress + 1 >= memory.size())
        {
            return 0;
        }

        const uint8_t partyIndex =
            memory[
                SelectedPartySlotAddress
            ];

        const uint16_t inventorySlot =
            static_cast<uint16_t>(
                memory[
                    SelectedInventorySlotAddress
                ]
                ) |
            (
                static_cast<uint16_t>(
                    memory[
                        SelectedInventorySlotAddress + 1
                    ]
                    ) << 8
                );

        if (partyIndex >= 8 ||
            inventorySlot >= 18)
        {
            return 0;
        }

        const size_t itemAddress =
            FirstInventoryItemIdsAddress +
            static_cast<size_t>(
                partyIndex
                ) *
            CharacterRecordSize +
            inventorySlot;

        if (itemAddress >= memory.size())
        {
            return 0;
        }

        return static_cast<int>(
            memory[
                itemAddress
            ]
            );
    }

    std::string ItemSource::selectedItemName() const
    {
        const int itemId =
            selectedItemId();

        const ItemExplorer::Item* item =
            findById(
                itemId
            );

        if (!item)
        {
            return {};
        }

        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        constexpr size_t SelectedPartySlotAddress =
            0x2068E;

        constexpr size_t SelectedInventorySlotAddress =
            0x304D0;

        constexpr size_t FirstMaterialAddress =
            0x2BFC8;

        constexpr size_t CharacterRecordSize =
            0x12F;

        if (SelectedPartySlotAddress >= memory.size() ||
            SelectedInventorySlotAddress + 1 >= memory.size())
        {
            return item->name;
        }

        const uint8_t partyIndex =
            memory[
                SelectedPartySlotAddress
            ];

        const uint16_t inventorySlot =
            static_cast<uint16_t>(
                memory[
                    SelectedInventorySlotAddress
                ]
                ) |
            (
                static_cast<uint16_t>(
                    memory[
                        SelectedInventorySlotAddress + 1
                    ]
                    ) << 8
                );

        if (partyIndex >= 8 ||
            inventorySlot >= 18)
        {
            return item->name;
        }

        const size_t materialAddress =
            FirstMaterialAddress +
            static_cast<size_t>(
                partyIndex
                ) *
            CharacterRecordSize +
            inventorySlot;

        if (materialAddress >= memory.size())
        {
            return item->name;
        }

        const uint8_t materialId =
            memory[
                materialAddress
            ];

        if (materialId == 0 ||
            materialId > 22)
        {
            return item->name;
        }

        size_t address =
            0x26250;

        for (int id = 1;
            id < materialId;
            ++id)
        {
            const std::string name =
                readString(
                    address
                );

            if (name.empty())
            {
                return item->name;
            }

            address +=
                name.size() + 1;
        }

        std::string material =
            readString(
                address
            );

        while (!material.empty() &&
            material.back() == ' ')
        {
            material.pop_back();
        }

        if (material.empty())
        {
            return item->name;
        }

        return material +
            " " +
            item->name;
    }

    int ItemSource::selectedCharacterLevel() const
    {
        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        constexpr size_t SelectedPartySlotAddress =
            0x2068E;

        constexpr size_t FirstCharacterAddress =
            0x2BF00;

        constexpr size_t CharacterRecordSize =
            0x12F;

        constexpr size_t LevelOffset =
            0x35;

        if (SelectedPartySlotAddress >= memory.size())
        {
            return 0;
        }

        const uint8_t partyIndex =
            memory[
                SelectedPartySlotAddress
            ];

        if (partyIndex >= 6)
        {
            return 0;
        }

        const size_t levelAddress =
            FirstCharacterAddress +
            static_cast<size_t>(
                partyIndex
                ) *
            CharacterRecordSize +
            LevelOffset;

        if (levelAddress >= memory.size())
        {
            return 0;
        }

        return static_cast<int>(
            memory[
                levelAddress
            ]
            );
    }

    int ItemSource::selectedCharacterAccuracy() const
    {
        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        constexpr size_t SelectedPartySlotAddress =
            0x2068E;

        constexpr size_t FirstCharacterAddress =
            0x2BF00;

        constexpr size_t CharacterRecordSize =
            0x12F;

        constexpr size_t AccuracyOffset =
            0x30;

        if (SelectedPartySlotAddress >= memory.size())
        {
            return 0;
        }

        const uint8_t partyIndex =
            memory[
                SelectedPartySlotAddress
            ];

        if (partyIndex >= 6)
        {
            return 0;
        }

        const size_t accuracyAddress =
            FirstCharacterAddress +
            static_cast<size_t>(
                partyIndex
                ) *
            CharacterRecordSize +
            AccuracyOffset;

        if (accuracyAddress >= memory.size())
        {
            return 0;
        }

        return static_cast<int>(
            memory[
                accuracyAddress
            ]
            );
    }

    int ItemSource::selectedCharacterAccuracyBonus() const
    {
        const int accuracy =
            selectedCharacterAccuracy();

        if (accuracy < 13)
        {
            return 0;
        }

        return (accuracy - 11) / 2;
    }

    int ItemSource::selectedMaterialId() const
    {
        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        constexpr size_t SelectedPartySlotAddress =
            0x2068E;

        constexpr size_t SelectedInventorySlotAddress =
            0x304D0;

        constexpr size_t FirstMaterialAddress =
            0x2BFC8;

        constexpr size_t CharacterRecordSize =
            0x12F;

        if (SelectedPartySlotAddress >= memory.size() ||
            SelectedInventorySlotAddress + 1 >= memory.size())
        {
            return 0;
        }

        const uint8_t partyIndex =
            memory[
                SelectedPartySlotAddress
            ];

        const uint16_t inventorySlot =
            static_cast<uint16_t>(
                memory[
                    SelectedInventorySlotAddress
                ]
                ) |
            (
                static_cast<uint16_t>(
                    memory[
                        SelectedInventorySlotAddress + 1
                    ]
                    ) << 8
                );

        if (partyIndex >= 8 ||
            inventorySlot >= 18)
        {
            return 0;
        }

        const size_t materialAddress =
            FirstMaterialAddress +
            static_cast<size_t>(
                partyIndex
                ) *
            CharacterRecordSize +
            inventorySlot;

        if (materialAddress >= memory.size())
        {
            return 0;
        }

        return static_cast<int>(
            memory[
                materialAddress
            ]
            );
    }

    std::string ItemSource::materialName(
        int materialId
    ) const
    {
        if (materialId <= 0 ||
            materialId > 22)
        {
            return {};
        }

        size_t address =
            0x26250;

        for (int id = 1;
            id < materialId;
            ++id)
        {
            const std::string name =
                readString(
                    address
                );

            if (name.empty())
            {
                return {};
            }

            address +=
                name.size() + 1;
        }

        std::string result =
            readString(
                address
            );

        while (!result.empty() &&
            result.back() == ' ')
        {
            result.pop_back();
        }

        return result;
    }
}
