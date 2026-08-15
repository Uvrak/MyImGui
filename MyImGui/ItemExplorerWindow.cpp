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

        ImGui::Separator();

        const auto& items =
            m_source->items();

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

        ImGui::End();
    }
}