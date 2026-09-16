#pragma once

namespace DosBoxX
{
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
    private:
        bool m_inputActive = false;
        int m_lastX = -1;
        int m_lastY = -1;
        bool m_clickPending = false;
        double m_clickStartTime = 0.0;
        
    };
}
