#include "pch.h"

#include "Cell.h"

#include <cstddef>

namespace
{
    constexpr std::size_t edgeIndex(
        EdgeDirection direction
    )
    {
        switch (direction)
        {
        case EdgeDirection::North:
            return 0;

        case EdgeDirection::East:
            return 1;

        case EdgeDirection::South:
            return 2;

        case EdgeDirection::West:
            return 3;
        }

        return 0;
    }
}

void Cell::setEdge(
    EdgeDirection direction,
    const std::string& edgeId,
    const std::string& colorId
)
{
    if (edgeId.empty())
    {
        removeEdge(direction);
        return;
    }

    const std::size_t index =
        edgeIndex(direction);

    m_edges[index] =
        edgeId;

    m_edgeColorIds[index] =
        colorId;
}

void Cell::removeEdge(
    EdgeDirection direction
)
{
    const std::size_t index =
        edgeIndex(direction);

    m_edges[index].clear();
    m_edgeColorIds[index].clear();
}

bool Cell::hasEdge(
    EdgeDirection direction
) const
{
    return !m_edges[
        edgeIndex(direction)
    ].empty();
}

const std::string& Cell::edgeId(
    EdgeDirection direction
) const
{
    return m_edges[
        edgeIndex(direction)
    ];
}

const std::string& Cell::edgeColorId(
    EdgeDirection direction
) const
{
    return m_edgeColorIds[
        edgeIndex(direction)
    ];
}

void Cell::setMisc(
    const std::string& miscId,
    const std::string& colorId
)
{
    if (miscId.empty())
    {
        removeMisc();
        return;
    }

    m_miscId =
        miscId;

    m_miscColorId =
        colorId;
}

void Cell::removeMisc()
{
    m_miscId.clear();
    m_miscColorId.clear();
}

void Cell::removeMiscText()
{
    m_miscText.clear();
}

bool Cell::hasMisc() const
{
    return !m_miscId.empty();
}

const std::string& Cell::miscId() const
{
    return m_miscId;
}

const std::string& Cell::miscColorId() const
{
    return m_miscColorId;
}

void Cell::setMiscText(
    const std::string& text
)
{
    m_miscText =
        text;
}

const std::string& Cell::miscText() const
{
    return m_miscText;
}

bool Cell::hasMiscText() const
{
    return !m_miscText.empty();
}

bool Cell::referencesColor(
    const std::string& colorId
) const
{
    for (std::size_t index = 0;
        index < m_edges.size();
        ++index)
    {
        if (!m_edges[index].empty() &&
            m_edgeColorIds[index] ==
            colorId)
        {
            return true;
        }
    }

    return
        !m_miscId.empty() &&
        m_miscColorId == colorId;
}

bool Cell::empty() const
{
    for (const std::string& edge :
        m_edges)
    {
        if (!edge.empty())
        {
            return false;
        }
    }

    return
        m_miscId.empty() &&
        m_miscText.empty();
}