#pragma once

#include <string>

namespace MyImGui
{
    class NamedPipeClient;
    class DosBoxFrameReader;
    class DosBoxFrameTexture;
    class DosBoxKeyboard;
    class DosBoxMouse;

    class DosBoxView
    {
    public:
        void draw(
            NamedPipeClient& NamedPipeClient,
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