#include "World3D.h"

namespace GridBuilder3D
{
    std::uint64_t World3D::makeKey(
        int x,
        int y
    )
    {
        const auto ux =
            static_cast<std::uint32_t>(x);

        const auto uy =
            static_cast<std::uint32_t>(y);

        return
            (static_cast<std::uint64_t>(ux) << 32) |
            static_cast<std::uint64_t>(uy);
    }

    void World3D::setCell(
        int x,
        int y,
        const WorldCell& cell
    )
    {
        m_cells[makeKey(x, y)] = cell;
    }

    const WorldCell* World3D::cell(
        int x,
        int y
    ) const
    {
        const auto it =
            m_cells.find(makeKey(x, y));

        if (it == m_cells.end())
        {
            return nullptr;
        }

        return &it->second;
    }

    void World3D::clear()
    {
        m_cells.clear();
    }
}