#include "EditorApplication.h"

#include <iostream>
#include <stdexcept>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "imgui_internal.h"
#include <algorithm>
#include <filesystem>

#include "IconsFontAwesome7.h"

#include <SDL3/SDL_dialog.h>
#include "ImageFileLoader.h"
#include "SvgFileWriter.h"
#include <cfloat>
#include <fstream>
#include <cstdio>

#include <SDL3/SDL.h>



namespace
{
    struct SaveMapRequest
    {
        EditorApplication* application;
        std::string filename;
    };

    struct OpenMapRequest
    {
        EditorApplication* application;
        std::string filename;
    };
}


EditorApplication::EditorApplication()
{

    if (!initializeSDL())
    {
        throw std::runtime_error(
            "Failed to initialize SDL."
        );
    }

    if (!createWindow())
    {
        SDL_Quit();

        throw std::runtime_error(
            "Failed to create SDL window."
        );
    }
    if (!createRenderer())
    {
        SDL_DestroyWindow(m_window);
        SDL_Quit();

        throw std::runtime_error(
            "Failed to create SDL renderer."
        );
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiStyle& style =
        ImGui::GetStyle();

    style.TabCloseButtonMinWidthSelected =
        FLT_MAX;

    style.TabCloseButtonMinWidthUnselected =
        FLT_MAX;

    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    constexpr float fontSize = 16.0f;

    ImFontConfig defaultFontConfig;
    defaultFontConfig.SizePixels = fontSize;

    io.Fonts->AddFontDefault(
        &defaultFontConfig
    );

    ImFontConfig iconFontConfig;
    iconFontConfig.MergeMode = true;
    iconFontConfig.PixelSnapH = true;

    static const ImWchar iconRanges[] =
    {
        ICON_MIN_FA,
        ICON_MAX_FA,
        0
    };

    ImFont* iconFont = io.Fonts->AddFontFromFileTTF(
        "resources/Fonts/Font Awesome 7 Free-Solid-900.otf",
        fontSize,
        &iconFontConfig,
        iconRanges
    );

    if (!iconFont)
    {
        throw std::runtime_error(
            "Failed to load Font Awesome font."
        );
    }
    ImGui_ImplSDL3_InitForSDLRenderer(
        m_window,
        m_renderer
    );

    ImGui_ImplSDLRenderer3_Init(
        m_renderer
    );

    m_editorToolbox = std::make_unique<EditorToolbox>(m_renderer);

    m_editorEdgeBox =
        std::make_unique<EditorEdgeBox>(
            m_renderer
        );

    m_editorMiscBox =
        std::make_unique<EditorMiscBox>(
            m_renderer
        );

    m_editorMiscBox->clearActiveMisc();

    dosBoxWindow =
        std::make_unique<DosBoxWindow>(
            m_window,
            m_renderer
        );

    m_memoryTools =
        std::make_unique<
        DosBoxMemoryTools::MemoryTools
        >(
            "MM3.EXE",
            nullptr
        );

    m_memoryTools->setGameId(
        "MM3.EXE"
    );

    loadRecentMaps();
    refreshMapFiles();
    loadLastMap();
    refreshEdgeFiles();
    loadLastEdge();
    loadWindowState();
    loadEditorSettings();
    m_dosBoxKeyBindings.load(
        dosBoxKeyBindingsFilename()
    );
    loadRecentGames();
}

bool EditorApplication::createWindow()
{
    m_window = SDL_CreateWindow(
        "GridBuilder Editor",
        1280,
        720,
        SDL_WINDOW_RESIZABLE
    );

    if (!m_window)
    {
        std::cerr
            << "SDL_CreateWindow failed: "
            << SDL_GetError()
            << std::endl;

        return false;
    }

    return true;
}

bool EditorApplication::createRenderer()
{
    m_renderer = SDL_CreateRenderer(
        m_window,
        nullptr
    );

    if (!m_renderer)
    {
        std::cerr
            << "SDL_CreateRenderer failed: "
            << SDL_GetError()
            << std::endl;

        return false;
    }

    return true;
}

void EditorApplication::processEvents()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL3_ProcessEvent(
            &event
        );

        GameModule* gameModule =
            m_gameModuleManager.active();

        if (gameModule)
        {
            if (event.type ==
                SDL_EVENT_KEY_DOWN)
            {
                gameModule->keyDown(
                    event.key.key
                );
            }
            else if (event.type ==
                SDL_EVENT_KEY_UP)
            {
                gameModule->keyUp(
                    event.key.key
                );
            }
        }

        if (event.type ==
            SDL_EVENT_QUIT)
        {
            requestExit();
        }
    }
}

void EditorApplication::run()
{
    while (m_running)
    {
        processEvents();
        render();
        ImGui::GetIO().FontGlobalScale =
            m_fontScale;
    }
}

