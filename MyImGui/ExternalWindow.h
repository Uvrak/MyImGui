#pragma once

#ifdef _WIN32
#include <Windows.h>
#include <dwmapi.h>
#endif

#include <string>

namespace MyImGui
{
    class ExternalWindow
    {
    public:
        ExternalWindow();
        ~ExternalWindow();

        bool startProcess(
            const std::string& executablePath
        );

        bool findByTitle(
            const std::string& title
        );

        void hideFromTaskbar();

        bool findProcessWindow();

        bool sendIpcCommand(
            const std::string& command
        );

        bool waitForProcessWindow(
            DWORD timeoutMilliseconds
        );

        HWND handle() const;
        HWND childHandle() const;

        const std::string& title() const;
        const std::string& className() const;

        const std::string& childTitle() const;
        const std::string& childClassName() const;

        bool attach(
            HWND parent
        );

        bool registerThumbnail(
            HWND destination
        );

        bool updateThumbnail(
            int x,
            int y,
            int width,
            int height
        );

        void setBounds(
            int x,
            int y,
            int width,
            int height
        );

        bool focus();
        bool sendEnter();
        bool sendKey(
            UINT virtualKey,
            bool pressed
        );

    private:
        HWND m_handle = nullptr;
        HWND m_childHandle = nullptr;

        HTHUMBNAIL m_thumbnail = nullptr;

        std::string m_title;
        std::string m_className;

        HANDLE m_processHandle = nullptr;
        DWORD m_processId = 0;

        std::string m_childTitle;
        std::string m_childClassName;
        
        HANDLE m_ipcPipe = INVALID_HANDLE_VALUE;
    };
}
