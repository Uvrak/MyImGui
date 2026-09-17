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
        int chunkSize
    )
        :
        worldViewWindow(
            chunkSize
        ),
        toolBox(),
        edgeBox(
            device
        ),
        miscBox(
            device
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

    PaintTarget paintTarget =
        PaintTarget::Edge;
};

GridBuilderGrid::GridBuilderGrid(
    ID3D11Device* device,
    int chunkSize
)
    :
    m_impl(
        std::make_unique<Impl>(
            device,
            chunkSize
        )
    )
{}

GridBuilderGrid::~GridBuilderGrid() =
default;

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

    if (m_impl->worldViewWindow.hasUnsavedChanges())
    {
        if (m_impl->currentMapKey.empty()) return false;
        const auto previous = m_impl->mapFiles.find(m_impl->currentMapKey);
        if (previous == m_impl->mapFiles.end() ||
            !m_impl->worldViewWindow.saveMap(previous->second)) return false;
    }

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
            ) -> ID3D11ShaderResourceView*
        {
            return m_impl->edgeBox.edgeTexture(
                edgeId,
                size
            );
        },
        [this](
            const std::string& miscId,
            int size
            ) -> ID3D11ShaderResourceView*
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
        []()
        {
        },

        m_impl->edgeBox.isColorMenuOpen(),
        false,
        isOpen
    );
}
