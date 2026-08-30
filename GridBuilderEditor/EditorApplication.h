#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_filesystem.h>

#include "PropertiesWindow.h"
#include "WorldViewWindow.h"
#include "EditorToolBox.h"
#include "EditorEdgeBox.h"
#include "EditorMiscBox.h"
#include "DosBoxWindow.h"
#include "PixelCanvasWindow.h"
#include "PixelTool.h"
#include "MainToolbar.h"

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "DosBoxKeyBindings.h"

#include "MemoryTools.h"
#include "ItemExplorerWindow.h"
#include "GameModuleManager.h"

enum class MapPaintTarget
{
    Edge,
    Misc
};

enum class PixelImageType
{
    Edge,
    Misc
};

class EditorApplication
{
public:
    EditorApplication();
    ~EditorApplication();

    GameModuleManager&
        gameModuleManager();

    DosBoxMemoryTools::MemoryReader&
        memoryReader();

    void run();

private:
    bool initializeSDL();
    bool createWindow();
    bool createRenderer();

    void processEvents();
    void render();

    void drawMainMenu();
    void drawWorkspace();
    void drawToolbar();
    void drawMapViewToolbar();
    void drawPixelViewToolbar();

    void shutdown();

private:
    SDL_Window* m_window =
        nullptr;

    SDL_Renderer* m_renderer =
        nullptr;

    bool m_running =
        true;

private:
    enum class PendingMapAction
    {
        None,
        Exit,
        NewMap,
        OpenMap
    };

    PropertiesWindow
        m_propertiesWindow;

    std::unique_ptr<EditorToolbox>
        m_editorToolbox;

    std::unique_ptr<EditorEdgeBox>
        m_editorEdgeBox;

    std::unique_ptr<EditorMiscBox>
        m_editorMiscBox;

    WorldViewWindow
        m_worldViewWindow{ 16 };

    MapPaintTarget m_mapPaintTarget =
        MapPaintTarget::Edge;

    PendingMapAction m_pendingMapAction =
        PendingMapAction::None;

    std::string m_pendingMapFilename;

private:
    std::vector<std::string>
        m_recentMaps;

    std::vector<std::string>
        m_mapFiles;

    std::string
        m_currentMapFilename;

    std::string
        m_mapPendingDeletion;

    bool m_requestDeleteMapPopup =
        false;

    std::string
        m_mapPendingRename;

    bool m_requestRenameMapPopup =
        false;

    char m_renameMapName[256] =
    {};

    bool m_requestNewMapPopup =
        false;

    char m_newMapName[256] =
    {};

private:
    void showSaveFileDialog();

    static void SDLCALL
        saveFileDialogCallback(
            void* userdata,
            const char* const* filelist,
            int filter
        );

    static void SDLCALL
        saveFileOnMainThread(
            void* userdata
        );

    void showOpenFileDialog();

    static void SDLCALL
        openFileDialogCallback(
            void* userdata,
            const char* const* filelist,
            int filter
        );

    static void SDLCALL
        openFileOnMainThread(
            void* userdata
        );


private:
    void saveCurrentMap();

    void loadRecentMaps();
    void saveRecentMaps() const;

    void addRecentMap(
        const std::string& filename
    );

    std::string
        recentMapsFilename() const;

    void removeRecentMap(
        const std::string& filename
    );

    void deleteMapFile(
        const std::string& filename
    );

    void renameMapFile(
        const std::string& filename,
        const std::string& newName
    );

    void drawNewMapPopup();
    void confirmCreateNewMap();
    void createNewMap();

    std::string
        lastMapFilename() const;

    void saveLastMap() const;
    void loadLastMap();

    void refreshMapFiles();

    void openDosGame(
        const std::string& filename
    );

private:
    PixelCanvasWindow
        m_pixelCanvasWindow;

    PixelTool m_activePixelTool =
        PixelTool::Pencil;

    MainToolbar
        m_pixelMainToolbar;

    std::vector<std::string>
        m_edgeFiles;

    std::string
        m_currentPixelFilename;

        PixelImageType m_currentPixelImageType =
            PixelImageType::Edge;

    void refreshEdgeFiles();

    std::string
        lastEdgeFilename() const;

    void saveLastEdge() const;
    void loadLastEdge();

    bool m_refreshEdgeTextures =
        false;

    bool m_reloadEditorEdgeBox =
        false;

private:
    bool m_showMapEditor =
        true;

    bool m_showPixelEditor =
        true;

    bool m_showMapCoordinates =
        false;

    bool m_focusMapEditor =
        false;

    bool m_focusPixelEditor =
        false;

    ImGuiID m_pixelEditorDockId =
        0;

    bool m_pixelEditorMovedToCenter =
        false;

private:
    std::string
        windowStateFilename() const;

    void saveWindowState() const;
    void loadWindowState();

    std::string
        editorSettingsFilename() const;

    void loadEditorSettings();
    void saveEditorSettings() const;

    void requestExit();
    void drawUnsavedChangesPopup();
    void continuePendingMapAction();

    void requestOpenMap(
        const std::string& filename
    );

    void openMap(
        const std::string& filename
    );

private:
    double m_lastEdgeCycleTime =
        -1.0;

    bool m_requestUnsavedChangesPopup = false;

    std::unique_ptr<DosBoxWindow> dosBoxWindow = nullptr;

    std::unique_ptr<
        DosBoxMemoryTools::MemoryTools
    > m_memoryTools;

    ItemExplorer::ItemExplorerWindow
        m_itemExplorerWindow;

    bool m_showItemExplorer =
        true;
    std::vector<std::string> m_recentGames;

    std::string recentGamesFilename() const;

    void loadRecentGames();
    void saveRecentGames() const;

    void drawKeyBindingsWindow();
    bool m_showKeyBindingsWindow =
        false;

    DosBoxKeyBindings
        m_dosBoxKeyBindings;

    DosBoxAction m_keyBindingBeingEdited =
        DosBoxAction::Count;

    std::string
        dosBoxKeyBindingsFilename() const;

    float m_fontScale = 1.0f;

    GameModuleManager
        m_gameModuleManager;
              
};