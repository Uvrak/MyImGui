#include "pch.h"
#include "MainMenu.h"
#include "imgui.h"
#include <filesystem>
#include <shobjidl.h>

#include <Windows.h>
#include <commdlg.h>

#pragma comment(lib, "Comdlg32.lib")

#include <shlobj.h>
#pragma comment(lib, "Shell32.lib")

namespace MyImGui
{
    void MainMenu::draw()
    {
        if (!ImGui::BeginMainMenuBar())
        {
            return;
        }

        if (ImGui::BeginMenu("Game"))
        {
            if (ImGui::MenuItem("Open Game..."))
            {
                char filename[4096] = {};

                OPENFILENAMEA dialog{};
                dialog.lStructSize =
                    sizeof(dialog);

                dialog.lpstrFile =
                    filename;

                dialog.nMaxFile =
                    sizeof(filename);

                dialog.lpstrFilter =
                    "DOS Executables (*.exe)\0*.exe\0"
                    "All Files (*.*)\0*.*\0";

                dialog.nFilterIndex = 1;

                dialog.Flags =
                    OFN_FILEMUSTEXIST |
                    OFN_PATHMUSTEXIST |
                    OFN_NOCHANGEDIR;
                if (GetOpenFileNameA(&dialog))
                {
                    m_selectedGameExe =
                        filename;

                    std::filesystem::path gamePath(
                        m_selectedGameExe
                    );

                    m_gameDirectory =
                        gamePath.parent_path().string();

                    m_gameFilename =
                        gamePath.filename().string();

                    std::filesystem::path exeDirectory =
                        gamePath.parent_path();

                    std::filesystem::path detectedMount =
                        exeDirectory;

                    // Einen Ordner nach oben prüfen
                    std::filesystem::path parentDirectory =
                        exeDirectory.parent_path();

                    if (!parentDirectory.empty())
                    {
                        bool foundDosBoxSetup = false;

                        for (const auto& entry :
                            std::filesystem::directory_iterator(
                                parentDirectory
                            ))
                        {
                            // .conf direkt im Elternordner
                            if (entry.is_regular_file() &&
                                entry.path().extension() == ".conf")
                            {
                                foundDosBoxSetup = true;
                                break;
                            }

                            // Typischer DOSBox-Unterordner
                            if (entry.is_directory())
                            {
                                std::string directoryName =
                                    entry.path()
                                    .filename()
                                    .string();

                                if (directoryName.rfind(
                                    "dosbox_",
                                    0
                                ) == 0)
                                {
                                    foundDosBoxSetup = true;
                                    break;
                                }
                            }
                        }

                        if (foundDosBoxSetup)
                        {
                            detectedMount =
                                parentDirectory;
                        }
                    }

                    m_mountDirectory =
                        detectedMount.string();

                    std::filesystem::path relativeDirectory =
                        std::filesystem::relative(
                            exeDirectory,
                            detectedMount
                        );

                    if (relativeDirectory == ".")
                    {
                        m_dosDirectory.clear();
                    }
                    else
                    {
                        m_dosDirectory =
                            relativeDirectory.string();
                    }
                    printf(
                        "Game directory: %s\n",
                        m_gameDirectory.c_str()
                    );

                    printf(
                        "Game executable: %s\n",
                        m_gameFilename.c_str()
                    );
					m_startGameRequested = true;
                }
            }
            
            bool canStartGame =
                !m_selectedGameExe.empty() &&
                !m_mountDirectory.empty();

            if (ImGui::MenuItem(
                "Start Game",
                nullptr,
                false,
                canStartGame
            ))
            {
                // DOSBox-Start kommt als Nächstes
               
            }
            

            if (!m_selectedGameExe.empty())
            {
                ImGui::Separator();

                ImGui::TextWrapped(
                    "%s",
                    m_selectedGameExe.c_str()
                );
            }

            if (!m_gameDirectory.empty())
            {
                ImGui::TextWrapped(
                    "Directory: %s",
                    m_gameDirectory.c_str()
                );

                ImGui::Text(
                    "Executable: %s",
                    m_gameFilename.c_str()
                );
            }

            if (!m_mountDirectory.empty())
            {
                ImGui::TextWrapped(
                    "Mount: %s",
                    m_mountDirectory.c_str()
                );
            }

            if (!m_dosDirectory.empty())
            {
                ImGui::Text(
                    "DOS Directory: %s",
                    m_dosDirectory.c_str()
                );
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("DOSBox"))
        {
            if (ImGui::BeginMenu(
                "Keyboard Layout"
            ))
            {
                if (ImGui::MenuItem(
                    "German",
                    nullptr,
                    m_germanKeyboardLayoutSelected
                ))
                {
                    m_germanKeyboardLayoutRequested =
                        true;

                    m_germanKeyboardLayoutSelected =
                        true;
                }

                if (ImGui::MenuItem(
                    "US",
                    nullptr,
                    !m_germanKeyboardLayoutSelected
                ))
                {
                    m_usKeyboardLayoutRequested =
                        true;

                    m_germanKeyboardLayoutSelected =
                        false;
                }
                {
                    m_usKeyboardLayoutRequested =
                        true;
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(
            "Settings"
        ))
        {
            if (ImGui::MenuItem(
                "Appearance..."
            ))
            {
                m_openSettingsRequested =
                    true;
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
    
    const std::string&
        MainMenu::selectedGameExe() const
    {
        return m_selectedGameExe;
    }

    bool MainMenu::consumeStartGameRequest()
    {
        if (!m_startGameRequested)
        {
            return false;
        }

        m_startGameRequested = false;

        return true;
    }
    const std::string&
        MainMenu::mountDirectory() const
    {
        return m_mountDirectory;
    }

    const std::string&
        MainMenu::dosDirectory() const
    {
        return m_dosDirectory;
    }

    const std::string&
        MainMenu::gameFilename() const
    {
        return m_gameFilename;
    }

   bool MainMenu::consumeGermanKeyboardLayoutRequest()
{
    if (!m_germanKeyboardLayoutRequested)
    {
        return false;
    }

    m_germanKeyboardLayoutRequested = false;

    return true;
}

bool MainMenu::consumeUSKeyboardLayoutRequest()
{
    if (!m_usKeyboardLayoutRequested)
    {
        return false;
    }

    m_usKeyboardLayoutRequested = false;

    return true;
}

bool MainMenu::
consumeOpenSettingsRequest()
{
    if (!m_openSettingsRequested)
    {
        return false;
    }

    m_openSettingsRequested =
        false;

    return true;
}


}