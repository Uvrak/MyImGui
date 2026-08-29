#ifdef _WIN32
#include <Windows.h>
#include <TlHelp32.h>
#endif
#include "DosBoxWindow.h"
#include "imgui.h"
#include <string>
#include <cstdio>
#include "Controller.h"

namespace
{
    constexpr const char* DosBoxExecutablePath =
        "C:\\Projects\\MyImGui\\dosbox-x\\bin\\x64\\Debug SDL2\\dosbox-x.exe";

    constexpr const char* DosBoxWorkingDirectory =
        "C:\\Projects\\MyImGui\\dosbox-x\\bin\\x64\\Debug SDL2";

    constexpr const char* DosBoxPipeName =
        R"(\\.\pipe\GridBuilderDOSBox)";

    constexpr int HiddenDosBoxX = -5000;
    constexpr int HiddenDosBoxY = 0;
    constexpr int HiddenDosBoxWidth = 640;
    constexpr int HiddenDosBoxHeight = 400;
}

DosBoxWindow::DosBoxWindow(
    SDL_Window* parentWindow,
    SDL_Renderer* renderer
)
    : m_parentWindow(parentWindow),
      m_frameTexture(renderer),
      m_pipeClient(
          DosBoxPipeName
      )

{

    ZeroMemory(
        &m_processInfo,
        sizeof(m_processInfo)
    );

    if (m_parentWindow == nullptr)
    {
        return;
    }

    const SDL_PropertiesID properties =
        SDL_GetWindowProperties(
            m_parentWindow
        );

    m_parentHwnd =
        static_cast<HWND>(
            SDL_GetPointerProperty(
                properties,
                SDL_PROP_WINDOW_WIN32_HWND_POINTER,
                nullptr
            )
            );
}

DosBoxWindow::~DosBoxWindow()
{
    shutdown();
}

