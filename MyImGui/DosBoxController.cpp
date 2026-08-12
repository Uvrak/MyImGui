#include "pch.h"
#include "DosBoxController.h"

#include "NamedPipeClient.h"

#include <filesystem>
#include <string>
#include <Windows.h>

namespace MyImGui
{
    void DosBoxController::setKeyboardLayout(
        NamedPipeClient& pipeClient,
        KeyboardLayout layout
    )
    {
        m_keyboardLayout =
            layout;

        if (layout ==
            KeyboardLayout::German)
        {
            pipeClient.send(
                "KEYBOARD_LAYOUT:GR"
            );
        }
        else
        {
            pipeClient.send(
                "KEYBOARD_LAYOUT:US"
            );
        }
    }

    void DosBoxController::clearCommandLine(
        NamedPipeClient& pipeClient
    )
    {
        pipeClient.send(
            "KEYDOWN:CTRL"
        );

        pipeClient.send(
            "KEYDOWN:C"
        );

        pipeClient.send(
            "KEYUP:C"
        );

        pipeClient.send(
            "KEYUP:CTRL"
        );
    }

    bool DosBoxController::sendDosKey(
        NamedPipeClient& pipeClient,
        const char* key
    )
    {
        std::string keyDown =
            "KEYDOWN:";

        keyDown += key;

        if (!pipeClient.send(
            keyDown
        ))
        {
            return false;
        }

        Sleep(50);

        std::string keyUp =
            "KEYUP:";

        keyUp += key;

        if (!pipeClient.send(
            keyUp
        ))
        {
            return false;
        }

        Sleep(50);

        return true;
    }

    bool DosBoxController::sendDosText(
        NamedPipeClient& pipeClient,
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

                if (m_keyboardLayout ==
                    KeyboardLayout::German)
                {
                    if (upper == 'Y')
                        upper = 'Z';
                    else if (upper == 'Z')
                        upper = 'Y';
                }

                char key[2] =
                {
                    upper,
                    '\0'
                };

                if (!sendDosKey(
                    pipeClient,
                    key
                ))
                {
                    return false;
                }
            }
            else if (ch >= 'A' && ch <= 'Z')
            {
                char upper = ch;

                if (m_keyboardLayout ==
                    KeyboardLayout::German)
                {
                    if (upper == 'Y')
                        upper = 'Z';
                    else if (upper == 'Z')
                        upper = 'Y';
                }

                char key[2] =
                {
                    upper,
                    '\0'
                };

                if (!sendDosKey(
                    pipeClient,
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
                    pipeClient,
                    key
                ))
                {
                    return false;
                }
            }
            else if (ch == ':')
            {
                pipeClient.send(
                    "KEYDOWN:SHIFT"
                );

                Sleep(10);

                if (m_keyboardLayout ==
                    KeyboardLayout::German)
                {
                    sendDosKey(
                        pipeClient,
                        "PERIOD"
                    );
                }
                else
                {
                    sendDosKey(
                        pipeClient,
                        "SEMICOLON"
                    );
                }

                pipeClient.send(
                    "KEYUP:SHIFT"
                );

                Sleep(10);
            }
            else if (ch == '\\')
            {
                if (m_keyboardLayout ==
                    KeyboardLayout::German)
                {
                    pipeClient.send(
                        "KEYDOWN:ALTGR"
                    );

                    Sleep(10);

                    sendDosKey(
                        pipeClient,
                        "MINUS"
                    );

                    pipeClient.send(
                        "KEYUP:ALTGR"
                    );

                    Sleep(10);
                }
                else
                {
                    sendDosKey(
                        pipeClient,
                        "BACKSLASH"
                    );
                }
            }
            else if (ch == ' ')
            {
                sendDosKey(
                    pipeClient,
                    "SPACE"
                );
            }
            else if (ch == '"')
            {
                pipeClient.send(
                    "KEYDOWN:SHIFT"
                );

                Sleep(10);

                sendDosKey(
                    pipeClient,
                    "2"
                );

                pipeClient.send(
                    "KEYUP:SHIFT"
                );

                Sleep(10);
            }
            else if (ch == '.')
            {
                sendDosKey(
                    pipeClient,
                    "PERIOD"
                );
            }
            else if (ch == '/')
            {
                if (m_keyboardLayout ==
                    KeyboardLayout::German)
                {
                    pipeClient.send(
                        "KEYDOWN:SHIFT"
                    );

                    Sleep(10);

                    sendDosKey(
                        pipeClient,
                        "7"
                    );

                    pipeClient.send(
                        "KEYUP:SHIFT"
                    );

                    Sleep(10);
                }
                else
                {
                    sendDosKey(
                        pipeClient,
                        "SLASH"
                    );
                }
                }
            else if (ch == '-')
            {
                if (m_keyboardLayout ==
                    KeyboardLayout::German)
                {
                    sendDosKey(
                        pipeClient,
                        "SLASH"
                    );
                }
                else
                {
                    sendDosKey(
                        pipeClient,
                        "MINUS"
                    );
                }
            }

