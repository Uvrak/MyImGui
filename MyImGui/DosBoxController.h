#pragma once

#include <string>

namespace MyImGui
{
    class ExternalWindow;

    enum class KeyboardLayout
    {
        US,
        German
    };

    class DosBoxController
    {
    public:
        void setKeyboardLayout(
            ExternalWindow& externalWindow,
            KeyboardLayout layout
        );

        void clearCommandLine(
            ExternalWindow& externalWindow
        );

        bool sendDosKey(
            ExternalWindow& externalWindow,
            const char* key
        );

        bool sendDosText(
            ExternalWindow& externalWindow,
            const std::string& text
        );

        bool openGame(
            ExternalWindow& externalWindow,
            const std::string& mountDirectory,
            const std::string& dosDirectory,
            const std::string& gameFilename
        );

    private:
        KeyboardLayout m_keyboardLayout =
            KeyboardLayout::German;
    };
}