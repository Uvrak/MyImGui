#include "GridBuilderGrid.h"

#include "WorldViewWindow.h"
#include "EditorToolBox.h"
#include "EditorEdgeBox.h"
#include "EditorMiscBox.h"
#include "ChunkManager.h"
#include "MapSerializer.h"

#include <utility>
#include <unordered_map>
#include <filesystem>
#include <algorithm>

#include "imgui.h"

class GridBuilderGrid::Impl
{
public:
    Impl(
        ID3D11Device* device,
        int chunkSize, const std::string& iconDirectory
    )
        :
        worldViewWindow(
            chunkSize
        ),
        toolBox(),
        edgeBox(
            device, iconDirectory.empty() ? std::filesystem::path{} : std::filesystem::path(iconDirectory)/"edges"
        ),
        miscBox(
            device, iconDirectory.empty() ? std::filesystem::path{} : std::filesystem::path(iconDirectory)/"misc"
        )
    {
        miscBox.clearActiveMisc();
    }

    enum class PaintTarget
    {
        Edge,
        Misc
    };

    WorldViewWindow worldViewWindow;
    std::unordered_map<std::string, std::string> mapFiles;
    std::filesystem::path mapDirectory;
    std::string currentMapKey;
    EditorToolbox toolBox;
    EditorEdgeBox edgeBox;
    EditorMiscBox miscBox;

    bool groundOnly = false;
    bool editorToolsEnabled = true;
    PaintTarget paintTarget =
        PaintTarget::Edge;
};

GridBuilderGrid::GridBuilderGrid(
    ID3D11Device* device,
    int chunkSize, const std::string& iconDirectory
)
    :
    m_impl(
        std::make_unique<Impl>(
            device,
            chunkSize, iconDirectory
        )
    )
{}

GridBuilderGrid::~GridBuilderGrid() =
default;

GroundViewState GridBuilderGrid::groundView() const { return m_impl->worldViewWindow.groundView(); }
void GridBuilderGrid::setGroundView(GroundViewState view) { m_impl->worldViewWindow.setGroundView(view); }
void GridBuilderGrid::setGroundCell(int x,int y,GroundMaterial material) { m_impl->worldViewWindow.setGroundCell(x,y,material); }
void GridBuilderGrid::setGroundBorder(int x,int y,std::uint8_t border) { m_impl->worldViewWindow.setGroundBorder(x,y,border); }
void GridBuilderGrid::setGroundDropCallback(GroundDropCallback callback) { m_impl->worldViewWindow.setGroundDropCallback(std::move(callback)); }
void GridBuilderGrid::setGroundNavigationCallback(GroundNavigationCallback callback) { m_impl->worldViewWindow.setGroundNavigationCallback(std::move(callback)); }

void GridBuilderGrid::setGroundLayer(GroundLayer layer, const std::string& title)
{
    m_impl->worldViewWindow.setGroundLayer(std::move(layer), title);
    m_impl->groundOnly = true;
    setEditorToolsEnabled(false);
    m_impl->toolBox.setActiveTool(EditorTool::Scroll);
}

void GridBuilderGrid::setEditorTextureResolver(EditorTextureResolver resolver)
{
    m_impl->toolBox.setTextLabels(bool(resolver));
    m_impl->edgeBox.setTextureResolver(resolver);
    m_impl->miscBox.setTextureResolver(std::move(resolver));
}

void GridBuilderGrid::setEditorToolsEnabled(bool enabled)
{
    m_impl->editorToolsEnabled = enabled;
    m_impl->worldViewWindow.setEditorToolsEnabled(enabled);
}

void GridBuilderGrid::focusGroundCell(int x, int y)
{
    m_impl->worldViewWindow.focusGroundCell(x, y);
}

void GridBuilderGrid::registerMap(const std::string& key,
                                  const std::string& filename)
{
    m_impl->mapFiles[key] = filename;
}

void GridBuilderGrid::setMapDirectory(const std::string& directory)
{
    m_impl->mapDirectory = directory;
}

