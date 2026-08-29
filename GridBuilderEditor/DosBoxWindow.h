#pragma once

#ifdef _WIN32
#include <Windows.h>
#include <SDL3/SDL.h>
#endif

#include <string>

#include "FrameReader.h"
#include "DosBoxFrameTextureSDL.h"
#include "ExternalWindow.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Controller.h"
#include "NamedPipeClient.h"
#include "MightAndMagic1Reader.h"
#include "DosBoxKeyBindings.h"
#include "DosBoxProgramReader.h"
#include "imgui.h"

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

    const MightAndMagic1State&
        mightAndMagic1State() const;

    void openGame(
        const std::string& mountDirectory,
        const std::string& dosDirectory,
        const std::string& gameFilename
    );

    void setGermanKeyboardLayout();
    void setUSKeyboardLayout();

    void setCustomSwitchViewKey(
        ImGuiKey key
    );

    void setKeyBindings(
        const DosBoxKeyBindings& keyBindings
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

    MightAndMagic1Reader
        m_mightAndMagic1Reader;

    DosBoxX::ExternalWindow m_externalWindow;
    DosBoxX::Keyboard m_keyboard;
    DosBoxX::Mouse m_mouse;

    bool m_inputActive = false;
    std::string m_gameFilename;

    bool m_pingTested = false;

    bool m_memorySnapshotRequested = false;

    Uint64 m_nextMightAndMagic1Update = 0;

    ImGuiKey m_customSwitchViewKey =
        ImGuiKey_None;

    bool m_focusRequested = false;

    const DosBoxKeyBindings*
        m_keyBindings = nullptr;

    DosBoxProgramReader
        m_programReader;
};