void DosBoxWindow::draw()
{
    if (!m_startAttempted)
    {
        m_startAttempted = true;

        m_started =
            start();

        if (m_started)
        {
            m_focusRequested = false;
            m_inputActive = false;
        }
    }

    if (m_started &&
        m_dosBoxHwnd == nullptr)
    {
        findDosBoxWindow();

        if (m_dosBoxHwnd != nullptr)
        {
            ShowWindow(
                m_dosBoxHwnd,
                SW_HIDE
            );
        }
    }

    if (m_started &&
        !m_pingTested)
    {
        std::string response;

        if (m_pipeClient.request(
            "PING",
            response
        ))
        {
            m_pingTested = true;

            if (response == "PONG")
            {
                OutputDebugStringA(
                    "GridBuilder IPC: PONG received\n"
                );

                m_focusRequested = false;
                m_inputActive = false;
            }
            else
            {
                OutputDebugStringA(
                    "GridBuilder IPC: unexpected response: ["
                );

                OutputDebugStringA(
                    response.c_str()
                );

                OutputDebugStringA(
                    "]\n"
                );
            }
        }

        else
        {
            OutputDebugStringA(
                "GridBuilder IPC: not ready, retrying\n"
            );
        }
    }

    ImGui::SetNextWindowSize(
        ImVec2(
            500.0f,
            350.0f
        ),
        ImGuiCond_FirstUseEver
    );

    const ImVec4 tabColor =
        m_inputActive
        ? ImVec4(
            0.0f,
            0.55f,
            0.0f,
            1.0f
        )
        : ImVec4(
            0.65f,
            0.0f,
            0.0f,
            1.0f
        );

    ImGui::PushStyleColor(
        ImGuiCol_Tab,
        tabColor
    );

    ImGui::PushStyleColor(
        ImGuiCol_TabSelected,
        tabColor
    );

    ImGui::PushStyleColor(
        ImGuiCol_TabHovered,
        tabColor
    );

    ImGui::PushStyleColor(
        ImGuiCol_TabDimmed,
        tabColor
    );

    ImGui::PushStyleColor(
        ImGuiCol_TabDimmedSelected,
        tabColor
    );

    std::string windowTitle =
        m_gameFilename.empty()
        ? "DOSBox"
        : m_gameFilename;

    windowTitle +=
        "###DOSBoxWindow";

    if (m_focusRequested)
    {
        ImGui::SetNextWindowFocus();
    }

    const bool windowContentsVisible =
        ImGui::Begin(
            windowTitle.c_str()
        );

    ImGui::PopStyleColor(5);
    
    if (!windowContentsVisible)
    {
        if (m_inputActive &&
            !m_focusRequested)
        {
            deactivateInput();
        }

        ImGui::End();
        return;
    }

    if (m_focusRequested)
    {
        m_focusRequested = false;
        m_inputActive = true;
    }
    if (m_started &&
        m_pingTested)
    {
        const Uint64 currentTicks =
            SDL_GetTicks();

        if (currentTicks >=
            m_nextMightAndMagic1Update)
        {
            m_mightAndMagic1Reader.update(
                m_pipeClient
            );

            m_programReader.update(
                m_pipeClient
            );

            m_nextMightAndMagic1Update =
                currentTicks + 100;
        }
        const MightAndMagic1State& mm1State =
            m_mightAndMagic1Reader.state();

        if (mm1State.valid)
        {
            ImGui::Text(
                "MM1: Area candidateA %d  Area candidateB %d  X %d  Y %d",
                mm1State.areaValueA,
                mm1State.areaValueB,
                mm1State.x,
                mm1State.y
            );
        }
        else
        {
            ImGui::TextUnformatted(
                "MM1: state unavailable"
            );
        }
    }

    const bool switchViewPressed =
        ImGui::IsKeyPressed(
            ImGuiKey_Tab,
            false
        ) ||
        (
            m_customSwitchViewKey !=
            ImGuiKey_None &&
            ImGui::IsKeyPressed(
                m_customSwitchViewKey,
                false
            )
            );

    const bool dosBoxWindowFocused =
        ImGui::IsWindowFocused(
            ImGuiFocusedFlags_RootAndChildWindows
        );

    if (switchViewPressed &&
        (
            !m_inputActive ||
            dosBoxWindowFocused
            ))
    {
        if (m_inputActive)
        {
            deactivateInput();
        }
        else
        {
            m_focusRequested = true;
        }
    }

    if (ImGui::Button(
        "Reset Memory Candidates"
    ))
    {
        if (m_pipeClient.send(
            "MEMORY_RESET"
        ))
        {
            OutputDebugStringA(
                "GridBuilder IPC: memory candidates reset\n"
            );
        }
        else
        {
            OutputDebugStringA(
                "GridBuilder IPC: memory reset request failed\n"
            );
        }
    }

    ImGui::SameLine();
    if (ImGui::Button(
        "Capture Memory Snapshot"
    ))
    {
        if (m_pipeClient.send(
            "MEMORY_SNAPSHOT"
        ))
        {
            m_memorySnapshotRequested = true;

            OutputDebugStringA(
                "GridBuilder IPC: memory snapshot requested\n"
            );
        }
        else
        {
            OutputDebugStringA(
                "GridBuilder IPC: memory snapshot request failed\n"
            );
        }
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Refine Unchanged"
    ))
    {
        if (m_pipeClient.send(
            "MEMORY_REFINE_UNCHANGED"
        ))
        {
            OutputDebugStringA(
                "GridBuilder IPC: unchanged refinement requested\n"
            );
        }
        else
        {
            OutputDebugStringA(
                "GridBuilder IPC: unchanged refinement failed\n"
            );
        }
    }

    m_frameReader.tryOpen();

    const DosBoxX::DosBoxFrameHeader* frameHeader =
        m_frameReader.header();
    if (frameHeader != nullptr &&
        frameHeader->width > 0 &&
        frameHeader->height > 0 &&
        frameHeader->contentWidth > 0 &&
        frameHeader->contentHeight > 0)
    {
        const uint8_t* framePixels =
            m_frameReader.pixels();

        if (framePixels != nullptr)
        {
            if (m_frameTexture.update(
                framePixels,
                frameHeader->width,
                frameHeader->height,
                frameHeader->pitch
            ))
            {
                SDL_Texture* texture =
                    m_frameTexture.texture();

                if (texture != nullptr)
                {
                    ImVec2 availableSize =
                        ImGui::GetContentRegionAvail();

                    availableSize.x =
                        (availableSize.x > 1.0f)
                        ? availableSize.x
                        : 1.0f;

                    availableSize.y =
                        (availableSize.y > 1.0f)
                        ? availableSize.y
                        : 1.0f;

                    const float scaleX =
                        availableSize.x /
                        static_cast<float>(
                            frameHeader->contentWidth
                            );

                    const float scaleY =
                        availableSize.y /
                        static_cast<float>(
                            frameHeader->contentHeight
                            );

                    const float scale =
                        (scaleX < scaleY)
                        ? scaleX
                        : scaleY;

                    ImVec2 imageSize(
                        frameHeader->contentWidth * scale,
                        frameHeader->contentHeight * scale
                    );

                    ImGui::Image(
                        reinterpret_cast<ImTextureID>(
                            texture
                            ),
                        imageSize,
                        ImVec2(
                            0.0f,
                            0.0f
                        ),
                        ImVec2(
                            static_cast<float>(
                                frameHeader->contentWidth
                                ) /
                            static_cast<float>(
                                frameHeader->width
                                ),
                            static_cast<float>(
                                frameHeader->contentHeight
                                ) /
                            static_cast<float>(
                                frameHeader->height
                                )
                        )
                    );

                    const ImVec2 imageMin =
                        ImGui::GetItemRectMin();

                    const ImVec2 imageMax =
                        ImGui::GetItemRectMax();

                    if (m_inputActive)
                    {
                        POINT topLeft{
                            static_cast<LONG>(imageMin.x),
                            static_cast<LONG>(imageMin.y)
                        };

                        POINT bottomRight{
                            static_cast<LONG>(imageMax.x),
                            static_cast<LONG>(imageMax.y)
                        };

                        ClientToScreen(
                            m_parentHwnd,
                            &topLeft
                        );

                        ClientToScreen(
                            m_parentHwnd,
                            &bottomRight
                        );

                        RECT clipRect{
                            topLeft.x,
                            topLeft.y,
                            bottomRight.x,
                            bottomRight.y
                        };

                        ClipCursor(
                            &clipRect
                        );
                    }
                    else
                    {
                        ClipCursor(
                            nullptr
                        );
                    }

                    const bool dosBoxImageHovered =
                        ImGui::IsItemHovered();

                    if (m_inputActive &&
                        dosBoxImageHovered)
                    {
                        m_mouse.update(
                            m_pipeClient,
                            *frameHeader,
                            imageSize.x,
                            imageSize.y,
                            imageMin.x,
                            imageMin.y
                        );
                    }

                    if (!m_inputActive &&
                        dosBoxImageHovered &&
                        ImGui::IsMouseClicked(
                            ImGuiMouseButton_Left
                        ))
                    {
                        m_inputActive = true;
                    }
                }
            }
        }
    }

    if (m_inputActive)
    {

        ImGui::GetIO().ConfigFlags |=
            ImGuiConfigFlags_NoMouseCursorChange;

        SDL_HideCursor();
    }

    if (m_inputActive)
    {
        m_keyboard.update(
            m_pipeClient,
            [this](
                ImGuiKey key,
                const char* defaultCommand
                ) -> std::string
            {
                if (m_keyBindings == nullptr)
                {
                    return defaultCommand;
                }

                const bool mightAndMagic1Active =
                    m_programReader.isRunning(
                        "MM"
                    );

                if (m_keyBindings->
                    germanKeyboardForUsGame())
                {
                    if (key == ImGuiKey_Y)
                    {
                        return "Z";
                    }

                    if (key == ImGuiKey_Z)
                    {
                        return "Y";
                    }
                }

                for (const DosBoxKeyBinding& binding :
                    m_keyBindings->bindings())
                {
                    if (binding.customKey != key)
                    {
                        continue;
                    }

                    if (binding.action ==
                        DosBoxAction::SwitchView)
                    {
                        return {};
                    }

                    if (mightAndMagic1Active)
                    {
                        return binding.dosCommand;
                    }
                }

                return defaultCommand;
            }
        );
    }
    ImGui::End();
} 

