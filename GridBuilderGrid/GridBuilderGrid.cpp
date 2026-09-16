#include "GridBuilderGrid.h"

#include "WorldViewWindow.h"
#include "EditorToolBox.h"
#include "EditorEdgeBox.h"
#include "EditorMiscBox.h"

#include <utility>

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
