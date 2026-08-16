#include "pch.h"
#include "DosBoxMouse.h"

#include <algorithm>
#include <string>

#include "NamedPipeClient.h"
#include "DosBoxFrameReader.h"
#include "imgui.h"

namespace MyImGui
{
    void DosBoxMouse::update(
        NamedPipeClient& NamedPipeClient,
        const DosBoxFrameHeader& frameHeader,
        float imageWidth,
        float imageHeight,
        float imageLeft,
        float imageTop
    )
    {
        ImVec2 mousePos =
            ImGui::GetMousePos();

        const float mouseInImageX =
            mousePos.x - imageLeft;

        const float mouseInImageY =
            mousePos.y - imageTop;

        const float mouseScaleX =
            static_cast<float>(
                frameHeader.contentWidth
                ) / imageWidth;

        const float mouseScaleY =
            static_cast<float>(
                frameHeader.contentHeight
                ) / imageHeight;

        int dosBoxMouseX =
            static_cast<int>(
                mouseInImageX * mouseScaleX
                );

        int dosBoxMouseY =
            static_cast<int>(
                mouseInImageY * mouseScaleY
                );

        dosBoxMouseX =
            std::clamp(
                dosBoxMouseX,
                0,
                static_cast<int>(
                    frameHeader.contentWidth
                    ) - 1
            );

        dosBoxMouseY =
            std::clamp(
                dosBoxMouseY,
                0,
                static_cast<int>(
                    frameHeader.contentHeight
                    ) - 1
            );

        ImGui::SetTooltip(
            "DOSBox: %d, %d",
            dosBoxMouseX,
            dosBoxMouseY
        );

        static int lastDosBoxMouseX = -1;
        static int lastDosBoxMouseY = -1;

        if (dosBoxMouseX != lastDosBoxMouseX ||
            dosBoxMouseY != lastDosBoxMouseY)
        {
            std::string command =
                "MOUSEMOVE:";

            command +=
                std::to_string(dosBoxMouseX);

            command += ":";

            command +=
                std::to_string(dosBoxMouseY);

            command += ":";

            command +=
                std::to_string(
                    frameHeader.contentWidth
                );

            command += ":";

            command +=
                std::to_string(
                    frameHeader.contentHeight
                );

            NamedPipeClient.send(
                command
            );

            lastDosBoxMouseX =
                dosBoxMouseX;

            lastDosBoxMouseY =
                dosBoxMouseY;
        }

        static bool leftMouseWasDown = false;

        const bool leftMouseIsDown =
            ImGui::IsMouseDown(
                ImGuiMouseButton_Left
            );

        if (leftMouseIsDown &&
            !leftMouseWasDown)
        {
            NamedPipeClient.send(
                "MOUSEDOWN:0"
            );
        }

        if (!leftMouseIsDown &&
            leftMouseWasDown)
        {
            NamedPipeClient.send(
                "MOUSEUP:0"
            );
        }

        leftMouseWasDown =
            leftMouseIsDown;

        static bool rightMouseWasDown = false;

        const bool rightMouseIsDown =
            ImGui::IsMouseDown(
                ImGuiMouseButton_Right
            );

        if (rightMouseIsDown &&
            !rightMouseWasDown)
        {
            NamedPipeClient.send(
                "MOUSEDOWN:1"
            );
        }

        if (!rightMouseIsDown &&
            rightMouseWasDown)
        {
            NamedPipeClient.send(
                "MOUSEUP:1"
            );
        }

        rightMouseWasDown =
            rightMouseIsDown;

        const float mouseWheel =
            ImGui::GetIO().MouseWheel;

        if (mouseWheel > 0.0f)
        {
            NamedPipeClient.send(
                "MOUSEWHEEL:UP"
            );
        }
        else if (mouseWheel < 0.0f)
        {
            NamedPipeClient.send(
                "MOUSEWHEEL:DOWN"
            );
        }
        if (m_clickPending)
        {
            const double currentTime =
                ImGui::GetTime();

            if (currentTime -
                m_clickStartTime >= 0.01)
            {
                NamedPipeClient.send(
                    "MOUSEUP:0"
                );

                m_clickPending = false;
            }
        }
    }

    void DosBoxMouse::click(
        NamedPipeClient& namedPipeClient,
        int x,
        int y,
        int contentWidth,
        int contentHeight
    )
    {
        std::string command =
            "MOUSEMOVE:" +
            std::to_string(x) +
            ":" +
            std::to_string(y) +
            ":" +
            std::to_string(contentWidth) +
            ":" +
            std::to_string(contentHeight);

        namedPipeClient.send(
            command
        );

        namedPipeClient.send(
            "MOUSEDOWN:0"
        );

        m_clickPending = true;

        m_clickStartTime =
            ImGui::GetTime();
    }

    void DosBoxMouse::updatePendingClick(
        NamedPipeClient& namedPipeClient
    )
    {
        if (!m_clickPending)
            return;

        const double currentTime =
            ImGui::GetTime();

        if (currentTime -
            m_clickStartTime >= 0.05)
        {
            namedPipeClient.send(
                "MOUSEUP:0"
            );

            m_clickPending = false;
        }
    }
}
