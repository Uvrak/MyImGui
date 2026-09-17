#include "View.h"

#include "FrameReader.h"
#include "FrameTexture.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "NamedPipeClient.h"

#include "imgui.h"
#include <algorithm>

namespace DosBoxX
{
    void View::requestRefresh()
    {
        m_refreshRequested = true;
    }

    void View::draw(
        NamedPipeClient& NamedPipeClient,
        FrameReader& frameReader,
        FrameTexture& frameTexture,
        Keyboard& keyboard,
        Mouse& mouse,
        const std::string& gameFilename
    )
    {
        std::string windowTitle =
            gameFilename.empty()
            ? "DOSBox"
            : gameFilename;

        windowTitle += "###DOSBoxWindow";

        
        ImVec4 tabColor;

        if (m_inputMode ==
            DosBoxInputMode::AlwaysActive)
        {
            tabColor = ImVec4(
                0.0f,
                0.35f,
                0.75f,
                1.0f
            );
        }
        else if (mouse.inputActive())
        {
            tabColor = ImVec4(
                0.0f,
                0.55f,
                0.0f,
                1.0f
            );
        }
        else
        {
            tabColor = ImVec4(
                0.65f,
                0.0f,
                0.0f,
                1.0f
            );
        }

        ImGui::PushStyleColor(
            ImGuiCol_Tab,
            tabColor
        );

        ImGui::PushStyleColor(
            ImGuiCol_TabSelected,
            tabColor
        );

        ImGui::PushStyleColor(
            ImGuiCol_TabHovered,
            tabColor
        );

        ImGui::PushStyleColor(
            ImGuiCol_TabDimmed,
            tabColor
        );

        ImGui::PushStyleColor(
            ImGuiCol_TabDimmedSelected,
            tabColor
        );

        ImGui::Begin(
            windowTitle.c_str()
        );

        ImGui::PopStyleColor(5);
        bool alwaysActive =
            m_inputMode ==
            DosBoxInputMode::AlwaysActive;

        if (ImGui::Checkbox(
            "Always Active",
            &alwaysActive
        ))
        
        {
            m_inputMode =
                alwaysActive
                ? DosBoxInputMode::AlwaysActive
                : DosBoxInputMode::Focused;
        }

        if (ImGui::IsWindowFocused(
            ImGuiFocusedFlags_RootAndChildWindows
        ) &&
            ImGui::IsKeyPressed(
                ImGuiKey_End,
                false
            ))
        {
            mouse.setInputActive(
                NamedPipeClient,
                !mouse.inputActive()
            );
        }

        frameReader.tryOpen();

        const DosBoxFrameHeader* frameHeader =
            frameReader.header();
        
        mouse.setLeftButtonDown(ImGui::IsMouseDown(ImGuiMouseButton_Left));
        mouse.updatePendingClick(
            NamedPipeClient
        );

        if (frameHeader != nullptr)
        {
            if (m_refreshRequested)
            {
                mouse.click(
                    NamedPipeClient,
                    54,
                    356,
                    frameHeader->contentWidth,
                    frameHeader->contentHeight
                );

                m_refreshRequested = false;
            }

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

                    //if (m_inputActive &&
                    //    dosBoxImageHovered)
                    //{
                    //    ImGui::SetMouseCursor(
                    //        ImGuiMouseCursor_None
                    //    );
                    //}

                    const bool dosBoxImageClicked =
                        ImGui::IsItemClicked(
                            ImGuiMouseButton_Left
                        );

                    ImVec2 imageMin =
                        ImGui::GetItemRectMin();

                    if (mouse.inputActive() &&
                        m_inputMode ==
                        DosBoxInputMode::Focused)
                    {
                        ImVec2 imageMax =
                            ImGui::GetItemRectMax();

                        RECT clipRect{
                            static_cast<LONG>(imageMin.x),
                            static_cast<LONG>(imageMin.y),
                            static_cast<LONG>(imageMax.x),
                            static_cast<LONG>(imageMax.y)
                        };

                        ClipCursor(
                            &clipRect
                        );
                    }
                    else
                    {
                        ClipCursor(
                            nullptr
                        );
                    }

                    if (mouse.inputActive() &&
                        dosBoxImageHovered)
                    {
                        mouse.update(
                            NamedPipeClient,
                            *frameHeader,
                            imageSize.x,
                            imageSize.y,
                            imageMin.x,
                            imageMin.y
                        );
                    }

                    if (dosBoxImageClicked &&
                        frameHeader->contentWidth > 1 &&
                        frameHeader->contentHeight > 1)
                    {
                        mouse.setInputActive(NamedPipeClient, true);
                        const ImVec2 mousePos = ImGui::GetMousePos();
                        const int contentX = std::clamp(static_cast<int>(
                            (mousePos.x - imageMin.x) / imageSize.x *
                            frameHeader->contentWidth), 0,
                            static_cast<int>(frameHeader->contentWidth) - 1);
                        const int contentY = std::clamp(static_cast<int>(
                            (mousePos.y - imageMin.y) / imageSize.y *
                            frameHeader->contentHeight), 0,
                            static_cast<int>(frameHeader->contentHeight) - 1);
                        mouse.click(NamedPipeClient, contentX, contentY,
                            frameHeader->contentWidth,
                            frameHeader->contentHeight);
                    }
                    else if (ImGui::IsMouseClicked(
                        ImGuiMouseButton_Left
                    ))
                    {
                        mouse.setInputActive(NamedPipeClient, false);
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
        
        if (mouse.inputActive() ||
            m_inputMode ==
            DosBoxInputMode::AlwaysActive)
        {
            keyboard.update(
                NamedPipeClient
            );
        }

        ImGui::End();
    }
}
