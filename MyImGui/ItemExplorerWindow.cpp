#include "pch.h"
#include "ItemExplorerWindow.h"

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
        m_selectedItemId =
            itemId;
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

        ImGui::Text(
            "Game: %s",
            m_source->gameName()
        );

        ImGui::Text(
            "Selected Item ID: %d",
            m_selectedItemId
        );

        const int characterLevel =
            m_source->selectedCharacterLevel();

        const int characterAccuracy =
            m_source->selectedCharacterAccuracy();

        if (characterLevel > 0)
        {
            ImGui::Text(
                "Character Level: %d",
                characterLevel
            );
        }

		ImGui::SameLine();

        ImGui::Text(
            "Character Accuracy: %d",
            characterAccuracy
        );
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
}