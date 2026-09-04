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

namespace MightAndMagic3
{
    class MM3Launcher;
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
            HostUi& hostUi,
            MightAndMagic3::MM3Launcher& mm3Launcher,
            DosBoxX::Keyboard& dosBoxKeyboard,
            DosBoxX::Mouse& dosBoxMouse,
            DosBoxX::Memory& dosBoxMemory,
            DosBoxX::NamedPipeClient& dosBoxPipeClient
        );
    };
}