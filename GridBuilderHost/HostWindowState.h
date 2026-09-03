#pragma once

#include <string>

namespace GridBuilderHost
{
    class HostWindowState
    {
    public:
        HostWindowState();

        bool load();
        bool save() const;

        int x() const;
        int y() const;
        int width() const;
        int height() const;

        void setPosition(
            int x,
            int y
        );

        void setSize(
            int width,
            int height
        );

    private:
        std::string m_filename =
            "../settings/gridbuilder_host_window.cfg";

        int m_x = 100;
        int m_y = 100;

        int m_width = 1280;
        int m_height = 720;
    };
}