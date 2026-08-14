#pragma once

#include "MyImGuiSettings.h"

namespace MyImGui
{
    class MyImGuiSettingsWindow
    {
    public:
        MyImGuiSettingsWindow(
            MyImGuiSettings& settings
        );

        void draw(
            bool* isOpen = nullptr
        );

    private:
        MyImGuiSettings&
            m_settings;
    };
}