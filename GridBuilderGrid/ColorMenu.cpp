#include "ColorMenu.h"

#include <algorithm>
#include <cstdint>

namespace
{
    ImVec4 toImVec4(
        const EdgeColor& color
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

    EdgeColor toEdgeColor(
        const ImVec4& color
    )
    {
        const auto colorByte =
            [](float value)
            {
                return static_cast<std::uint8_t>(
                    value * 255.0f +
                    0.5f
                    );
            };

        return EdgeColor
        {
            colorByte(color.x),
            colorByte(color.y),
            colorByte(color.z),
            colorByte(color.w)
        };
    }
}

void ColorMenu::synchronizeButtons(
    SvgButtonBar& buttonBar,
    const std::vector<std::string>& itemIds,
    const MapColorPalette& colorPalette,
    const ColorIdResolver& colorIdResolver
) const
{
    if (!colorIdResolver)
    {
        return;
    }

    for (int itemIndex = 0;
        itemIndex <
        static_cast<int>(
            itemIds.size()
            );
        ++itemIndex)
    {
        const std::string colorId =
            colorIdResolver(
                itemIds[itemIndex]
            );

        const MapColor* mapColor =
            colorPalette.find(
                colorId
            );

        if (mapColor == nullptr)
        {
            continue;
        }

        buttonBar.setButtonColor(
            itemIndex,
            toImVec4(
                mapColor->color
            )
        );
    }
}
void ColorMenu::beginFrame()
{
    const int frameIndex =
        ImGui::GetFrameCount();

    if (m_lastFrameIndex != frameIndex)
    {
        m_lastFrameIndex = frameIndex;
        m_isAnyMenuOpen = false;
    }

    m_isOpen = false;
}

void ColorMenu::requestOpen(
    int itemValue,
    const std::string& assignedColorId,
    const AssignColorCallback& assignColor
)
{
    m_requestedItemValue =
        itemValue;

    m_requestedAssignedColorId =
        assignedColorId;

    m_requestedAssignColor =
        assignColor;
}
bool ColorMenu::isOpen() const
{
    return m_isAnyMenuOpen;
}

bool ColorMenu::draw(
    const SvgButtonDefinition& definition,
    ImVec2 buttonMin,
    ImVec2 buttonMax,
    SvgButtonBar& buttonBar,
    MapColorPalette& colorPalette,
    const std::string& assignedColorId,
    const AssignColorCallback& assignColor,
    const RemoveColorCallback& removeColor
)
{
    ImGui::PushID(this);
    ImGui::PushID(definition.value);

    bool changed = false;

    std::string currentAssignedColorId =
        assignedColorId;

    if (m_activeAssignColor &&
        !m_activeAssignedColorId.empty())
    {
        currentAssignedColorId =
            m_activeAssignedColorId;
    }

    const bool buttonHovered =
        ImGui::IsMouseHoveringRect(
            buttonMin,
            buttonMax
        );

    const bool popupOpen =
        ImGui::IsPopupOpen(
            nullptr,
            ImGuiPopupFlags_AnyPopupId
        );

    const bool openRequested =
        m_requestedItemValue ==
        definition.value;

    if (!popupOpen &&
        (
            (
                buttonHovered &&
                ImGui::IsMouseClicked(
                    ImGuiMouseButton_Right
                )
                ) ||
            openRequested
            ))
    {
        if (openRequested)
        {
            m_activeAssignColor =
                m_requestedAssignColor;

            m_activeAssignedColorId =
                m_requestedAssignedColorId;
        }
        else
        {
            m_activeAssignColor = {};
            m_activeAssignedColorId.clear();
        }

        m_requestedItemValue = -1;
        m_requestedAssignColor = {};
        m_requestedAssignedColorId.clear();

        m_deleteColorBlocked = false;

        if (colorPalette.find(
            currentAssignedColorId
        ) != nullptr)
        {
            m_editColorId =
                currentAssignedColorId;
        }
        else if (!colorPalette.colors().empty())
        {
            m_editColorId =
                colorPalette.colors().front().id;
        }
        else
        {
            m_editColorId.clear();
        }

        m_popupWasHovered = false;
        if (openRequested)
        {
            const ImVec2 mousePosition =
                ImGui::GetMousePos();

            ImGui::SetNextWindowPos(
                ImVec2(
                    mousePosition.x + 30.0f,
                    mousePosition.y + 30.0f
                ),
                ImGuiCond_Appearing
            );
        }

        ImGui::OpenPopup(
            "##ColorContextMenu"
        );
    }

    if (ImGui::BeginPopup(
        "##ColorContextMenu"
    ))
    {
        m_isOpen = true;
        m_isAnyMenuOpen = true;

        ImGui::TextUnformatted(
            definition.tooltip.c_str()
        );

        ImGui::Separator();

        ImGui::TextUnformatted(
            "Map Colors"
        );

        ImGui::Separator();

        constexpr float colorButtonSize =
            32.0f;

        const float spacing =
            ImGui::GetStyle().ItemSpacing.x;

        const float availableWidth =
            ImGui::GetContentRegionAvail().x;

        const int colorColumnCount =
            std::max(
                1,
                static_cast<int>(
                    (availableWidth + spacing) /
                    (colorButtonSize + spacing)
                    )
            );

        const std::vector<MapColor>& colors =
            colorPalette.colors();

        std::string requestedDeleteColorId;
        
        bool childPopupOpen = false;
        
        for (int index = 0;
            index <
            static_cast<int>(
                colors.size()
                );
            ++index)
        {
            const MapColor& mapColor =
                colors[index];

            ImGui::PushID(
                mapColor.id.c_str()
            );

            if (ImGui::ColorButton(
                "##MapColor",
                toImVec4(mapColor.color),
                ImGuiColorEditFlags_NoTooltip,
                ImVec2(
                    colorButtonSize,
                    colorButtonSize
                )
            ))
            {
                currentAssignedColorId =
                    mapColor.id;

                if (m_activeAssignColor)
                {
                    m_activeAssignedColorId =
                        mapColor.id;
                }

                m_editColorId =
                    mapColor.id;

                if (m_activeAssignColor)
                {
                    m_activeAssignColor(
                        mapColor.id
                    );
                }
                else if (assignColor)
                {
                    assignColor(
                        mapColor.id
                    );
                }

                buttonBar.setButtonColor(
                    definition.value,
                    toImVec4(mapColor.color)
                );

                changed = true;
            }

            if (mapColor.id ==
                currentAssignedColorId)
            {
                ImGui::GetWindowDrawList()->
                    AddRect(
                        ImGui::GetItemRectMin(),
                        ImGui::GetItemRectMax(),
                        IM_COL32(
                            255,
                            220,
                            80,
                            255
                        ),
                        3.0f,
                        0,
                        3.0f
                    );
            }

            const bool colorButtonHovered =
                ImGui::IsItemHovered();

            if (index > 0 &&
                colorButtonHovered &&
                ImGui::IsKeyPressed(
                    ImGuiKey_Delete
                ))
            {
                requestedDeleteColorId =
                    mapColor.id;
            }

            if (ImGui::BeginPopupContextItem(
                "##MapColorContextMenu"
            ))
            {
                childPopupOpen = true;

                if (index > 0)
                {
                    if (ImGui::MenuItem(
                        "Delete"
                    ))
                    {
                        requestedDeleteColorId =
                            mapColor.id;
                    }
                }

                
                ImGui::EndPopup();
            }

            ImGui::PopID();

            if ((index + 1) %
                colorColumnCount != 0)
            {
                ImGui::SameLine();
            }
        }

        if (!requestedDeleteColorId.empty())
        {
            const std::string fallbackColorId =
                colorPalette.colors().front().id;

            if (removeColor &&
                removeColor(
                    requestedDeleteColorId
                ))
            {
                if (m_editColorId ==
                    requestedDeleteColorId)
                {
                    m_editColorId =
                        fallbackColorId;
                }

                if (currentAssignedColorId ==
                    requestedDeleteColorId)
                {
                    currentAssignedColorId =
                        fallbackColorId;
                }

                m_deleteColorBlocked = false;
                changed = true;
            }
            else
            {
                m_deleteColorBlocked = true;

                ImGui::OpenPopup(
                    "Cannot delete color##ColorDeleteBlocked"
                );
            }
        }
        if (ImGui::Button(
            "+##AddMapColor",
            ImVec2(
                colorButtonSize,
                colorButtonSize
            )
        ))
        {
            const std::string colorId =
                colorPalette.addColor(
                    EdgeColor
                    {
                        128,
                        128,
                        128,
                        255
                    }
                );

            m_editColorId =
                colorId;

            currentAssignedColorId =
                colorId;

            if (m_activeAssignColor)
            {
                m_activeAssignedColorId =
                    colorId;
            }

            if (m_activeAssignColor)
            {
                m_activeAssignColor(
                    colorId
                );
            }
            else if (assignColor)
            {
                assignColor(
                    colorId
                );
            }

            const MapColor* mapColor =
                colorPalette.find(colorId);

            if (mapColor != nullptr)
            {
                buttonBar.setButtonColor(
                    definition.value,
                    toImVec4(mapColor->color)
                );
            }

            changed = true;
        }

        if (ImGui::BeginPopupModal(
            "Cannot delete color##ColorDeleteBlocked",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize
        ))
        {
            ImGui::TextWrapped(
                "This color cannot be deleted "
                "because it is used by map cells."
            );

            ImGui::Separator();

            if (ImGui::Button(
                "OK",
                ImVec2(100.0f, 0.0f)
            ))
            {
                m_deleteColorBlocked = false;

                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        MapColor* editColor =
            colorPalette.find(
                m_editColorId
            );

        if (editColor != nullptr)
        {
            ImGui::Separator();

            ImVec4 color =
                toImVec4(
                    editColor->color
                );

            ImGui::PushID(
                editColor->id.c_str()
            );

            const bool pickerChanged =
                ImGui::ColorPicker3(
                    "##MapColorPicker",
                    &color.x
                );

            ImGui::PopID();

            if (pickerChanged)
            {
                editColor->color =
                    toEdgeColor(color);

                if (editColor->id ==
                    currentAssignedColorId)
                {
                    buttonBar.setButtonColor(
                        definition.value,
                        color
                    );
                }

                changed = true;
            }
        }

        const ImVec2 popupMin =
            ImGui::GetWindowPos();

        const ImVec2 popupMax =
        {
            popupMin.x +
                ImGui::GetWindowSize().x,
            popupMin.y +
                ImGui::GetWindowSize().y
        };

        const ImVec2 mousePosition =
            ImGui::GetMousePos();

        const bool mouseInsidePopup =
            mousePosition.x >= popupMin.x &&
            mousePosition.x <= popupMax.x &&
            mousePosition.y >= popupMin.y &&
            mousePosition.y <= popupMax.y;

        if (mouseInsidePopup)
        {
            m_popupWasHovered = true;
        }
        else if (m_popupWasHovered &&
            !childPopupOpen &&
            !m_deleteColorBlocked)
        {
            m_popupWasHovered = false;

            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::PopID();
    ImGui::PopID();

    return changed;
}