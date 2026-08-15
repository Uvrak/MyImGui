#pragma once

#include <string>

namespace MyImGui
{
    class NamedPipeClient;
    class DosBoxFrameReader;
    class DosBoxFrameTexture;
    class DosBoxKeyboard;
    class DosBoxMouse;

    enum class DosBoxInputMode
    {
        Focused,
        AlwaysActive
    };

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
        DosBoxInputMode m_inputMode =
            DosBoxInputMode::AlwaysActive;
        bool m_inputActive = true;
    };
}