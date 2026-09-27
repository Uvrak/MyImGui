#pragma once
#include <functional>

namespace ow3d
{
    class Window;
    class ResolutionWindow;
    class Renderer;

    class MainMenu
    {
    public:
        std::function<void()> additionalMenu;
        void draw(
            Window& window,
            ResolutionWindow& resolutionWindow,
            Renderer& renderer
        );
    };
}
