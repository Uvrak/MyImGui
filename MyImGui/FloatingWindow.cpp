#include "pch.h"
#include "FloatingWindow.h"

#include "imgui.h"

#include <utility>
#include <algorithm>

#include "imgui_internal.h"

namespace MyImGui
{
    FloatingWindow::FloatingWindow(
        std::string title,
        FloatingWindowOptions options
    )
        : m_title(std::move(title)),
        m_options(options)
    {
    }

    bool FloatingWindow::begin()
    {
        m_hasBegun = false;

        if (!m_isOpen)
        {
            return false;
        }

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_None;

        if (!m_options.movable)
        {
            flags |= ImGuiWindowFlags_NoMove;
        }

        if (!m_options.resizable)
        {
            flags |= ImGuiWindowFlags_NoResize;
        }

        if (!m_options.collapsible)
        {
            flags |= ImGuiWindowFlags_NoCollapse;
        }

        if (!m_options.titleBar)
        {
            flags |= ImGuiWindowFlags_NoTitleBar;
        }

        if (!m_options.dockable)
        {
            flags |= ImGuiWindowFlags_NoDocking;
        }

        bool* openState =
            m_options.closable
            ? &m_isOpen
            : nullptr;

        m_hasBegun = true;

        m_contentVisible =
            ImGui::Begin(
                m_title.c_str(),
                openState,
                flags
            );

        return m_contentVisible;
    }

    void FloatingWindow::end()
    {
        if (!m_hasBegun)
        {
            return;
        }

        if (m_options.autoResizeHeight &&
            m_contentVisible &&
            m_options.resizable)
        {
            const ImGuiWindow* window =
                ImGui::GetCurrentWindow();

            if (window->ResizeBorderHeld != -1)
            {
                m_heightManuallyResized = true;
            }
        }

        if (m_options.autoResizeHeight &&
            m_contentVisible &&
            !m_heightManuallyResized)
        {
            constexpr float bottomMargin = 4.0f;

            const ImGuiWindow* window =
                ImGui::GetCurrentWindow();

            const float requiredHeight =
                window->DC.CursorMaxPos.y -
                window->Pos.y +
                ImGui::GetStyle().WindowPadding.y +
                bottomMargin;

            const ImVec2 requiredSize(
                ImGui::GetWindowSize().x,
                requiredHeight
            );

            const ImGuiID dockId =
                ImGui::GetWindowDockID();

            if (dockId == 0)
            {
                ImGui::SetWindowSize(
                    requiredSize,
                    ImGuiCond_Always
                );
            }
        }

        ImGui::End();

        m_hasBegun = false;
        m_contentVisible = false;
    }

    bool FloatingWindow::isOpen() const
    {
        return m_isOpen;
    }

    void FloatingWindow::open()
    {
        m_isOpen = true;
    }

    void FloatingWindow::close()
    {
        m_isOpen = false;
    }
}