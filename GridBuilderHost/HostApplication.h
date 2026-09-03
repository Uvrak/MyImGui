#pragma once

namespace MyImGui
{
    class SettingsWindow;
}

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
    class MainMenu;
    class HostUi;

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
            HostUi& hostUi
        );
    };
}