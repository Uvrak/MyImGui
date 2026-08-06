#pragma once

#include "imgui.h"

namespace MyImGui
{
    struct FlowLayoutOptions
    {
        bool wrap = true;
        float horizontalSpacing = -1.0f;
    };

    class FlowLayout
    {
    public:
        explicit FlowLayout(
            FlowLayoutOptions options = {}
        );

        void begin();

        void beginItem(
            const ImVec2& itemSize,
            float horizontalSpacing = -1.0f
        );

        void endItem();
        void end();

    private:
        FlowLayoutOptions m_options;

        float m_contentRight = 0.0f;
        float m_lastItemRight = 0.0f;

        bool m_firstItem = true;
        bool m_active = false;

        float m_contentBottom = 0.0f;
    };
}