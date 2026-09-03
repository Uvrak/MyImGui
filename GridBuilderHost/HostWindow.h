#pragma once

struct SDL_Window;

#ifdef _WIN32
struct HWND__;
using HWND = HWND__*;
#endif

namespace GridBuilderHost
{
    class HostWindowState;

    class HostWindow
    {
    public:
        HostWindow();
        ~HostWindow();

        bool initialize(
            HostWindowState& windowState
        );

        void saveState(
            HostWindowState& windowState
        ) const;

        HWND nativeHandle() const;

        SDL_Window* window() const;

    private:
        SDL_Window* m_window =
            nullptr;
    };
}