#include "DosBoxWindow.h"
#include "Mouse.h"
#include "NamedPipeClient.h"
#include "FrameTexture.h"

#include "imgui.h"

namespace GridBuilderHost
{
    void DosBoxWindow::onLeftMouseButtonDown(
        float mouseX,
        float mouseY,
        DosBoxX::Mouse& dosBoxMouse,
        DosBoxX::NamedPipeClient& dosBoxPipeClient
    )
    {
        if (m_imageWidth <= 0.0f || m_imageHeight <= 0.0f ||
            mouseX < m_imageLeft || mouseX >= m_imageLeft + m_imageWidth ||
            mouseY < m_imageTop || mouseY >= m_imageTop + m_imageHeight)
            return;

        const int x = static_cast<int>(
            (mouseX - m_imageLeft) / m_imageWidth * m_contentWidth);
        const int y = static_cast<int>(
            (mouseY - m_imageTop) / m_imageHeight * m_contentHeight);

        dosBoxMouse.setInputActive(dosBoxPipeClient, true);
        dosBoxMouse.click(dosBoxPipeClient, x, y,
            static_cast<int>(m_contentWidth),
            static_cast<int>(m_contentHeight));
    }

    void DosBoxWindow::draw(
        DosBoxX::FrameTexture& frameTexture,
        uint32_t contentWidth,
        uint32_t contentHeight,
        bool showCoordinates,
        DosBoxX::Mouse& dosBoxMouse,
        DosBoxX::NamedPipeClient& dosBoxPipeClient
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

            const ImVec2 mousePos =
    ImGui::GetMousePos();

const ImVec2 imageMax(
    imageMin.x + imageSize.x,
    imageMin.y + imageSize.y
);

mouseInsideImage =
    mousePos.x >= imageMin.x &&
    mousePos.x < imageMax.x &&
    mousePos.y >= imageMin.y &&
    mousePos.y < imageMax.y;


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

    if (!dosBoxMouse.inputActive())
    {
        dosBoxMouse.setInputActive(dosBoxPipeClient, true);
    }

    dosBoxMouse.move(
        dosBoxPipeClient,
        contentX,
        contentY,
        static_cast<int>(contentWidth),
        static_cast<int>(contentHeight)
    );

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
