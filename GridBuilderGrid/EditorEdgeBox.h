#pragma once

#include <SDL3/SDL.h>

#include "imgui.h"
#include "SvgButtonBar.h"

#include <string>
#include <vector>
#include <functional>

#include "ColorMenu.h"

class EditorEdgeBox
{
public:
    explicit EditorEdgeBox(
        SDL_Renderer* renderer
    );

    ~EditorEdgeBox() = default;
    
    using ColorIdResolver =
        ColorMenu::ColorIdResolver;

    using SetColorIdCallback =
        std::function<
        void(
            const std::string& edgeId,
            const std::string& colorId
            )
        >;

    using RemoveColorCallback =
        ColorMenu::RemoveColorCallback;

    bool draw(
        MapColorPalette& colorPalette,
        const ColorIdResolver& colorIdResolver,
        const SetColorIdCallback& setColorId,
        const RemoveColorCallback& removeColor
    );

    const std::string&
        activeEdgeId() const;

    void clearActiveEdge();

    SDL_Texture* edgeTexture(
        const std::string& edgeId,
        int size
    );

    void refreshTextures();

    void cycleActiveEdge(
        int direction
    );
    
    void addOrRefreshEdge(
        const std::string& edgeId
    );

    bool renameEdge(
        const std::string& oldEdgeId,
        const std::string& newEdgeId
    );

    void setActiveEdgeId(
        const std::string& edgeId
    );

    void openColorMenu(
        const std::string& edgeId,
        const std::string& assignedColorId,
        const ColorMenu::AssignColorCallback&
        assignColor
    );

    bool isColorMenuOpen() const;

private:
    bool drawEdgeOverlay(
        const SvgButtonDefinition& definition,
        ImVec2 buttonMin,
        ImVec2 buttonMax,
        MapColorPalette& colorPalette,
        const std::string& assignedColorId,
        const ColorMenu::AssignColorCallback& assignColor,
        const RemoveColorCallback& removeColor
    );

    bool drawEdgeCheckbox(
        const char* id,
        int edgeIndex,
        ImVec2 buttonMin,
        ImVec2 buttonMax
    );

    bool isEdgeEnabled(
        int edgeIndex
    ) const;

    void setEdgeEnabled(
        int edgeIndex,
        bool enabled
    );

private:
    MyImGui::FloatingWindow m_window;

    std::vector<std::string>
        m_edgeIds;

    SvgButtonBar m_buttonBar;

    int m_activeEdgeIndex = 0;

    std::vector<int>
        m_enabledEdgeIndices;

    double m_lastEdgeCycleTime =
        -1.0;

    void saveSettings() const;

    void loadEnabledStates();

    std::vector<int>
        m_lastButtonOrder;

	ColorMenu m_colorMenu;
};