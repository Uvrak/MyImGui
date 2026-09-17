#include "Mouse.h"

#include <algorithm>
#include <string>

#include "NamedPipeClient.h"
#include "FrameReader.h"
#include "imgui.h"
#include "../MouseLatencyTrace.h"

#include <cstdio>
#include <windows.h>


namespace DosBoxX
{
    bool Mouse::inputActive() const
    {
        return m_inputActive;
    }

    void Mouse::setInputActive(
        NamedPipeClient& namedPipeClient,
        bool active
    )
    {
        if (m_inputActive == active)
            return;

        m_inputActive = active;

        if (!active)
        {
            ClipCursor(nullptr);
            m_clickQueue.clear();
            m_leftButtonDown = false;
            if (m_clickPending)
            {
                if (namedPipeClient.send("MOUSEUP:0"))
                {
                    m_clickPending = false;
                    m_lastReleaseTime = ImGui::GetTime();
                }
            }
            namedPipeClient.send("RELEASE_ALL");
            m_lastX = -1;
            m_lastY = -1;
        }
    }

    void Mouse::move(
        NamedPipeClient& namedPipeClient,
        int x,
        int y,
        int contentWidth,
        int contentHeight
    )
    {
        if (!m_inputActive || contentWidth <= 1 || contentHeight <= 1 ||
            (x == m_lastX && y == m_lastY))
            return;

        namedPipeClient.send(
            "MOUSEMOVE:" + std::to_string(x) + ":" +
            std::to_string(y) + ":" +
            std::to_string(contentWidth) + ":" +
            std::to_string(contentHeight)
        );
        m_lastX = x;
        m_lastY = y;
    }

    void Mouse::update(
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

        const bool mouseInsideImage =
            mouseInImageX >= 0.0f &&
            mouseInImageY >= 0.0f &&
            mouseInImageX < imageWidth &&
            mouseInImageY < imageHeight;

        char debug[256];

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

        //ImGui::SetTooltip(
        //    "DOSBox: %d, %d",
        //    dosBoxMouseX,
        //    dosBoxMouseY
        //);

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
    }

    void Mouse::click(
        NamedPipeClient& namedPipeClient,
        int x,
        int y,
        int contentWidth,
        int contentHeight
    )
    {
        if (contentWidth <= 1 || contentHeight <= 1)
            return;

        const std::string command =
            "MOUSEDOWNAT:" +
            std::to_string(x) +
            ":" +
            std::to_string(y) +
            ":" +
            std::to_string(contentWidth) +
            ":" +
            std::to_string(contentHeight);

        // Keep at most one queued retry while a click is already held.
        const QueuedClick queued{command, {x, y}};
        if (m_clickQueue.empty())
            m_clickQueue.push_back(queued);
        else
            m_clickQueue.back() = queued;
        updatePendingClick(namedPipeClient);
    }

    void Mouse::updatePendingClick(
        NamedPipeClient& namedPipeClient
    )
    {
        if (m_clickPending)
        {
            // Keep a brief tap pressed long enough for a guest that polls the
            // current button state to observe it before forwarding release.
            constexpr double minimumPressSeconds = 0.10;
            if (!m_leftButtonDown &&
                ImGui::GetTime() - m_clickStartTime >= minimumPressSeconds &&
                namedPipeClient.send("MOUSEUP:0"))
            {
                TraceGridBuilderMouse("HOST_UP");
                m_clickPending = false;
                m_lastReleaseTime = ImGui::GetTime();
            }
        }

        if (!m_clickPending && !m_clickQueue.empty() &&
            (m_lastReleaseTime < 0.0 ||
                ImGui::GetTime() - m_lastReleaseTime >= 0.01))
        {
            TraceGridBuilderMouse("HOST_DOWN_BEGIN");
            const bool sent = namedPipeClient.send(m_clickQueue.front().command);
            TraceGridBuilderMouse("HOST_DOWN_END", sent ? 1 : 0);
            if (sent)
            {
                TraceGridBuilderMouse("HOST_CLICK_XY",
                    (static_cast<unsigned long long>(m_clickQueue.front().position.x) << 32) |
                    static_cast<unsigned int>(m_clickQueue.front().position.y));
                m_emulatedClicks.push_back(m_clickQueue.front().position);
                m_clickQueue.pop_front();
                m_clickPending = true;
                m_clickStartTime = ImGui::GetTime();
            }
        }
    }

    std::vector<EmulatedMouseClick> Mouse::takeEmulatedClicks()
    {
        std::vector<EmulatedMouseClick> result;
        result.swap(m_emulatedClicks);
        return result;
    }

    void Mouse::setLeftButtonDown(bool down)
    {
        m_leftButtonDown = down;
    }


}
