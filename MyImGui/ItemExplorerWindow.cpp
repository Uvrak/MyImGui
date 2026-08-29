#include "pch.h"
#include "ItemExplorerWindow.h"

#include <algorithm>
#include <fstream>

#include "imgui.h"

namespace ItemExplorer
{
    void ItemExplorerWindow::setSource(
        ItemSource* source
    )
    {
        m_source = source;
    }

    void ItemExplorerWindow::selectItem(
        int itemId
    )
    {
        if (itemId <= 0)
        {
            return;
        }

        if (m_selectedItemId ==
            itemId)
        {
            return;
        }

        m_selectedItemId =
            itemId;

        saveSelectedItem();
    }

    void ItemExplorerWindow::loadSelectedItem()
    {
        std::ifstream file(
            "settings/item_explorer.cfg"
        );

        if (!file)
        {
            return;
        }

        int itemId = 0;
        int characterIndex = -1;

        if (file >> itemId >> characterIndex)
        {
            m_selectedItemId =
                itemId;

            m_selectedCharacterIndex =
                characterIndex;
        }
    }

    void ItemExplorerWindow::saveSelectedItem() const
    {
        std::ofstream file(
            "settings/item_explorer.cfg"
        );

        if (!file)
        {
            return;
        }

        file <<
            m_selectedItemId <<
            " " <<
            m_selectedCharacterIndex;
    }

    void ItemExplorerWindow::draw(
        bool* isOpen
    )
    {
        if (isOpen &&
            !*isOpen)
        {
            return;
        }

        if (!m_selectedItemLoaded)
        {
            loadSelectedItem();

            m_selectedItemLoaded =
                true;
        }

        if (!ImGui::Begin(
            "Item Explorer",
            isOpen
        ))
        {
            ImGui::End();
            return;
        }

        const char* viewModes[] =
        {
            "Show Item",
            "Show All"
        };

        ImGui::SetNextItemWidth(
            150.0f
        );

        ImGui::Combo(
            "##ItemExplorerView",
            &m_viewMode,
            viewModes,
            IM_ARRAYSIZE(viewModes)
        );

        ImGui::Separator();


        if (!m_source)
        {
            ImGui::TextUnformatted(
                "No item source."
            );

            ImGui::End();
            return;
        }

        const int characterLevel =
            m_source->characterLevel(
                m_selectedCharacterIndex
            );

        const int characterClassId =
            m_source->characterClassId(
                m_selectedCharacterIndex
            );

        const std::vector<std::string> classNames =
             m_source->classNames();

        const bool characterDataValid =
            m_selectedCharacterIndex >= 0 &&
            characterLevel > 0 &&
            characterClassId >= 0 &&
            static_cast<size_t>(
                characterClassId
                ) < classNames.size();

        ImGui::Text(
            "DEBUG char=%d level=%d class=%d names=%zu",
            m_selectedCharacterIndex,
            characterLevel,
            characterClassId,
            classNames.size()
        );

        if (characterDataValid)

        if (characterDataValid)
        {
            ImGui::Text(
                "%s    Level %d",
                classNames[
                    static_cast<size_t>(
                        characterClassId
                        )
                ].c_str(),
                        characterLevel
                        );
        }
        else
        {
            ImGui::TextUnformatted(
                "--    Level --"
            );
        }

        ImGui::Separator();

        const auto& items =
            m_source->items();
        if (m_viewMode == 0)
        {
            const Item* selectedItem =
                m_source->findById(
                    m_selectedItemId
                );

            if (selectedItem)
            {
                const std::string selectedItemName =
                    m_source->selectedItemName();

                ImGui::Text(
                    "%d - %s",
                    selectedItem->id,
                    selectedItemName.empty()
                    ? selectedItem->name.c_str()
                    : selectedItemName.c_str()
                );

                for (const ItemProperty& property :
                    selectedItem->properties)
                {
                    ImGui::Text(
                        "%s: %s",
                        property.name.c_str(),
                        property.value.c_str()
                    );
                }

                const int materialHitBonus =
                    m_source->selectedMaterialHitBonus();

                const int materialDamageBonus =
                    m_source->selectedMaterialDamageBonus();

                ImGui::Separator();

                ImGui::TextUnformatted(
                    "To Hit"
                );

                ImGui::Text(
                    "Material: %+d",
                    materialHitBonus
                );

                if (selectedItem->diceCount > 0 &&
                    selectedItem->diceSides > 0)
                {
                    const int baseDamageMin =
                        selectedItem->diceCount;

                    const int baseDamageMax =
                        selectedItem->diceCount *
                        selectedItem->diceSides;

                    const int weaponDamageMin =
                        std::max(
                            1,
                            baseDamageMin +
                            materialDamageBonus
                        );

                    const int weaponDamageMax =
                        std::max(
                            1,
                            baseDamageMax +
                            materialDamageBonus
                        );

                    ImGui::Separator();

                    ImGui::TextUnformatted(
                        "Damage"
                    );

                    ImGui::Text(
                        "Base: %d - %d",
                        baseDamageMin,
                        baseDamageMax
                    );

                    ImGui::Text(
                        "Material: %+d",
                        materialDamageBonus
                    );

                    ImGui::Text(
                        "Calculated: %d - %d",
                        weaponDamageMin,
                        weaponDamageMax
                    );
                }
            }
            else
            {
                ImGui::TextUnformatted(
                    "No item selected."
                );
            }
        }
        else
        {
            ImGui::Text(
                "Items: %zu",
                items.size()
            );

            ImGui::Separator();

            for (const Item& item :
                items)
            {
                ImGui::Text(
                    "%d - %s",
                    item.id,
                    item.name.c_str()
                );
            }
        }

        ImGui::End();
    }

    void ItemExplorerWindow::updateSelection(
        int itemId,
        int characterIndex
    )
    {
        if (itemId <= 0 ||
            characterIndex < 0)
        {
            return;
        }

        if (m_selectedItemId == itemId &&
            m_selectedCharacterIndex ==
            characterIndex)
        {
            return;
        }

        // Item UND Owner gemeinsam wechseln
        m_selectedItemId =
            itemId;

        m_selectedCharacterIndex =
            characterIndex;

        saveSelectedItem();
    }
}