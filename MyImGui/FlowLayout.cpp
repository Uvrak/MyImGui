#include "pch.h"
#include "FlowLayout.h"

#include <algorithm>

namespace MyImGui
{
    FlowLayout::FlowLayout(
        FlowLayoutOptions options
    )
        : m_options(options)
    {}

    void FlowLayout::begin()
    {
        const ImVec2 windowPosition =
            ImGui::GetWindowPos();

        const ImVec2 contentRegionMax =
            ImGui::GetWindowContentRegionMax();

        m_contentRight =
            windowPosition.x + contentRegionMax.x;

        m_contentBottom =
            ImGui::GetCursorScreenPos().y;

        m_lastItemRight = 0.0f;
        m_firstItem = true;
        m_active = true;
    }

    void FlowLayout::beginItem(
        const ImVec2& itemSize,
        float horizontalSpacing
    )
    {
        if (!m_active || m_firstItem)
        {
            return;
        }

        const float spacing =
            horizontalSpacing >= 0.0f
            ? horizontalSpacing
            : (
                m_options.horizontalSpacing >= 0.0f
                ? m_options.horizontalSpacing
                : ImGui::GetStyle().ItemSpacing.x
                );

        const float nextItemRight =
            m_lastItemRight +
            spacing +
            itemSize.x;

        if (!m_options.wrap ||
            nextItemRight <= m_contentRight)
        {
            ImGui::SameLine(
                0.0f,
                spacing
            );
        }
    }

    void FlowLayout::endItem()
    {
        if (!m_active)
        {
            return;
        }

        const ImVec2 itemMax =
            ImGui::GetItemRectMax();

        m_lastItemRight =
            itemMax.x;

        m_contentBottom =
            std::max(
                m_contentBottom,
                itemMax.y
            );

        m_firstItem = false;
    }

    void FlowLayout::end()
    {
        if (!m_active)
        {
            return;
        }

        ImVec2 cursorPosition =
            ImGui::GetCursorScreenPos();

        if (cursorPosition.y <
            m_contentBottom)
        {
            cursorPosition.y =
                m_contentBottom;

            ImGui::SetCursorScreenPos(
                cursorPosition
            );

            ImGui::Dummy(
                ImVec2(0.0f, 0.0f)
            );
        }

        m_active = false;
    }
}