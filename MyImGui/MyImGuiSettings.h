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

        float fontScale() const;

    private:
        void load();
        void save() const;

        float m_fontScale = 1.0f;
    };
}