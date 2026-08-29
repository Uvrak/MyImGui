#pragma once

#include "EdgeDirection.h"

#include <array>
#include <string>

class Cell
{
public:
    Cell() = default;

    void setEdge(
        EdgeDirection direction,
        const std::string& edgeId,
        const std::string& colorId
    );
    void removeEdge(
        EdgeDirection direction
    );

    bool hasEdge(
        EdgeDirection direction
    ) const;

    const std::string& edgeId(
        EdgeDirection direction
    ) const;

    const std::string& edgeColorId(
        EdgeDirection direction
    ) const;

    void setMisc(
        const std::string& miscId
    );

    void setMisc(
        const std::string& miscId,
        const std::string& colorId
    );

    void removeMisc();

    void removeMiscText();

    bool hasMisc() const;

    const std::string& miscId() const;

    const std::string& miscColorId() const;

    void setMiscText(
        const std::string& text
    );

    const std::string& miscText() const;

    bool hasMiscText() const;
    bool referencesColor(
        const std::string& colorId
    ) const;

    bool empty() const;

private:
    std::array<std::string, 4>
        m_edges;

    std::array<std::string, 4>
        m_edgeColorIds;

    std::string m_miscId;
    std::string m_miscColorId;

    std::string m_miscText;
};