#pragma once

#ifdef _WIN32
#include <Windows.h>
#include <SDL3/SDL.h>
#endif

#include <string>
#include <functional>

#include "FrameReader.h"
#include "DosBoxFrameTextureSDL.h"
#include "ExternalWindow.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Controller.h"
#include "NamedPipeClient.h"
#include "DosBoxKeyBindings.h"
#include "DosBoxProgramReader.h"
#include "GameModule.h"

#include "imgui.h"

enum class DosBoxInputMode
{
    Focused,
    AlwaysActive
};

class DosBoxWindow
{
public:
    explicit DosBoxWindow(
        SDL_Window* parentWindow,
        SDL_Renderer* renderer
    );

    ~DosBoxWindow();

    void draw();
    bool start();

    bool inputActive() const;

    DosBoxX::NamedPipeClient&
        pipeClient();

    void openGame(
        const std::string& mountDirectory,
        const std::string& dosDirectory,
        const std::string& gameFilename
    );

    bool sendDosKey(
        const char* key
    );

    void setGermanKeyboardLayout();
    void setUSKeyboardLayout();

    void setCustomSwitchViewKey(
        ImGuiKey key
    );

    void setKeyBindings(
        const DosBoxKeyBindings& keyBindings
    );

    void setDirectKeyboardBlocked(
        bool blocked
    );

    void setGameButtonSelection(
        bool active,
        const GameButtonRect& rect
    );

    void setButtonRectSaveCallback(
        std::function<void(
            const GameButtonRect&
            )> callback
    );

    void setButtonRectModifyCallback(
        std::function<void(
            const GameButtonRect&
            )> callback
    );

    bool handleButtonRectEditorKeyDown(
        SDL_Keycode key
    );

    void setButtonRectDeleteCallback(
        std::function<void()> callback
    );

    bool sendDosMouseClick(
        float x,
        float y
    );

    const DosBoxX::DosBoxFrameHeader*
        frameHeader() const;

    const uint8_t*
        framePixels() const;

    bool sendDosMouseDoubleClick(
        float x,
        float y
    );

    bool sendDosMousePosition(
        float x,
        float y
    );

private:
    SDL_Window* m_parentWindow =
        nullptr;

    DosBoxX::FrameReader m_frameReader;

    DosBoxFrameTextureSDL m_frameTexture;

    HWND m_parentHwnd =
        nullptr;

    HWND m_dosBoxHwnd =
        nullptr;

    void findDosBoxWindow();
	void shutdown();
    void deactivateInput();

    bool m_started = false;
    bool m_startAttempted = false;
    bool m_tabWasPressed = false;

    PROCESS_INFORMATION m_processInfo{};
    DosBoxX::Controller m_controller;

    DosBoxX::NamedPipeClient m_pipeClient;

    DosBoxX::ExternalWindow m_externalWindow;
    DosBoxX::Keyboard m_keyboard;
    DosBoxX::Mouse m_mouse;

    bool m_inputActive = false;

    DosBoxInputMode m_inputMode =
        DosBoxInputMode::Focused;

    std::string m_gameFilename;

    bool m_pingTested = false;

    bool m_memorySnapshotRequested = false;

    ImGuiKey m_customSwitchViewKey =
        ImGuiKey_None;

    bool m_focusRequested = false;

    const DosBoxKeyBindings*
        m_keyBindings = nullptr;

    DosBoxProgramReader
        m_programReader;

    bool m_directKeyboardBlocked =
        false;

    bool m_gameButtonSelectionActive =
        false;

    GameButtonRect m_gameButtonRect;

    GameButtonRect m_debugButtonRect;

    bool m_buttonRectToolActive =
        false;

    bool m_buttonRectDragging =
        false;

    bool m_buttonRectDefined =
        false;

    bool m_buttonRectMoving =
        false;

    bool m_buttonRectResizing =
        false;

    bool m_modifyingGameButton =
        false;

    ImVec2 m_buttonRectMoveOffset;

    ImVec2 m_buttonRectStart;
    ImVec2 m_buttonRectEnd;

    std::function<void(
        const GameButtonRect&
        )> m_buttonRectSaveCallback;

    std::function<void(
        const GameButtonRect&
        )> m_buttonRectModifyCallback;

    std::function<void()>
        m_buttonRectDeleteCallback;
};
