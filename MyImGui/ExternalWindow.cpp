#include "pch.h"
#include "ExternalWindow.h"

#pragma comment(lib, "dwmapi.lib")

namespace MyImGui
{
    ExternalWindow::ExternalWindow()
    {}
    ExternalWindow::~ExternalWindow()
    {
        if (m_processHandle != nullptr)
        {
            TerminateProcess(
                m_processHandle,
                0
            );

            CloseHandle(
                m_processHandle
            );

            m_processHandle = nullptr;
            m_processId = 0;
        }
    }

    bool ExternalWindow::startProcess(
        const std::string& executablePath
    )
    {
        STARTUPINFOA startupInfo{};
        startupInfo.cb = sizeof(startupInfo);

        PROCESS_INFORMATION processInfo{};

        BOOL success = CreateProcessA(
            executablePath.c_str(),
            nullptr,
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo
        );

        if (!success)
        {
            return false;
        }

        CloseHandle(
            processInfo.hThread
        );

        m_processHandle =
            processInfo.hProcess;

        m_processId =
            processInfo.dwProcessId;

        return true;
    }
    
    bool ExternalWindow::findByTitle(
        const std::string& title
    )
    {
        m_handle = nullptr;
        m_title.clear();
        m_className.clear();

        struct SearchData
        {
            ExternalWindow* self;
            const std::string* title;
        };

        SearchData data{
            this,
            &title
        };

        EnumWindows(
            [](HWND hwnd, LPARAM lParam) -> BOOL
            {
                if (!IsWindowVisible(hwnd))
                {
                    return TRUE;
                }

                char windowTitle[512] = {};

                GetWindowTextA(
                    hwnd,
                    windowTitle,
                    sizeof(windowTitle)
                );

                std::string currentTitle =
                    windowTitle;

                SearchData* data =
                    reinterpret_cast<SearchData*>(
                        lParam
                        );

                if (currentTitle.find(
                    *data->title
                ) == std::string::npos)
                {
                    return TRUE;
                }

                char windowClass[256] = {};

                GetClassNameA(
                    hwnd,
                    windowClass,
                    sizeof(windowClass)
                );

                if (std::string(windowClass) != "SDLParent")
                {
                    return TRUE;
                }

                data->self->m_handle = hwnd;
                data->self->m_title =
                    currentTitle;
                data->self->m_className =
                    windowClass;

                return FALSE;
            },
            reinterpret_cast<LPARAM>(
                &data
                )
        );
        if (m_handle != nullptr)
        {
            m_childHandle =
                FindWindowExA(
                    m_handle,
                    nullptr,
                    nullptr,
                    nullptr
                );

            if (m_childHandle != nullptr)
            {
                char childTitle[512] = {};
                char childClass[256] = {};

                GetWindowTextA(
                    m_childHandle,
                    childTitle,
                    sizeof(childTitle)
                );

                GetClassNameA(
                    m_childHandle,
                    childClass,
                    sizeof(childClass)
                );

                m_childTitle = childTitle;
                m_childClassName = childClass;
            }
            else
            {
                m_childTitle.clear();
                m_childClassName.clear();
            }
        }
        else
        {
            m_childHandle = nullptr;
            m_childTitle.clear();
            m_childClassName.clear();
        }


        return m_handle != nullptr;
    }

    void ExternalWindow::hideFromTaskbar()
    {
        if (m_handle == nullptr)
        {
            return;
        }

        LONG_PTR style =
            GetWindowLongPtr(
                m_handle,
                GWL_EXSTYLE
            );

        style &= ~WS_EX_APPWINDOW;
        style |= WS_EX_TOOLWINDOW;

        SetWindowLongPtr(
            m_handle,
            GWL_EXSTYLE,
            style
        );

        SetWindowPos(
            m_handle,
            nullptr,
            0,
            0,
            0,
            0,
            SWP_NOMOVE |
            SWP_NOSIZE |
            SWP_NOZORDER |
            SWP_FRAMECHANGED
        );
    }
    bool ExternalWindow::findProcessWindow()
    {
        if (m_processId == 0)
        {
            return false;
        }

        m_handle = nullptr;

        EnumWindows(
            [](HWND hwnd, LPARAM lParam) -> BOOL
            {
                auto* self =
                    reinterpret_cast<ExternalWindow*>(
                        lParam
                        );

                DWORD processId = 0;

                GetWindowThreadProcessId(
                    hwnd,
                    &processId
                );

                if (processId != self->m_processId)
                {
                    return TRUE;
                }

                if (!IsWindowVisible(hwnd))
                {
                    return TRUE;
                }

                self->m_handle = hwnd;

                return FALSE;
            },
            reinterpret_cast<LPARAM>(this)
        );

        return m_handle != nullptr;
    }

