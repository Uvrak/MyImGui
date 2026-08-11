#include "pch.h"
#include "DosBoxController.h"

#include "ExternalWindow.h"

#include <filesystem>
#include <string>
#include <Windows.h>

namespace MyImGui
{
    void DosBoxController::setKeyboardLayout(
        KeyboardLayout layout
    )
    {
        m_keyboardLayout =
            layout;
    }

    void DosBoxController::clearCommandLine(
        ExternalWindow& externalWindow
    )
    {
        externalWindow.sendIpcCommand(
            "KEYDOWN:CTRL"
        );

        externalWindow.sendIpcCommand(
            "KEYDOWN:C"
        );

        externalWindow.sendIpcCommand(
            "KEYUP:C"
        );

        externalWindow.sendIpcCommand(
            "KEYUP:CTRL"
        );
    }

    bool DosBoxController::sendDosKey(
        ExternalWindow& externalWindow,
        const char* key
    )
    {
        std::string keyDown =
            "KEYDOWN:";

        keyDown += key;

        if (!externalWindow.sendIpcCommand(
            keyDown
        ))
        {
            return false;
        }

        Sleep(50);

        std::string keyUp =
            "KEYUP:";

        keyUp += key;

        if (!externalWindow.sendIpcCommand(
            keyUp
        ))
        {
            return false;
        }

        Sleep(50);

        return true;
    }

    bool DosBoxController::sendDosText(
        ExternalWindow& externalWindow,
        const std::string& text
    )
    {
        for (char ch : text)
        {
            if (ch >= 'a' && ch <= 'z')
            {
                char upper =
                    static_cast<char>(
                        ch - 'a' + 'A'
                        );

                // German keyboard layout:
                // physical Y/Z positions are swapped
                if (upper == 'Y')
                    upper = 'Z';
                else if (upper == 'Z')
                    upper = 'Y';

                char key[2] =
                {
                    upper,
                    '\0'
                };

                if (!sendDosKey(
                    externalWindow,
                    key
                ))
                {
                    return false;
                }
            }
            else if (ch >= 'A' && ch <= 'Z')
            {
                char upper = ch;

                if (upper == 'Y')
                    upper = 'Z';
                else if (upper == 'Z')
                    upper = 'Y';

                char key[2] =
                {
                    upper,
                    '\0'
                };

                if (!sendDosKey(
                    externalWindow,
                    key
                ))
                {
                    return false;
                }
            }
            else if (ch >= '0' && ch <= '9')
            {
                char key[2] =
                {
                    ch,
                    '\0'
                };

                if (!sendDosKey(
                    externalWindow,
                    key
                ))
                {
                    return false;
                }
            }
            else if (ch == ':')
            {
                externalWindow.sendIpcCommand(
                    "KEYDOWN:SHIFT"
                );

                Sleep(50);

                sendDosKey(
                    externalWindow,
                    "PERIOD"
                );

                externalWindow.sendIpcCommand(
                    "KEYUP:SHIFT"
                );

                Sleep(10);
            }
            else if (ch == '\\')
            {
                externalWindow.sendIpcCommand(
                    "KEYDOWN:ALTGR"
                );

                Sleep(10);

                sendDosKey(
                    externalWindow,
                    "MINUS"
                );

                externalWindow.sendIpcCommand(
                    "KEYUP:ALTGR"
                );

                Sleep(10);
            }
            else if (ch == ' ')
            {
                sendDosKey(
                    externalWindow,
                    "SPACE"
                );
            }
            else if (ch == '"')
            {
                externalWindow.sendIpcCommand(
                    "KEYDOWN:SHIFT"
                );

                Sleep(10);

                sendDosKey(
                    externalWindow,
                    "2"
                );

                externalWindow.sendIpcCommand(
                    "KEYUP:SHIFT"
                );

                Sleep(10);
            }
            else if (ch == '.')
            {
                sendDosKey(
                    externalWindow,
                    "PERIOD"
                );
            }
            else if (ch == '/')
            {
                externalWindow.sendIpcCommand(
                    "KEYDOWN:SHIFT"
                );

                Sleep(10);

                sendDosKey(
                    externalWindow,
                    "7"
                );

                externalWindow.sendIpcCommand(
                    "KEYUP:SHIFT"
                );

                Sleep(10);
            }
            else if (ch == '-')
            {
                if (m_keyboardLayout ==
                    KeyboardLayout::German)
                {
                    sendDosKey(
                        externalWindow,
                        "SLASH"
                    );
                }
                else
                {
                    sendDosKey(
                        externalWindow,
                        "MINUS"
                    );
                }
            }
        }

        return true;
    }

    void DosBoxController::openGame(
        ExternalWindow& externalWindow,
        const std::string& mountDirectory,
        const std::string& dosDirectory,
        const std::string& gameFilename
    )
    {
        clearCommandLine(
            externalWindow
        );

        Sleep(100);

        sendDosText(
            externalWindow,
            "MOUNT C -U"
        );

        sendDosKey(
            externalWindow,
            "ENTER"
        );

        Sleep(100);
        const std::string mountCommand =
            "MOUNT C \"" +
            mountDirectory +
            "\"";

        sendDosText(
            externalWindow,
            mountCommand
        );

        sendDosKey(
            externalWindow,
            "ENTER"
        );

        Sleep(100);

        sendDosText(
            externalWindow,
            "C:"
        );

        sendDosKey(
            externalWindow,
            "ENTER"
        );

        Sleep(100);

        sendDosText(
            externalWindow,
            "CD \\"
        );

        sendDosKey(
            externalWindow,
            "ENTER"
        );

        Sleep(100);

        if (!dosDirectory.empty())
        {
            OutputDebugStringA(
                "DOS directory in controller: ["
            );

            OutputDebugStringA(
                dosDirectory.c_str()
            );

            OutputDebugStringA(
                "]\n"
            );

            const std::string directoryCommand =
                "CD " +
                dosDirectory;

            sendDosText(
                externalWindow,
                directoryCommand
            );

            sendDosKey(
                externalWindow,
                "ENTER"
            );
        }

        Sleep(100);

        sendDosText(
            externalWindow,
            gameFilename
        );

        sendDosKey(
            externalWindow,
            "ENTER"
        );
    }
}
