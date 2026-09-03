#pragma once

namespace DosBoxX
{
    class FrameTexture;
}

namespace GridBuilderHost
{
    class ImGuiHost;
    class DosBoxFramePipeline;
    class HostRenderer;
    class DosBoxWindow;

    class HostApplication
    {
    public:
        HostApplication();
        ~HostApplication();

        void run(
            DosBoxFramePipeline& dosBoxFramePipeline,
            HostRenderer& hostRenderer,
            DosBoxX::FrameTexture& frameTexture,
            ImGuiHost& imGuiHost,
            DosBoxWindow& dosBoxWindow
        );
    };
}