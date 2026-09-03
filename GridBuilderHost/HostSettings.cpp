#include "HostSettings.h"

#include <filesystem>
#include <fstream>

namespace GridBuilderHost
{
    HostSettings::HostSettings()
    {
        load();
    }

    HostSettings::~HostSettings()
    {
        save();
    }

    void HostSettings::load()
    {
        std::ifstream file(
            "../settings/gridbuilder_host.cfg"
        );

        if (!file)
        {
            return;
        }

        file >>
            m_fontSize;
    }

    void HostSettings::save() const
    {
        std::filesystem::create_directories(
            "../settings"
        );

        std::ofstream file(
            "../settings/gridbuilder_host.cfg"
        );

        if (!file)
        {
            return;
        }

        file <<
            m_fontSize;
    }

    float HostSettings::fontSize() const
    {
        return m_fontSize;
    }

    void HostSettings::setFontSize(
        float fontSize
    )
    {
        m_fontSize =
            fontSize;
    }
}