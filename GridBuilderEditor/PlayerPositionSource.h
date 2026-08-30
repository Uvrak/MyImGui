#pragma once
#pragma once

#include "WorldViewWindow.h"

class PlayerPositionSource
{
public:
    virtual ~PlayerPositionSource() = default;

    virtual bool update(
        MapPlayerMarker& marker
    ) = 0;
};