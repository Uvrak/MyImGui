#include "SvgButtonBar.h"

#include <utility>

SvgButtonBar::SvgButtonBar(
    SDL_Renderer* renderer,
    std::vector<SvgButtonDefinition>
    definitions
)
    : m_textureCache(renderer)
{
    m_buttons.reserve(
        definitions.size()
    );

    for (std::size_t index = 0;
        index < definitions.size();
        ++index)
    {
        SvgButtonDefinition& definition =
            definitions[index];

        Button button;

        button.definition =
            std::move(definition);

        button.id =
            "##SvgButton" +
            std::to_string(index);

        button.texture =
            m_textureCache.texture(
                button.definition.iconPath,
                button.definition.style.textureWidth,
                button.definition.style.textureHeight
            );

        m_buttons.push_back(
            std::move(button)
        );
    }
}

void SvgButtonBar::setButtonColor(
    int value,
    const ImVec4& color
)
{
    for (Button& button :
        m_buttons)
    {
        if (button.definition.value ==
            value)
        {
            button.definition.customColor =
                color;

            button.definition.hasCustomColor =
                true;

            return;
        }
    }
}

bool SvgButtonBar::draw(
    int& activeValue,
    const OverlayCallback& overlay
)
{
    bool anyButtonClicked = false;

    m_dragDropReorder.begin();

    m_flowLayout.begin();

    for (std::size_t index = 0;
        index < m_buttons.size();
        ++index)
    {
        Button& button =
            m_buttons[index];

        if (button.texture == nullptr)
        {
            continue;
        }

        const ImVec2 framePadding =
            ImGui::GetStyle().FramePadding;

        const ImVec2 buttonSize(
            button.definition.style.width +
            framePadding.x * 2.0f,
            button.definition.style.height +
            framePadding.y * 2.0f
        );

        m_flowLayout.beginItem(
            buttonSize,
            button.definition.style.spacing
        );

        const bool isActive =
            activeValue ==
            button.definition.value;

        if (isActive)
        {
            ImGui::PushStyleColor(
                ImGuiCol_Button,
                ImGui::GetStyleColorVec4(
                    ImGuiCol_ButtonActive
                )
            );
        }


        const bool clicked =
            ImGui::InvisibleButton(
                button.id.c_str(),
                buttonSize
            );

        const ImVec2 buttonMin =
            ImGui::GetItemRectMin();

        const ImVec2 buttonMax =
            ImGui::GetItemRectMax();

        const bool hovered =
            ImGui::IsItemHovered();

        const bool held =
            ImGui::IsItemActive();

        ImU32 backgroundColor =
            ImGui::GetColorU32(
                ImGuiCol_Button
            );

        if (held)
        {
            backgroundColor =
                ImGui::GetColorU32(
                    ImGuiCol_ButtonActive
                );
        }
        else if (hovered)
        {
            backgroundColor =
                ImGui::GetColorU32(
                    ImGuiCol_ButtonHovered
                );
        }

        ImDrawList* drawList =
            ImGui::GetWindowDrawList();

        drawList->AddRectFilled(
            buttonMin,
            buttonMax,
            backgroundColor,
            ImGui::GetStyle().FrameRounding
        );

        drawList->AddRect(
            buttonMin,
            buttonMax,
            ImGui::GetColorU32(
                ImGuiCol_Border
            ),
            ImGui::GetStyle().FrameRounding
        );

        const ImVec2 imageMinimum(
            buttonMin.x + framePadding.x,
            buttonMin.y + framePadding.y
        );

        const ImVec2 imageMaximum(
            buttonMax.x - framePadding.x,
            buttonMax.y - framePadding.y
        );

        ImU32 imageColor =
            IM_COL32_WHITE;

        if (button.definition.hasCustomColor)
        {
            imageColor =
                ImGui::GetColorU32(
                    button.definition.customColor
                );
        }
        ImVec2 uvTopLeft;
        ImVec2 uvTopRight;
        ImVec2 uvBottomRight;
        ImVec2 uvBottomLeft;

        if (button.definition.rotation ==
            SvgButtonRotation::Right)
        {
            uvTopLeft =
                ImVec2(0.0f, 1.0f);

            uvTopRight =
                ImVec2(0.0f, 0.0f);

            uvBottomRight =
                ImVec2(1.0f, 0.0f);

            uvBottomLeft =
                ImVec2(1.0f, 1.0f);
        }
        else
        {
            uvTopLeft =
                ImVec2(0.0f, 0.0f);

            uvTopRight =
                ImVec2(1.0f, 0.0f);

            uvBottomRight =
                ImVec2(1.0f, 1.0f);

            uvBottomLeft =
                ImVec2(0.0f, 1.0f);
        }

        drawList->AddImageQuad(
            (ImTextureID)(intptr_t)
            button.texture,

            imageMinimum,

            ImVec2(
                imageMaximum.x,
                imageMinimum.y
            ),

            imageMaximum,

            ImVec2(
                imageMinimum.x,
                imageMaximum.y
            ),

            uvTopLeft,
            uvTopRight,
            uvBottomRight,
            uvBottomLeft,
            imageColor
        );

        m_dragDropReorder.handleItem(
            index
        );

        bool overlayClicked = false;

        if (overlay)
        {
            overlayClicked =
                overlay(
                    button.definition,
                    buttonMin,
                    buttonMax
                );
        }

        if (clicked &&
            !overlayClicked)
        {
            activeValue =
                button.definition.value;

            anyButtonClicked = true;
        }

        if (hovered &&
            !overlayClicked &&
            !button.definition.tooltip.empty())
        {
            ImGui::SetTooltip(
                "%s",
                button.definition.tooltip.c_str()
            );
        }


        if (isActive)
        {
            ImGui::PopStyleColor();
        }

        m_flowLayout.endItem();
    }

    m_dragDropReorder.apply(
        m_buttons
    );

    m_flowLayout.end();

    return anyButtonClicked;
}


