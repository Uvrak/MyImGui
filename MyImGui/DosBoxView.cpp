#include "pch.h"
#include "DosBoxView.h"

#include "DosBoxFrameReader.h"
#include "DosBoxFrameTexture.h"
#include "DosBoxKeyboard.h"
#include "DosBoxMouse.h"
#include "NamedPipeClient.h"

#include "imgui.h"

namespace MyImGui
{
    void DosBoxView::draw(
        NamedPipeClient& NamedPipeClient,
        DosBoxFrameReader& frameReader,
        DosBoxFrameTexture& frameTexture,
        DosBoxKeyboard& keyboard,
        DosBoxMouse& mouse,
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
        else if (m_inputActive)
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
            m_inputActive =
                !m_inputActive;

            if (!m_inputActive)
            {
                ClipCursor(
                    nullptr
                );

                NamedPipeClient.send(
                    "RELEASE_ALL"
                );
            }
        }

        frameReader.tryOpen();

        const DosBoxFrameHeader* frameHeader =
            frameReader.header();

        if (frameHeader != nullptr)
        {
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

                    if (m_inputActive &&
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


                    if (dosBoxImageClicked)
                    {
                        m_inputActive = true;
                    }
                    else if (ImGui::IsMouseClicked(
                        ImGuiMouseButton_Left
                    ))
                    {
                        m_inputActive = false;

                        NamedPipeClient.send(
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

        if (m_inputActive ||
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