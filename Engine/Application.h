// Application.h
#pragma once
#include <string>

#include "Window.h"
#include"Renderer.h"
#include "MainMenu.h"
#include "ResolutionWindow.h"
#include "WorldManager.h"

namespace ow3d
{
    class Application
    {
    public:
        Application();
        ~Application();

        bool initialize(
            WorldManager::TileWorldInitializer setupWorld = {},
            const std::function<void(Renderer&)>& setupRenderer = {},
            WorldManager::WorldFactory worldFactory = {});
        void run();
        void setWorldMode(WorldMode mode) { m_worldManager.setMode(mode); }
        void setCaptureUiTest(bool value) { m_captureUiTest=value; }
        void setCaptureOutput(const std::string& prefix) { m_capturePrefix = prefix; }
        void setCaptureHoverTest(bool enabled) { m_captureHoverTest = enabled; }
        void setViewportControlActivated(std::function<void()> callback){m_viewportControlActivated=std::move(callback);}
        void setUiDraw(std::function<bool(float,float,float,float)> draw) { m_uiDraw = std::move(draw); }
        void setWindowDraw(std::function<void()> draw) { m_windowDraw = std::move(draw); }
        void setSettingsDraw(std::function<void()> draw) { m_settingsDraw=std::move(draw); }
        void setSettingsVisible(bool visible) { m_settingsVisible=visible; }
        bool settingsVisible() const { return m_settingsVisible; }
        void setFramerateVisible(bool visible) { m_framerateVisible=visible; }
        void setMenuDraw(std::function<void()> draw) { m_mainMenu.additionalMenu = std::move(draw); }

    private:
        std::function<void()> m_viewportControlActivated;
        bool m_running = true;
        bool m_framerateVisible = false;
        bool m_settingsVisible = true;
        std::string m_capturePrefix;
        bool m_captureHoverTest = false;
        bool m_captureUiTest = false;
        std::function<bool(float,float,float,float)> m_uiDraw;
        std::function<void()> m_windowDraw;
        std::function<void()> m_settingsDraw;
        Window m_window;
        Renderer m_renderer;
        MainMenu m_mainMenu;
        ResolutionWindow m_resolutionWindow;
        WorldManager m_worldManager;
    };
}
