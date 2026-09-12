#pragma once

#include <SDL3/SDL.h>

#include "MyImGui.h"
#include "SvgButtonBar.h"

#include <string>
#include <vector>
#include <functional>

#include "ColorMenu.h"

#include <filesystem>

class EditorMiscBox
{
public:

    using ColorIdResolver =
        ColorMenu::ColorIdResolver;

    using SetColorIdCallback =
        std::function<
        void(
            const std::string& miscId,
            const std::string& colorId
            )
        >;

    explicit EditorMiscBox(
        SDL_Renderer* renderer
    );

    using RemoveColorCallback =
        ColorMenu::RemoveColorCallback;

    ~EditorMiscBox() = default;

    bool draw(
        MapColorPalette& colorPalette,
        const ColorIdResolver& colorIdResolver,
        const SetColorIdCallback& setColorId,
        const RemoveColorCallback& removeColor  
    );

    
    void saveColors(
        const std::filesystem::path& path,
        const SvgButtonBar& buttonBar,
        const std::vector<std::string>& itemIds
    ) const;

    void loadColors(
        const std::filesystem::path& path,
        SvgButtonBar& buttonBar,
        const std::vector<std::string>& itemIds
    );

    const std::string&
        activeMiscId() const;

    void setActiveMiscId(
        const std::string& miscId
    );

    void clearActiveMisc();

    void openColorMenu(
        const std::string& miscId,
        const std::string& assignedColorId,
        const ColorMenu::AssignColorCallback&
        assignColor
    );

    SDL_Texture* miscTexture(
        const std::string& miscId,
        int size
    );

    void refreshTextures();

    void addOrRefreshMisc(
        const std::string& miscId
    );

    bool renameMisc(
        const std::string& oldMiscId,
        const std::string& newMiscId
    );

private:
    bool drawMiscOverlay(
        const SvgButtonDefinition& definition,
        ImVec2 buttonMin,
        ImVec2 buttonMax,
        MapColorPalette& colorPalette,
        const std::string& assignedColorId,
        const ColorMenu::AssignColorCallback& assignColor,
        const RemoveColorCallback& removeColor  
    );

private:
    MyImGui::FloatingWindow m_window;

    std::vector<std::string>
        m_miscIds;

    SvgButtonBar m_buttonBar;

    int m_activeMiscIndex = 0;

    ColorMenu m_colorMenu;
};