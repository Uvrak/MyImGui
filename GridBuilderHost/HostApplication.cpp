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
#include "FrameReader.h"
#include "../MouseLatencyTrace.h"


#include <cstdio>
#include <Windows.h>
#include <cstdint>
#include <cmath>

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
        DebugWindow clickLogWindow("Emulierte Mausklicks");
        std::optional<GameButtonPoint> lastEmulatedClick;
        std::size_t emulatedClickCount = 0;
        bool mapWasAvailable = false;
        bool mapSaveFailed = false;
        std::optional<GameButtonPoint> pendingModeClick;
        float modeClickX = 0.0f;
        float modeClickY = 0.0f;
        bool modeClickDragged = false;

        const auto sendDosBoxClick = [&](GameButtonPoint click) {
            if (!gameModule.onDosBoxMouseClick(click))
                dosBoxMouse.click(dosBoxPipeClient, click.x, click.y,
                    static_cast<int>(dosBoxFramePipeline.contentWidth()),
                    static_cast<int>(dosBoxFramePipeline.contentHeight()));
        };

        gameModule.start();
        
        while (running)
        {
            SDL_Event event;
            // A menu-closing key belongs to the menu for this entire input frame.
            bool menuHandledKeyboard = !gameModule.inventoryMenuTitle().empty();

            while (SDL_PollEvent(
                &event
            ))
            {
                if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
                {
                    if (SDL_GetKeyboardFocus() == nullptr)
                        dosBoxKeyboard.recordBackgroundEvent(event.key);
                    int key = static_cast<int>(event.key.key);
                    switch (event.key.scancode)
                    {
                    case SDL_SCANCODE_LEFT: key = SDLK_LEFT; break;
                    case SDL_SCANCODE_RIGHT: key = SDLK_RIGHT; break;
                    case SDL_SCANCODE_UP: key = SDLK_UP; break;
                    case SDL_SCANCODE_DOWN: key = SDLK_DOWN; break;
                    default: break;
                    }
                    gameModule.keyDown(key);
                    menuHandledKeyboard = menuHandledKeyboard ||
                        !gameModule.inventoryMenuTitle().empty();
                }
                else if (event.type == SDL_EVENT_KEY_UP)
                {
                    if (SDL_GetKeyboardFocus() == nullptr)
                        dosBoxKeyboard.recordBackgroundEvent(event.key);
                    gameModule.keyUp(static_cast<int>(event.key.key));
                }

                if (event.type ==
                    SDL_EVENT_QUIT)
                {
                    if (gridBuilderGrid.saveCurrentMap())
                        running = false;
                    else
                        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                            "Map speichern",
                            "Die Map konnte nicht gespeichert werden. GridBuilder bleibt geoeffnet.",
                            nullptr);
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
                    pendingModeClick.reset();
                    if (const auto click = hostUi.onLeftMouseButtonDown(
                        event.button.x, event.button.y,
                        dosBoxMouse, dosBoxPipeClient))
                    {
                        dosBoxMouse.setLeftButtonDown(true);
                        if (gameModule.buttonEditingActive())
                        {
                            pendingModeClick = click;
                            modeClickX = event.button.x;
                            modeClickY = event.button.y;
                            modeClickDragged = false;
                        }
                        else sendDosBoxClick(*click);
                    }
                }
                else if (event.type == SDL_EVENT_MOUSE_MOTION && pendingModeClick)
                {
                    if (std::abs(event.motion.x - modeClickX) > 4.0f ||
                        std::abs(event.motion.y - modeClickY) > 4.0f)
                        modeClickDragged = true;
                }
                else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
                    event.button.button == SDL_BUTTON_LEFT)
                {
                    if (pendingModeClick && !modeClickDragged &&
                        std::abs(event.button.x - modeClickX) <= 4.0f &&
                        std::abs(event.button.y - modeClickY) <= 4.0f)
                        sendDosBoxClick(*pendingModeClick);
                    pendingModeClick.reset();
                    dosBoxMouse.setLeftButtonDown(false);
                }
                else if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
                {
                    pendingModeClick.reset();
                    dosBoxMouse.setLeftButtonDown(false);
                }

                ImGui_ImplSDL3_ProcessEvent(
                    &event
                );
            }

            if (!running) break;
            dosBoxFramePipeline.update();
            const auto* frameHeader = dosBoxFramePipeline.frameReader().header();
            gameModule.setFrame(dosBoxFramePipeline.frameReader().pixels(),
                frameHeader ? frameHeader->width : 0,
                frameHeader ? frameHeader->height : 0,
                frameHeader ? frameHeader->pitch : 0);
            gameModule.update();
            bool mapOpen = false;
            if (const auto key = gameModule.currentMapKey())
            {
                mapOpen = gridBuilderGrid.openMap(
                    *key, gameModule.currentMapName().value_or("")
                );
                mapWasAvailable = true;
                mapSaveFailed = false;
            }
            else if (mapWasAvailable)
            {
                mapSaveFailed = !gridBuilderGrid.saveCurrentMap();
                if (!mapSaveFailed) mapWasAvailable = false;
            }

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

            hostRenderer.beginFrame();

            imGuiHost.beginFrame();

            dosBoxKeyboard.update(
                dosBoxPipeClient,
                [&gameModule](ImGuiKey key, const char* command) -> std::string {
                    // Mode switches are handled by the game module.
                    if (key == ImGuiKey_End || key == ImGuiKey_Delete) return "";
                    if (gameModule.blockDirectDosBoxVerticalKeys() &&
                        (key == ImGuiKey_UpArrow || key == ImGuiKey_DownArrow)) return "";
                    if (gameModule.blockDirectDosBoxInventoryKeys() &&
                        (key == ImGuiKey_Enter ||
                         (key == ImGuiKey_Escape &&
                          !gameModule.inventoryMenuTitle().empty()))) return "";
                    return command;
                },
                !gameModule.blockDirectDosBoxKeyboard() && !menuHandledKeyboard,
                SDL_GetKeyboardFocus() == nullptr
            );

            if (!gameModule.buttonEditingActive())
                hostUi.setButtonSaveFailed(false);

            hostUi.draw(
                frameTexture,
                dosBoxFramePipeline.contentWidth(),
                dosBoxFramePipeline.contentHeight(),
                dosBoxMouse,
                dosBoxPipeClient,
                gameModule.buttonRects(),
                gameModule.selectedButtonRect(),
                gameModule.selectedInventoryRect(),
                gameModule.buttonEditingActive(),
                gameModule.buttonViewAvailable(),
                gameModule.buttonViewName()
            );

            if (const auto drawn = hostUi.takeDrawnButton())
                hostUi.setButtonSaveFailed(!gameModule.addButton(*drawn));

            if (const auto deleted = hostUi.takeDeletedButton())
                hostUi.setButtonSaveFailed(!gameModule.deleteButton(*deleted));

            if (const auto modified = hostUi.takeModifiedButton())
                hostUi.setButtonSaveFailed(!gameModule.modifyButton(*modified));

            // Let an earlier click finish, including physical button release,
            // before the module observes its result and advances a click sequence.
            if (!dosBoxMouse.clickPending())
            {
                if (const auto click = gameModule.takeButtonClick())
                    dosBoxMouse.click(dosBoxPipeClient, click->x, click->y,
                        static_cast<int>(dosBoxFramePipeline.contentWidth()),
                        static_cast<int>(dosBoxFramePipeline.contentHeight()));
            }

            dosBoxMouse.updatePendingClick(
                dosBoxPipeClient
            );
            for (const auto& click : dosBoxMouse.takeEmulatedClicks())
            {
                lastEmulatedClick = GameButtonPoint{click.x, click.y};
                clickLogWindow.addLine("#" + std::to_string(++emulatedClickCount) +
                    "  (" + std::to_string(click.x) + ", " +
                    std::to_string(click.y) + ")");
            }

            bool gridOpen = true;

            gridBuilderGrid.draw(
                &gridOpen
            );

            std::string viewNames;
            for (const auto& name : gameModule.activeViewNames())
            {
                if (!viewNames.empty()) viewNames += ", ";
                viewNames += name;
            }
            debugWindow.setLine("Aktive Views: " +
                (viewNames.empty() ? std::string("Unknown") : viewNames));
            debugWindow.addLine("Steuerung: " + gameModule.buttonViewName());
            if (mapSaveFailed)
                debugWindow.addLine("Map konnte nicht gespeichert werden; erneuter Versuch folgt.");
            debugWindow.addLine("Buttons in Steuerungs-View: " +
                std::to_string(gameModule.buttonCount()));
            if (const auto inventory = gameModule.inventoryDebugLine())
                debugWindow.addLine(*inventory);
            if (const auto itemClick = gameModule.inventoryClickDebugLine())
                debugWindow.addLine(*itemClick);
            if (lastEmulatedClick)
                debugWindow.addLine("Click an Position (" +
                    std::to_string(lastEmulatedClick->x) + ", " +
                    std::to_string(lastEmulatedClick->y) + ")");
            debugWindow.draw();
            clickLogWindow.draw();

            const auto menuEntries = gameModule.inventoryMenuEntries();
            if (!gameModule.inventoryMenuTitle().empty())
            {
                if (const auto position = gameModule.inventoryMenuPosition())
                    if (const auto screen = hostUi.dosBoxScreenPosition(*position))
                        ImGui::SetNextWindowPos(ImVec2(static_cast<float>(screen->x),
                            static_cast<float>(screen->y)), ImGuiCond_Always);
                ImGui::Begin(gameModule.inventoryMenuTitle().c_str(), nullptr,
                    ImGuiWindowFlags_AlwaysAutoResize);
                for (int i = 0; i < static_cast<int>(menuEntries.size()); ++i)
                    ImGui::Text("%s %s", i == gameModule.inventoryMenuSelection() ? ">" : " ",
                        menuEntries[i].c_str());
                ImGui::End();
            }

            imGuiHost.endFrame();

            hostRenderer.present();
        }
    }
}
