#pragma once

#include <string>

namespace MyImGui
{
    class ExternalWindow;
    class DosBoxFrameReader;
    class DosBoxFrameTexture;
    class DosBoxKeyboard;
    class DosBoxMouse;

    class DosBoxView
    {
    public:
        void draw(
            ExternalWindow& externalWindow,
            DosBoxFrameReader& frameReader,
            DosBoxFrameTexture& frameTexture,
            DosBoxKeyboard& keyboard,
            DosBoxMouse& mouse,
            const std::string& gameFilename
        );

    private:
        bool m_inputActive = false;
    };
}