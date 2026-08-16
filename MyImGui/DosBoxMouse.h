#pragma once

namespace MyImGui
{
    class NamedPipeClient;
    struct DosBoxFrameHeader;

    class DosBoxMouse
    {
    public:
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
        bool m_clickPending = false;
        double m_clickStartTime = 0.0;
        
    };
}
