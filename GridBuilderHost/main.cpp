#include "Controller.h"
#include "NamedPipeClient.h"
#include "FrameReader.h"

#include "HostWindow.h"
#include "HostApplication.h"
#include "HostWindowState.h"
#include "HostRenderer.h"
#include "FrameTexture.h"
#include "DosBoxFramePipeline.h"
#include "Process.h"
#include "ProcessManager.h"
#include "ImGuiHost.h"
#include "DosBoxWindow.h"
#include "MainMenu.h"
#include "HostUi.h"
#include "SettingsWindow.h"
#include "HostFontSettings.h"
#include "HostSettings.h"
#include "MM3Launcher.h"
#include "Keyboard.h"

int main()
{
    GridBuilderHost::HostWindowState hostWindowState;
    GridBuilderHost::HostWindow hostWindow;
    GridBuilderHost::HostRenderer hostRenderer;
    GridBuilderHost::HostApplication hostApplication;
    GridBuilderHost::HostSettings hostSettings;

    GridBuilderHost::MainMenu mainMenu(
        hostSettings
    );

    GridBuilderHost::DosBoxWindow dosBoxWindow;
    MyImGui::SettingsWindow settingsWindow;
    settingsWindow.setFontSize(
        hostSettings.fontSize()
    );
    GridBuilderHost::HostFontSettings hostFontSettings(
        settingsWindow,
        hostSettings
    );

    GridBuilderHost::HostUi hostUi(
        mainMenu,
        dosBoxWindow,
        settingsWindow,
        hostFontSettings
    );

    DosBoxX::Controller dosBoxController;
    DosBoxX::Process dosBoxProcess;

    DosBoxX::NamedPipeClient dosBoxPipeClient(
        R"(\\.\pipe\GridBuilderDOSBox)"
    );

    DosBoxX::Keyboard dosBoxKeyboard;

    MightAndMagic3::MM3Launcher mm3Launcher(
        dosBoxController,
        dosBoxPipeClient
    );

    DosBoxX::FrameReader dosBoxFrameReader;

    if (!hostWindow.initialize(
        hostWindowState
    ))
    {
        return 1;
    }

    if (!hostRenderer.initialize(
        hostWindow.nativeHandle()
    ))
    {
        return 1;
    }

    DosBoxX::FrameTexture dosBoxFrameTexture(
        hostRenderer.device(),
        hostRenderer.context()
    );

    GridBuilderHost::DosBoxFramePipeline dosBoxFramePipeline(
        dosBoxFrameReader,
        dosBoxFrameTexture
    );

    GridBuilderHost::ImGuiHost imGuiHost;

    if (!imGuiHost.initialize(
        hostWindow.window(),
        hostRenderer.device(),
        hostRenderer.context()
    ))
    {
        return 1;
    }

    DosBoxX::ProcessManager::terminateRunningInstances();

    if (!dosBoxProcess.start(
        L"C:\\Projects\\MyImGui\\dosbox-x\\bin\\x64\\Debug SDL2\\dosbox-x.exe"
    ))
    {
        return 1;
    }

    hostApplication.run(
    dosBoxFramePipeline,
    hostRenderer,
    dosBoxFrameTexture,
    imGuiHost,
    hostUi,
    mm3Launcher,
    dosBoxKeyboard,
    dosBoxPipeClient
);

    DosBoxX::ProcessManager::terminateRunningInstances();

    hostWindow.saveState(
        hostWindowState
    );

    return 0;
}