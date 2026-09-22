#pragma once

#include <SDL3/SDL.h>
#include <d3d11.h>

#include "imgui.h"
#include "SvgButtonBar.h"

#include <string>
#include <filesystem>
#include <vector>
#include <functional>

#include "ColorMenu.h"

class EditorEdgeBox
{
public:
    explicit EditorEdgeBox(
        ID3D11Device* device,
        const std::filesystem::path& iconDirectory = {}
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

    void handleMouseWheel();

    const std::string&
        activeEdgeId() const;

    void clearActiveEdge();

    ImTextureID edgeTexture(
        const std::string& edgeId,
        int size
    );

    void setTextureResolver(EditorTextureResolver resolver) { m_externalTextures = bool(resolver); m_buttonBar.setTextureResolver(std::move(resolver)); }
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
    std::filesystem::path m_iconDirectory;
    bool m_externalTextures = false;
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