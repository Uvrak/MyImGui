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
#include "MM3GameModule.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Memory.h"
#include "GridBuilderGrid.h"
#include <fstream>
#include <filesystem>
#include <optional>
#include <string>
#include <Windows.h>

namespace
{
    std::optional<std::size_t> loadMapConfiguration(GridBuilderGrid& grid)
    {
        wchar_t executablePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
        const auto projectRoot =
            std::filesystem::path(executablePath).parent_path().parent_path().parent_path();
        grid.setMapDirectory((projectRoot / "resources/maps").string());
        const auto configPath = projectRoot / "settings/mm3_maps.cfg";
        std::ifstream input(configPath);
        std::optional<std::size_t> address;
        std::string line;
        while (std::getline(input, line))
        {
            if (line.empty() || line[0] == '#') continue;
            const auto separator = line.find('=');
            if (separator == std::string::npos) continue;
            const auto name = line.substr(0, separator);
            const auto value = line.substr(separator + 1);
            try
            {
                if (name == "map_id_address")
                    address = static_cast<std::size_t>(std::stoull(value, nullptr, 0));
                else if (name.rfind("map_", 0) == 0)
                {
                    const auto id = std::stoul(name.substr(4), nullptr, 0);
                    if (id <= 255 && !value.empty())
                    {
                        std::u8string utf8Value;
                        for (unsigned char byte : value)
                            utf8Value.push_back(static_cast<char8_t>(byte));
                        grid.registerMap("mm3/" + std::to_string(id),
                                         (projectRoot / std::filesystem::path(utf8Value)).string());
                    }
                }
            }
            catch (const std::exception&) { /* Ignore malformed entries. */ }
        }
        return address;
    }
}

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
    DosBoxX::Mouse dosBoxMouse;
    DosBoxX::Memory dosBoxMemory;

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

    GridBuilderGrid gridBuilderGrid(
        hostRenderer.device(),
        16
    );
    MightAndMagic3::MM3GameModule mm3GameModule(
        dosBoxController, dosBoxPipeClient,
        loadMapConfiguration(gridBuilderGrid)
    );

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
        gridBuilderGrid,
        hostUi,
        mm3GameModule,
        dosBoxKeyboard,
        dosBoxMouse,
        dosBoxMemory,
        dosBoxPipeClient
    );

    DosBoxX::ProcessManager::terminateRunningInstances();

    hostWindow.saveState(
        hostWindowState
    );

    return 0;
}
