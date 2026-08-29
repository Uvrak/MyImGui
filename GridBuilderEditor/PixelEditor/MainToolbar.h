#pragma once

#include "EditorCommand.h"
#include "IconButtonBar.h"
#include <string>
#include <vector>

enum class RecentFileAction
{
    None,
    Open,
    Remove,
	Delete,
	Rename
};

struct RecentFileRequest
{
    RecentFileAction action =
        RecentFileAction::None;

    std::string filename;
    std::string newName;
};

class MainToolbar
{
public:
    MainToolbar();

    EditorCommand draw(
        const std::vector<std::string>& recentFiles,
        RecentFileRequest& recentRequest,
        const std::string& currentFilename
    );

private:
    IconButtonBar m_buttonBar;

    std::string m_pendingDeleteFile;
    bool m_openDeletePopup = false;

    std::string m_pendingRenameFile;
    bool m_openRenamePopup = false;

    char m_renameFileName[256] =
    {};
};