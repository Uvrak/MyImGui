#include "gridbuilder_ipc.h"

#ifdef WIN32

#include <windows.h>
#include <thread>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <set>
#include <queue>
#include <mutex>
#include <string>
#include <vector>
#include <algorithm>


#include "keyboard.h"
#include "mouse.h"
#include "dosbox.h"
#include "mem.h"
#include <cstdio>

#include "gridbuilder_memory.h"
#include "mightandmagic1.h"

extern std::string RunningProgram;

extern Bitu DOS_SwitchKeyboardLayout(
    const char* new_layout,
    int32_t& tried_cp
);

static std::string
g_gridBuilderRunningProgram;

static std::mutex
g_gridBuilderRunningProgramMutex;

static std::thread g_ipcThread;
static std::atomic<bool> g_ipcRunning{ false };

static std::set<KBD_KEYS>
    g_gridBuilderPressedKeys;

static std::queue<std::string>
g_gridBuilderCommandQueue;

static std::mutex
g_gridBuilderCommandMutex;

static GridBuilderMemory
g_gridBuilderMemory;

static std::atomic<int>
g_mightAndMagic1X{ 0 };

static std::atomic<int>
g_mightAndMagic1Y{ 0 };

static std::atomic<int>
g_mightAndMagic1AreaValueA{ 0 };

static std::atomic<int>
g_mightAndMagic1AreaValueB{ 0 };

static std::atomic<int>
g_mightAndMagic1Direction{
    static_cast<int>(
        MightAndMagic1Direction::Unknown
    )
};

static std::atomic<bool>
g_mightAndMagic1StateValid{ false };

static void GRIDBUILDER_IPC_QueueCommand(
    const char* command
)
{
    std::lock_guard<std::mutex> lock(
        g_gridBuilderCommandMutex
    );

    g_gridBuilderCommandQueue.push(
        command
    );
}

static void GRIDBUILDER_IPC_SetKey(
    KBD_KEYS key,
    bool pressed
)
{
    OutputDebugStringA(
        pressed
        ? "IPC SetKey: DOWN\n"
        : "IPC SetKey: UP\n"
    );

    FILE* debugFile =
        std::fopen(
            "C:\\Projects\\MyImGui\\ipc_key_debug.txt",
            "a"
        );

    if(debugFile != nullptr)
    {
        std::fprintf(
            debugFile,
            "SetKey %s\n",
            pressed
            ? "DOWN"
            : "UP"
        );

        std::fclose(
            debugFile
        );
    }

    KEYBOARD_AddKey(
        key,
        pressed
    );

    if(pressed)
    {
        g_gridBuilderPressedKeys.insert(
            key
        );
    }
    else
    {
        g_gridBuilderPressedKeys.erase(
            key
        );
    }
}

