#pragma once

#include <string>
#include "EdgeColor.h"

class ChunkManager;

class MapSerializer
{
public:
    static bool save(
        const ChunkManager& chunkManager,
        const std::string& filename
    );

    static bool load(
        ChunkManager& chunkManager,
        const std::string& filename
    );
};