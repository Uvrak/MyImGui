#include "HostWindow.h"

#include "HostWindowState.h"

#include <SDL3/SDL.h>

namespace GridBuilderHost
{
    HostWindow::HostWindow()
    {
    }

    HostWindow::~HostWindow()
    {
        if (m_window != nullptr)
        {
            SDL_DestroyWindow(
                m_window
            );

            m_window =
                nullptr;
        }

        SDL_Quit();
    }

    bool HostWindow::initialize(
        HostWindowState& windowState
    )
    {
        if (!SDL_Init(
            SDL_INIT_VIDEO
        ))
        {
            return false;
        }

        windowState.load();

        m_window =
            SDL_CreateWindow(
                "GridBuilderHost",
                windowState.width(),
                windowState.height(),
                SDL_WINDOW_RESIZABLE
            );

        if (m_window == nullptr)
        {
            SDL_Quit();
            return false;
        }

        SDL_SetWindowPosition(
            m_window,
            windowState.x(),
            windowState.y()
        );

        return true;
    }

    void HostWindow::saveState(
        HostWindowState& windowState
    ) const
    {
        if (m_window == nullptr)
        {
            return;
        }

        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;

        SDL_GetWindowPosition(
            m_window,
            &x,
            &y
        );

        SDL_GetWindowSize(
            m_window,
            &width,
            &height
        );

        windowState.setPosition(
            x,
            y
        );

        windowState.setSize(
            width,
            height
        );

        windowState.save();
    }
   
    HWND HostWindow::nativeHandle() const
    {
        if (m_window == nullptr)
        {
            return nullptr;
        }

        const SDL_PropertiesID properties =
            SDL_GetWindowProperties(
                m_window
            );

        return static_cast<HWND>(
            SDL_GetPointerProperty(
                properties,
                SDL_PROP_WINDOW_WIN32_HWND_POINTER,
                nullptr
            )
            );
    }
}