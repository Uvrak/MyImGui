#include "Window.h"
#include <glad/gl.h>
#include <SDL3/SDL.h>

#include <fstream>

namespace ow3d
{
    Window::Window() = default;

    Window::~Window()
    {
        destroy();
    }

    bool Window::create(const char* title, int width, int height, bool hidden)
    {
        if (!SDL_Init(SDL_INIT_VIDEO))
            return false;

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
        SDL_GL_SetAttribute(
            SDL_GL_CONTEXT_PROFILE_MASK,
            SDL_GL_CONTEXT_PROFILE_CORE
        );

        m_window = SDL_CreateWindow(
            title,
            width,
            height,
            SDL_WINDOW_OPENGL |
            SDL_WINDOW_RESIZABLE | (hidden ? SDL_WINDOW_HIDDEN : 0)
        );

        SDL_SetWindowPosition(
            m_window,
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED
        );

        if (!m_window)
            return false;

        m_glContext = SDL_GL_CreateContext(m_window);

        if (!m_glContext)
            return false;

        SDL_GL_MakeCurrent(
            m_window,
            static_cast<SDL_GLContext>(m_glContext)
        );

        if (!gladLoadGL(
            reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress)))
        {
            return false;
        }

        SDL_GL_SetSwapInterval(1);

        return true;
    }

    void Window::swapBuffers()
    {
        SDL_GL_SwapWindow(m_window);
    }

    void Window::getPosition(int& x, int& y) const
    {
        SDL_GetWindowPosition(
            m_window,
            &x,
            &y
        );
    }

    void Window::getSize(int& width, int& height) const
    {
        SDL_GetWindowSize(
            m_window,
            &width,
            &height
        );
    }

    bool Window::saveState(
        const char* path,
        int viewportWidth,
        int viewportHeight,
        int worldMode
    ) const
    {
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

        const std::array<int,7> state{x,y,width,height,viewportWidth,viewportHeight,worldMode};
        if(m_hasSavedState && m_savedStatePath==path && m_savedState==state)return true;
        std::ofstream file(path);

        if (!file.is_open())
            return false;

        file << x << '\n';
        file << y << '\n';
        file << width << '\n';
        file << height << '\n';
        file << viewportWidth << '\n';
        file << viewportHeight << '\n';
        file << worldMode << '\n';
        file.close();
        if(!file)return false;
        m_savedState=state;m_savedStatePath=path;m_hasSavedState=true;
        return true;
    }

    bool Window::loadState(
        const char* path,
        int& viewportWidth,
        int& viewportHeight,
        int& worldMode
    )
    {
        std::ifstream file(path);

        if (!file.is_open())
            return false;

        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;

        file >> x;
        file >> y;
        file >> width;
        file >> height;
        file >> viewportWidth;
        file >> viewportHeight;
        file >> worldMode;

        if (!file.good() && !file.eof())
            return false;

        SDL_SetWindowPosition(
            m_window,
            x,
            y
        );

        SDL_SetWindowSize(
            m_window,
            width,
            height
        );

        return true;
    }

    void Window::setSize(int width, int height)
    {
        SDL_SetWindowSize(
            m_window,
            width,
            height
        );
    }
    void Window::destroy()
    {
        if (m_glContext)
        {
            SDL_GL_DestroyContext(
                static_cast<SDL_GLContext>(m_glContext)
            );

            m_glContext = nullptr;
        }

        if (m_window)
        {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }

        SDL_Quit();
    }
}