void EditorApplication::render()
{
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    if (m_reloadEditorEdgeBox)
    {
        m_editorEdgeBox =
            std::make_unique<EditorEdgeBox>(
                m_renderer
            );

        m_reloadEditorEdgeBox = false;
        m_refreshEdgeTextures = false;
    }
    if (m_refreshEdgeTextures)
    {
        m_editorEdgeBox->refreshTextures();
        m_refreshEdgeTextures = false;
    }
    drawMainMenu();

    ImGui::DockSpaceOverViewport();

    m_editorToolbox->draw(
        m_editorToolbox->activeTool()
    );

    if (!dosBoxWindow->inputActive())
    {
        ImGui::SetMouseCursor(
            ImGuiMouseCursor_None
        );
        const ImVec2 mousePosition =
            ImGui::GetMousePos();

        ImDrawList* foregroundDrawList =
            ImGui::GetForegroundDrawList();

        foregroundDrawList->AddText(
            ImVec2(
                mousePosition.x +
                (
                    m_editorToolbox->activeTool() ==
                    EditorTool::Pencil &&
                    m_worldViewWindow.isMouseOverCanvas()
                    ? 6.0f
                    : 0.0f
                    ),
                mousePosition.y -
                (
                    m_editorToolbox->activeTool() ==
                    EditorTool::Pencil
                    ? (
                        m_worldViewWindow.isMouseOverCanvas()
                        ? 24.0f
                        : 12.0f
                        )
                    : 0.0f
                    )
            ),
            IM_COL32(255, 220, 80, 255),
            m_editorToolbox->activeToolIcon()
        );
    }
    m_propertiesWindow.draw();

    if (m_editorEdgeBox->draw(
        m_worldViewWindow.colorPalette(),
        [this](
            const std::string& edgeId
            )
        {
            return m_worldViewWindow.edgeColorId(
                edgeId
            );
        },
        [this](
            const std::string& edgeId,
            const std::string& colorId
            )
        {
            m_worldViewWindow.setEdgeColorId(
                edgeId,
                colorId
            );
        },
        [this](
            const std::string& colorId
            )
        {
            return m_worldViewWindow.removeMapColor(
                colorId
            );
        }
    ))
    {
        m_editorMiscBox->clearActiveMisc();

        m_mapPaintTarget =
            MapPaintTarget::Edge;

        m_currentPixelImageType =
            PixelImageType::Edge;

        m_editorToolbox->setActiveTool(
            EditorTool::Pencil
        );

        const std::string& edgeId =
            m_editorEdgeBox->activeEdgeId();


        if (!edgeId.empty())
        {
            const std::string filename =
                (
                    std::filesystem::path(
                        "resources/icons/edges"
                    ) /
                    (edgeId + ".svg")
                    ).string();

            PixelImage loadedImage{
                1,
                1
            };

            const bool loaded =
                ImageFileLoader::load(
                    filename,
                    loadedImage
                );

            if (loaded &&
                m_pixelCanvasWindow.setImage(
                    loadedImage
                ))
            {
                m_currentPixelFilename =
                    filename;
            }
            else
            {
                SDL_Log(
                    "Failed to synchronize pixel view: %s",
                    filename.c_str()
                );
            }

        }

    }

    dosBoxWindow->setCustomSwitchViewKey(
        m_dosBoxKeyBindings.binding(
            DosBoxAction::SwitchView
        ).customKey
    );

    dosBoxWindow->setKeyBindings(
        m_dosBoxKeyBindings
    );

    GameModule* activeGameModule =
        m_gameModuleManager.active();

    dosBoxWindow->setDirectKeyboardBlocked(
        activeGameModule&&
        activeGameModule->
        blockDirectDosBoxKeyboard()
    );

    int selectedGameButton =
        -1;

    const bool gameButtonSelectionActive =
        activeGameModule &&
        activeGameModule->gameButtonSelection(
            selectedGameButton
        );

    dosBoxWindow->setGameButtonSelection(
        gameButtonSelectionActive,
        selectedGameButton
    );

    dosBoxWindow->draw();

    if (m_memoryTools)
    {
        m_memoryTools->refreshMemory();
        m_memoryTools->draw();
    }

    if (m_memoryTools)
    {
        m_memoryTools->refreshMemory();
    }

    GameModule* gameModule =
        m_gameModuleManager.active();

    if (gameModule)
    {
        gameModule->update();

        std::string dosKey;

        while (gameModule->takeDosKey(
            dosKey
        ))
        {
            dosBoxWindow->sendDosKey(
                dosKey.c_str()
            );
        }

        MapPlayerMarker playerMarker;

        if (gameModule->playerMarker(
            playerMarker
        ))
        {
            m_worldViewWindow.setPlayerMarker(
                playerMarker
            );
        }
    }

    if (m_showItemExplorer)
    {
        m_itemExplorerWindow.draw(
            &m_showItemExplorer
        );
    }

    if (m_editorMiscBox->draw(
        m_worldViewWindow.colorPalette(),
        [this](
            const std::string& miscId
            )
        {
            return m_worldViewWindow.miscColorId(
                miscId
            );
        },
        [this](
            const std::string& miscId,
            const std::string& colorId
            )
        {
            m_worldViewWindow.setMiscColorId(
                miscId,
                colorId
            );
        },
        [this](
            const std::string& colorId
            )
        {
            return m_worldViewWindow.removeMapColor(
                colorId
            );
        }
    ))
    {
        m_editorEdgeBox->clearActiveEdge();

        m_mapPaintTarget =
            MapPaintTarget::Misc;

        m_currentPixelImageType =
            PixelImageType::Misc;
        m_editorToolbox->setActiveTool(
            EditorTool::Pencil
        );

        const std::string& miscId =
            m_editorMiscBox->activeMiscId();

        if (!miscId.empty())
        {
            const std::string filename =
                (
                    std::filesystem::path(
                        "resources/icons/misc"
                    ) /
                    (miscId + ".svg")
                    ).string();

            PixelImage loadedImage{
                1,
                1
            };

            const bool loaded =
                ImageFileLoader::load(
                    filename,
                    loadedImage
                );

            if (loaded &&
                m_pixelCanvasWindow.setImage(
                    loadedImage
                ))
            {
                m_currentPixelFilename =
                    filename;
            }
            else
            {
                SDL_Log(
                    "Failed to synchronize pixel view: %s",
                    filename.c_str()
                );
            }
        }

    }

    if (m_showMapEditor)
    {
        m_worldViewWindow.draw(
            m_editorToolbox->activeTool(),
            m_editorEdgeBox->activeEdgeId(),
            m_worldViewWindow.edgeColorId(
                m_editorEdgeBox->activeEdgeId()
            ),
            m_editorMiscBox->activeMiscId(),
            m_worldViewWindow.miscColorId(
                m_editorMiscBox->activeMiscId()
            ),
            m_mapPaintTarget ==
            MapPaintTarget::Misc,
            [this](
                const std::string& edgeId,
                int size
                ) -> SDL_Texture*
            {
                return m_editorEdgeBox->
                    edgeTexture(
                        edgeId,
                        size
                    );
            },

            [this](
                const std::string& miscId,
                int size
                ) -> SDL_Texture*
            {
                return m_editorMiscBox->
                    miscTexture(
                        miscId,
                        size
                    );
            },

            [this](
                const std::string& edgeId,
                const std::string& assignedColorId,
                const AssignEdgeColorCallback&
                assignColor
                )
            {
                m_editorEdgeBox->
                    openColorMenu(
                        edgeId,
                        assignedColorId,
                        assignColor
                    );
            },
            [this](
                const std::string& miscId,
                const std::string& assignedColorId,
                const AssignEdgeColorCallback&
                assignColor
                )
            {
                m_editorMiscBox->
                    openColorMenu(
                        miscId,
                        assignedColorId,
                        assignColor
                    );
            },
            [this]()
            {
                drawMapViewToolbar();
            },
            m_editorEdgeBox->isColorMenuOpen(),
            m_showMapCoordinates,
            &m_showMapEditor
        );
    }

    switch (m_editorToolbox->activeTool())
    {
    case EditorTool::Pencil:
        m_activePixelTool =
            PixelTool::Pencil;
        break;

    case EditorTool::Eraser:
        m_activePixelTool =
            PixelTool::Eraser;
        break;

    case EditorTool::Scroll:
        m_activePixelTool =
            PixelTool::Pan;
        break;
    }

    if (m_showPixelEditor)
    {
        m_pixelCanvasWindow.draw(
            m_activePixelTool,
            nullptr,
            [this]()
            {
                drawPixelViewToolbar();
            },
            &m_showPixelEditor
        );
    }

    if (m_focusMapEditor)
    {
        ImGui::SetWindowFocus(
            "Map Editor"
        );

        m_focusMapEditor = false;
    }

    ImGuiIO& io =
        ImGui::GetIO();

    if (m_pixelCanvasWindow.isMouseOverCanvas() &&
        !io.KeyCtrl &&
        io.MouseWheel != 0.0f)
    {
        const EditorTool currentTool =
            m_editorToolbox->activeTool();

        EditorTool nextTool =
            currentTool;

        if (io.MouseWheel > 0.0f)
        {
            switch (currentTool)
            {
            case EditorTool::Pencil:
                nextTool = EditorTool::Eraser;
                break;

            case EditorTool::Eraser:
                nextTool = EditorTool::Scroll;
                break;

            case EditorTool::Scroll:
                nextTool = EditorTool::Pencil;
                break;
            }
        }
        else
        {
            switch (currentTool)
            {
            case EditorTool::Pencil:
                nextTool = EditorTool::Scroll;
                break;

            case EditorTool::Eraser:
                nextTool = EditorTool::Pencil;
                break;

            case EditorTool::Scroll:
                nextTool = EditorTool::Eraser;
                break;
            }
        }

        m_editorToolbox->setActiveTool(
            nextTool
        );
    }
    else if (m_editorToolbox->activeTool() ==
        EditorTool::Pencil &&
        m_worldViewWindow.isMouseOverCanvas() &&
        !io.KeyCtrl)
    {
        if (io.MouseWheel > 0.0f)
        {
            m_editorEdgeBox->cycleActiveEdge(1);
        }
        else if (io.MouseWheel < 0.0f)
        {
            m_editorEdgeBox->cycleActiveEdge(-1);
        }
    }
    drawNewMapPopup();
    drawUnsavedChangesPopup();

    ImGui::Render();

    SDL_SetRenderDrawColor(
        m_renderer,
        30,
        30,
        30,
        255
    );

    SDL_RenderClear(m_renderer);

    ImGui_ImplSDLRenderer3_RenderDrawData(
        ImGui::GetDrawData(),
        m_renderer
    );

    SDL_RenderPresent(m_renderer);


}

