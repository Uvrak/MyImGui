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

        void draw(
            bool* isOpen = nullptr
        );

    private:
        ItemSource* m_source =
            nullptr;

        int m_viewMode = 0;

    };
}