    bool ExternalWindow::sendIpcCommand(
        const std::string& command
    )
    {
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

        if (pipe == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        DWORD bytesWritten = 0;

        BOOL result =
            WriteFile(
                pipe,
                command.c_str(),
                static_cast<DWORD>(
                    command.size()
                    ),
                &bytesWritten,
                nullptr
            );

        CloseHandle(
            pipe
        );

        return result != FALSE;
    }


    char windowTitle[512] = {};
    char windowClass[256] = {};

    HWND ExternalWindow::handle() const
    {
        return m_handle;
    }

    HWND ExternalWindow::childHandle() const
    {
        return m_childHandle;
    }

    const std::string&
        ExternalWindow::title() const
    {
        return m_title;
    }

    const std::string&
        ExternalWindow::className() const
    {
        return m_className;
    }

    const std::string&
        ExternalWindow::childTitle() const
    {
        return m_childTitle;
    }

    const std::string&
        ExternalWindow::childClassName() const
    {
        return m_childClassName;
    }

    bool ExternalWindow::attach(
        HWND parent
    )
    {
        if (m_handle == nullptr ||
            parent == nullptr)
        {
            return false;
        }

        LONG_PTR style =
            GetWindowLongPtr(
                m_handle,
                GWL_STYLE
            );

        style &= ~(
            WS_POPUP |
            WS_CAPTION |
            WS_THICKFRAME |
            WS_MINIMIZEBOX |
            WS_MAXIMIZEBOX |
            WS_SYSMENU
            );

        style |= WS_CHILD;

        SetWindowLongPtr(
            m_handle,
            GWL_STYLE,
            style
        );

        SetMenu(
            m_handle,
            nullptr
        );

        SetLastError(0);

        HWND previousParent =
            SetParent(
                m_childHandle,
                parent
            );

        if (previousParent == nullptr &&
            GetLastError() != 0)
        {
            return false;
        }

        return true;
    }

    bool ExternalWindow::registerThumbnail(
        HWND destination
    )
    {
        if (m_handle == nullptr ||
            destination == nullptr)
        {
            return false;
        }

        if (m_thumbnail != nullptr)
        {
            DwmUnregisterThumbnail(
                m_thumbnail
            );

            m_thumbnail = nullptr;
        }

        HRESULT result =
            DwmRegisterThumbnail(
                destination,
                m_handle,
                &m_thumbnail
            );

        return SUCCEEDED(result);
    }

    void ExternalWindow::setBounds(
        int x,
        int y,
        int width,
        int height
    )
    {
        if (m_handle == nullptr)
        {
            return;
        }

        char title[256]{};

        GetWindowTextA(
            m_handle,
            title,
            sizeof(title)
        );

        char className[256]{};

        GetClassNameA(
            m_handle,
            className,
            sizeof(className)
        );

        printf(
            "MoveWindow handle=%p title=%s class=%s\n",
            m_handle,
            title,
            className
        );

        fflush(stdout);

        /*
        MoveWindow(
            m_handle,
            x,
            y,
            width,
            height,
            TRUE
        );
        */

        SetWindowPos(
            m_handle,
            nullptr,
            x,
            y,
            width,
            height,
            SWP_NOZORDER |
            SWP_NOACTIVATE |
            SWP_SHOWWINDOW
        );
    }

    bool ExternalWindow::focus()
    {
        if (m_handle == nullptr)
        {
            return false;
        }

        return SetForegroundWindow(
            m_handle
        ) != FALSE;
    }

    bool ExternalWindow::sendEnter()
    {
        if (m_childHandle == nullptr)
        {
            return false;
        }

        BOOL downResult =
            PostMessage(
                m_childHandle,
                WM_KEYDOWN,
                VK_RETURN,
                0
            );

        BOOL upResult =
            PostMessage(
                m_childHandle,
                WM_KEYUP,
                VK_RETURN,
                0
            );

        return downResult != FALSE &&
            upResult != FALSE;
    }

    bool ExternalWindow::sendKey(
        UINT virtualKey,
        bool pressed
    )
    {
        printf(
            "sendKey child=%p\n",
            m_childHandle
        );
        if (m_childHandle == nullptr)
        {
            return false;
        }

        UINT message =
            pressed
            ? WM_KEYDOWN
            : WM_KEYUP;

        return PostMessage(
            m_childHandle,
            message,
            static_cast<WPARAM>(virtualKey),
            0
        ) != FALSE;
    }

    bool ExternalWindow::updateThumbnail(
        int x,
        int y,
        int width,
        int height
    )
    {
        if (m_thumbnail == nullptr)
        {
            return false;
        }

        DWM_THUMBNAIL_PROPERTIES properties = {};

        properties.dwFlags =
            DWM_TNP_RECTDESTINATION |
            DWM_TNP_VISIBLE |
            DWM_TNP_OPACITY;

        properties.rcDestination.left =
            x;

        properties.rcDestination.top =
            y;

        properties.rcDestination.right =
            x + width;

        properties.rcDestination.bottom =
            y + height;

        properties.opacity = 255;
        properties.fVisible = TRUE;

        HRESULT result =
            DwmUpdateThumbnailProperties(
                m_thumbnail,
                &properties
            );

        return SUCCEEDED(result);
    }
}

