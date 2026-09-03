#include "ImGuiHost.h"

#include "imgui.h"

#include "imgui_impl_sdl3.h"
#include "imgui_impl_dx11.h"

namespace GridBuilderHost
{
    ImGuiHost::ImGuiHost()
    {}

    ImGuiHost::~ImGuiHost()
    {
        shutdown();
    }

    bool ImGuiHost::initialize(
        SDL_Window* window,
        ID3D11Device* device,
        ID3D11DeviceContext* context
    )
    {
        IMGUI_CHECKVERSION();

        ImGui::CreateContext();

        ImGuiIO& io =
            ImGui::GetIO();

        io.ConfigFlags |=
            ImGuiConfigFlags_DockingEnable;

        if (!ImGui_ImplSDL3_InitForD3D(
            window
        ))
        {
            return false;
        }

        if (!ImGui_ImplDX11_Init(
            device,
            context
        ))
        {
            return false;
        }

        return true;
    }

    void ImGuiHost::beginFrame()
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport(
            0,
            ImGui::GetMainViewport()
        );
    }

    void ImGuiHost::endFrame()
    {
        ImGui::Render();

        ImGui_ImplDX11_RenderDrawData(
            ImGui::GetDrawData()
        );
    }

    void ImGuiHost::shutdown()
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplSDL3_Shutdown();

        if (ImGui::GetCurrentContext() != nullptr)
        {
            ImGui::DestroyContext();
        }
    }
}