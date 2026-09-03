#include "DosBoxWindow.h"

#include "FrameTexture.h"

#include "imgui.h"

namespace GridBuilderHost
{
    void DosBoxWindow::draw(
        DosBoxX::FrameTexture& frameTexture,
        uint32_t contentWidth,
        uint32_t contentHeight,
        bool showCoordinates
    )
    {
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

            if (showCoordinates)
            {
                const ImVec2 mousePos =
                    ImGui::GetMousePos();

                const ImVec2 imageMax(
                    imageMin.x + imageSize.x,
                    imageMin.y + imageSize.y
                );

                const bool mouseInsideImage =
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

                    ImGui::BeginTooltip();

                    ImGui::Text(
                        "X: %d  Y: %d",
                        contentX,
                        contentY
                    );

                    ImGui::EndTooltip();
                }
            }
        }



        ImGui::End();
    }
}