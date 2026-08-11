#include "pch.h"
#include "DosBoxView.h"

#include "DosBoxFrameReader.h"
#include "DosBoxFrameTexture.h"
#include "DosBoxKeyboard.h"
#include "DosBoxMouse.h"
#include "ExternalWindow.h"

#include "imgui.h"

namespace MyImGui
{
    void DosBoxView::draw(
        ExternalWindow& externalWindow,
        DosBoxFrameReader& frameReader,
        DosBoxFrameTexture& frameTexture,
        DosBoxKeyboard& keyboard,
        DosBoxMouse& mouse
    )
    {
        ImGui::Begin("DOSBox");

        frameReader.tryOpen();

        const DosBoxFrameHeader* frameHeader =
            frameReader.header();

        if (frameHeader != nullptr)
        {
            static uint64_t lastFrameCounter = 0;

            if (frameHeader->frameCounter < lastFrameCounter)
            {
                char buffer[256];

                sprintf_s(
                    buffer,
                    "NEW DOSBOX FRAME: width=%u height=%u content=%ux%u pitch=%u frame=%llu\n",
                    frameHeader->width,
                    frameHeader->height,
                    frameHeader->contentWidth,
                    frameHeader->contentHeight,
                    frameHeader->pitch,
                    static_cast<unsigned long long>(
                        frameHeader->frameCounter
                        )
                );

                OutputDebugStringA(buffer);
            }

            lastFrameCounter =
                frameHeader->frameCounter;
            const uint8_t* framePixels =
                frameReader.pixels();

            if (framePixels != nullptr)
            {
                frameTexture.update(
                    framePixels,
                    frameHeader->width,
                    frameHeader->height,
                    frameHeader->pitch
                );

                ID3D11ShaderResourceView* sharedTexture =
                    frameTexture.textureView();

                if (sharedTexture != nullptr)
                {
                    ImVec2 availableSize =
                        ImGui::GetContentRegionAvail();

                    const float scaleX =
                        availableSize.x /
                        static_cast<float>(
                            frameHeader->contentWidth
                            );

                    const float scaleY =
                        availableSize.y /
                        static_cast<float>(
                            frameHeader->contentHeight
                            );

                    const float scale =
                        (scaleX < scaleY)
                        ? scaleX
                        : scaleY;

                    ImVec2 imageSize(
                        frameHeader->contentWidth * scale,
                        frameHeader->contentHeight * scale
                    );

                    ImGui::Image(
                        reinterpret_cast<ImTextureID>(
                            sharedTexture
                            ),
                        imageSize,
                        ImVec2(
                            0.0f,
                            0.0f
                        ),
                        ImVec2(
                            static_cast<float>(
                                frameHeader->contentWidth
                                ) /
                            static_cast<float>(
                                frameHeader->width
                                ),
                            static_cast<float>(
                                frameHeader->contentHeight
                                ) /
                            static_cast<float>(
                                frameHeader->height
                                )
                        )
                    );
                    const bool dosBoxImageHovered =
                        ImGui::IsMouseHoveringRect(
                            ImGui::GetItemRectMin(),
                            ImGui::GetItemRectMax()
                        );

                    if (m_inputActive &&
                        dosBoxImageHovered)
                    {
                        ImGui::SetMouseCursor(
                            ImGuiMouseCursor_None
                        );
                    }

                    const bool dosBoxImageClicked =
                        ImGui::IsItemClicked(
                            ImGuiMouseButton_Left
                        );

                    ImVec2 imageMin =
                        ImGui::GetItemRectMin();

                    if (m_inputActive &&
                        dosBoxImageHovered)
                    {
                        mouse.update(
                            externalWindow,
                            *frameHeader,
                            imageSize.x,
                            imageSize.y,
                            imageMin.x,
                            imageMin.y
                        );
                    }

                    if (dosBoxImageClicked)
                    {
                        m_inputActive = true;
                    }
                    else if (ImGui::IsMouseClicked(
                        ImGuiMouseButton_Left
                    ))
                    {
                        m_inputActive = false;

                        externalWindow.sendIpcCommand(
                            "RELEASE_ALL"
                        );
                    }

                }
            }
        }
        else
        {
            ImGui::TextUnformatted(
                "Shared frame not available"
            );
        }
        ImGui::Text(
            "DOSBox input: %s",
            m_inputActive
            ? "ACTIVE"
            : "INACTIVE"
        );

        if (m_inputActive)
        {
            keyboard.update(
                externalWindow
            );
        }

        ImGui::End();
    }
}