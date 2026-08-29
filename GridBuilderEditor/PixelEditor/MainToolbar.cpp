#include "MainToolbar.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "IconsFontAwesome7.h"
#include <filesystem>
#include <cstdio>

MainToolbar::MainToolbar()
    : m_buttonBar(
        {
            {
                static_cast<int>(
                    EditorCommand::NewImage
                ),
                ICON_FA_FILE,
                "New",
                32.0f,
                28.0f
            },
            {
                static_cast<int>(
                    EditorCommand::SaveImage
                ),
                ICON_FA_DOWNLOAD,
                "Save",
                32.0f,
                28.0f
            },
        }
    )
{}

EditorCommand MainToolbar::draw(
    const std::vector<std::string>& recentFiles,
    RecentFileRequest& recentRequest,
    const std::string& currentFilename
)
{
    EditorCommand command =
        EditorCommand::None;

    int selectedValue =
            static_cast<int>(
                EditorCommand::None
                );

        if (m_buttonBar.draw(selectedValue))
        {
            command =
                static_cast<EditorCommand>(
                    selectedValue
                    );
        }
        ImGui::SameLine();

        ImGui::SetNextItemWidth(180.0f);

        std::string currentDisplayName =
            "Open";

        if (!currentFilename.empty())
        {
            currentDisplayName =
                std::filesystem::path(
                    currentFilename
                ).filename().string();
        }

        const bool recentFilesOpen =
            ImGui::BeginCombo(
                "##RecentFiles",
                currentDisplayName.c_str()
            );

        const bool currentFileRightClicked =
            !currentFilename.empty() &&
            ImGui::IsItemHovered() &&
            ImGui::IsMouseClicked(
                ImGuiMouseButton_Right
            );

        if (recentFilesOpen)
        {
            if (ImGui::Selectable("Open..."))
            {
                command =
                    EditorCommand::OpenImage;
            }

            ImGui::Separator();

            if (recentFiles.empty())
            {
                ImGui::TextDisabled(
                    "No recent files"
                );
            }
            else
            {
                for (std::size_t index = 0;
                    index < recentFiles.size();
                    ++index)
                {
                    const std::string& filename =
                        recentFiles[index];
                    const std::filesystem::path filePath(
                        filename
                    );

                    const std::string displayName =
                        filePath.filename().string() +
                        "  (" +
                        filePath.parent_path().filename().string() +
                        ")";
                    ImGui::PushID(
                        static_cast<int>(index)
                    );

                    if (ImGui::Selectable(
                        displayName.c_str()
                    ))
                    {
                        recentRequest.action =
                            RecentFileAction::Open;

                        recentRequest.filename =
                            filename;
                    }

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip(
                            "%s",
                            filename.c_str()
                        );
                    }

                    if (ImGui::BeginPopupContextItem(
                        "##RecentFileContext"
                    ))
                    {
                        if (ImGui::MenuItem(
                            "Rename..."
                        ))
                        {
                            m_pendingRenameFile =
                                filename;

                            const std::string currentName =
                                std::filesystem::path(
                                    filename
                                ).stem().string();

                            std::snprintf(
                                m_renameFileName,
                                sizeof(m_renameFileName),
                                "%s",
                                currentName.c_str()
                            );

                            m_openRenamePopup =
                                true;
                        }

                        if (ImGui::MenuItem(
                            "Remove from list"
                        ))
                        {
                            recentRequest.action =
                                RecentFileAction::Remove;

                            recentRequest.filename =
                                filename;
                        }

                        if (ImGui::MenuItem(
                            "Delete file..."
                        ))
                        {
                            m_pendingDeleteFile =
                                filename;

                            m_openDeletePopup = true;
                        }

                        ImGui::EndPopup();
                    }

                    ImGui::PopID();
                }
            }

            ImGui::EndCombo();
        }
            if (currentFileRightClicked)
            {
                ImGui::OpenPopup(
                    "##CurrentFileContext"
                );
            }

            if (ImGui::BeginPopup(
                "##CurrentFileContext"
            ))
            {
                if (ImGui::MenuItem(
                    "Delete file..."
                ))
                {
                    m_pendingDeleteFile =
                        currentFilename;

                    m_openDeletePopup = true;
                }

                ImGui::EndPopup();
            }

            if (m_openRenamePopup)
            {
                ImGui::OpenPopup(
                    "Rename file"
                );

                m_openRenamePopup =
                    false;
            }

            if (ImGui::BeginPopupModal(
                "Rename file",
                nullptr,
                ImGuiWindowFlags_AlwaysAutoResize
            ))
            {
                ImGui::TextUnformatted(
                    "New file name:"
                );

                ImGui::SetNextItemWidth(
                    300.0f
                );

                ImGui::InputText(
                    "##RenameFileName",
                    m_renameFileName,
                    sizeof(m_renameFileName)
                );

                ImGui::Spacing();

                const bool hasNewName =
                    m_renameFileName[0] != '\0';

                if (!hasNewName)
                {
                    ImGui::BeginDisabled();
                }

                if (ImGui::Button("Rename"))
                {
                    recentRequest.action =
                        RecentFileAction::Rename;

                    recentRequest.filename =
                        m_pendingRenameFile;

                    recentRequest.newName =
                        m_renameFileName;

                    m_pendingRenameFile.clear();
                    m_renameFileName[0] = '\0';

                    ImGui::CloseCurrentPopup();
                }

                if (!hasNewName)
                {
                    ImGui::EndDisabled();
                }

                ImGui::SameLine();

                if (ImGui::Button("Cancel"))
                {
                    m_pendingRenameFile.clear();
                    m_renameFileName[0] = '\0';

                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        if (m_openDeletePopup)
        {
            ImGui::OpenPopup(
                "Delete file?"
            );

            m_openDeletePopup = false;
        }

        if (ImGui::BeginPopupModal(
            "Delete file?",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize
        ))
        {
            ImGui::TextWrapped(
                "Really delete this file?"
            );

            ImGui::TextWrapped(
                "%s",
                m_pendingDeleteFile.c_str()
            );

            ImGui::Separator();

            if (ImGui::Button("Delete"))
            {
                recentRequest.action =
                    RecentFileAction::Delete;

                recentRequest.filename =
                    m_pendingDeleteFile;

                m_pendingDeleteFile.clear();

                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel"))
            {
                m_pendingDeleteFile.clear();

                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        return command;
    }