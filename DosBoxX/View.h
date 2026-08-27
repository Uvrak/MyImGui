#pragma once

#include <string>

namespace DosBoxX
{
    class NamedPipeClient;
    class FrameReader;
    class FrameTexture;
    class Keyboard;
    class Mouse;

    enum class DosBoxInputMode
    {
        Focused,
        AlwaysActive
    };

    class View
   
    {
    public:
        void draw(
            NamedPipeClient& NamedPipeClient,
            FrameReader& frameReader,
            FrameTexture& frameTexture,
            Keyboard& keyboard,
            Mouse& mouse,
            const std::string& gameFilename
        );

        void requestRefresh();

    private:
        DosBoxInputMode m_inputMode =
            DosBoxInputMode::AlwaysActive;
        bool m_inputActive = true;

        bool m_refreshRequested = false;
    };
}