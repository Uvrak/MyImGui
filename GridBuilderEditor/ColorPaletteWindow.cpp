#include "pch.h"

#include "ColorPaletteWindow.h"

#include "imgui.h"

#include <cstdint>

namespace
{
    ImVec4 toImVec4(
        const PixelColor& color
    )
    {
        constexpr float scale =
            1.0f / 255.0f;

        return ImVec4(
            color.red * scale,
            color.green * scale,
            color.blue * scale,
            color.alpha * scale
        );
    }
}

void ColorPaletteWindow::draw(
    bool* isOpen,
    bool eyedropperActive
)
{
    const bool windowVisible =
        ImGui::Begin(
            "Colors",
            isOpen
        );

    if (!windowVisible)
    {
        ImGui::End();
        return;
    }

    ImGui::ColorPicker3(
        "##ColorPicker",
        m_pickerColor
    );

    ImGui::Separator();

    for (int index = 0;
        index <
        static_cast<int>(m_colors.size());
        ++index)
    {
        ImGui::PushID(index);

        const bool isSelected =
            index == m_selectedColorIndex;

        const ImVec4 color =
            toImVec4(
                m_colors[index]
            );

        ImGui::PushStyleColor(
            ImGuiCol_Button,
            color
        );

        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered,
            ImVec4(
                color.x * 0.8f + 0.2f,
                color.y * 0.8f + 0.2f,
                color.z * 0.8f + 0.2f,
                1.0f
            )
        );

        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive,
            color
        );

        if (isSelected)
        {
            ImGui::PushStyleVar(
                ImGuiStyleVar_FrameBorderSize,
                3.0f
            );

            ImGui::PushStyleColor(
                ImGuiCol_Border,
                ImVec4(
                    1.0f,
                    1.0f,
                    1.0f,
                    1.0f
                )
            );
        }

        if (ImGui::Button(
            "##PaletteColor",
            ImVec2(40.0f, 40.0f)
        ))
        {
            if (eyedropperActive)
            {
                m_colors[index] =
                    pickerColor();
            }

            m_selectedColorIndex =
                index;
        }

        if (isSelected)
        {
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }

        ImGui::PopStyleColor(3);
        ImGui::PopID();

        if ((index + 1) % 4 != 0)
        {
            ImGui::SameLine();
        }
    }
    
    ImGui::End();
}

const PixelColor&
ColorPaletteWindow::selectedColor() const
{
    return m_colors[
        m_selectedColorIndex
    ];
}

PixelColor ColorPaletteWindow::pickerColor() const
{
    return
    {
        static_cast<std::uint8_t>(
            m_pickerColor[0] *
            255.0f +
            0.5f
        ),
        static_cast<std::uint8_t>(
            m_pickerColor[1] *
            255.0f +
            0.5f
        ),
        static_cast<std::uint8_t>(
            m_pickerColor[2] *
            255.0f +
            0.5f
        ),
        255
    };
}