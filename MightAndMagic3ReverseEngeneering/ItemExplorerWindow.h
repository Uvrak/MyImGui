#pragma once

#include "ItemSource.h"

namespace ItemExplorer
{
    class ItemExplorerWindow
    {
    public:
        void setSource(
            ItemSource* source
        );

        void selectItem(
            int itemId
        );

        void draw(
            bool* isOpen = nullptr
        );

        void updateSelection(
            int itemId,
            int characterIndex
        );

    private:
        ItemSource* m_source =
            nullptr;

        int m_viewMode = 0;

        int m_selectedItemId = 0;

        int m_selectedCharacterIndex = -1;

        float m_memoryRowHeight = 0.0f;

        bool m_selectedItemLoaded = false;

        void loadSelectedItem();

        void saveSelectedItem() const;
    };
}