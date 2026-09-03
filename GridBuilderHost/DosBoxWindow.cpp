#include "DosBoxWindow.h"

#include "FrameTexture.h"

#include "imgui.h"

namespace GridBuilderHost
{
    void DosBoxWindow::draw(
        DosBoxX::FrameTexture& frameTexture,
        uint32_t contentWidth,
        uint32_t contentHeight
    )
    {
        ImGui::Begin(
            "DOSBox"
        );

        ID3D11ShaderResourceView* textureView =
            frameTexture.textureView();

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
        }

        ImGui::End();
    }
}