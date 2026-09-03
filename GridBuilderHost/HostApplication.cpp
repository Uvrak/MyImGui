#include "HostApplication.h"
#include "DosBoxFramePipeline.h"
#include "HostRenderer.h"
#include "FrameTexture.h"
#include "ImGuiHost.h"
#include "DosBoxWindow.h"
#include "MainMenu.h"
#include "HostUi.h"
#include "MM3Launcher.h"
#include "Keyboard.h"
#include "NamedPipeClient.h"

#include <cstdint>

#include <SDL3/SDL.h>
#include "imgui.h"

#include "imgui_impl_sdl3.h"

namespace GridBuilderHost
{
    HostApplication::HostApplication()
    {}

    HostApplication::~HostApplication()
    {}

    void HostApplication::run(
        DosBoxFramePipeline& dosBoxFramePipeline,
        HostRenderer& hostRenderer,
        DosBoxX::FrameTexture& frameTexture,
        ImGuiHost& imGuiHost,
        HostUi& hostUi,
        MightAndMagic3::MM3Launcher& mm3Launcher,
        DosBoxX::Keyboard& dosBoxKeyboard,
        DosBoxX::NamedPipeClient& dosBoxPipeClient
    )
    {
        bool running =
            true;

        mm3Launcher.start();
        
        while (running)
        {
            SDL_Event event;

            while (SDL_PollEvent(
                &event
            ))
            {
                if (event.type ==
                    SDL_EVENT_QUIT)
                {
                    running =
                        false;
                }

                if (event.type ==
                    SDL_EVENT_WINDOW_RESIZED)
                {
                    hostRenderer.resize(
                        static_cast<uint32_t>(
                            event.window.data1
                            ),
                        static_cast<uint32_t>(
                            event.window.data2
                            )
                    );
                }

                ImGui_ImplSDL3_ProcessEvent(
                    &event
                );
            }

            mm3Launcher.update();

            dosBoxFramePipeline.update();

            hostRenderer.beginFrame();

            imGuiHost.beginFrame();

            dosBoxKeyboard.update(
                dosBoxPipeClient,
                {}
            );

            hostUi.draw(
                frameTexture,
                dosBoxFramePipeline.contentWidth(),
                dosBoxFramePipeline.contentHeight()
            );

            imGuiHost.endFrame();

            hostRenderer.present();
        }
    }
}