#ifdef _WIN32
namespace
{
    void closeExistingDosBoxInstances()
    {
        HANDLE snapshot =
            CreateToolhelp32Snapshot(
                TH32CS_SNAPPROCESS,
                0
            );

        if (snapshot == INVALID_HANDLE_VALUE)
        {
            return;
        }

        PROCESSENTRY32 processEntry{};
        processEntry.dwSize =
            sizeof(processEntry);

        if (Process32First(
            snapshot,
            &processEntry
        ))
        {
            do
            {
                if (_wcsicmp(
                    processEntry.szExeFile,
                    L"dosbox-x.exe"
                ) != 0)
                {
                    continue;
                }

                HANDLE process =
                    OpenProcess(
                        PROCESS_TERMINATE,
                        FALSE,
                        processEntry.th32ProcessID
                    );

                if (process == nullptr)
                {
                    continue;
                }

                TerminateProcess(
                    process,
                    0
                );

                CloseHandle(
                    process
                );

            } while (Process32Next(
                snapshot,
                &processEntry
            ));
        }

        CloseHandle(
            snapshot
        );
    }
}
#endif

bool DosBoxWindow::start()
{
#ifdef _WIN32
    closeExistingDosBoxInstances();

    HANDLE existingPipe =
        CreateFileA(
            DosBoxPipeName,
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

    if (existingPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(
            existingPipe
        );

        OutputDebugStringA(
            "DOSBox-X already running - not starting another instance.\n"
        );

        return false;
    }

    if (m_processInfo.hProcess != nullptr)
    {
        return false;
    }

    const char* dosBoxPath =
        DosBoxExecutablePath;

    STARTUPINFOA startupInfo{};
    startupInfo.cb =
        sizeof(startupInfo);

    startupInfo.dwFlags |=
        STARTF_USESHOWWINDOW;

    startupInfo.wShowWindow =
        SW_HIDE;

    char commandLine[MAX_PATH] = {};

    std::snprintf(
        commandLine,
        sizeof(commandLine),
        "\"%s\"",
        dosBoxPath
    );

    BOOL started =
        CreateProcessA(
            nullptr,
            commandLine,
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            DosBoxWorkingDirectory,
            &startupInfo,
            &m_processInfo
        );

    if (!started)
    {
        char text[128] = {};

        std::snprintf(
            text,
            sizeof(text),
            "DOSBox-X start failed: %lu\n",
            GetLastError()
        );

        OutputDebugStringA(
            text
        );

        return false;
    }

    CloseHandle(
        m_processInfo.hThread
    );

    m_processInfo.hThread =
        nullptr;

    return true;
#else
    return false;
#endif
}

bool DosBoxWindow::inputActive() const
{
    return m_inputActive;
}

DosBoxX::NamedPipeClient&
DosBoxWindow::pipeClient()
{
    return m_pipeClient;
}

const MightAndMagic1State&
DosBoxWindow::mightAndMagic1State() const
{
    return
        m_mightAndMagic1Reader.state();
}

void DosBoxWindow::openGame(
    const std::string& mountDirectory,
    const std::string& dosDirectory,
    const std::string& gameFilename
)
{
    m_gameFilename = gameFilename;

    m_controller.openGame(
        m_pipeClient,
        mountDirectory,
        dosDirectory,
        gameFilename
    );
}

void DosBoxWindow::setGermanKeyboardLayout()
{
    m_externalWindow.sendIpcCommand(
        "KEYBOARD_LAYOUT:GR"
    );
}

void DosBoxWindow::setUSKeyboardLayout()
{
    m_externalWindow.sendIpcCommand(
        "KEYBOARD_LAYOUT:US"
    );
}

void DosBoxWindow::setCustomSwitchViewKey(
    ImGuiKey key
)
{
    m_customSwitchViewKey =
        key;
}

void DosBoxWindow::setKeyBindings(
    const DosBoxKeyBindings& keyBindings
)
{
    m_keyBindings =
        &keyBindings;
}

void DosBoxWindow::findDosBoxWindow()
{
    if (m_dosBoxHwnd != nullptr &&
        IsWindow(m_dosBoxHwnd))
    {
        return;
    }

    m_dosBoxHwnd = nullptr;

    const DWORD targetProcessId =
        m_processInfo.dwProcessId;

    if (targetProcessId == 0)
    {
        return;
    }

    EnumWindows(
        [](HWND hwnd, LPARAM parameter) -> BOOL
        {
            auto* window =
                reinterpret_cast<DosBoxWindow*>(
                    parameter
                    );

            DWORD processId = 0;

            GetWindowThreadProcessId(
                hwnd,
                &processId
            );

            if (processId !=
                window->m_processInfo.dwProcessId)
            {
                return TRUE;
            }

            window->m_dosBoxHwnd =
                hwnd;

            char className[256] = {};
            char title[256] = {};
            char text[512] = {};

            GetClassNameA(
                hwnd,
                className,
                sizeof(className)
            );

            GetWindowTextA(
                hwnd,
                title,
                sizeof(title)
            );

            std::snprintf(
                text,
                sizeof(text),
                "FOUND DOSBOX PROCESS WINDOW HWND=%p CLASS=%s TITLE=%s\n",
                hwnd,
                className,
                title
            );

            OutputDebugStringA(text);

            return FALSE;
        },
        reinterpret_cast<LPARAM>(this)
    );
}

void DosBoxWindow::shutdown()
{
#ifdef _WIN32
    deactivateInput();

    if (m_processInfo.hProcess != nullptr)
    {
        m_externalWindow.sendIpcCommand(
            "SHUTDOWN"
        );
    
        const DWORD waitResult =
            WaitForSingleObject(
                m_processInfo.hProcess,
                1000
            );
    
        if (waitResult == WAIT_TIMEOUT)
        {
            TerminateProcess(
                m_processInfo.hProcess,
                0
            );
        }
    
        CloseHandle(
            m_processInfo.hProcess
        );
    
        ZeroMemory(
            &m_processInfo,
            sizeof(m_processInfo)
        );
    }
#endif
}

void DosBoxWindow::deactivateInput()
{
    ClipCursor(
        nullptr
    );

    if (ImGui::GetCurrentContext() != nullptr)
    {
        ImGui::GetIO().ConfigFlags &=
            ~ImGuiConfigFlags_NoMouseCursorChange;
    }

    SDL_ShowCursor();


    if (!m_inputActive)
    {
        return;
    }

    m_externalWindow.sendIpcCommand(
        "RELEASE_ALL"
    );

    m_inputActive = false;
}