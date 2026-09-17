#include "DosBoxWindow.h"
#include "Mouse.h"
#include "NamedPipeClient.h"
#include "FrameTexture.h"

#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace GridBuilderHost
{
    std::optional<GameButtonPoint> DosBoxWindow::screenPosition(GameButtonPoint point) const
    {
        if (m_imageWidth <= 0.0f || m_imageHeight <= 0.0f ||
            m_contentWidth == 0 || m_contentHeight == 0) return std::nullopt;
        return GameButtonPoint{
            static_cast<int>(m_imageLeft + point.x * m_imageWidth / m_contentWidth),
            static_cast<int>(m_imageTop + point.y * m_imageHeight / m_contentHeight)
        };
    }

    std::optional<GameButtonEdit> DosBoxWindow::takeModifiedButton()
    {
        auto result = m_modifiedButton;
        m_modifiedButton.reset();
        return result;
    }

    std::optional<std::size_t> DosBoxWindow::takeDeletedButton()
    {
        auto result = m_deletedButton;
        m_deletedButton.reset();
        return result;
    }

    std::optional<GameButtonRect> DosBoxWindow::takeDrawnButton()
    {
        auto result = m_drawnButton;
        m_drawnButton.reset();
        return result;
    }

    std::optional<GameButtonPoint> DosBoxWindow::onLeftMouseButtonDown(
        float mouseX,
        float mouseY,
        DosBoxX::Mouse& dosBoxMouse,
        DosBoxX::NamedPipeClient& dosBoxPipeClient
    )
    {
        if (m_imageWidth <= 0.0f || m_imageHeight <= 0.0f ||
            mouseX < m_imageLeft || mouseX >= m_imageLeft + m_imageWidth ||
            mouseY < m_imageTop || mouseY >= m_imageTop + m_imageHeight)
            return std::nullopt;

        const int x = static_cast<int>(
            (mouseX - m_imageLeft) / m_imageWidth * m_contentWidth);
        const int y = static_cast<int>(
            (mouseY - m_imageTop) / m_imageHeight * m_contentHeight);

        dosBoxMouse.setInputActive(dosBoxPipeClient, true);
        // The host lets the game module intercept this point before dispatch.
        return GameButtonPoint{x, y};
    }

    void DosBoxWindow::draw(
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
    )
    {
        m_imageWidth = 0.0f;
        m_imageHeight = 0.0f;
        ImGui::Begin(
            "DOSBox"
        );

        if (showCoordinates)
        {
            ImGui::TextUnformatted(
                "Coordinates ON"
            );
        }

        if (buttonEditingActive)
        {
            if (buttonViewAvailable)
                ImGui::Text("%s: Button mit linker Maustaste zeichnen", buttonViewName.c_str());
            else
                ImGui::TextUnformatted("MM3-Ansicht nicht erkannt: Button kann hier nicht gespeichert werden");
            ImGui::TextUnformatted("K: Buttons dieser View markieren | Rechtsklick: Modify Button / Delete");
            ImGui::TextUnformatted("Ziehen: verschieben | Ecke: Groesse | Neuer Button: Rechtsklick zum Speichern | Esc: abbrechen");
            if (buttonSaveFailed)
                ImGui::TextUnformatted("Button konnte nicht gespeichert werden");
        }
        else
            m_drawingButton = false;

        ID3D11ShaderResourceView* textureView =
            frameTexture.textureView();

        bool mouseInsideImage = false;

        if (textureView != nullptr &&
            contentWidth > 0 &&
            contentHeight > 0)
        {
            const ImVec2 availableSize =
                ImGui::GetContentRegionAvail();

            const float scaleX =
                availableSize.x /
                static_cast<float>(
                    contentWidth
                    );

            const float scaleY =
                availableSize.y /
                static_cast<float>(
                    contentHeight
                    );

            const float scale =
                (scaleX < scaleY)
                ? scaleX
                : scaleY;

            const ImVec2 imageSize(
                static_cast<float>(
                    contentWidth
                    ) * scale,
                static_cast<float>(
                    contentHeight
                    ) * scale
            );

            const ImVec2 imageMin =
                ImGui::GetCursorScreenPos();

            m_imageLeft = imageMin.x;
            m_imageTop = imageMin.y;
            m_imageWidth = imageSize.x;
            m_imageHeight = imageSize.y;
            m_contentWidth = contentWidth;
            m_contentHeight = contentHeight;

            ImGui::Image(
                reinterpret_cast<ImTextureID>(
                    textureView
                    ),
                imageSize,
                ImVec2(
                    0.0f,
                    0.0f
                ),
                ImVec2(
                    static_cast<float>(
                        contentWidth
                        ) /
                    static_cast<float>(
                        frameTexture.width()
                        ),
                    static_cast<float>(
                        contentHeight
                        ) /
                    static_cast<float>(
                        frameTexture.height()
                        )
                )
            );

            for (const auto& rect : buttonRects)
            {
                const ImVec2 topLeft(imageMin.x + rect.x * scale,
                    imageMin.y + rect.y * scale);
                const ImVec2 bottomRight(imageMin.x + (rect.x + rect.width) * scale,
                    imageMin.y + (rect.y + rect.height) * scale);
                ImGui::GetWindowDrawList()->AddRect(topLeft, bottomRight,
                    IM_COL32(0, 0, 0, 255), 0.0f, 0, 5.0f);
                ImGui::GetWindowDrawList()->AddRect(topLeft, bottomRight,
                    IM_COL32(0, 255, 255, 255), 0.0f, 0, 2.5f);
            }

            if (selectedButton)
            {
                ImGui::GetWindowDrawList()->AddRect(
                    ImVec2(imageMin.x + selectedButton->x * scale,
                           imageMin.y + selectedButton->y * scale),
                    ImVec2(imageMin.x + (selectedButton->x + selectedButton->width) * scale,
                           imageMin.y + (selectedButton->y + selectedButton->height) * scale),
                    IM_COL32(255, 230, 0, 255), 0.0f, 0, 4.0f);
            }

            if (selectedInventory)
            {
                const ImVec2 topLeft(
                    imageMin.x + selectedInventory->x * scale,
                    imageMin.y + selectedInventory->y * scale);
                const ImVec2 bottomRight(
                    imageMin.x + (selectedInventory->x + selectedInventory->width) * scale,
                    imageMin.y + (selectedInventory->y + selectedInventory->height) * scale);
                ImGui::GetWindowDrawList()->AddRectFilled(
                    topLeft, bottomRight, IM_COL32(255, 230, 0, 35));
                ImGui::GetWindowDrawList()->AddRect(
                    topLeft, bottomRight, IM_COL32(255, 230, 0, 255), 0.0f, 0, 2.0f);
            }

            const ImVec2 mousePos =
    ImGui::GetMousePos();

const ImVec2 imageMax(
    imageMin.x + imageSize.x,
    imageMin.y + imageSize.y
);

mouseInsideImage =
    ImGui::IsItemHovered() &&
    mousePos.x >= imageMin.x &&
    mousePos.x < imageMax.x &&
    mousePos.y >= imageMin.y &&
    mousePos.y < imageMax.y;

if (m_editingButton && (!buttonEditingActive || !buttonViewAvailable ||
    m_editingView != buttonViewName ||
    (!m_creatingButton && m_editingButton->index >= buttonRects.size()) ||
    ImGui::IsKeyPressed(ImGuiKey_Escape)))
{
    m_editingButton.reset();
    m_creatingButton = false;
    m_editDragging = false;
}

if (buttonEditingActive && buttonViewAvailable)
{
    const auto contentX = [&](float screenX) {
        return std::clamp((screenX - imageMin.x) / scale,
            0.0f, static_cast<float>(contentWidth));
    };
    const auto contentY = [&](float screenY) {
        return std::clamp((screenY - imageMin.y) / scale,
            0.0f, static_cast<float>(contentHeight));
    };
    if (mouseInsideImage && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        m_drawingButton = false;
        m_editDragging = false;
        const float x = contentX(mousePos.x);
        const float y = contentY(mousePos.y);
        if (m_creatingButton && m_editingButton &&
            x >= m_editingButton->rect.x && x <= m_editingButton->rect.x + m_editingButton->rect.width &&
            y >= m_editingButton->rect.y && y <= m_editingButton->rect.y + m_editingButton->rect.height)
        {
            ImGui::OpenPopup("New button context");
        }
        else if (!m_creatingButton)
        for (std::size_t i = buttonRects.size(); i > 0; --i)
        {
            const auto& rect = buttonRects[i - 1];
            if (x >= rect.x && x <= rect.x + rect.width &&
                y >= rect.y && y <= rect.y + rect.height)
            {
                m_contextButton = i - 1;
                m_contextView = buttonViewName;
                m_editingButton.reset();
                ImGui::OpenPopup("Button context");
                break;
            }
        }
    }
    // A saved button takes precedence over starting a new rectangle.
    if (mouseInsideImage && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        const float x = contentX(mousePos.x);
        const float y = contentY(mousePos.y);
        const auto hit = [&](const GameButtonRect& rect) {
            if (x >= rect.x && x <= rect.x + rect.width &&
                y >= rect.y && y <= rect.y + rect.height) return true;
            for (int corner = 0; corner < 4; ++corner)
            {
                const float cx = rect.x + ((corner & 1) ? rect.width : 0);
                const float cy = rect.y + ((corner & 2) ? rect.height : 0);
                if (std::abs(x - cx) * scale <= 7 && std::abs(y - cy) * scale <= 7)
                    return true;
            }
            return false;
        };
        if (!m_editingButton || !hit(m_editingButton->rect))
        {
            m_editingButton.reset();
            m_creatingButton = false;
            for (std::size_t i = buttonRects.size(); i > 0; --i)
            {
                if (hit(buttonRects[i - 1]))
                {
                    m_editingButton = GameButtonEdit{i - 1, buttonRects[i - 1]};
                    m_editingView = buttonViewName;
                    break;
                }
            }
        }
        m_drawingButton = !m_editingButton.has_value();
        m_dragStartX = x;
        m_dragStartY = y;
    }
    if (m_editingButton)
    {
        auto& rect = m_editingButton->rect;
        if (!m_editDragging && !m_creatingButton) rect = buttonRects[m_editingButton->index];
        const float x = contentX(mousePos.x);
        const float y = contentY(mousePos.y);
        if (mouseInsideImage && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_resizeCorner = -1;
            for (int corner = 0; corner < 4; ++corner)
            {
                const float cx = rect.x + ((corner & 1) ? rect.width : 0);
                const float cy = rect.y + ((corner & 2) ? rect.height : 0);
                if (std::abs(x - cx) * scale <= 7 && std::abs(y - cy) * scale <= 7)
                    m_resizeCorner = corner;
            }
            m_editDragging = m_resizeCorner >= 0 ||
                (x >= rect.x && x <= rect.x + rect.width &&
                 y >= rect.y && y <= rect.y + rect.height);
            m_dragStartX = x;
            m_dragStartY = y;
            m_editStartRect = rect;
        }
        if (m_editDragging)
        {
            if (m_resizeCorner < 0)
            {
                rect.x = std::clamp(m_editStartRect.x + x - m_dragStartX,
                    0.0f, (std::max)(0.0f, contentWidth - rect.width));
                rect.y = std::clamp(m_editStartRect.y + y - m_dragStartY,
                    0.0f, (std::max)(0.0f, contentHeight - rect.height));
            }
            else
            {
                const float oppositeX = m_editStartRect.x +
                    ((m_resizeCorner & 1) ? 0 : m_editStartRect.width);
                const float oppositeY = m_editStartRect.y +
                    ((m_resizeCorner & 2) ? 0 : m_editStartRect.height);
                const float edgeX = (m_resizeCorner & 1)
                    ? (std::max)(x, oppositeX + 2) : (std::min)(x, oppositeX - 2);
                const float edgeY = (m_resizeCorner & 2)
                    ? (std::max)(y, oppositeY + 2) : (std::min)(y, oppositeY - 2);
                rect.x = (std::min)(edgeX, oppositeX);
                rect.y = (std::min)(edgeY, oppositeY);
                rect.width = std::abs(edgeX - oppositeX);
                rect.height = std::abs(edgeY - oppositeY);
            }
        }
        const ImVec2 editMin(imageMin.x + rect.x * scale, imageMin.y + rect.y * scale);
        const ImVec2 editMax(editMin.x + rect.width * scale, editMin.y + rect.height * scale);
        auto* drawList = ImGui::GetWindowDrawList();
        drawList->AddRect(editMin, editMax, IM_COL32(255, 128, 0, 255), 0.0f, 0, 3.0f);
        for (int corner = 0; corner < 4; ++corner)
        {
            const ImVec2 point((corner & 1) ? editMax.x : editMin.x,
                (corner & 2) ? editMax.y : editMin.y);
            drawList->AddRectFilled(ImVec2(point.x - 4, point.y - 4),
                ImVec2(point.x + 4, point.y + 4), IM_COL32(255, 128, 0, 255));
        }
        if (m_editDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            const bool moved = std::abs(x - m_dragStartX) * scale > 4.0f ||
                std::abs(y - m_dragStartY) * scale > 4.0f;
            if (moved && !m_creatingButton &&
                m_editingButton->index < buttonRects.size())
            {
                const auto& original = buttonRects[m_editingButton->index];
                if (rect.x != original.x || rect.y != original.y ||
                    rect.width != original.width || rect.height != original.height)
                    m_modifiedButton = m_editingButton;
            }
            m_editDragging = false;
        }
    }
    if (m_drawingButton)
    {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
            !ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            m_drawingButton = false;
    }
    if (m_drawingButton)
    {
        const float endX = contentX(mousePos.x);
        const float endY = contentY(mousePos.y);
        const float left = (std::min)(m_dragStartX, endX);
        const float top = (std::min)(m_dragStartY, endY);
        const float right = (std::max)(m_dragStartX, endX);
        const float bottom = (std::max)(m_dragStartY, endY);
        ImGui::GetWindowDrawList()->AddRect(
            ImVec2(imageMin.x + left * scale, imageMin.y + top * scale),
            ImVec2(imageMin.x + right * scale, imageMin.y + bottom * scale),
            IM_COL32(255, 220, 0, 255), 0.0f, 0, 2.0f);
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            if (right - left >= 2.0f && bottom - top >= 2.0f)
            {
                m_editingButton = GameButtonEdit{0,
                    GameButtonRect{left, top, right - left, bottom - top}};
                m_editingView = buttonViewName;
                m_creatingButton = true;
                m_editDragging = false;
            }
            m_drawingButton = false;
        }
    }
}
else
    m_drawingButton = false;

if (ImGui::BeginPopup("New button context"))
{
    if (!m_creatingButton || !m_editingButton)
        ImGui::CloseCurrentPopup();
    else if (ImGui::MenuItem("Als Button speichern"))
    {
        m_drawnButton = m_editingButton->rect;
        m_editingButton.reset();
        m_creatingButton = false;
    }
    else if (ImGui::MenuItem("Verwerfen"))
    {
        m_editingButton.reset();
        m_creatingButton = false;
    }
    ImGui::EndPopup();
}

if (ImGui::BeginPopup("Button context"))
{
    if (!buttonEditingActive || !buttonViewAvailable ||
        m_contextView != buttonViewName || !m_contextButton ||
        *m_contextButton >= buttonRects.size())
        ImGui::CloseCurrentPopup();
    else if (ImGui::MenuItem("Modify Button"))
    {
        m_editingButton = GameButtonEdit{*m_contextButton, buttonRects[*m_contextButton]};
        m_editingView = buttonViewName;
        m_drawingButton = false;
        m_editDragging = false;
    }
    else if (ImGui::MenuItem("Delete"))
    {
        m_deletedButton = m_contextButton;
        m_contextButton.reset();
    }
    ImGui::EndPopup();
}

if (mouseInsideImage)
{
    const float localX =
        mousePos.x - imageMin.x;

    const float localY =
        mousePos.y - imageMin.y;

    const int contentX =
        static_cast<int>(
            localX /
            imageSize.x *
            static_cast<float>(
                contentWidth
            )
        );

    const int contentY =
        static_cast<int>(
            localY /
            imageSize.y *
            static_cast<float>(
                contentHeight
            )
        );

    if (showCoordinates)
    {
        ImGui::BeginTooltip();

        ImGui::Text(
            "X: %d  Y: %d",
            contentX,
            contentY
        );

        ImGui::EndTooltip();
    }

    if (!buttonEditingActive)
    {
        if (!dosBoxMouse.inputActive())
            dosBoxMouse.setInputActive(dosBoxPipeClient, true);

        dosBoxMouse.move(
            dosBoxPipeClient,
            contentX,
            contentY,
            static_cast<int>(contentWidth),
            static_cast<int>(contentHeight)
        );
    }

}
        }

        if (!mouseInsideImage && dosBoxMouse.inputActive())
        {
            dosBoxMouse.setInputActive(
                dosBoxPipeClient,
                false
            );

        }

        ImGui::End();
    }
}