void GRIDBUILDER_IPC_ProcessCommands()
{
    {
        std::lock_guard<std::mutex> lock(
            g_gridBuilderRunningProgramMutex
        );

        g_gridBuilderRunningProgram =
            RunningProgram;
    }

    const MightAndMagic1State state =
        MightAndMagic1::readState();

    g_mightAndMagic1X =
        state.x;

    g_mightAndMagic1Y =
        state.y;

    g_mightAndMagic1AreaValueA =
        state.areaValueA;

    g_mightAndMagic1AreaValueB =
        state.areaValueB;

    g_mightAndMagic1Direction =
        static_cast<int>(
            state.direction
            );

    g_mightAndMagic1StateValid =
        state.valid;

    std::queue<std::string> commands;

    {
        std::lock_guard<std::mutex> lock(
            g_gridBuilderCommandMutex
        );

        std::swap(
            commands,
            g_gridBuilderCommandQueue
        );
    }

    while(!commands.empty())
    {
        const std::string command =
            commands.front();

        commands.pop();

        printf(
            "GridBuilder main thread: %s\n",
            command.c_str()
        );

        const char* text =
            command.c_str();

        if(std::strcmp(
            text,
            "MEMORY_RESET"
        ) == 0)
        {
            g_gridBuilderMemory.resetCandidates();

            printf(
                "GridBuilder memory candidates reset\n"
            );

            continue;
        }

        if(std::strcmp(
            text,
            "MEMORY_SNAPSHOT"
        ) == 0)
        {
            if(g_gridBuilderMemory.captureSnapshot())
            {
                printf(
                    "GridBuilder memory snapshot captured: %zu bytes\n",
                    g_gridBuilderMemory
                    .snapshot()
                    .size()
                );

                const std::vector<size_t> changedAddresses =
                    g_gridBuilderMemory.changedAddresses();

                printf(
                    "GridBuilder changed addresses: %zu\n",
                    changedAddresses.size()
                );

                g_gridBuilderMemory.refineChangedAddresses(
                    {}
                );

                const std::vector<size_t>& candidateAddresses =
                    g_gridBuilderMemory.candidateAddresses();

                char candidateCountText[128]{};

                std::snprintf(
                    candidateCountText,
                    sizeof(candidateCountText),
                    "GridBuilder candidate addresses: %zu\n",
                    candidateAddresses.size()
                );

                OutputDebugStringA(
                    candidateCountText
                );
                constexpr size_t Mm1DumpStart =
                    0x1C610;

                constexpr size_t Mm1DumpEnd =
                    0x1C630;

                        const size_t addressesToPrint =
                    std::min<size_t>(
                        candidateAddresses.size(),
                        100
                    );

                for(size_t index = 0;
                    index < addressesToPrint;
                    ++index)
                {
                    const size_t address =
                        candidateAddresses[index];

                    char debugText[128]{};

                    std::snprintf(
                        debugText,
                        sizeof(debugText),
                        "candidate 0x%05zX = %u\n",
                        address,
                        static_cast<unsigned int>(
                            g_gridBuilderMemory
                            .snapshot()[address]
                            )
                    );

                    OutputDebugStringA(
                        debugText
                    );
                }
            }
            else
            {
                printf(
                    "GridBuilder memory snapshot failed\n"
                );
            }

            continue;
        }

        if(std::strcmp(
            text,
            "MEMORY_REFINE_UNCHANGED"
        ) == 0)
        {
            if(g_gridBuilderMemory.captureSnapshot())
            {
                g_gridBuilderMemory.
                    refineUnchangedAddresses();

                const std::vector<size_t>& candidateAddresses =
                    g_gridBuilderMemory.candidateAddresses();

                printf(
                    "GridBuilder unchanged candidate addresses: %zu\n",
                    candidateAddresses.size()
                );

                const size_t addressesToPrint =
                    std::min<size_t>(
                        candidateAddresses.size(),
                        100
                    );

                for(size_t index = 0;
                    index < addressesToPrint;
                    ++index)
                {
                    const size_t address =
                        candidateAddresses[index];

                    printf(
                        "candidate 0x%05zX = %u\n",
                        address,
                        static_cast<unsigned int>(
                            g_gridBuilderMemory
                            .snapshot()[address]
                            )
                    );
                }
            }
            else
            {
                printf(
                    "GridBuilder unchanged snapshot failed\n"
                );
            }

            continue;
        }
        if(std::strcmp(
            text,
            "RELEASE_ALL"
        ) == 0)
        {
            for(const KBD_KEYS key :
            g_gridBuilderPressedKeys)
            {
                KEYBOARD_AddKey(
                    key,
                    false
                );
            }

            g_gridBuilderPressedKeys.clear();

            KEYBOARD_ClrBuffer();

            continue;
        }

        if(std::strcmp(
            text,
            "KEYBOARD_LAYOUT:GR"
        ) == 0)
        {
            int32_t triedCodepage = 0;

            DOS_SwitchKeyboardLayout(
                "gr",
                triedCodepage
            );

            continue;
        }

        if(std::strcmp(
            text,
            "KEYBOARD_LAYOUT:US"
        ) == 0)
        {
            int32_t triedCodepage = 0;

            DOS_SwitchKeyboardLayout(
                "us",
                triedCodepage
            );

            continue;
        }

        const char* mouseDoubleClickPrefix =
            "MOUSEDOUBLECLICK:";

        if(std::strncmp(
            text,
            mouseDoubleClickPrefix,
            std::strlen(mouseDoubleClickPrefix)
        ) == 0)
        {
            int x = 0;
            int y = 0;
            int width = 0;
            int height = 0;

            if(std::sscanf(
                text + std::strlen(
                    mouseDoubleClickPrefix
                ),
                "%d:%d:%d:%d",
                &x,
                &y,
                &width,
                &height
            ) == 4)
            {
                if(width > 1 &&
                    height > 1)
                {
                    const float normalizedX =
                        static_cast<float>(x) /
                        static_cast<float>(
                            width - 1
                            );

                    const float normalizedY =
                        static_cast<float>(y) /
                        static_cast<float>(
                            height - 1
                            );

                    Mouse_GridBuilderDoubleClick(
                        normalizedX,
                        normalizedY
                    );
                }
            }

            continue;
        }

        const char* mouseClickPrefix =
            "MOUSECLICK:";

        if(std::strncmp(
            text,
            mouseClickPrefix,
            std::strlen(mouseClickPrefix)
        ) == 0)
        {
            int x = 0;
            int y = 0;
            int width = 0;
            int height = 0;

            if(std::sscanf(
                text + std::strlen(mouseClickPrefix),
                "%d:%d:%d:%d",
                &x,
                &y,
                &width,
                &height
            ) == 4)
            {
                if(width > 1 &&
                    height > 1)
                {
                    const float normalizedX =
                        static_cast<float>(x) /
                        static_cast<float>(
                            width - 1
                            );

                    const float normalizedY =
                        static_cast<float>(y) /
                        static_cast<float>(
                            height - 1
                            );

                    Mouse_GridBuilderClick(
                        normalizedX,
                        normalizedY
                    );
                }
            }

            continue;
        }

        const char* mouseSetPosPrefix =
            "MOUSESETPOS:";

        if(std::strncmp(
            text,
            mouseSetPosPrefix,
            std::strlen(mouseSetPosPrefix)
        ) == 0)
        {
            int x = 0;
            int y = 0;
            int width = 0;
            int height = 0;

            if(std::sscanf(
                text + std::strlen(mouseSetPosPrefix),
                "%d:%d:%d:%d",
                &x,
                &y,
                &width,
                &height
            ) == 4)
            {
                if(width > 1 &&
                    height > 1)
                {
                    const float normalizedX =
                        static_cast<float>(x) /
                        static_cast<float>(
                            width - 1
                            );

                    const float normalizedY =
                        static_cast<float>(y) /
                        static_cast<float>(
                            height - 1
                            );

                    Mouse_GridBuilderSetPosition(
                        normalizedX,
                        normalizedY
                    );
                }
            }

            continue;
        }

        const char* mouseMovePrefix =
            "MOUSEMOVE:";

        if(std::strncmp(
            text,
            mouseMovePrefix,
            std::strlen(mouseMovePrefix)
        ) == 0)
        {
            int x = 0;
            int y = 0;
            int width = 0;
            int height = 0;

            if(std::sscanf(
                text + std::strlen(mouseMovePrefix),
                "%d:%d:%d:%d",
                &x,
                &y,
                &width,
                &height
            ) == 4)
            {
                if(width > 1 &&
                    height > 1)
                {
                    const float normalizedX =
                        static_cast<float>(x) /
                        static_cast<float>(
                            width - 1
                            );

                    const float normalizedY =
                        static_cast<float>(y) /
                        static_cast<float>(
                            height - 1
                            );

                   Mouse_GridBuilderMove(
                        normalizedX,
                        normalizedY
                    );
                }
            }
        }

        if(std::strcmp(
            text,
            "MOUSEDOWN:0"
        ) == 0)
        {
            Mouse_ButtonPressed(0);

            continue;
        }

        if(std::strcmp(
            text,
            "MOUSEDOWN:1"
        ) == 0)
        {
            Mouse_ButtonPressed(1);

            continue;
        }

        if(std::strcmp(
            text,
            "MOUSEUP:0"
        ) == 0)
        {
            Mouse_ButtonReleased(0);

            continue;
        }

        if(std::strcmp(
            text,
            "MOUSEUP:1"
        ) == 0)
        {
            Mouse_ButtonReleased(1);

            continue;
        }

        if(std::strcmp(
            text,
            "MOUSEWHEEL:UP"
        ) == 0)
        {
            Mouse_WheelMoved(1);
            continue;
        }

        if(std::strcmp(
            text,
            "MOUSEWHEEL:DOWN"
        ) == 0)
        {
            Mouse_WheelMoved(-1);
            continue;
        }
        const char* keyDownPrefix =
            "KEYDOWN:";

        const char* keyUpPrefix =
            "KEYUP:";

        const bool isKeyDown =
            std::strncmp(
                text,
                keyDownPrefix,
                std::strlen(keyDownPrefix)
            ) == 0;

        const bool isKeyUp =
            std::strncmp(
                text,
                keyUpPrefix,
                std::strlen(keyUpPrefix)
            ) == 0;

        const char* ipcKeyName = nullptr;

        if(isKeyDown)
        {
            ipcKeyName =
                text + std::strlen(
                    keyDownPrefix
                );
        }
        else if(isKeyUp)
        {
            ipcKeyName =
                text + std::strlen(
                    keyUpPrefix
                );
        }

        struct GridBuilderKeyMapping
        {
            const char* command;
            KBD_KEYS key;
        };

        static const GridBuilderKeyMapping ipcLetterMappings[] =
        {
            { "A", KBD_a },
            { "B", KBD_b },
            { "C", KBD_c },
            { "D", KBD_d },
            { "E", KBD_e },
            { "F", KBD_f },
            { "G", KBD_g },
            { "H", KBD_h },
            { "I", KBD_i },
            { "J", KBD_j },
            { "K", KBD_k },
            { "L", KBD_l },
            { "M", KBD_m },
            { "N", KBD_n },
            { "O", KBD_o },
            { "P", KBD_p },
            { "Q", KBD_q },
            { "R", KBD_r },
            { "S", KBD_s },
            { "T", KBD_t },
            { "U", KBD_u },
            { "V", KBD_v },
            { "W", KBD_w },
            { "X", KBD_x },
            { "Y", KBD_y },
            { "Z", KBD_z }
        };

        static const GridBuilderKeyMapping ipcDigitMappings[] =
        {
            { "0", KBD_0 },
            { "1", KBD_1 },
            { "2", KBD_2 },
            { "3", KBD_3 },
            { "4", KBD_4 },
            { "5", KBD_5 },
            { "6", KBD_6 },
            { "7", KBD_7 },
            { "8", KBD_8 },
            { "9", KBD_9 }
        };

        static const GridBuilderKeyMapping ipcFunctionKeyMappings[] =
        {
            { "F1",  KBD_f1 },
            { "F2",  KBD_f2 },
            { "F3",  KBD_f3 },
            { "F4",  KBD_f4 },
            { "F5",  KBD_f5 },
            { "F6",  KBD_f6 },
            { "F7",  KBD_f7 },
            { "F8",  KBD_f8 },
            { "F9",  KBD_f9 },
            { "F10", KBD_f10 },
            { "F11", KBD_f11 },
            { "F12", KBD_f12 }
        };

        static const GridBuilderKeyMapping ipcNavigationKeyMappings[] =
        {
            { "HOME",     KBD_home },
            { "END",      KBD_end },
            { "INSERT",   KBD_insert },
            { "DELETE",   KBD_delete },
            { "PAGEUP",   KBD_pageup },
            { "PAGEDOWN", KBD_pagedown }
        };

        static const GridBuilderKeyMapping ipcSymbolKeyMappings[] =
        {
            { "MINUS",        KBD_minus },
            { "EQUALS",       KBD_equals },
            { "LEFTBRACKET",  KBD_leftbracket },
            { "RIGHTBRACKET", KBD_rightbracket },
            { "BACKSLASH",    KBD_backslash },
            { "SEMICOLON",    KBD_semicolon },
            { "QUOTE",        KBD_quote },
            { "COMMA",        KBD_comma },
            { "PERIOD",       KBD_period },
            { "SLASH",        KBD_slash }
        };
        if(ipcKeyName != nullptr)
        {
            for(const auto& mapping :
                ipcLetterMappings)
            {
                if(std::strcmp(
                    ipcKeyName,
                    mapping.command
                ) == 0)
                {
                    GRIDBUILDER_IPC_SetKey(
                        mapping.key,
                        isKeyDown
                    );

                    break;
                }
            }

            for(const auto& mapping :
                ipcDigitMappings)
            {
                if(std::strcmp(
                    ipcKeyName,
                    mapping.command
                ) == 0)
                {
                    GRIDBUILDER_IPC_SetKey(
                        mapping.key,
                        isKeyDown
                    );

                    break;
                }
            }

            for(const auto& mapping :
                ipcFunctionKeyMappings)
            {
                if(std::strcmp(
                    ipcKeyName,
                    mapping.command
                ) == 0)
                {
                    GRIDBUILDER_IPC_SetKey(
                        mapping.key,
                        isKeyDown
                    );

                    break;
                }
            }

            for(const auto& mapping :
                ipcNavigationKeyMappings)
            {
                if(std::strcmp(
                    ipcKeyName,
                    mapping.command
                ) == 0)
                {
                    GRIDBUILDER_IPC_SetKey(
                        mapping.key,
                        isKeyDown
                    );

                    break;
                }
            }

            for(const auto& mapping :
                ipcSymbolKeyMappings)
            {
                if(std::strcmp(
                    ipcKeyName,
                    mapping.command
                ) == 0)
                {
                    GRIDBUILDER_IPC_SetKey(
                        mapping.key,
                        isKeyDown
                    );

                    break;
                }
            }
            if(std::strcmp(ipcKeyName, "ENTER") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_enter,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "SPACE") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_space,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "BACKSPACE") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_backspace,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "TAB") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_tab,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "ESC") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_esc,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "UP") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_up,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "DOWN") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_down,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "LEFT") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_left,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "RIGHT") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_right,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "SHIFT") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_leftshift,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "CTRL") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_leftctrl,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "ALT") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_leftalt,
                    isKeyDown
                );
            }

            if(std::strcmp(ipcKeyName, "ALTGR") == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_rightalt,
                    isKeyDown
                );
            }
            if(std::strcmp(
                ipcKeyName,
                "CAPSLOCK"
            ) == 0)
            {
                GRIDBUILDER_IPC_SetKey(
                    KBD_capslock,
                    isKeyDown
                );

                continue;
            }
        }
    }
}
static void GRIDBUILDER_IPC_Thread()
{
    OutputDebugStringA(
        "GRIDBUILDER IPC THREAD: NEW BUILD\n"
    );

    while(g_ipcRunning)
    {
        HANDLE pipe =
            CreateNamedPipeA(
                "\\\\.\\pipe\\GridBuilderDOSBox",
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_MESSAGE |
                PIPE_READMODE_MESSAGE |
                PIPE_WAIT,
                1,
                256,
                256,
                0,
                nullptr
            );

        if(pipe == INVALID_HANDLE_VALUE)
        {
            printf(
                "GridBuilder IPC: CreateNamedPipe failed\n"
            );

            return;
        }

        printf(
            "GridBuilder IPC: waiting for client\n"
        );

        BOOL connected =
            ConnectNamedPipe(
                pipe,
                nullptr
            );

        if(!connected &&
            GetLastError() != ERROR_PIPE_CONNECTED)
        {
            CloseHandle(pipe);

            if(g_ipcRunning)
            {
                printf(
                    "GridBuilder IPC: ConnectNamedPipe failed\n"
                );
            }

            continue;
        }

        if(!g_ipcRunning)
        {
            CloseHandle(pipe);
            break;
        }

        printf(
            "GridBuilder IPC: client connected\n"
        );

        while(g_ipcRunning)
        {
            char buffer[256]{};

            DWORD bytesRead = 0;

            BOOL result =
                ReadFile(
                    pipe,
                    buffer,
                    sizeof(buffer) - 1,
                    &bytesRead,
                    nullptr
                );

            if(!result || bytesRead == 0)
            {
                break;
            }

            buffer[bytesRead] = '\0';

            if(std::strcmp(
                buffer,
                "RUNNING_PROGRAM"
            ) == 0)
            {
                std::string runningProgram;

                {
                    std::lock_guard<std::mutex> lock(
                        g_gridBuilderRunningProgramMutex
                    );

                    runningProgram =
                        g_gridBuilderRunningProgram;
                }

                std::string response =
                    "RUNNING_PROGRAM:";

                response +=
                    runningProgram;

                DWORD bytesWritten = 0;

                WriteFile(
                    pipe,
                    response.data(),
                    static_cast<DWORD>(
                        response.size()
                        ),
                    &bytesWritten,
                    nullptr
                );

                continue;
            }

            if(std::strcmp(
                buffer,
                "MM1_STATE"
            ) == 0)
            {
                const int directionValue =
                    g_mightAndMagic1Direction.load();

                const char* direction =
                    "UNKNOWN";

                switch(
                    static_cast<
                    MightAndMagic1Direction
                    >(directionValue)
                    )
                {
                case MightAndMagic1Direction::North:
                    direction = "NORTH";
                    break;

                case MightAndMagic1Direction::East:
                    direction = "EAST";
                    break;

                case MightAndMagic1Direction::South:
                    direction = "SOUTH";
                    break;

                case MightAndMagic1Direction::West:
                    direction = "WEST";
                    break;

                case MightAndMagic1Direction::Unknown:
                    break;
                }

                char response[256]{};

                std::snprintf(
                    response,
                    sizeof(response),
                    "MM1_STATE:%d:%d:%d:%d:%s:%d",
                    g_mightAndMagic1X.load(),
                    g_mightAndMagic1Y.load(),
                    g_mightAndMagic1AreaValueA.load(),
                    g_mightAndMagic1AreaValueB.load(),
                    direction,
                    g_mightAndMagic1StateValid.load()
                    ? 1
                    : 0
                );

                DWORD bytesWritten = 0;

                WriteFile(
                    pipe,
                    response,
                    static_cast<DWORD>(
                        std::strlen(response)
                        ),
                    &bytesWritten,
                    nullptr
                );

                continue;
            }

            if(std::strcmp(
                buffer,
                "PING"
            ) == 0)
            {
                const char* response =
                    "PONG";

                DWORD bytesWritten = 0;

                WriteFile(
                    pipe,
                    response,
                    static_cast<DWORD>(
                        std::strlen(response)
                        ),
                    &bytesWritten,
                    nullptr
                );

                continue;
            }

            GRIDBUILDER_IPC_QueueCommand(
                buffer
            );
        }

        DisconnectNamedPipe(
            pipe
        );

        CloseHandle(
            pipe
        );
    }

    printf(
        "GridBuilder IPC: stopped\n"
    );
}

#endif

void GRIDBUILDER_IPC_Init()
{
#ifdef WIN32

    if(g_ipcRunning)
    {
        return;
    }

    g_ipcRunning = true;

    g_ipcThread =
        std::thread(
            GRIDBUILDER_IPC_Thread
        );

#endif
}

void GRIDBUILDER_IPC_Shutdown()
{
#ifdef WIN32

    g_ipcRunning = false;

    HANDLE pipe =
        CreateFileA(
            "\\\\.\\pipe\\GridBuilderDOSBox",
            GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

    if(pipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(pipe);
    }

    if(g_ipcThread.joinable())
    {
        g_ipcThread.join();
    }

#endif
}
