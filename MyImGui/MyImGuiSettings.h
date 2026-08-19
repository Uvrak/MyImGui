#pragma once

namespace MyImGui
{
    class MyImGuiSettings
    {
    public:
        MyImGuiSettings();
        ~MyImGuiSettings();

        void apply() const;

        void setFontScale(
            float scale
        );

        void setWindowPlacement(
            int x,
            int y,
            int width,
            int height
        );

        float fontScale() const;

        int windowX() const;
        int windowY() const;
        int windowWidth() const;
        int windowHeight() const;

    private:
        void load();
        void save() const;

        float m_fontScale = 1.0f;

        int m_windowX = 100;
        int m_windowY = 100;
        int m_windowWidth = 1280;
        int m_windowHeight = 800;
    };
}