bool GridBuilderGrid::openMap(const std::string& key, const std::string& displayName)
{
    if (key == m_impl->currentMapKey)
    {
        m_impl->worldViewWindow.setMapName(displayName);
        return true;
    }

    const auto found = m_impl->mapFiles.find(key);
    std::filesystem::path target;
    if (found != m_impl->mapFiles.end())
        target = found->second;
    else
    {
        if (m_impl->mapDirectory.empty() || key.empty() ||
            !std::all_of(key.begin(), key.end(), [](unsigned char c) {
                return (c >= 'a' && c <= 'z') ||
                       (c >= 'A' && c <= 'Z') ||
                       (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '/';
            })) return false;
        std::string filename = displayName.empty()
            ? std::filesystem::path(key).filename().string()
            : displayName;
        for (char& character : filename)
        {
            if (static_cast<unsigned char>(character) < 32 ||
                std::string("<>:\"/\\|?*").find(character) != std::string::npos)
                character = '_';
        }
        while (!filename.empty() &&
               (filename.back() == ' ' || filename.back() == '.'))
            filename.pop_back();
        if (filename.empty() || filename == "." || filename == "..") return false;

        std::u8string utf8Filename;
        for (unsigned char byte : filename)
            utf8Filename.push_back(static_cast<char8_t>(byte));
        utf8Filename += u8".map";
        target = m_impl->mapDirectory / std::filesystem::path(key).parent_path() /
            std::filesystem::path(utf8Filename);

        // Preserve maps created by the earlier ID-based naming scheme.
        const auto legacy = m_impl->mapDirectory / (key + ".map");
        std::error_code lookupError;
        if (!std::filesystem::exists(target, lookupError) && !lookupError &&
            std::filesystem::exists(legacy, lookupError) && !lookupError)
        {
            std::filesystem::create_directories(target.parent_path(), lookupError);
            if (lookupError) return false;
            std::filesystem::rename(legacy, target, lookupError);
            if (lookupError) target = legacy;
        }
    }

    if (!saveCurrentMap()) return false;

    std::error_code error;
    const bool exists = std::filesystem::exists(target, error);
    if (error) return false;
    if (exists)
    {
        if (!m_impl->worldViewWindow.loadMap(target.string())) return false;
    }
    else
    {
        std::filesystem::create_directories(target.parent_path(), error);
        if (error) return false;
        ChunkManager emptyMap(m_impl->worldViewWindow.map().chunkSize());
        if (!MapSerializer::save(emptyMap, target.string()) ||
            !m_impl->worldViewWindow.loadMap(target.string())) return false;
    }
    m_impl->mapFiles[key] = target.string();
    m_impl->currentMapKey = key;
    m_impl->worldViewWindow.setMapName(displayName);
    return true;
}

bool GridBuilderGrid::saveCurrentMap()
{
    if (!m_impl->worldViewWindow.hasUnsavedChanges()) return true;
    const auto current = m_impl->mapFiles.find(m_impl->currentMapKey);
    if (current == m_impl->mapFiles.end()) return true;
    std::error_code error;
    const bool exists = std::filesystem::exists(current->second, error);
    if (error) return false;
    return !exists || m_impl->worldViewWindow.saveMap(current->second);
}

void GridBuilderGrid::setPlayerMarker(const MapPlayerMarker& marker)
{
    m_impl->worldViewWindow.setPlayerMarker(marker);
}

const ChunkManager& GridBuilderGrid::map() const
{
    return m_impl->worldViewWindow.map();
}

void GridBuilderGrid::draw(
    bool* isOpen
)
{
    if (isOpen && !*isOpen) return;
    if (m_impl->groundOnly && !m_impl->editorToolsEnabled) {
        m_impl->worldViewWindow.draw(EditorTool::Scroll, {}, {}, {}, {}, false,
            {}, {}, {}, {}, {}, false, true, isOpen);
        return;
    }
    const auto* viewport = ImGui::GetMainViewport();
    const float sideX = viewport->WorkPos.x + (std::max)(600.f, viewport->WorkSize.x - 290.f);
    if (m_impl->groundOnly) ImGui::SetNextWindowPos({sideX, viewport->WorkPos.y + 130}, ImGuiCond_FirstUseEver);
    // 1. EdgeBox zeichnen und Interaktionen verarbeiten
    if (m_impl->edgeBox.draw(
        m_impl->worldViewWindow.colorPalette(),
        [this](
            const std::string& edgeId
            )
        {
            return m_impl->worldViewWindow.edgeColorId(
                edgeId
            );
        },
        [this](
            const std::string& edgeId,
            const std::string& colorId
            )
        {
            m_impl->worldViewWindow.setEdgeColorId(
                edgeId,
                colorId
            );
        },
        [this](
            const std::string& colorId
            )
        {
            return m_impl->worldViewWindow.removeMapColor(
                colorId
            );
        }
    ))
    {
        m_impl->miscBox.clearActiveMisc();

        m_impl->paintTarget =
            Impl::PaintTarget::Edge;

        m_impl->toolBox.setActiveTool(
            EditorTool::Pencil
        );
    }

    if (m_impl->toolBox.activeTool() == EditorTool::Pencil &&
        m_impl->worldViewWindow.isMouseOverCanvas() &&
        !ImGui::GetIO().KeyCtrl)
    {
        m_impl->edgeBox.handleMouseWheel();
    }

    if (m_impl->groundOnly) ImGui::SetNextWindowPos({sideX, viewport->WorkPos.y + 430}, ImGuiCond_FirstUseEver);
    // 2. MiscBox zeichnen und Interaktionen verarbeiten
    if (m_impl->miscBox.draw(
        m_impl->worldViewWindow.colorPalette(),
        [this](
            const std::string& miscId
            )
        {
            return m_impl->worldViewWindow.miscColorId(
                miscId
            );
        },
        [this](
            const std::string& miscId,
            const std::string& colorId
            )
        {
            m_impl->worldViewWindow.setMiscColorId(
                miscId,
                colorId
            );
        },
        [this](
            const std::string& colorId
            )
        {
            return m_impl->worldViewWindow.removeMapColor(
                colorId
            );
        }
    ))
    {
        m_impl->edgeBox.clearActiveEdge();

        m_impl->paintTarget =
            Impl::PaintTarget::Misc;

        m_impl->toolBox.setActiveTool(
            EditorTool::Pencil
        );
    }

    if (m_impl->groundOnly) ImGui::SetNextWindowPos({sideX, viewport->WorkPos.y + 30}, ImGuiCond_FirstUseEver);
    m_impl->toolBox.draw(
        m_impl->toolBox.activeTool()
    );

    // 3. Hauptfenster (WorldViewWindow) zeichnen
    m_impl->worldViewWindow.draw(
        m_impl->toolBox.activeTool(),
        m_impl->edgeBox.activeEdgeId(),
        m_impl->worldViewWindow.edgeColorId(
            m_impl->edgeBox.activeEdgeId()
        ),
        m_impl->miscBox.activeMiscId(),
        m_impl->worldViewWindow.miscColorId(
            m_impl->miscBox.activeMiscId()
        ),
        m_impl->paintTarget ==
        Impl::PaintTarget::Misc,
        [this](
            const std::string& edgeId,
            int size
            ) -> ImTextureID
        {
            return m_impl->edgeBox.edgeTexture(
                edgeId,
                size
            );
        },
        [this](
            const std::string& miscId,
            int size
            ) -> ImTextureID
        {
            return m_impl->miscBox.miscTexture(
                miscId,
                size
            );
        },
        [this](
            const std::string& edgeId,
            const std::string& assignedColorId,
            const AssignEdgeColorCallback& assignColor
            )
        {
            m_impl->edgeBox.openColorMenu(
                edgeId,
                assignedColorId,
                assignColor
            );
        },
        [this](
            const std::string& miscId,
            const std::string& assignedColorId,
            const AssignEdgeColorCallback& assignColor
            )
        {
            m_impl->miscBox.openColorMenu(
                miscId,
                assignedColorId,
                assignColor
            );
        },
        {},

        m_impl->edgeBox.isColorMenuOpen(),
        m_impl->groundOnly,
        isOpen
    );
}
