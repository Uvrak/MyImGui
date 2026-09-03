#pragma once

struct SDL_Window;
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace GridBuilderHost
{
    class ImGuiHost
    {
    public:
        ImGuiHost();
        ~ImGuiHost();

        bool initialize(
            SDL_Window* window,
            ID3D11Device* device,
            ID3D11DeviceContext* context
        );

        void beginFrame();
        void endFrame();

        void shutdown();
    };
}