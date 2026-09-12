#pragma once

#include "MapColorPalette.h"
#include "SvgButtonBar.h"

#include "imgui.h"

#include <functional>
#include <string>
#include <vector>

class ColorMenu
{
public:
    using ColorIdResolver =
        std::function<
        std::string(
            const std::string& itemId
        )
        >;

    void synchronizeButtons(
        SvgButtonBar& buttonBar,
        const std::vector<std::string>& itemIds,
        const MapColorPalette& colorPalette,
        const ColorIdResolver& colorIdResolver
    ) const;

    using AssignColorCallback =
        std::function<
        void(const std::string& colorId)
        >;

    using RemoveColorCallback =
        std::function<
        bool(const std::string& colorId)
        >;

    ColorMenu() = default;

    void beginFrame();
    
    void requestOpen(
        int itemValue,
        const std::string& assignedColorId,
        const AssignColorCallback& assignColor
    );

    bool draw(
        const SvgButtonDefinition& definition,
        ImVec2 buttonMin,
        ImVec2 buttonMax,
        SvgButtonBar& buttonBar,
        MapColorPalette& colorPalette,
        const std::string& assignedColorId,
        const AssignColorCallback& assignColor,
        const RemoveColorCallback& removeColor
    );

    bool isOpen() const;

private:
    inline static bool
        m_isAnyMenuOpen = false;

    inline static int
        m_lastFrameIndex = -1;

    bool m_isOpen = false;
    bool m_deleteColorBlocked = false;
    bool m_popupWasHovered = false;

    std::string m_editColorId;

    int m_requestedItemValue = -1;

    AssignColorCallback
        m_requestedAssignColor;

    AssignColorCallback
        m_activeAssignColor;

    std::string m_requestedAssignedColorId;
    std::string m_activeAssignedColorId;
};