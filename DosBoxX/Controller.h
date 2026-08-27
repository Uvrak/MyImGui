#pragma once

#include <string>

namespace DosBoxX
{
    class NamedPipeClient;

    enum class KeyboardLayout
    {
        US,
        German
    };

    class Controller
    {
    public:
        void closeExistingInstances();
        void setKeyboardLayout(
            NamedPipeClient& pipeClient,
            KeyboardLayout layout
        );

        void clearCommandLine(
            NamedPipeClient& pipeClient
        );

        bool sendDosKey(
            NamedPipeClient& pipeClient,
            const char* key
        );

        bool sendDosText(
            NamedPipeClient& pipeClient,
            const std::string& text
        );

        bool openGame(
            NamedPipeClient& pipeClient,
            const std::string& mountDirectory,
            const std::string& dosDirectory,
            const std::string& gameFilename
        );

    private:
        KeyboardLayout m_keyboardLayout =
            KeyboardLayout::German;
    };
}