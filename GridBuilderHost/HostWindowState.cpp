#include "HostWindowState.h"

#include <filesystem>
#include <fstream>

namespace GridBuilderHost
{
    HostWindowState::HostWindowState()
    {}

    bool HostWindowState::load()
    {
        std::ifstream file(
            m_filename
        );

        if (!file)
        {
            return false;
        }

        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;

        if (!(file >>
            x >>
            y >>
            width >>
            height))
        {
            return false;
        }

        if (width <= 0 ||
            height <= 0)
        {
            return false;
        }

        m_x = x;
        m_y = y;
        m_width = width;
        m_height = height;

        return true;
    }

    bool HostWindowState::save() const
    {
        const std::filesystem::path path(
            m_filename
        );

        if (path.has_parent_path())
        {
            std::filesystem::create_directories(
                path.parent_path()
            );
        }

        std::ofstream file(
            m_filename
        );

        if (!file)
        {
            return false;
        }

        file <<
            m_x << ' ' <<
            m_y << ' ' <<
            m_width << ' ' <<
            m_height << '\n';

        return true;
    }

    int HostWindowState::x() const
    {
        return m_x;
    }

    int HostWindowState::y() const
    {
        return m_y;
    }

    int HostWindowState::width() const
    {
        return m_width;
    }

    int HostWindowState::height() const
    {
        return m_height;
    }

    void HostWindowState::setPosition(
        int x,
        int y
    )
    {
        m_x = x;
        m_y = y;
    }

    void HostWindowState::setSize(
        int width,
        int height
    )
    {
        m_width = width;
        m_height = height;
    }
}