SDL_Texture* SvgButtonBar::texture(
    int value
) const
{
    for (const Button& button : m_buttons)
    {
        if (button.definition.value ==
            value)
        {
            return button.texture;
        }
    }

    return nullptr;
}

void SvgButtonBar::refreshTextures()
{
    m_textureCache.clear();

    for (Button& button : m_buttons)
    {
        button.texture =
            m_textureCache.texture(
                button.definition.iconPath,
                button.definition.style.textureWidth,
                button.definition.style.textureHeight
            );
    }
}

void SvgButtonBar::addButton(
    SvgButtonDefinition definition
)
{
    Button button;

    button.definition =
        std::move(definition);

    button.id =
        "##SvgButton" +
        std::to_string(
            m_buttons.size()
        );

    button.texture =
        m_textureCache.texture(
            button.definition.iconPath,
            button.definition.style.textureWidth,
            button.definition.style.textureHeight
        );

    m_buttons.push_back(
        std::move(button)
    );
}

bool SvgButtonBar::replaceButton(
    int value,
    SvgButtonDefinition definition
)
{
    for (Button& button :
        m_buttons)
    {
        if (button.definition.value !=
            value)
        {
            continue;
        }

        definition.value =
            value;

        definition.hasCustomColor =
            button.definition.hasCustomColor;

        definition.customColor =
            button.definition.customColor;

        button.definition =
            std::move(definition);

        button.texture =
            m_textureCache.texture(
                button.definition.iconPath,
                button.definition.style.textureWidth,
                button.definition.style.textureHeight
            );

        return true;
    }

    return false;
}

std::vector<int>
SvgButtonBar::orderedValues() const
{
    std::vector<int> values;

    values.reserve(
        m_buttons.size()
    );

    for (const Button& button :
        m_buttons)
    {
        values.push_back(
            button.definition.value
        );
    }

    return values;
}

ImVec4 SvgButtonBar::buttonColor(
    int value
) const
{
    for (const Button& button :
        m_buttons)
    {
        if (button.definition.value !=
            value)
        {
            continue;
        }

        if (button.definition.hasCustomColor)
        {
            return button.definition.customColor;
        }

        return ImVec4(
            1.0f,
            1.0f,
            1.0f,
            1.0f
        );
    }

    return ImVec4(
        1.0f,
        1.0f,
        1.0f,
        1.0f
    );
}
