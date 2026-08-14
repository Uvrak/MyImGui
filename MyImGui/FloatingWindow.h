#pragma once
#pragma once

#include <string>

namespace MyImGui
{
    struct FloatingWindowOptions
    {
        bool movable = true;
        bool resizable = true;
        bool collapsible = true;
        bool closable = false;
        bool titleBar = true;
        bool autoResizeHeight = false;
        bool dockable = true;
    };

    class FloatingWindow
    {
    public:
        explicit FloatingWindow(
            std::string title,
            FloatingWindowOptions options = {}
        );

        bool begin();
        void end();

        bool isOpen() const;
        void open();
        void close();

    private:
        std::string m_title;
        FloatingWindowOptions m_options;

        bool m_isOpen = true;
        bool m_hasBegun = false;
        bool m_contentVisible = false;
        bool m_heightManuallyResized = false;
    };
}