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

    private:
        ItemSource* m_source =
            nullptr;

        int m_viewMode = 0;

        int m_selectedItemId = 0;

        float m_memoryRowHeight = 0.0f;

    };
}