#include "MightAndMagic3ItemSource.h"

namespace MightAndMagic3
{
    constexpr int MaxItemId = 100;

    ItemSource::ItemSource(
        DosBoxMemoryTools::MemoryReader& memoryReader
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

    std::vector<std::string>
        ItemSource::classNames() const
    {
        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        constexpr size_t ClassTextAddress =
            0x246C0;

        std::vector<std::string> result;

        size_t address =
            ClassTextAddress;

        while (address + 4 < memory.size() &&
            result.size() < 10)
        {
            if (memory[address] == 0x0C &&
                memory[address + 1] == 0x25 &&
                memory[address + 2] == 0x32 &&
                memory[address + 3] == 0x64)
            {
                address += 4;

                std::string name;

                while (address < memory.size())
                {
                    const uint8_t value =
                        memory[address];

                    const bool letter =
                        (value >= 'A' && value <= 'Z') ||
                        (value >= 'a' && value <= 'z');

                    if (!letter)
                    {
                        break;
                    }

                    name.push_back(
                        static_cast<char>(
                            value
                            )
                    );

                    ++address;
                }

                if (!name.empty())
                {
                    result.push_back(
                        name
                    );
                }

                continue;
            }

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

                    item.diceCount =
                        static_cast<int>(
                            diceCount
                            );

                    item.diceSides =
                        static_cast<int>(
                            diceSides
                            );

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
        if (!itemWindowActive())
        {
            return 0;
        }

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

    int ItemSource::selectedCharacterIndex() const
    {
        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        constexpr size_t SelectedPartySlotAddress =
            0x2068E;

        if (SelectedPartySlotAddress >=
            memory.size())
        {
            return -1;
        }

        const int characterIndex =
            static_cast<int>(
                memory[
                    SelectedPartySlotAddress
                ]
                );

        if (characterIndex < 0 ||
            characterIndex >= 6)
        {
            return -1;
        }

        return characterIndex;
    }

    int ItemSource::characterLevel(
        int characterIndex
    ) const
    {
        if (characterIndex < 0 ||
            characterIndex >= 8)
        {
            return 0;
        }

        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        constexpr size_t FirstCharacterAddress =
            0x2BF00;

        constexpr size_t CharacterRecordSize =
            0x12F;

        constexpr size_t LevelOffset =
            0x35;

        const size_t levelAddress =
            FirstCharacterAddress +
            static_cast<size_t>(
                characterIndex
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

    int ItemSource::characterClassId(
        int characterIndex
    ) const
    {
        if (characterIndex < 0 ||
            characterIndex >= 6)
        {
            return 0;
        }

        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        constexpr size_t FirstCharacterAddress =
            0x2BF00;

        constexpr size_t CharacterRecordSize =
            0x12F;

        constexpr size_t ClassOffset =
            0x25;

        const size_t classAddress =
            FirstCharacterAddress +
            static_cast<size_t>(
                characterIndex
                ) *
            CharacterRecordSize +
            ClassOffset;

        if (classAddress >= memory.size())
        {
            return 0;
        }

        return static_cast<int>(
            memory[
                classAddress
            ]
            );
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

    int ItemSource::selectedCharacterClassId() const
    {
        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        constexpr size_t SelectedPartySlotAddress =
            0x2068E;

        constexpr size_t FirstCharacterAddress =
            0x2BF00;

        constexpr size_t CharacterRecordSize =
            0x12F;

        constexpr size_t ClassOffset =
            0x25;

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

        const size_t classAddress =
            FirstCharacterAddress +
            static_cast<size_t>(
                partyIndex
                ) *
            CharacterRecordSize +
            ClassOffset;

        if (classAddress >= memory.size())
        {
            return 0;
        }

        return static_cast<int>(
            memory[
                classAddress
            ]
            );
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

    int ItemSource::selectedMaterialHitBonus() const
    {
        const int materialId =
            selectedMaterialId();

        if (materialId <= 0 ||
            materialId > 22)
        {
            return 0;
        }

        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        constexpr size_t MaterialHitBonusAddress =
            0x20FA2;

        const size_t address =
            MaterialHitBonusAddress +
            static_cast<size_t>(
                materialId - 1
                );

        if (address >= memory.size())
        {
            return 0;
        }

        return static_cast<int8_t>(
            memory[address]
            );
    }

    int ItemSource::selectedMaterialDamageBonus() const
    {
        const int materialId =
            selectedMaterialId();

        if (materialId <= 0 ||
            materialId > 22)
        {
            return 0;
        }

        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        constexpr size_t MaterialDamageBonusAddress =
            0x20FB9;

        const size_t address =
            MaterialDamageBonusAddress +
            static_cast<size_t>(
                materialId - 1
                );

        if (address >= memory.size())
        {
            return 0;
        }

        return static_cast<int8_t>(
            memory[address]
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

    bool ItemSource::itemWindowActive() const
    {
        const std::vector<uint8_t>& memory =
            m_memoryReader.memory();

        constexpr size_t SelectedPartySlotAddress =
            0x2068E;

        constexpr size_t SelectedPartySlotMirrorAddress =
            0x304F4;

        constexpr size_t SelectedInventorySlotAddress =
            0x304D0;

        constexpr size_t SelectedInventorySlotMirrorAddress =
            0x304D2;

        if (SelectedPartySlotAddress >= memory.size() ||
            SelectedPartySlotMirrorAddress >= memory.size() ||
            SelectedInventorySlotAddress + 1 >= memory.size() ||
            SelectedInventorySlotMirrorAddress + 1 >= memory.size())
        {
            return false;
        }

        const uint8_t partyIndex =
            memory[
                SelectedPartySlotAddress
            ];

        const uint8_t partyMirror =
            memory[
                SelectedPartySlotMirrorAddress
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

        const uint16_t inventorySlotMirror =
            static_cast<uint16_t>(
                memory[
                    SelectedInventorySlotMirrorAddress
                ]
                ) |
            (
                static_cast<uint16_t>(
                    memory[
                        SelectedInventorySlotMirrorAddress + 1
                    ]
                    ) << 8
                );

        return
            partyIndex < 6 &&
            partyIndex == partyMirror &&
            inventorySlot < 18 &&
            inventorySlot == inventorySlotMirror;
    }
}
