#pragma once

namespace DosBoxX
{
    class FrameTexture;
}

namespace GridBuilderHost
{
    class DosBoxFramePipeline;
    class HostRenderer;

    class HostApplication
    {
    public:
        HostApplication();
        ~HostApplication();

        void run(
            DosBoxFramePipeline& dosBoxFramePipeline,
            HostRenderer& hostRenderer,
            DosBoxX::FrameTexture& frameTexture
        );
    };
}