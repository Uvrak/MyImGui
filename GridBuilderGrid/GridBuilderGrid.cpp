#include "GridBuilderGrid.h"

GridBuilderGrid::GridBuilderGrid(
    ID3D11Device* device,
    int chunkSize
)
    :
    m_worldViewWindow(
        chunkSize
    ),
    m_toolBox(),
    m_edgeBox(
        device
    ),
    m_miscBox(
        device
    )
{
    m_miscBox.clearActiveMisc();
}

void GridBuilderGrid::draw(
    bool* isOpen
)
{
    if (m_miscBox.draw(
        m_worldViewWindow.colorPalette(),
        [this](
            const std::string& miscId
            )
        {
            return m_worldViewWindow.miscColorId(
                miscId
            );
        },
        [this](
            const std::string& miscId,
            const std::string& colorId
            )
        {
            m_worldViewWindow.setMiscColorId(
                miscId,
                colorId
            );
        },
        [this](
            const std::string& colorId
            )
        {
            return m_worldViewWindow.removeMapColor(
                colorId
            );
        }
    ))
    {
        m_edgeBox.clearActiveEdge();

        m_paintTarget =
            GridPaintTarget::Misc;

        m_toolBox.setActiveTool(
            EditorTool::Pencil
        );
    }

    m_worldViewWindow.draw(
        m_toolBox.activeTool(),
        m_edgeBox.activeEdgeId(),
        m_worldViewWindow.edgeColorId(
            m_edgeBox.activeEdgeId()
        ),
        m_miscBox.activeMiscId(),
        m_worldViewWindow.miscColorId(
            m_miscBox.activeMiscId()
        ),
        m_paintTarget ==
        GridPaintTarget::Misc,
        [this](
            const std::string& edgeId,
            int size
            ) -> ID3D11ShaderResourceView*
        {
            return m_edgeBox.edgeTexture(
                edgeId,
                size
            );
        },
        [this](
            const std::string& miscId,
            int size
            ) -> ID3D11ShaderResourceView*
        {
            return m_miscBox.miscTexture(
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
            m_edgeBox.openColorMenu(
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
            m_miscBox.openColorMenu(
                miscId,
                assignedColorId,
                assignColor
            );
        },
        []()
        {
        },
        m_edgeBox.isColorMenuOpen(),
        false,
        isOpen
        );
}