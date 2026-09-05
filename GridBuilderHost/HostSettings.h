#pragma once

namespace GridBuilderHost
{
    class HostSettings
    {
    public:
        HostSettings();
        ~HostSettings();

        void load();
        void save() const;

        float fontSize() const;
        void setFontSize(
            float fontSize
        );

        bool showDosBoxCoordinates() const;

        void setShowDosBoxCoordinates(
            bool show
        );

    private:
        float m_fontSize =
            18.0f;

        bool m_showDosBoxCoordinates =
            false;
    };
}