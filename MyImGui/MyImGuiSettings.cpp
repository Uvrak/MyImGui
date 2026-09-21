#include "pch.h"
#include "MyImGuiSettings.h"
#include <filesystem>
#include <fstream>
#include "imgui.h"

namespace MyImGui
{
    static const char* settingsPath() { return "settings/myimgui_settings.cfg"; }

    MyImGuiSettings::MyImGuiSettings() { load(); }
    MyImGuiSettings::~MyImGuiSettings() { save(); }

    void MyImGuiSettings::apply() const { ImGui::GetIO().FontGlobalScale = m_fontScale; }
    void MyImGuiSettings::setFontScale(float scale) { m_fontScale = scale; }
    float MyImGuiSettings::fontScale() const { return m_fontScale; }

    void MyImGuiSettings::load()
    {
        std::ifstream file(settingsPath());
        if (!file) return;
        file >> m_fontScale >> m_windowX >> m_windowY >> m_windowWidth >> m_windowHeight;
    }

    void MyImGuiSettings::save() const
    {
        std::filesystem::create_directories("settings");
        std::ofstream file(settingsPath());
        if (!file) return;
        file << m_fontScale << '\n' << m_windowX << '\n' << m_windowY << '\n'
             << m_windowWidth << '\n' << m_windowHeight;
    }

    void MyImGuiSettings::setWindowPlacement(int x, int y, int width, int height)
    { m_windowX=x; m_windowY=y; m_windowWidth=width; m_windowHeight=height; }
    int MyImGuiSettings::windowX() const { return m_windowX; }
    int MyImGuiSettings::windowY() const { return m_windowY; }
    int MyImGuiSettings::windowWidth() const { return m_windowWidth; }
    int MyImGuiSettings::windowHeight() const { return m_windowHeight; }
}