void EditorApplication::drawMainMenu()
{
    if (!ImGui::BeginMainMenuBar())
    {
        return;
    }

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem(
            "New Map",
            "Ctrl+N"
        ))
        {
            createNewMap();
        }

        ImGui::Separator();

        ImGui::MenuItem(
            "Save Map",
            "Ctrl+S",
            false,
            false
        );

        ImGui::MenuItem(
            "Load Map",
            "Ctrl+O",
            false,
            false
        );

        ImGui::Separator();

        if (ImGui::MenuItem("Exit"))
        {
            requestExit();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
        ImGui::MenuItem(
            "Undo",
            "Ctrl+Z",
            false,
            false
        );

        ImGui::MenuItem(
            "Redo",
            "Ctrl+Y",
            false,
            false
        );

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
        ImGui::MenuItem(
            "Toolbox",
            nullptr,
            true,
            false
        );

        ImGui::MenuItem(
            "Properties",
            nullptr,
            true,
            false
        );

        if (ImGui::MenuItem(
            "Map Editor",
            nullptr,
            &m_showMapEditor
        ))
        {
            if (m_showMapEditor)
            {
                m_focusMapEditor =
                    true;
            }
        }

        ImGui::MenuItem(
            "Map Coordinates",
            nullptr,
            &m_showMapCoordinates
        );

        if (ImGui::MenuItem(
            "Pixel Editor",
            nullptr,
            &m_showPixelEditor
        ))
        {
            if (m_showPixelEditor)
            {
                m_focusPixelEditor =
                    true;
            }
        }

        ImGui::Separator();

        ImGui::TextUnformatted(
            "Font Scale"
        );

        ImGui::SetNextItemWidth(
            120.0f
        );

        ImGui::SliderFloat(
            "##FontScale",
            &m_fontScale,
            0.75f,
            2.0f,
            "%.2f"
        );

        ImGui::Separator();

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Game"))
    {
        const auto& gameModules =
            m_gameModuleManager.modules();

        if (!gameModules.empty())
        {
            ImGui::TextUnformatted(
                "Active Module"
            );

            for (size_t index = 0;
                index < gameModules.size();
                ++index)
            {
                const bool selected =
                    m_gameModuleManager.active() ==
                    gameModules[index].module;

                if (ImGui::MenuItem(
                    gameModules[index].name.c_str(),
                    nullptr,
                    selected
                ))
                {
                    m_gameModuleManager.setActive(
                        index
                    );
                }
            }

            ImGui::Separator();
        }

        if (ImGui::MenuItem(
            "Open Game..."
        ))
        {
            static const SDL_DialogFileFilter filters[] =
            {
                {
                    "DOS Executables",
                    "exe"
                }
            };

            SDL_ShowOpenFileDialog(
                [](void* userdata,
                    const char* const* filelist,
                    int)
                {
                    if (filelist == nullptr ||
                        filelist[0] == nullptr)
                    {
                        return;
                    }

                    auto* application =
                        static_cast<EditorApplication*>(
                            userdata
                            );

                    application->openDosGame(
                        filelist[0]
                    );
                },
                this,
                m_window,
                filters,
                1,
                nullptr,
                false
            );
        }
        if (!m_recentGames.empty())
        {
            ImGui::Separator();

            std::string gameToRemove;

            for (const std::string& game :
                m_recentGames)
            {
                const std::filesystem::path gamePath(
                    game
                );

                const std::string displayName =
                    gamePath.filename().string();

                ImGui::PushID(
                    game.c_str()
                );

                if (ImGui::MenuItem(
                    displayName.c_str()
                ))
                {
                    openDosGame(
                        game
                    );
                }

                const bool gameItemHovered =
                    ImGui::IsItemHovered();

                if (gameItemHovered &&
                    ImGui::IsMouseClicked(
                        ImGuiMouseButton_Right
                    ))
                {
                    ImGui::OpenPopup(
                        "##RecentGameContext"
                    );
                }

                if (ImGui::BeginPopup(
                    "##RecentGameContext"
                ))
                {
                    if (ImGui::MenuItem(
                        "Remove from list"
                    ))
                    {
                        gameToRemove =
                            game;
                    }

                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }

            if (!gameToRemove.empty())
            {
                const auto position =
                    std::find(
                        m_recentGames.begin(),
                        m_recentGames.end(),
                        gameToRemove
                    );

                if (position !=
                    m_recentGames.end())
                {
                    m_recentGames.erase(
                        position
                    );

                    saveRecentGames();
                }
            }
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("DOSBox"))
    {
        if (ImGui::MenuItem(
            "Key Bindings..."
        ))
        {
            m_showKeyBindingsWindow =
                true;
        }

        ImGui::Separator();

        if (ImGui::BeginMenu(
            "Keyboard Layout"
        ))
        {
            if (ImGui::MenuItem(
                "German"
            ))
            {
                if (dosBoxWindow)
                {
                    dosBoxWindow->
                        setGermanKeyboardLayout();
                }
            }

            if (ImGui::MenuItem(
                "US"
            ))
            {
                if (dosBoxWindow)
                {
                    dosBoxWindow->
                        setUSKeyboardLayout();
                }
            }

            ImGui::EndMenu();
        }

        ImGui::Separator();

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();

    drawKeyBindingsWindow();
}

bool EditorApplication::initializeSDL()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr
            << "SDL_Init failed: "
            << SDL_GetError()
            << std::endl;

        return false;
    }

    return true;
}

void EditorApplication::refreshMapFiles()
{
    m_mapFiles.clear();

    const std::filesystem::path mapDirectory =
        "resources/maps";

    std::error_code error;

    if (!std::filesystem::exists(
        mapDirectory,
        error
    ))
    {
        SDL_Log(
            "Map directory not found: %s",
            mapDirectory.string().c_str()
        );

        return;
    }

    for (const auto& entry :
        std::filesystem::directory_iterator(
            mapDirectory,
            error
        ))
    {
        if (error)
        {
            break;
        }

        if (!entry.is_regular_file())
        {
            continue;
        }

        const std::string extension =
            entry.path().extension().string();

        if (extension != ".map" &&
            extension != ".MAP")
        {
            continue;
        }

        m_mapFiles.push_back(
            entry.path().string()
        );
    }

    std::sort(
        m_mapFiles.begin(),
        m_mapFiles.end()
    );
}

void EditorApplication::openDosGame(
    const std::string& filename
)
{
    if (filename.empty())
    {
        return;
    }

    auto existingGame =
        std::find(
            m_recentGames.begin(),
            m_recentGames.end(),
            filename
        );

    if (existingGame == m_recentGames.end())
    {
        m_recentGames.push_back(
            filename
        );

        saveRecentGames();
    }

    const std::filesystem::path gamePath(
        filename
    );

    const std::filesystem::path exeDirectory =
        gamePath.parent_path();

    std::filesystem::path detectedMount =
        exeDirectory;

    const std::filesystem::path parentDirectory =
        exeDirectory.parent_path();

    if (!parentDirectory.empty())
    {
        bool foundDosBoxSetup = false;

        for (const auto& entry :
            std::filesystem::directory_iterator(
                parentDirectory
            ))
        {
            if (entry.is_regular_file() &&
                entry.path().extension() == ".conf")
            {
                foundDosBoxSetup = true;
                break;
            }

            if (entry.is_directory())
            {
                const std::string directoryName =
                    entry.path()
                    .filename()
                    .string();

                if (directoryName.rfind(
                    "dosbox_",
                    0
                ) == 0)
                {
                    foundDosBoxSetup = true;
                    break;
                }
            }
        }

        if (foundDosBoxSetup)
        {
            detectedMount =
                parentDirectory;
        }
    }

    std::filesystem::path relativeDirectory =
        std::filesystem::relative(
            exeDirectory,
            detectedMount
        );

    std::string dosDirectory;

    if (relativeDirectory != ".")
    {
        dosDirectory =
            relativeDirectory.string();
    }

    const std::string gameFilename =
        gamePath.filename().string();

    SDL_Log(
        "DOS mount: %s",
        detectedMount.string().c_str()
    );

    SDL_Log(
        "DOS directory: %s",
        dosDirectory.c_str()
    );

    SDL_Log(
        "DOS executable: %s",
        gameFilename.c_str()
    );

    if (dosBoxWindow)
    {
        dosBoxWindow->openGame(
            detectedMount.string(),
            dosDirectory,
            gameFilename
        );
    }
}

std::string EditorApplication::editorSettingsFilename() const
{
    const std::string recentFilename =
        recentMapsFilename();

    if (recentFilename.empty())
    {
        return {};
    }

    const std::size_t separatorPosition =
        recentFilename.find_last_of(
            "\\/"
        );

    if (separatorPosition ==
        std::string::npos)
    {
        return "editor-settings.txt";
    }

    return recentFilename.substr(
        0,
        separatorPosition + 1
    ) + "editor-settings.txt";
}

void EditorApplication::loadEditorSettings()
{
    const std::string filename =
        editorSettingsFilename();

    if (filename.empty())
    {
        return;
    }

    std::ifstream file(filename);

    if (!file.is_open())
    {
        return;
    }

    std::string setting;

    while (file >> setting)
    {
        if (setting ==
            "showLowerLayer")
        {
            int value = 0;

            file >> value;

            m_worldViewWindow.setShowLowerLayer(
                value != 0
            );
        }
        else if (setting ==
            "fontScale")
        {
            file >>
                m_fontScale;
        }
    }
}

void EditorApplication::saveEditorSettings() const
{
    const std::string filename =
        editorSettingsFilename();

    if (filename.empty())
    {
        return;
    }

    std::ofstream file(filename);

    if (!file.is_open())
    {
        return;
    }

    file
        << "showLowerLayer "
        << (
            m_worldViewWindow.showLowerLayer()
            ? 1
            : 0
            )
        << '\n';

    file
        << "fontScale "
        << m_fontScale
        << '\n';
}

void EditorApplication::requestExit()
{
    if (!m_worldViewWindow.hasUnsavedChanges())
    {
        m_running = false;
        return;
    }

    m_pendingMapAction =
        PendingMapAction::Exit;

    m_requestUnsavedChangesPopup = true;
}

void EditorApplication::drawUnsavedChangesPopup()
{
    if (m_requestUnsavedChangesPopup)
    {
        ImGui::OpenPopup(
            "Unsaved Changes"
        );

        m_requestUnsavedChangesPopup = false;
    }

    if (!ImGui::BeginPopupModal(
        "Unsaved Changes",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize
    ))
    {
        return;
    }

    if (!m_worldViewWindow.hasUnsavedChanges())
    {
        ImGui::CloseCurrentPopup();
        continuePendingMapAction();
        ImGui::EndPopup();
        return;
    }

    ImGui::TextUnformatted(
        "The map has unsaved changes."
    );

    ImGui::TextUnformatted(
        "Do you want to save them?"
    );

    ImGui::Separator();

    if (ImGui::Button(
        "Save",
        ImVec2(100.0f, 0.0f)
    ))
    {
        if (m_currentMapFilename.empty())
        {
            showSaveFileDialog();
        }
        else if (m_worldViewWindow.saveMap(
            m_currentMapFilename
        ))
        {
            ImGui::CloseCurrentPopup();
            continuePendingMapAction();
        }
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Don't Save",
        ImVec2(100.0f, 0.0f)
    ))
    {
        ImGui::CloseCurrentPopup();
        continuePendingMapAction();
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Cancel",
        ImVec2(100.0f, 0.0f)
    ))
    {
        m_pendingMapAction =
            PendingMapAction::None;

        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void EditorApplication::continuePendingMapAction()
{
    const PendingMapAction action =
        m_pendingMapAction;

    m_pendingMapAction =
        PendingMapAction::None;

    switch (action)
    {
    case PendingMapAction::Exit:
        m_running = false;
        break;

    case PendingMapAction::NewMap:
        m_newMapName[0] = '\0';
        m_requestNewMapPopup = true;
        break;

    case PendingMapAction::OpenMap:
    {
        const std::string filename =
            m_pendingMapFilename;

        m_pendingMapFilename.clear();

        openMap(
            filename
        );
        break;
    }

    case PendingMapAction::None:
        break;
    }
}

void EditorApplication::requestOpenMap(
    const std::string& filename
)
{
    if (filename.empty())
    {
        return;
    }

    if (!m_worldViewWindow.hasUnsavedChanges())
    {
        openMap(
            filename
        );
        return;
    }

    m_pendingMapFilename =
        filename;

    m_pendingMapAction =
        PendingMapAction::OpenMap;

    m_requestUnsavedChangesPopup = true;
}

void EditorApplication::openMap(
    const std::string& filename
)
{
    m_worldViewWindow.blockMapInputOnce();

    if (!m_worldViewWindow.loadMap(
        filename
    ))
    {
        SDL_Log(
            "Failed to load map: %s",
            filename.c_str()
        );

        return;
    }

    m_currentMapFilename =
        filename;

    addRecentMap(
        filename
    );
}

std::string EditorApplication::recentGamesFilename() const
{
    char* preferencePath =
        SDL_GetPrefPath(
            "Daniel Horn",
            "GridBuilder Editor"
        );

    if (preferencePath == nullptr)
    {
        return {};
    }

    const std::string filename =
        std::string(preferencePath) +
        "recent-games.txt";

    SDL_free(
        preferencePath
    );

    return filename;
}

void EditorApplication::loadRecentGames()
{
    m_recentGames.clear();

    const std::string filename =
        recentGamesFilename();

    if (filename.empty())
    {
        return;
    }

    std::ifstream file(
        filename
    );

    if (!file.is_open())
    {
        return;
    }

    std::string game;

    while (std::getline(
        file,
        game
    ))
    {
        if (!game.empty())
        {
            m_recentGames.push_back(
                game
            );
        }
    }
}

void EditorApplication::saveRecentGames() const
{
    const std::string filename =
        recentGamesFilename();

    if (filename.empty())
    {
        return;
    }

    std::ofstream file(
        filename,
        std::ios::trunc
    );

    if (!file.is_open())
    {
        return;
    }

    for (const std::string& game :
        m_recentGames)
    {
        file
            << game
            << '\n';
    }
}

void EditorApplication::refreshEdgeFiles()
{
    m_edgeFiles.clear();

    const std::filesystem::path edgeDirectory =
        "resources/icons/edges";

    std::error_code error;

    if (!std::filesystem::exists(
        edgeDirectory,
        error
    ))
    {
        SDL_Log(
            "Edge directory not found: %s",
            edgeDirectory.string().c_str()
        );

        return;
    }

    for (const std::filesystem::directory_entry& entry :
        std::filesystem::directory_iterator(
            edgeDirectory,
            error
        ))
    {
        if (error)
        {
            break;
        }

        if (!entry.is_regular_file())
        {
            continue;
        }

        const std::string extension =
            entry.path().extension().string();

        if (extension != ".svg" &&
            extension != ".SVG")
        {
            continue;
        }

        m_edgeFiles.push_back(
            entry.path().string()
        );
    }

    std::sort(
        m_edgeFiles.begin(),
        m_edgeFiles.end()
    );
}
std::string EditorApplication::lastEdgeFilename() const
{
    char* preferencePath =
        SDL_GetPrefPath(
            "Daniel Horn",
            "GridBuilder Editor"
        );

    if (preferencePath == nullptr)
    {
        return {};
    }

    const std::string filename =
        std::string(preferencePath) +
        "last-edge.txt";

    SDL_free(preferencePath);

    return filename;
}

void EditorApplication::saveLastEdge() const
{
    const std::string settingsFilename =
        lastEdgeFilename();

    if (settingsFilename.empty())
    {
        return;
    }

    std::ofstream file(
        settingsFilename,
        std::ios::trunc
    );

    if (!m_currentPixelFilename.empty())
    {
        file <<
            m_currentPixelFilename;
    }
}

void EditorApplication::loadLastEdge()
{
    const std::string settingsFilename =
        lastEdgeFilename();

    if (settingsFilename.empty())
    {
        return;
    }

    std::ifstream file(
        settingsFilename
    );

    std::string edgeFilename;

    std::getline(
        file,
        edgeFilename
    );

    if (edgeFilename.empty() ||
        !std::filesystem::exists(
            edgeFilename
        ))
    {
        return;
    }

    PixelImage loadedImage{
        1,
        1
    };

    if (!ImageFileLoader::load(
        edgeFilename,
        loadedImage
    ) ||
        !m_pixelCanvasWindow.setImage(
            loadedImage
        ))
    {
        SDL_Log(
            "Failed to restore edge: %s",
            edgeFilename.c_str()
        );

        return;
    }

    m_currentPixelFilename =
        edgeFilename;

    const std::string edgeId =
        std::filesystem::path(
            edgeFilename
        ).stem().string();

    m_editorEdgeBox->setActiveEdgeId(
        edgeId
    );

    SDL_Log(
        "Edge restored: %s",
        edgeFilename.c_str()
    );
}



void EditorApplication::drawPixelViewToolbar()
{
    RecentFileRequest recentRequest;

    std::vector<std::string> pixelImageFiles =
        m_edgeFiles;

    const std::filesystem::path miscDirectory =
        "resources/icons/misc";

    std::error_code error;

    if (std::filesystem::exists(
        miscDirectory,
        error
    ))
    {
        for (const std::filesystem::directory_entry& entry :
            std::filesystem::directory_iterator(
                miscDirectory,
                error
            ))
        {
            if (error)
            {
                break;
            }

            if (!entry.is_regular_file())
            {
                continue;
            }

            const std::string extension =
                entry.path().extension().string();

            if (extension != ".svg" &&
                extension != ".SVG")
            {
                continue;
            }

            pixelImageFiles.push_back(
                entry.path().string()
            );
        }
    }

    std::sort(
        pixelImageFiles.begin(),
        pixelImageFiles.end()
    );

    const EditorCommand command =
        m_pixelMainToolbar.draw(
            pixelImageFiles,
            recentRequest,
            m_currentPixelFilename
        );

    ImGui::SameLine();

    ImGui::TextDisabled(
        "| Type: %s",
        m_currentPixelImageType ==
        PixelImageType::Edge
        ? "Edge"
        : "Misc"
    );


    if (recentRequest.action ==
        RecentFileAction::Open)
    {
        PixelImage loadedImage{
            1,
            1
        };

        if (ImageFileLoader::load(
            recentRequest.filename,
            loadedImage
        ) &&
            m_pixelCanvasWindow.setImage(
                loadedImage
            ))
        {
            m_currentPixelFilename =
                recentRequest.filename;

            const std::filesystem::path imagePath =
                recentRequest.filename;

            const std::string imageId =
                imagePath.stem().string();

            const bool isMisc =
                imagePath.parent_path().filename() ==
                "misc";

            if (isMisc)
            {
                m_currentPixelImageType =
                    PixelImageType::Misc;

                m_editorEdgeBox->clearActiveEdge();

                m_editorMiscBox->setActiveMiscId(
                    imageId
                );

            }
            else
            {
                m_currentPixelImageType =
                    PixelImageType::Edge;

                m_editorMiscBox->clearActiveMisc();

                m_editorEdgeBox->setActiveEdgeId(
                    imageId
                );
            }
        }
        else
        {
            SDL_Log(
                "Failed to load edge image: %s",
                recentRequest.filename.c_str()
            );
        }
    }

    if (recentRequest.action ==
        RecentFileAction::Rename)
    {
        const std::filesystem::path oldPath(
            recentRequest.filename
        );

        const std::filesystem::path newPath =
            oldPath.parent_path() /
            (
                recentRequest.newName +
                oldPath.extension().string()
                );

        if (oldPath != newPath)
        {
            std::error_code error;

            if (std::filesystem::exists(
                newPath,
                error
            ))
            {
                SDL_Log(
                    "Image file already exists: %s",
                    newPath.string().c_str()
                );
            }
            else
            {
                error.clear();

                std::filesystem::rename(
                    oldPath,
                    newPath,
                    error
                );

                if (error)
                {
                    SDL_Log(
                        "Failed to rename image: %s",
                        oldPath.string().c_str()
                    );
                }
                else
                {
                    if (m_currentPixelFilename ==
                        recentRequest.filename)
                    {
                        m_currentPixelFilename =
                            newPath.string();
                    }

                    const std::string oldImageId =
                        oldPath.stem().string();

                    const std::string newImageId =
                        newPath.stem().string();

                    const bool isMisc =
                        newPath.parent_path().
                        filename() == "misc";

                    if (isMisc)
                    {
                        m_worldViewWindow.
                            renameMiscInAllCells(
                                oldImageId,
                                newImageId
                            );
                    }
                    else
                    {
                        m_worldViewWindow.
                            renameEdgeInAllCells(
                                oldImageId,
                                newImageId
                            );
                    }

                    if (!m_currentMapFilename.empty())
                    {
                        if (!m_worldViewWindow.saveMap(
                            m_currentMapFilename
                        ))
                        {
                            SDL_Log(
                                "Failed to save renamed image references: %s",
                                m_currentMapFilename.c_str()
                            );
                        }
                    }

                    refreshEdgeFiles();

                    if (isMisc)
                    {
                        m_editorMiscBox->renameMisc(
                            oldImageId,
                            newImageId
                        );
                    }
                    else
                    {
                        m_editorEdgeBox->renameEdge(
                            oldImageId,
                            newImageId
                        );
                    }

                    SDL_Log(
                        "Image renamed: %s",
                        newPath.string().c_str()
                    );
                }
            }
        }
    }

    if (recentRequest.action ==
        RecentFileAction::Delete)
    {
        const auto deletedPosition =
            std::find(
                m_edgeFiles.begin(),
                m_edgeFiles.end(),
                recentRequest.filename
            );

        std::size_t deletedIndex = 0;

        if (deletedPosition !=
            m_edgeFiles.end())
        {
            deletedIndex =
                static_cast<std::size_t>(
                    std::distance(
                        m_edgeFiles.begin(),
                        deletedPosition
                    )
                    );
        }

        const bool deletedCurrentFile =
            m_currentPixelFilename ==
            recentRequest.filename;

        std::error_code error;

        std::filesystem::remove(
            recentRequest.filename,
            error
        );

        if (error)
        {
            SDL_Log(
                "Failed to delete edge image: %s",
                recentRequest.filename.c_str()
            );
        }
        else
        {
            const std::string deletedEdgeId =
                std::filesystem::path(
                    recentRequest.filename
                ).stem().string();

            m_worldViewWindow.
                removeEdgeFromAllCells(
                    deletedEdgeId
                );
            if (!m_currentMapFilename.empty())
            {
                if (!m_worldViewWindow.saveMap(
                    m_currentMapFilename
                ))
                {
                    SDL_Log(
                        "Failed to save cleaned map: %s",
                        m_currentMapFilename.c_str()
                    );
                }
                else
                {
                    SDL_Log(
                        "Cleaned map saved: %s",
                        m_currentMapFilename.c_str()
                    );
                }
            }
            refreshEdgeFiles();

            if (deletedCurrentFile)
            {
                m_currentPixelFilename.clear();

                m_pixelCanvasWindow.newImage();
            }
        }

        if (m_currentPixelImageType ==
            PixelImageType::Edge)
        {
            m_reloadEditorEdgeBox = true;
        }
        else
        {
            m_editorMiscBox =
                std::make_unique<EditorMiscBox>(
                    m_renderer
                );
        }

        SDL_Log(
            "Edge image deleted: %s",
            recentRequest.filename.c_str()
        );


    }

    static char imageName[128] = {};

    switch (command)
    {
    case EditorCommand::NewImage:
        imageName[0] = '\0';

        ImGui::OpenPopup(
            "New Pixel Image"
        );
        break;

    case EditorCommand::SaveImage:
    {
        if (m_currentPixelFilename.empty())
        {
            imageName[0] = '\0';

            ImGui::OpenPopup(
                "Save Pixel Image"
            );

            break;
        }

        if (!SvgFileWriter::save(
            m_currentPixelFilename,
            m_pixelCanvasWindow.image()
        ))
        {
            SDL_Log(
                "Failed to save image: %s",
                m_currentPixelFilename.c_str()
            );

            break;
        }

        const std::string imageId =
            std::filesystem::path(
                m_currentPixelFilename
            ).stem().string();

        if (m_currentPixelImageType ==
            PixelImageType::Edge)
        {
            m_editorEdgeBox->
                addOrRefreshEdge(
                    imageId
                );

            refreshEdgeFiles();
        }
        else
        {
            m_editorMiscBox->
                addOrRefreshMisc(
                    imageId
                );
        }

        SDL_Log(
            "%s image saved: %s",
            m_currentPixelImageType ==
            PixelImageType::Edge
            ? "Edge"
            : "Misc",
            m_currentPixelFilename.c_str()
        );

        break;
    }

    case EditorCommand::None:
    case EditorCommand::OpenImage:
    case EditorCommand::ExportSvg:
        break;
    }

    if (ImGui::BeginPopupModal(
        "New Pixel Image",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize
    ))
    {
        ImGui::TextUnformatted(
            "Create image as:"
        );
        ImGui::SetNextItemWidth(
            260.0f
        );

        ImGui::InputText(
            "Name",
            imageName,
            sizeof(imageName)
        );

        const bool hasNewImageName =
            imageName[0] != '\0';

        if (!hasNewImageName)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button(
            "Edge",
            ImVec2(100.0f, 0.0f)
        ))
        {
            m_currentPixelImageType =
                PixelImageType::Edge;

            m_pixelCanvasWindow.newImage();

            m_currentPixelFilename =
                (
                    std::filesystem::path(
                        "resources/icons/edges"
                    ) /
                    (std::string(imageName) + ".svg")
                    ).string();

            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button(
            "Misc",
            ImVec2(100.0f, 0.0f)
        ))
        {
            m_currentPixelImageType =
                PixelImageType::Misc;

            m_pixelCanvasWindow.newImage();

            m_currentPixelFilename =
                (
                    std::filesystem::path(
                        "resources/icons/misc"
                    ) /
                    (std::string(imageName) + ".svg")
                    ).string();

            ImGui::CloseCurrentPopup();
        }
        if (!hasNewImageName)
        {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        if (ImGui::Button(
            "Cancel",
            ImVec2(100.0f, 0.0f)
        ))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal(
        "Save Pixel Image",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize
    ))
    {
        ImGui::Text(
            "%s name:",
            m_currentPixelImageType ==
            PixelImageType::Edge
            ? "Edge"
            : "Misc"
        );

        ImGui::SetNextItemWidth(
            260.0f
        );

        ImGui::InputText(
            "##EdgeName",
            imageName,
            sizeof(imageName)
        );

        const bool hasName =
            imageName[0] != '\0';

        if (!hasName)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button(
            "Save",
            ImVec2(100.0f, 0.0f)
        ))
        {
            const std::string edgeId =
                imageName;

            const std::filesystem::path filename =
                std::filesystem::path(
                    "resources/icons/edges"
                ) /
                (edgeId + ".svg");

            if (SvgFileWriter::save(
                filename.string(),
                m_pixelCanvasWindow.image()
            ))
            {
                m_currentPixelFilename =
                    filename.string();

                m_editorEdgeBox->
                    addOrRefreshEdge(
                        edgeId
                    );

                refreshEdgeFiles();
                const std::string imageId =
                    std::filesystem::path(
                        m_currentPixelFilename
                    ).stem().string();

                if (m_currentPixelImageType ==
                    PixelImageType::Edge)
                {
                    m_editorEdgeBox->
                        addOrRefreshEdge(
                            imageId
                        );

                    refreshEdgeFiles();
                }
                else
                {
                    m_editorMiscBox->
                        addOrRefreshMisc(
                            imageId
                        );
                }

                SDL_Log(
                    "%s image saved: %s",
                    m_currentPixelImageType ==
                    PixelImageType::Edge
                    ? "Edge"
                    : "Misc",
                    m_currentPixelFilename.c_str()
                );

                ImGui::CloseCurrentPopup();
            }
            else
            {
                SDL_Log(
                    "Failed to save edge image: %s",
                    filename.string().c_str()
                );
            }
        }

        if (!hasName)
        {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        if (ImGui::Button(
            "Cancel",
            ImVec2(100.0f, 0.0f)
        ))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void EditorApplication::drawToolbar()
{
    ImGuiViewport* viewport =
        ImGui::GetMainViewport();

    constexpr float toolbarHeight = 38.0f;

    constexpr ImGuiWindowFlags toolbarFlags =
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(6.0f, 5.0f)
    );

    if (ImGui::BeginViewportSideBar(
        "##MainToolbar",
        viewport,
        ImGuiDir_Up,
        toolbarHeight,
        toolbarFlags
    ))
    {
        ImGui::PushStyleColor(
            ImGuiCol_Button,
            IM_COL32(35, 105, 190, 255)
        );

        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered,
            IM_COL32(45, 125, 220, 255)
        );

        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive,
            IM_COL32(25, 85, 165, 255)
        );

        if (ImGui::Button(
            ICON_FA_DOWNLOAD "##SaveMap",
            ImVec2(34.0f, 28.0f)
        ))
        {
            saveCurrentMap();
        }

        ImGui::PopStyleColor(3);

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Save Map");
        }

        ImGui::SameLine();

        std::string currentMapName = "Load";

        if (!m_currentMapFilename.empty())
        {
            currentMapName =
                std::filesystem::path(
                    m_currentMapFilename
                ).filename().string();
        }

        ImGui::SetNextItemWidth(180.0f);

        std::string mapToLoad;
        std::string mapToRemoveFromRecent;

        if (ImGui::BeginCombo(
            "##LoadMap",
            currentMapName.c_str()
        ))
        {
            if (m_recentMaps.empty())
            {
                ImGui::TextDisabled(
                    "No recent maps"
                );
            }
            else
            {
                for (const std::string& filename :
                    m_recentMaps)
                {
                    const std::string displayName =
                        std::filesystem::path(
                            filename
                        ).filename().string();

                    const bool isSelected =
                        filename ==
                        m_currentMapFilename;

                    ImGui::PushID(filename.c_str());

                    if (ImGui::Selectable(
                        displayName.c_str(),
                        isSelected
                    ))
                    {
                        mapToLoad =
                            filename;
                    }

                    const bool mapItemHovered =
                        ImGui::IsItemHovered();

                    const bool mapItemRightClicked =
                        mapItemHovered &&
                        ImGui::IsMouseClicked(
                            ImGuiMouseButton_Right
                        );

                    if (mapItemRightClicked)
                    {
                        SDL_Log(
                            "Map entry right-clicked: %s",
                            filename.c_str()
                        );
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }

                    if (mapItemRightClicked)
                    {
                        ImGui::OpenPopup(
                            "##RecentMapContext"
                        );
                    }

                    if (ImGui::BeginPopup(
                        "##RecentMapContext"
                    ))
                    {
                        if (ImGui::MenuItem(
                            "Remove from recent"
                        ))
                        {
                            mapToRemoveFromRecent =
                                filename;
                        }

                        if (ImGui::MenuItem(
                            "Delete file..."
                        ))
                        {
                            m_mapPendingDeletion =
                                filename;

                            m_requestDeleteMapPopup =
                                true;
                        }

                        ImGui::EndPopup();
                    }

                    if (mapItemHovered)
                    {
                        ImGui::SetTooltip(
                            "%s",
                            filename.c_str()
                        );
                    }

                    ImGui::PopID();
                }
            }

            ImGui::Separator();

            if (ImGui::Selectable("Open Map..."))
            {
                showOpenFileDialog();
            }

            ImGui::EndCombo();
        }

        if (!m_currentMapFilename.empty() &&
            ImGui::BeginPopupContextItem(
                "##CurrentMapContext"
            ))
        {
            if (ImGui::MenuItem(
                "Remove from recent"
            ))
            {
                mapToRemoveFromRecent =
                    m_currentMapFilename;
            }

            if (ImGui::MenuItem(
                "Delete file..."
            ))
            {
                m_mapPendingDeletion =
                    m_currentMapFilename;

                m_requestDeleteMapPopup =
                    true;
            }

            ImGui::EndPopup();
        }
        if (!mapToLoad.empty())
        {
            requestOpenMap(
                mapToLoad
            );
        }
        if (!mapToRemoveFromRecent.empty())
        {
            removeRecentMap(
                mapToRemoveFromRecent
            );
        }


        if (ImGui::BeginPopup(
            "##CurrentMapViewContext"
        ))
        {
            if (ImGui::MenuItem(
                "Rename..."
            ))
            {
                m_mapPendingRename =
                    m_currentMapFilename;

                const std::string currentName =
                    std::filesystem::path(
                        m_currentMapFilename
                    ).stem().string();

                std::snprintf(
                    m_renameMapName,
                    sizeof(m_renameMapName),
                    "%s",
                    currentName.c_str()
                );

                m_requestRenameMapPopup =
                    true;
            }

            if (ImGui::MenuItem(
                "Delete file..."
            ))
            {
                m_mapPendingDeletion =
                    m_currentMapFilename;

                m_requestDeleteMapPopup =
                    true;
            }

            ImGui::EndPopup();
        }

        if (m_requestRenameMapPopup)
        {
            ImGui::OpenPopup(
                "Rename Map"
            );

            m_requestRenameMapPopup =
                false;
        }

        if (ImGui::BeginPopupModal(
            "Rename Map",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize
        ))
        {
            ImGui::TextUnformatted(
                "New map name:"
            );

            ImGui::SetNextItemWidth(
                300.0f
            );

            ImGui::InputText(
                "##RenameMapName",
                m_renameMapName,
                sizeof(m_renameMapName)
            );

            ImGui::Spacing();

            const bool hasNewName =
                m_renameMapName[0] != '\0';

            if (!hasNewName)
            {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("Rename"))
            {
                renameMapFile(
                    m_mapPendingRename,
                    m_renameMapName
                );

                m_mapPendingRename.clear();
                m_renameMapName[0] = '\0';

                ImGui::CloseCurrentPopup();
            }

            if (!hasNewName)
            {
                ImGui::EndDisabled();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel"))
            {
                m_mapPendingRename.clear();
                m_renameMapName[0] = '\0';

                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        if (m_requestDeleteMapPopup)
        {
            ImGui::OpenPopup("Delete Map?");
            m_requestDeleteMapPopup = false;
        }

        if (ImGui::BeginPopupModal(
            "Delete Map?",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize
        ))
        {
            ImGui::TextUnformatted(
                "Really delete this map file?"
            );

            ImGui::TextWrapped(
                "%s",
                m_mapPendingDeletion.c_str()
            );

            ImGui::Spacing();

            if (ImGui::Button("Delete"))
            {
                deleteMapFile(
                    m_mapPendingDeletion
                );

                m_mapPendingDeletion.clear();
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel"))
            {
                m_mapPendingDeletion.clear();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    ImGui::End();

    ImGui::PopStyleVar();
}
void EditorApplication::drawMapViewToolbar()
{
    ImGui::PushStyleColor(
        ImGuiCol_Button,
        IM_COL32(35, 105, 190, 255)
    );

    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered,
        IM_COL32(45, 125, 220, 255)
    );

    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive,
        IM_COL32(25, 85, 165, 255)
    );
    if (ImGui::Button(
        ICON_FA_FILE "##MapViewNew",
        ImVec2(34.0f, 28.0f)
    ))
    {
        createNewMap();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "New Map"
        );
    }

    ImGui::SameLine();

    if (ImGui::Button(
        ICON_FA_DOWNLOAD "##MapViewSave",
        ImVec2(34.0f, 28.0f)
    ))
    {
        saveCurrentMap();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "Save Map"
        );
    }

    ImGui::SameLine();

    std::string currentMapName =
        "Load";

    if (!m_currentMapFilename.empty())
    {
        currentMapName =
            std::filesystem::path(
                m_currentMapFilename
            ).filename().string();
    }

    ImGui::SetNextItemWidth(
        180.0f
    );

    std::string mapToLoad;

    const bool mapFilesOpen =
        ImGui::BeginCombo(
            "##MapViewLoad",
            currentMapName.c_str()
        );

    if (!m_currentMapFilename.empty() &&
        ImGui::IsItemHovered() &&
        ImGui::IsMouseClicked(
            ImGuiMouseButton_Right
        ))
    {
        ImGui::OpenPopup(
            "##CurrentMapViewContext"
        );
    }

    if (mapFilesOpen)
    {
        m_worldViewWindow.blockMapInputOnce();

        if (m_mapFiles.empty())
        {
            ImGui::TextDisabled(
                "No map files"
            );
        }
        else
        {
            for (const std::string& filename :
                m_mapFiles)
            {
                const std::string displayName =
                    std::filesystem::path(
                        filename
                    ).filename().string();

                const bool isSelected =
                    filename ==
                    m_currentMapFilename;

                ImGui::PushID(
                    filename.c_str()
                );

                if (ImGui::Selectable(
                    displayName.c_str(),
                    isSelected
                ))
                {
                    mapToLoad =
                        filename;
                }

                if (ImGui::BeginPopupContextItem(
                    "##MapFileContext"
                ))
                {
                    if (ImGui::MenuItem(
                        "Rename..."
                    ))
                    {
                        m_mapPendingRename =
                            filename;

                        const std::string currentName =
                            std::filesystem::path(
                                filename
                            ).stem().string();

                        std::snprintf(
                            m_renameMapName,
                            sizeof(m_renameMapName),
                            "%s",
                            currentName.c_str()
                        );

                        m_requestRenameMapPopup =
                            true;
                    }


                    if (ImGui::MenuItem(
                        "Delete file..."
                    ))
                    {
                        m_mapPendingDeletion =
                            filename;

                        m_requestDeleteMapPopup =
                            true;
                    }

                    ImGui::EndPopup();
                }

                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }

                ImGui::PopID();
            }
        }

        ImGui::Separator();

        if (ImGui::Selectable(
            "Open Map..."
        ))
        {
            showOpenFileDialog();
        }

        ImGui::EndCombo();
    }

    if (ImGui::BeginPopup(
        "##CurrentMapViewContext"
    ))
    {
        if (ImGui::MenuItem(
            "Rename..."
        ))
        {
            m_mapPendingRename =
                m_currentMapFilename;

            const std::string currentName =
                std::filesystem::path(
                    m_currentMapFilename
                ).stem().string();

            std::snprintf(
                m_renameMapName,
                sizeof(m_renameMapName),
                "%s",
                currentName.c_str()
            );

            m_requestRenameMapPopup =
                true;
        }

        if (ImGui::MenuItem(
            "Delete file..."
        ))
        {
            m_mapPendingDeletion =
                m_currentMapFilename;

            m_requestDeleteMapPopup =
                true;
        }

        ImGui::EndPopup();
    }

    if (m_requestRenameMapPopup)
    {
        ImGui::OpenPopup(
            "Rename Map"
        );

        m_requestRenameMapPopup =
            false;
    }

    if (ImGui::BeginPopupModal(
        "Rename Map",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize
    ))
    {
        ImGui::TextUnformatted(
            "New map name:"
        );

        ImGui::SetNextItemWidth(
            300.0f
        );

        ImGui::InputText(
            "##RenameMapName",
            m_renameMapName,
            sizeof(m_renameMapName)
        );

        ImGui::Spacing();

        const bool hasNewName =
            m_renameMapName[0] != '\0';

        if (!hasNewName)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("Rename"))
        {
            renameMapFile(
                m_mapPendingRename,
                m_renameMapName
            );

            m_mapPendingRename.clear();
            m_renameMapName[0] = '\0';

            ImGui::CloseCurrentPopup();
        }

        if (!hasNewName)
        {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
        {
            m_mapPendingRename.clear();
            m_renameMapName[0] = '\0';

            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (m_requestDeleteMapPopup)
    {
        ImGui::OpenPopup(
            "Delete Map?"
        );

        m_requestDeleteMapPopup = false;
    }

    if (ImGui::BeginPopupModal(
        "Delete Map?",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize
    ))
    {
        ImGui::TextUnformatted(
            "Really delete this map file?"
        );

        ImGui::TextWrapped(
            "%s",
            m_mapPendingDeletion.c_str()
        );

        ImGui::Spacing();

        if (ImGui::Button("Delete"))
        {
            deleteMapFile(
                m_mapPendingDeletion
            );

            m_mapPendingDeletion.clear();

            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
        {
            m_mapPendingDeletion.clear();

            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (!mapToLoad.empty())
    {
        requestOpenMap(
            mapToLoad
        );
    }
    ImGui::PopStyleColor(3);
}
void EditorApplication::showSaveFileDialog()
{
    SDL_Log("Opening save file dialog...");

    static const SDL_DialogFileFilter filters[] =
    {
        {
            "GridBuilder Map",
            "map"
        }
    };

    SDL_ShowSaveFileDialog(
        saveFileDialogCallback,
        this,
        m_window,
        filters,
        1,
        nullptr
    );
}
void SDLCALL EditorApplication::saveFileDialogCallback(
    void* userdata,
    const char* const* filelist,
    int filter

)
{

    SDL_Log("Save file dialog callback called.");

    if (filelist == nullptr)
    {
        SDL_Log(
            "Save dialog failed: %s",
            SDL_GetError()
        );

        return;
    }

    if (*filelist == nullptr)
    {
        return;
    }

    auto* request = new SaveMapRequest
    {
        static_cast<EditorApplication*>(userdata),
        *filelist
    };

    if (!SDL_RunOnMainThread(
        saveFileOnMainThread,
        request,
        false
    ))
    {
        SDL_Log(
            "Could not queue save operation: %s",
            SDL_GetError()
        );

        delete request;
    }
}

void SDLCALL EditorApplication::saveFileOnMainThread(
    void* userdata
)
{
    std::unique_ptr<SaveMapRequest> request(
        static_cast<SaveMapRequest*>(userdata)
    );

    EditorApplication& application =
        *request->application;

    std::string filename =
        request->filename;

    if (filename.size() < 4 ||
        filename.substr(
            filename.size() - 4
        ) != ".map")
    {
        filename += ".map";
    }

    if (!application.m_worldViewWindow.saveMap(
        filename
    ))
    {
        SDL_Log(
            "Failed to save map: %s",
            filename.c_str()
        );

        return;
    }

    application.m_currentMapFilename =
        filename;

    application.refreshMapFiles();

    application.addRecentMap(
        filename
    );

    SDL_Log(
        "Map saved: %s",
        filename.c_str()
    );
}
void EditorApplication::drawWorkspace()
{
    const ImGuiViewport* viewport =
        ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(
        viewport->WorkPos
    );

    ImGui::SetNextWindowSize(
        viewport->WorkSize
    );

    ImGui::SetNextWindowViewport(
        viewport->ID
    );

    constexpr ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowRounding,
        0.0f
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowBorderSize,
        0.0f
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(0.0f, 0.0f)
    );

    ImGui::Begin(
        "Main Workspace",
        nullptr,
        windowFlags
    );

    ImGui::PopStyleVar(3);

    drawToolbar();

    const ImGuiID dockspaceId =
        ImGui::GetID("Main Dockspace");

    ImGui::DockSpace(
        dockspaceId,
        ImVec2(0.0f, 0.0f)
    );

    ImGui::End();
}

void EditorApplication::showOpenFileDialog()
{
    static const SDL_DialogFileFilter filters[] =
    {
        {
            "GridBuilder Map",
            "map"
        }
    };

    SDL_ShowOpenFileDialog(
        openFileDialogCallback,
        this,
        m_window,
        filters,
        1,
        nullptr,
        false
    );
}

void SDLCALL EditorApplication::openFileDialogCallback(
    void* userdata,
    const char* const* filelist,
    int filter
)
{
    if (filelist == nullptr)
    {
        SDL_Log(
            "Open dialog failed: %s",
            SDL_GetError()
        );

        return;
    }

    if (*filelist == nullptr)
    {
        return;
    }

    auto* request = new OpenMapRequest
    {
        static_cast<EditorApplication*>(userdata),
        *filelist
    };

    if (!SDL_RunOnMainThread(
        openFileOnMainThread,
        request,
        false
    ))
    {
        SDL_Log(
            "Could not queue load operation: %s",
            SDL_GetError()
        );

        delete request;
    }
}

void SDLCALL EditorApplication::openFileOnMainThread(
    void* userdata
)
{
    std::unique_ptr<OpenMapRequest> request(
        static_cast<OpenMapRequest*>(userdata)
    );

    EditorApplication& application =
        *request->application;

    const std::string& filename =
        request->filename;

    application.requestOpenMap(
        filename
    );
}

void EditorApplication::saveCurrentMap()
{
    if (m_currentMapFilename.empty())
    {
        showSaveFileDialog();
        return;
    }

    if (!m_worldViewWindow.saveMap(
        m_currentMapFilename
    ))
    {
        SDL_Log(
            "Failed to save map: %s",
            m_currentMapFilename.c_str()
        );

        return;
    }

    SDL_Log(
        "Map saved: %s",
        m_currentMapFilename.c_str()
    );
}

std::string EditorApplication::recentMapsFilename() const
{
    char* preferencePath =
        SDL_GetPrefPath(
            "Daniel Horn",
            "GridBuilder Editor"
        );

    if (preferencePath == nullptr)
    {
        return {};
    }

    std::string filename =
        std::string(preferencePath) +
        "recent-maps.txt";

    SDL_free(preferencePath);

    return filename;
}

void EditorApplication::removeRecentMap(const std::string& filename)
{
    const auto existing =
        std::find(
            m_recentMaps.begin(),
            m_recentMaps.end(),
            filename
        );

    if (existing != m_recentMaps.end())
    {
        m_recentMaps.erase(existing);
        saveRecentMaps();
    }
}



void EditorApplication::addRecentMap(
    const std::string& filename
)
{
    const auto existing =
        std::find(
            m_recentMaps.begin(),
            m_recentMaps.end(),
            filename
        );

    if (existing != m_recentMaps.end())
    {
        m_recentMaps.erase(existing);
    }

    m_recentMaps.insert(
        m_recentMaps.begin(),
        filename
    );

    constexpr std::size_t maximumEntries = 10;

    if (m_recentMaps.size() > maximumEntries)
    {
        m_recentMaps.resize(maximumEntries);
    }

    saveRecentMaps();
}

void EditorApplication::shutdown()
{
    saveLastMap();
    saveLastEdge();
    saveWindowState();
    saveEditorSettings();

    m_editorEdgeBox.reset();
    m_editorToolbox.reset();

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (m_renderer)
    {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }

    if (m_window)
    {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    SDL_Quit();
}

void EditorApplication::loadRecentMaps()
{
    const std::string filename =
        recentMapsFilename();

    if (filename.empty())
    {
        return;
    }

    std::ifstream file(filename);
    std::string mapFilename;

    while (std::getline(
        file,
        mapFilename
    ))
    {
        if (!mapFilename.empty())
        {
            m_recentMaps.push_back(
                mapFilename
            );
        }
    }
}

void EditorApplication::saveRecentMaps() const
{
    const std::string filename =
        recentMapsFilename();

    if (filename.empty())
    {
        return;
    }

    std::ofstream file(filename);

    for (const std::string& mapFilename :
        m_recentMaps)
    {
        file
            << mapFilename
            << '\n';
    }
}
void EditorApplication::deleteMapFile(
    const std::string& filename
)
{
    std::error_code error;

    const bool deleted =
        std::filesystem::remove(
            filename,
            error
        );

    if (!deleted || error)
    {
        SDL_Log(
            "Failed to delete map: %s",
            filename.c_str()
        );

        return;
    }

    removeRecentMap(filename);

    refreshMapFiles();

    if (m_currentMapFilename == filename)
    {
        m_currentMapFilename.clear();
    }

    SDL_Log(
        "Map deleted: %s",
        filename.c_str()
    );
}

void EditorApplication::renameMapFile(
    const std::string& filename,
    const std::string& newName
)
{
    if (filename.empty() ||
        newName.empty())
    {
        return;
    }

    const std::filesystem::path oldPath(
        filename
    );

    const std::filesystem::path newPath =
        oldPath.parent_path() /
        (
            newName +
            oldPath.extension().string()
            );

    if (oldPath == newPath)
    {
        return;
    }

    std::error_code error;

    if (std::filesystem::exists(
        newPath,
        error
    ))
    {
        SDL_Log(
            "Map already exists: %s",
            newPath.string().c_str()
        );

        return;
    }

    error.clear();

    std::filesystem::rename(
        oldPath,
        newPath,
        error
    );

    if (error)
    {
        SDL_Log(
            "Failed to rename map: %s",
            filename.c_str()
        );

        return;
    }

    const std::string renamedFilename =
        newPath.string();

    for (std::string& recentMap :
        m_recentMaps)
    {
        if (recentMap == filename)
        {
            recentMap =
                renamedFilename;
        }
    }

    saveRecentMaps();

    if (m_currentMapFilename ==
        filename)
    {
        m_currentMapFilename =
            renamedFilename;
    }

    refreshMapFiles();

    SDL_Log(
        "Map renamed: %s",
        renamedFilename.c_str()
    );
}

void EditorApplication::drawNewMapPopup()
{
    if (m_requestNewMapPopup)
    {
        ImGui::OpenPopup(
            "Create New Map"
        );

        m_requestNewMapPopup = false;
    }

    if (!ImGui::BeginPopupModal(
        "Create New Map",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize
    ))
    {
        return;
    }

    ImGui::TextUnformatted(
        "Map name:"
    );

    ImGui::SetNextItemWidth(
        260.0f
    );

    const bool enterPressed =
        ImGui::InputText(
            "##NewMapName",
            m_newMapName,
            sizeof(m_newMapName),
            ImGuiInputTextFlags_EnterReturnsTrue
        );

    std::string mapName =
        m_newMapName;

    if (mapName.size() >= 4 &&
        mapName.substr(
            mapName.size() - 4
        ) == ".map")
    {
        mapName.resize(
            mapName.size() - 4
        );
    }

    const bool validName =
        !mapName.empty() &&
        std::filesystem::path(
            mapName
        ).filename().string() ==
        mapName;

    const std::filesystem::path filename =
        std::filesystem::path(
            "resources/maps"
        ) /
        (mapName + ".map");

    const bool fileExists =
        validName &&
        std::filesystem::exists(
            filename
        );

    if (!validName &&
        !mapName.empty())
    {
        ImGui::TextColored(
            ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
            "Please enter a valid filename."
        );
    }
    else if (fileExists)
    {
        ImGui::TextColored(
            ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
            "A map with this name already exists."
        );
    }

    const bool canCreate =
        validName &&
        !fileExists;

    ImGui::BeginDisabled(
        !canCreate
    );

    const bool createClicked =
        ImGui::Button(
            "Create"
        );

    ImGui::EndDisabled();

    if ((createClicked ||
        enterPressed) &&
        canCreate)
    {
        std::error_code error;

        std::filesystem::create_directories(
            filename.parent_path(),
            error
        );

        m_worldViewWindow.newMap();

        m_currentMapFilename =
            filename.string();

        if (m_worldViewWindow.saveMap(
            m_currentMapFilename
        ))
        {
            refreshMapFiles();

            addRecentMap(
                m_currentMapFilename
            );

            saveLastMap();

            SDL_Log(
                "New map created: %s",
                m_currentMapFilename.c_str()
            );

            ImGui::CloseCurrentPopup();
        }
        else
        {
            SDL_Log(
                "Failed to create map: %s",
                m_currentMapFilename.c_str()
            );

            m_currentMapFilename.clear();
        }
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Cancel"
    ))
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}
void EditorApplication::createNewMap()
{
    if (m_worldViewWindow.hasUnsavedChanges())
    {
        m_pendingMapAction =
            PendingMapAction::NewMap;

        m_requestUnsavedChangesPopup = true;
        return;
    }

    m_newMapName[0] = '\0';
    m_requestNewMapPopup = true;
}

std::string EditorApplication::lastMapFilename() const
{
    char* preferencePath =
        SDL_GetPrefPath(
            "Daniel Horn",
            "GridBuilder Editor"
        );

    if (preferencePath == nullptr)
    {
        return {};
    }

    const std::string filename =
        std::string(preferencePath) +
        "last-map.txt";

    SDL_free(preferencePath);

    return filename;
}

std::string
EditorApplication::windowStateFilename() const
{
    char* preferencePath =
        SDL_GetPrefPath(
            "Daniel Horn",
            "GridBuilder Editor"
        );

    if (preferencePath == nullptr)
    {
        return {};
    }

    const std::string filename =
        std::string(preferencePath) +
        "window-state.txt";

    SDL_free(preferencePath);

    return filename;
}

void EditorApplication::saveWindowState() const
{
    const std::string filename =
        windowStateFilename();

    if (filename.empty() ||
        !m_window)
    {
        return;
    }

    std::ofstream file(
        filename,
        std::ios::trunc
    );

    if (!file)
    {
        return;
    }

    int windowX = 0;
    int windowY = 0;
    int windowWidth = 1280;
    int windowHeight = 720;

    SDL_GetWindowPosition(
        m_window,
        &windowX,
        &windowY
    );

    SDL_GetWindowSize(
        m_window,
        &windowWidth,
        &windowHeight
    );

    const SDL_WindowFlags windowFlags =
        SDL_GetWindowFlags(
            m_window
        );

    const bool maximized =
        (windowFlags &
            SDL_WINDOW_MAXIMIZED) != 0;

    file <<
        (m_showMapEditor ? 1 : 0) <<
        '\n';

    file <<
        (m_showPixelEditor ? 1 : 0) <<
        '\n';

    file <<
        windowX << ' ' <<
        windowY << ' ' <<
        windowWidth << ' ' <<
        windowHeight << ' ' <<
        (maximized ? 1 : 0) <<
        '\n';
}

EditorApplication::~EditorApplication()
{
    shutdown();
}

GameModuleManager&
EditorApplication::gameModuleManager()
{
    return m_gameModuleManager;
}

DosBoxMemoryTools::MemoryReader&
EditorApplication::memoryReader()
{
    return m_memoryTools->memoryReader();
}

void EditorApplication::loadWindowState()
{
    const std::string filename =
        windowStateFilename();

    if (filename.empty() ||
        !m_window)
    {
        return;
    }

    std::ifstream file(filename);

    if (!file)
    {
        return;
    }

    int showMapEditor = 1;
    int showPixelEditor = 1;

    if (!(file >>
        showMapEditor >>
        showPixelEditor))
    {
        return;
    }

    m_showMapEditor =
        showMapEditor != 0;

    m_showPixelEditor =
        showPixelEditor != 0;

    int windowX = 0;
    int windowY = 0;
    int windowWidth = 1280;
    int windowHeight = 720;
    int maximized = 0;

    if (file >>
        windowX >>
        windowY >>
        windowWidth >>
        windowHeight >>
        maximized)
    {
        SDL_SetWindowPosition(
            m_window,
            windowX,
            windowY
        );

        SDL_SetWindowSize(
            m_window,
            windowWidth,
            windowHeight
        );

        if (maximized != 0)
        {
            SDL_MaximizeWindow(
                m_window
            );
        }
    }
}

void EditorApplication::saveLastMap() const
{
    const std::string settingsFilename =
        lastMapFilename();

    if (settingsFilename.empty())
    {
        return;
    }

    std::ofstream file(
        settingsFilename,
        std::ios::trunc
    );

    if (!m_currentMapFilename.empty())
    {
        file << m_currentMapFilename;
    }
}

void EditorApplication::loadLastMap()
{
    const std::string settingsFilename =
        lastMapFilename();

    if (settingsFilename.empty())
    {
        return;
    }

    std::ifstream file(settingsFilename);
    std::string mapFilename;

    std::getline(file, mapFilename);

    if (mapFilename.empty() ||
        !std::filesystem::exists(mapFilename))
    {
        return;
    }

    if (!m_worldViewWindow.loadMap(
        mapFilename
    ))
    {
        SDL_Log(
            "Failed to restore map: %s",
            mapFilename.c_str()
        );

        return;
    }

    m_currentMapFilename =
        mapFilename;

    addRecentMap(mapFilename);

    SDL_Log(
        "Map restored: %s",
        mapFilename.c_str()
    );
}

void EditorApplication::drawKeyBindingsWindow()
{
    if (!m_showKeyBindingsWindow)
    {
        return;
    }

    ImGui::SetNextWindowSize(
        ImVec2(
            620.0f,
            500.0f
        ),
        ImGuiCond_FirstUseEver
    );

    if (!ImGui::Begin(
        "DOSBox Key Bindings",
        &m_showKeyBindingsWindow
    ))
    {
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted(
        "Keyboard input"
    );

    bool germanKeyboardForUsGame =
        m_dosBoxKeyBindings.
        germanKeyboardForUsGame();

    if (ImGui::Checkbox(
        "German keyboard for US game",
        &germanKeyboardForUsGame
    ))
    {
        m_dosBoxKeyBindings.
            setGermanKeyboardForUsGame(
                germanKeyboardForUsGame
            );

        m_dosBoxKeyBindings.save(
            dosBoxKeyBindingsFilename()
        );
    }

    ImGui::Separator();

    if (ImGui::BeginTable(
        "##KeyBindings",
        3,
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable
    ))
    {
        ImGui::TableSetupColumn(
            "Function"
        );

        ImGui::TableSetupColumn(
            "Default"
        );

        ImGui::TableSetupColumn(
            "Custom"
        );

        ImGui::TableHeadersRow();

        for (const DosBoxKeyBinding& binding :
            m_dosBoxKeyBindings.bindings())
        {
            ImGui::PushID(
                static_cast<int>(
                    binding.action
                    )
            );

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);

            ImGui::TextUnformatted(
                binding.functionName
            );

            ImGui::TableSetColumnIndex(1);

            ImGui::TextUnformatted(
                binding.defaultKeyName
            );

            ImGui::TableSetColumnIndex(2);

            const bool waitingForKey =
                m_keyBindingBeingEdited ==
                binding.action;

            const char* buttonText =
                "Not assigned";

            if (waitingForKey)
            {
                buttonText =
                    "Press a key...";
            }
            else if (binding.customKey !=
                ImGuiKey_None)
            {
                buttonText =
                    ImGui::GetKeyName(
                        binding.customKey
                    );
            }

            if (ImGui::Button(
                buttonText,
                ImVec2(
                    -1.0f,
                    0.0f
                )
            ))
            {
                m_keyBindingBeingEdited =
                    binding.action;
            }

            if (ImGui::IsItemClicked(
                ImGuiMouseButton_Right
            ))
            {
                m_dosBoxKeyBindings.
                    clearCustomKey(
                        binding.action
                    );

                m_dosBoxKeyBindings.save(
                    dosBoxKeyBindingsFilename()
                );

                m_keyBindingBeingEdited =
                    DosBoxAction::Count;
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    if (m_keyBindingBeingEdited !=
        DosBoxAction::Count)
    {
        if (ImGui::IsKeyPressed(
            ImGuiKey_Escape,
            false
        ))
        {
            m_keyBindingBeingEdited =
                DosBoxAction::Count;
        }
        else
        {
            for (int keyValue =
                ImGuiKey_NamedKey_BEGIN;
                keyValue <
                ImGuiKey_NamedKey_END;
                ++keyValue)
            {
                const ImGuiKey key =
                    static_cast<ImGuiKey>(
                        keyValue
                        );

                if (ImGui::IsKeyPressed(
                    key,
                    false
                ))
                {
                    m_dosBoxKeyBindings.
                        setCustomKey(
                            m_keyBindingBeingEdited,
                            key
                        );

                    m_dosBoxKeyBindings.save(
                        dosBoxKeyBindingsFilename()
                    );

                    m_keyBindingBeingEdited =
                        DosBoxAction::Count;

                    break;
                }
            }
        }
    }

    ImGui::TextDisabled(
        "Click a custom binding and press a key. "
        "Right-click to clear it. Escape cancels."
    );

    ImGui::End();
}

std::string
EditorApplication::dosBoxKeyBindingsFilename() const
{
    return
        "settings/dosbox-key-bindings.cfg";
}

