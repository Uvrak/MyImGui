#include "pch.h"
#include "ImGuiSystem.h"

#include <imgui.h>

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>

namespace MyImGui
{
    static float g_fontSize = 18.0f;

    bool initialize(SDL_Window* window)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        io.Fonts->Clear();

        ImFontConfig fontConfig;
        fontConfig.SizePixels = g_fontSize;

        io.Fonts->AddFontDefault(&fontConfig);

        ImGui::StyleColorsDark();

        if (!ImGui_ImplSDL3_InitForOpenGL(
            window,
            SDL_GL_GetCurrentContext()))
        {
            return false;
        }

        if (!ImGui_ImplOpenGL3_Init("#version 460"))
        {
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();

            return false;
        }

        return true;
    }

    void processEvent(const SDL_Event& event)
    {
        ImGui_ImplSDL3_ProcessEvent(&event);
    }

    void beginFrame()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();

        ImGui::NewFrame();
    }

    void beginDockspace()
    {
        ImGui::DockSpaceOverViewport(
            0,
            ImGui::GetMainViewport()
        );
    }

    void endFrame()
    {
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(
            ImGui::GetDrawData()
        );
    }

    void shutdown()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();

        ImGui::DestroyContext();
    }

    void setFontSize(float size)
    {
        g_fontSize = size;
    }

    float fontSize()
    {
        return g_fontSize;
    }
}