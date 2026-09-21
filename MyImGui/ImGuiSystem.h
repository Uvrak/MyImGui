#pragma once

#include <SDL3/SDL.h>

namespace MyImGui
{
    bool initialize(SDL_Window* window);

    void processEvent(const SDL_Event& event);

    void beginFrame();
    void beginDockspace();
    void endFrame();

    void shutdown();

    void setFontSize(float size);
    float fontSize();
}