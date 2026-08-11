#pragma once

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
            DosBoxMouse& mouse
        );

    private:
        bool m_inputActive = false;
    };
}