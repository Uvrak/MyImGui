#pragma once

#include <SDL3/SDL.h>

#include "imgui.h"
#include "SvgTextureCache.h"

#include <string>
#include <vector>

#include <functional>
#include "MyImGui.h"

struct SvgButtonStyle
{
    float width = 48.0f;
    float height = 48.0f;

    int textureWidth = 64;
    int textureHeight = 64;

    float spacing = 4.0f;
};

enum class SvgButtonRotation
{
    None,
    Right
};

struct SvgButtonDefinition
{
    int value = 0;

    std::string iconPath;
    std::string tooltip;

    SvgButtonRotation rotation =
        SvgButtonRotation::Right;
    
    SvgButtonStyle style;

    bool hasCustomColor = false;

    ImVec4 customColor =
        ImVec4(
            0.0f,
            0.0f,
            0.0f,
            1.0f
        );
};

class SvgButtonBar
{
public:
    using OverlayCallback =
        std::function<bool(
            const SvgButtonDefinition&,
            ImVec2 buttonMin,
            ImVec2 buttonMax
            )>;

    SvgButtonBar(
        SDL_Renderer* renderer,
        std::vector<SvgButtonDefinition>
        definitions
    );

    void setButtonColor(
        int value,
        const ImVec4& color
    );

    bool draw(
        int& activeValue,
		const OverlayCallback& overlayCallback = {}
    );

    SDL_Texture* texture(
        int value
    ) const;

    void refreshTextures();

    void addButton(
        SvgButtonDefinition definition
    );

    bool replaceButton(
        int value,
        SvgButtonDefinition definition
    );

    std::vector<int> orderedValues() const;

    ImVec4 buttonColor(
        int value
    ) const;

private:
    struct Button
    {
        SvgButtonDefinition definition;

        std::string id;

        SDL_Texture* texture = nullptr;
    };

    SvgTextureCache m_textureCache;

    std::vector<Button> m_buttons;

    MyImGui::FlowLayout m_flowLayout;

    MyImGui::DragDropReorder
        m_dragDropReorder;
};
