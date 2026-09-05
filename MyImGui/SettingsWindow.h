#pragma once

namespace MyImGui
{
    class SettingsWindow
    {
    public:
        void open();
        void draw();

        bool isOpen() const;

        float fontSize() const;

        void setFontSize(
            float fontSize
        );
    private:
        bool m_open =
            false;

        float m_fontSize =
            18.0f;
    };
}