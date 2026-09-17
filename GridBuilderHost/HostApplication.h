#pragma once

namespace MyImGui
{
    class SettingsWindow;
}

namespace DosBoxX
{
    class FrameTexture;
    class Keyboard;
    class Mouse;
    class NamedPipeClient;
    class Memory;
}

class GameModule;

class GridBuilderGrid;

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
            GridBuilderGrid& gridBuilderGrid,
            HostUi& hostUi,
            GameModule& gameModule,
            DosBoxX::Keyboard& dosBoxKeyboard,
            DosBoxX::Mouse& dosBoxMouse,
            DosBoxX::Memory& dosBoxMemory,
            DosBoxX::NamedPipeClient& dosBoxPipeClient           

        );
    };
}
