#pragma once

#include <cstdint>
#include <optional>
#include <vector>
#include "../GridBuilderCore/GameModule.h"

namespace DosBoxX
{
    class FrameTexture;
    class Mouse;
    class NamedPipeClient;
}

namespace GridBuilderHost
{
    class DosBoxWindow
    {
    public:
        std::optional<GameButtonPoint> onLeftMouseButtonDown(
            float mouseX,
            float mouseY,
            DosBoxX::Mouse& dosBoxMouse,
            DosBoxX::NamedPipeClient& dosBoxPipeClient
        );
        void draw(
            DosBoxX::FrameTexture& frameTexture,
            uint32_t contentWidth,
            uint32_t contentHeight,
            bool showCoordinates,
            DosBoxX::Mouse& dosBoxMouse,
            DosBoxX::NamedPipeClient& dosBoxPipeClient,
            const std::vector<GameButtonRect>& buttonRects,
            std::optional<GameButtonRect> selectedButton,
            std::optional<GameButtonRect> selectedInventory,
            bool buttonEditingActive,
            bool buttonViewAvailable,
            const std::string& buttonViewName,
            bool buttonSaveFailed
        );
        std::optional<GameButtonRect> takeDrawnButton();
        std::optional<std::size_t> takeDeletedButton();
        std::optional<GameButtonEdit> takeModifiedButton();
        std::optional<GameButtonPoint> screenPosition(GameButtonPoint point) const;
    private:
        float m_imageLeft = 0.0f;
        float m_imageTop = 0.0f;
        float m_imageWidth = 0.0f;
        float m_imageHeight = 0.0f;
        uint32_t m_contentWidth = 0;
        uint32_t m_contentHeight = 0;
        bool m_drawingButton = false;
        float m_dragStartX = 0;
        float m_dragStartY = 0;
        std::optional<GameButtonRect> m_drawnButton;
        std::optional<std::size_t> m_deletedButton;
        std::optional<std::size_t> m_contextButton;
        std::string m_contextView;
        std::optional<GameButtonEdit> m_modifiedButton;
        std::optional<GameButtonEdit> m_editingButton;
        std::string m_editingView;
        GameButtonRect m_editStartRect;
        bool m_editDragging = false;
        bool m_creatingButton = false;
        int m_resizeCorner = -1;
    };
}
