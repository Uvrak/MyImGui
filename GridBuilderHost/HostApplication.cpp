#include "HostApplication.h"
#include "DosBoxFramePipeline.h"
#include "HostRenderer.h"
#include "FrameTexture.h"
#include "ImGuiHost.h"
#include "DosBoxWindow.h"
#include "MainMenu.h"
#include "HostUi.h"
#include "../GridBuilderCore/GameModule.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "NamedPipeClient.h"
#include "Memory.h"
#include "GridBuilderGrid.h"
#include "DebugWindow.h"
#include "../MouseLatencyTrace.h"


#include <cstdio>
#include <Windows.h>
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
        GridBuilderGrid& gridBuilderGrid,
        HostUi& hostUi,
        GameModule& gameModule,
        DosBoxX::Keyboard& dosBoxKeyboard,
        DosBoxX::Mouse& dosBoxMouse,
        DosBoxX::Memory& dosBoxMemory,
        DosBoxX::NamedPipeClient& dosBoxPipeClient
    )
    {
        bool running =
            true;

        DebugWindow debugWindow;

        gameModule.start();
        
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

                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                    event.button.button == SDL_BUTTON_LEFT)
                {
                    TraceGridBuilderMouse("SDL_DOWN",
                        static_cast<unsigned long long>(
                            (SDL_GetTicksNS() - event.button.timestamp) / 1000000));
                    dosBoxMouse.setLeftButtonDown(true);
                    hostUi.onLeftMouseButtonDown(
                        event.button.x,
                        event.button.y,
                        dosBoxMouse,
                        dosBoxPipeClient
                    );
                }
                else if ((event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
                    event.button.button == SDL_BUTTON_LEFT) ||
                    event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
                {
                    dosBoxMouse.setLeftButtonDown(false);
                }

                ImGui_ImplSDL3_ProcessEvent(
                    &event
                );
            }

            gameModule.update();
            bool mapOpen = false;
            if (const auto key = gameModule.currentMapKey())
                mapOpen = gridBuilderGrid.openMap(
                    *key, gameModule.currentMapName().value_or("")
                );

            MapPlayerMarker marker;
            if (mapOpen)
            {
                if (const auto position = gameModule.currentPosition())
                {
                    marker.x = position->x;
                    marker.y = position->y;
                    marker.visible = true;
                    switch (position->direction)
                    {
                    case GameFacingDirection::North:
                        marker.direction = MapFacingDirection::North; break;
                    case GameFacingDirection::East:
                        marker.direction = MapFacingDirection::East; break;
                    case GameFacingDirection::South:
                        marker.direction = MapFacingDirection::South; break;
                    case GameFacingDirection::West:
                        marker.direction = MapFacingDirection::West; break;
                    }
                }
            }
            gridBuilderGrid.setPlayerMarker(marker);

            dosBoxFramePipeline.update();

            hostRenderer.beginFrame();

            imGuiHost.beginFrame();

            dosBoxKeyboard.update(
                dosBoxPipeClient,
                {}
            );

            static uint8_t memoryValue = 0;
            static bool memoryValueValid = false;

            if (ImGui::IsKeyPressed(
                ImGuiKey_Space,
                false
            ))
            {
                memoryValueValid =
                    dosBoxMemory.readByte(
                        dosBoxPipeClient,
                        0x30418,
                        memoryValue
                    );
            }

            if (ImGui::IsKeyPressed(
                ImGuiKey_DownArrow,
                false
            ))
            {
                const uint8_t newValue =
                    (memoryValue == 0)
                    ? 1
                    : 0;

                dosBoxMemory.writeValue(
                    dosBoxPipeClient,
                    0x30418,
                    newValue,
                    1
                );
            }

            hostUi.draw(
                frameTexture,
                dosBoxFramePipeline.contentWidth(),
                dosBoxFramePipeline.contentHeight(),
                dosBoxMouse,
                dosBoxPipeClient
            );

            dosBoxMouse.updatePendingClick(
                dosBoxPipeClient
            );

            bool gridOpen = true;

            gridBuilderGrid.draw(
                &gridOpen
            );

            debugWindow.draw();

            imGuiHost.endFrame();

            hostRenderer.present();
        }
    }
}
