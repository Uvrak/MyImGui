#include "pch.h"

#include "PixelColorWindow.h"

#include "imgui.h"

#include <cstdint>

void PixelColorWindow::draw(
    bool* isOpen
)
{
    const bool windowVisible =
        ImGui::Begin(
            "Colors",
            isOpen
        );

    if (windowVisible)
    {
        ImGui::ColorPicker3(
            "##PencilColor",
            m_color
        );
    }

    ImGui::End();
}

PixelColor PixelColorWindow::color() const
{
    return
    {
        static_cast<std::uint8_t>(
            m_color[0] * 255.0f +
            0.5f
        ),
        static_cast<std::uint8_t>(
            m_color[1] * 255.0f +
            0.5f
        ),
        static_cast<std::uint8_t>(
            m_color[2] * 255.0f +
            0.5f
        ),
        255
    };
}