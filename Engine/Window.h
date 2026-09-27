#pragma once
#include <array>
#include <string>

struct SDL_Window;

namespace ow3d
{
    class Window
    {
    public:
        Window();
        ~Window();

        bool create(const char* title, int width, int height, bool hidden = false);
        void setSize(int width, int height);
        void destroy();

        void swapBuffers();

        SDL_Window* nativeHandle() const { return m_window; }

        SDL_Window* handle() const
        {
            return m_window;
        }

        void getPosition(int& x, int& y) const;
        void getSize(int& width, int& height) const;

        bool saveState(
            const char* path,
            int viewportWidth,
            int viewportHeight,
            int worldMode
        ) const;

        bool loadState(
            const char* path,
            int& viewportWidth,
            int& viewportHeight,
            int& worldMode
        );

    private:
        SDL_Window* m_window = nullptr;
        void* m_glContext = nullptr;
        mutable std::array<int,7> m_savedState{};
        mutable std::string m_savedStatePath;
        mutable bool m_hasSavedState=false;
    };
}