            else if (ch == '#')
            {
                if (m_keyboardLayout ==
                    KeyboardLayout::German)
                {
                    sendDosKey(
                        pipeClient,
                        "BACKSLASH"
                    );
                }
                else
                {
                    pipeClient.send(
                        "KEYDOWN:SHIFT"
                    );

                    Sleep(10);

                    sendDosKey(
                        pipeClient,
                        "3"
                    );

                    pipeClient.send(
                        "KEYUP:SHIFT"
                    );

                    Sleep(10);
                }
                }
        }

        return true;
    }

    bool DosBoxController::openGame(
        NamedPipeClient& pipeClient,
        const std::string& mountDirectory,
        const std::string& dosDirectory,
        const std::string& gameFilename
    )
    {
        clearCommandLine(
            pipeClient
        );

        Sleep(100);

        if (!sendDosText(
            pipeClient,
            "MOUNT C -U"
        ))
        {
            OutputDebugStringA(
                "openGame failed: unmount text\n"
            );
            return false;
        }

        if (!sendDosKey(
            pipeClient,
            "ENTER"
        ))
        {
            OutputDebugStringA(
                "openGame failed: unmount text\n"
            );
           
            OutputDebugStringA(
                "openGame failed: unmount text\n"
            );
            return false;
        }

        Sleep(100);
        const std::string mountCommand =
            "MOUNT C \"" +
            mountDirectory +
            "\"";

        if (!sendDosText(
            pipeClient,
            mountCommand
        ))
        {
            OutputDebugStringA(
                "openGame failed: unmount text\n"
            );
            return false;
        }

        if (!sendDosKey(
            pipeClient,
            "ENTER"
        ))
        {
            OutputDebugStringA(
                "openGame failed: unmount text\n"
            );
            return false;
        }

        Sleep(100);

        if (!sendDosText(
            pipeClient,
            "C:"
        ))
        {
            OutputDebugStringA(
                "openGame failed: unmount text\n"
            );
            return false;
        }

        if (!sendDosKey(
            pipeClient,
            "ENTER"
        ))
        {
            return false;
        }

        Sleep(100);

        if (!sendDosText(
            pipeClient,
            "CD \\"
        ))
        {
            return false;
        }

        if (!sendDosKey(
            pipeClient,
            "ENTER"
        ))
        {
            return false;
        }

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

            if (!sendDosText(
                pipeClient,
                directoryCommand
            ))
            {
                return false;
            }

            if (!sendDosKey(
                pipeClient,
                "ENTER"
            ))
            {
                return false;
            }
        }

        Sleep(100);

        if (!sendDosText(
            pipeClient,
            gameFilename
        ))
        {
            return false;
        }

        if (!sendDosKey(
            pipeClient,
            "ENTER"
        ))
        {
            return false;
        }

        return true;
    }
}
