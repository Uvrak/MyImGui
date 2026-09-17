#pragma once

#include <deque>
#include <string>
#include <vector>

namespace DosBoxX
{
    struct EmulatedMouseClick { int x = 0; int y = 0; };

    class NamedPipeClient;
    struct DosBoxFrameHeader;

    class Mouse
    {
    public:
        bool inputActive() const;
        void setInputActive(NamedPipeClient& namedPipeClient, bool active);
        void move(
            NamedPipeClient& namedPipeClient,
            int x,
            int y,
            int contentWidth,
            int contentHeight
        );
            void update(
            NamedPipeClient& NamedPipeClient,
            const DosBoxFrameHeader& frameHeader,
            float imageWidth,
            float imageHeight,
            float imageLeft,
            float imageTop
        );

        void click(
            NamedPipeClient& namedPipeClient,
            int x,
            int y,
            int contentWidth,
            int contentHeight
        );
        void updatePendingClick(NamedPipeClient& namedPipeClient);
        std::vector<EmulatedMouseClick> takeEmulatedClicks();
        void setLeftButtonDown(bool down);
        bool clickPending() const
        {
            return m_clickPending || m_leftButtonDown || !m_clickQueue.empty();
        }
    private:
        bool m_inputActive = false;
        int m_lastX = -1;
        int m_lastY = -1;
        bool m_clickPending = false;
        double m_clickStartTime = 0.0;
        double m_lastReleaseTime = -1.0;
        bool m_leftButtonDown = false;
        struct QueuedClick
        {
            std::string command;
            EmulatedMouseClick position;
        };
        std::deque<QueuedClick> m_clickQueue;
        std::vector<EmulatedMouseClick> m_emulatedClicks;
        
    };
}
