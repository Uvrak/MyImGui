#include "pch.h"
#include "DosBoxMouse.h"

#include <algorithm>
#include <string>

#include "ExternalWindow.h"
#include "DosBoxFrameReader.h"
#include "imgui.h"

namespace MyImGui
{
    void DosBoxMouse::update(
        ExternalWindow& externalWindow,
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

            externalWindow.sendIpcCommand(
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
            externalWindow.sendIpcCommand(
                "MOUSEDOWN:0"
            );
        }

        if (!leftMouseIsDown &&
            leftMouseWasDown)
        {
            externalWindow.sendIpcCommand(
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
            externalWindow.sendIpcCommand(
                "MOUSEDOWN:1"
            );
        }

        if (!rightMouseIsDown &&
            rightMouseWasDown)
        {
            externalWindow.sendIpcCommand(
                "MOUSEUP:1"
            );
        }

        rightMouseWasDown =
            rightMouseIsDown;

        const float mouseWheel =
            ImGui::GetIO().MouseWheel;

        if (mouseWheel > 0.0f)
        {
            externalWindow.sendIpcCommand(
                "MOUSEWHEEL:UP"
            );
        }
        else if (mouseWheel < 0.0f)
        {
            externalWindow.sendIpcCommand(
                "MOUSEWHEEL:DOWN"
            );
        }

    }

}
