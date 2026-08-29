#include "pch.h"

#include "MapSerializer.h"

#include "ChunkManager.h"
#include "Chunk.h"
#include "Cell.h"
#include "EdgeDirection.h"

#include <array>
#include <fstream>
#include <iomanip>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <cstdint>
#include <algorithm>

bool MapSerializer::save(
    const ChunkManager& chunkManager,
    const std::string& filename
)
{
    std::ofstream file(filename);

    if (!file.is_open())
    {
        return false;
    }

    file << "GridBuilderMap 8\n";

    file
        << "chunkSize "
        << chunkManager.chunkSize()
        << '\n';

    file
        << "activeLayer "
        << chunkManager.activeLayer()
        << '\n';

    const std::vector<MapColor>& mapColors =
        chunkManager.colorPalette().colors();

    file
        << "mapColors "
        << mapColors.size()
        << '\n';

    for (const MapColor& mapColor :
        mapColors)
    {
        file
            << "mapColor "
            << std::quoted(mapColor.id)
            << ' '
            << static_cast<int>(
                mapColor.color.red
                )
            << ' '
            << static_cast<int>(
                mapColor.color.green
                )
            << ' '
            << static_cast<int>(
                mapColor.color.blue
                )
            << ' '
            << static_cast<int>(
                mapColor.color.alpha
                )
            << '\n';
    }

    std::vector<std::pair<std::string, std::string>>
        edgeColorAssignments(
            chunkManager
            .edgeColorAssignments()
            .begin(),
            chunkManager
            .edgeColorAssignments()
            .end()
        );

    std::sort(
        edgeColorAssignments.begin(),
        edgeColorAssignments.end()
    );

    file
        << "edgeColorAssignments "
        << edgeColorAssignments.size()
        << '\n';

    for (const auto& [edgeId, colorId] :
        edgeColorAssignments)
    {
        file
            << "edgeColorAssignment "
            << std::quoted(edgeId)
            << ' '
            << std::quoted(colorId)
            << '\n';
    }

    std::vector<std::pair<std::string, std::string>>
        miscColorAssignments(
            chunkManager
            .miscColorAssignments()
            .begin(),
            chunkManager
            .miscColorAssignments()
            .end()
        );

    std::sort(
        miscColorAssignments.begin(),
        miscColorAssignments.end()
    );

    file
        << "miscColorAssignments "
        << miscColorAssignments.size()
        << '\n';

    for (const auto& [miscId, colorId] :
        miscColorAssignments)
    {
        file
            << "miscColorAssignment "
            << std::quoted(miscId)
            << ' '
            << std::quoted(colorId)
            << '\n';
    }

    for (const auto& [layer, chunks] :
        chunkManager.layers())
    {
        for (const auto& [chunkPosition, chunk] :
            chunks)
        {
            const int chunkSize =
                chunk.size();

            for (int localY = 0;
                localY < chunkSize;
                ++localY)
            {
                for (int localX = 0;
                    localX < chunkSize;
                    ++localX)
                {
                    const Cell& cell =
                        chunk.cell(
                            localX,
                            localY
                        );

                    if (cell.empty())
                    {
                        continue;
                    }

                    const int worldX =
                        chunkPosition.x *
                        chunkSize +
                        localX;

                    const int worldY =
                        chunkPosition.y *
                        chunkSize +
                        localY;

                    file
                        << "cell "
                        << layer << ' '
                        << worldX << ' '
                        << worldY << ' '
                        << std::quoted(
                            cell.edgeId(
                                EdgeDirection::North
                            )
                        )
                        << ' '
                        << std::quoted(
                            cell.edgeId(
                                EdgeDirection::East
                            )
                        )
                        << ' '
                        << std::quoted(
                            cell.edgeId(
                                EdgeDirection::South
                            )
                        )
                        << ' '
                        << std::quoted(
                            cell.edgeId(
                                EdgeDirection::West
                            )
                        )
                        << ' '
                        << std::quoted(
                            cell.edgeColorId(
                                EdgeDirection::North
                            )
                        )
                        << ' '
                        << std::quoted(
                            cell.edgeColorId(
                                EdgeDirection::East
                            )
                        )
                        << ' '
                        << std::quoted(
                            cell.edgeColorId(
                                EdgeDirection::South
                            )
                        )
                        << ' '
                        << std::quoted(
                            cell.edgeColorId(
                                EdgeDirection::West
                            )
                        )
                        << ' '
                        << std::quoted(
                            cell.miscId()
                        )
                        << ' '
                        << std::quoted(
                            cell.miscColorId()
                        )
                        << ' '
                        << std::quoted(
                            cell.miscText()
                        )
                        << '\n';
                }
            }
        }
    }

    return file.good();
}

bool MapSerializer::load(
    ChunkManager& chunkManager,
    const std::string& filename
)
{
    std::ifstream file(filename);

    if (!file.is_open())
    {
        return false;
    }

    std::string identifier;
    int version = 0;

    if (!(file >>
        identifier >>
        version))
    {
        return false;
    }

    if (identifier != "GridBuilderMap" ||
        (
            version != 7 &&
            version != 8
            ))
    {
        return false;
    }

    std::string chunkSizeLabel;
    int savedChunkSize = 0;

    if (!(file >>
        chunkSizeLabel >>
        savedChunkSize))
    {
        return false;
    }

    if (chunkSizeLabel != "chunkSize" ||
        savedChunkSize !=
        chunkManager.chunkSize())
    {
        return false;
    }

    std::string activeLayerLabel;
    int savedActiveLayer = 0;

    if (!(file >>
        activeLayerLabel >>
        savedActiveLayer))
    {
        return false;
    }

    if (activeLayerLabel !=
        "activeLayer")
    {
        return false;
    }

    std::string mapColorsLabel;
    std::size_t mapColorCount = 0;

    if (!(file >>
        mapColorsLabel >>
        mapColorCount))
    {
        return false;
    }

    if (mapColorsLabel !=
        "mapColors")
    {
        return false;
    }

    MapColorPalette loadedPalette;
    loadedPalette.clear();

    for (std::size_t index = 0;
        index < mapColorCount;
        ++index)
    {
        std::string entryType;
        std::string colorId;

        int red = 0;
        int green = 0;
        int blue = 0;
        int alpha = 0;

        if (!(file >>
            entryType >>
            std::quoted(colorId) >>
            red >>
            green >>
            blue >>
            alpha))
        {
            return false;
        }

        if (entryType != "mapColor" ||
            red < 0 || red > 255 ||
            green < 0 || green > 255 ||
            blue < 0 || blue > 255 ||
            alpha < 0 || alpha > 255)
        {
            return false;
        }

        const EdgeColor color =
        {
            static_cast<std::uint8_t>(red),
            static_cast<std::uint8_t>(green),
            static_cast<std::uint8_t>(blue),
            static_cast<std::uint8_t>(alpha)
        };

        if (!loadedPalette.addColor(
            colorId,
            color
        ))
        {
            return false;
        }
    }

    ChunkManager::ColorAssignmentCollection
        loadedEdgeColorAssignments;

    std::string edgeAssignmentsLabel;
    std::size_t edgeAssignmentCount = 0;

    if (!(file >>
        edgeAssignmentsLabel >>
        edgeAssignmentCount))
    {
        return false;
    }

    if (edgeAssignmentsLabel !=
        "edgeColorAssignments")
    {
        return false;
    }

    for (std::size_t index = 0;
        index < edgeAssignmentCount;
        ++index)
    {
        std::string assignmentType;
        std::string edgeId;
        std::string colorId;

        if (!(file >>
            assignmentType >>
            std::quoted(edgeId) >>
            std::quoted(colorId)))
        {
            return false;
        }

        if (assignmentType !=
            "edgeColorAssignment" ||
            edgeId.empty() ||
            loadedPalette.find(colorId) ==
            nullptr)
        {
            return false;
        }

        const bool inserted =
            loadedEdgeColorAssignments
            .emplace(
                edgeId,
                colorId
            )
            .second;

        if (!inserted)
        {
            return false;
        }
    }

    ChunkManager::ColorAssignmentCollection
        loadedMiscColorAssignments;

    std::string miscAssignmentsLabel;
    std::size_t miscAssignmentCount = 0;

    if (!(file >>
        miscAssignmentsLabel >>
        miscAssignmentCount))
    {
        return false;
    }

    if (miscAssignmentsLabel !=
        "miscColorAssignments")
    {
        return false;
    }

    for (std::size_t index = 0;
        index < miscAssignmentCount;
        ++index)
    {
        std::string assignmentType;
        std::string miscId;
        std::string colorId;

        if (!(file >>
            assignmentType >>
            std::quoted(miscId) >>
            std::quoted(colorId)))
        {
            return false;
        }

        if (assignmentType !=
            "miscColorAssignment" ||
            miscId.empty() ||
            loadedPalette.find(colorId) ==
            nullptr)
        {
            return false;
        }

        const bool inserted =
            loadedMiscColorAssignments
            .emplace(
                miscId,
                colorId
            )
            .second;

        if (!inserted)
        {
            return false;
        }
    }

    using SavedCell =
        std::tuple<
        int,
        int,
        int,
        std::array<std::string, 4>,
        std::array<std::string, 4>,
        std::string,
        std::string,
        std::string
        >;

    std::vector<SavedCell>
        savedCells;

    std::string entryType;

    while (file >> entryType)
    {
        if (entryType != "cell")
        {
            return false;
        }

        int layer = 0;
        int worldX = 0;
        int worldY = 0;

        std::array<std::string, 4>
            edgeIds;

        std::array<std::string, 4>
            edgeColorIds;

        std::string miscId;
        std::string miscColorId;
        std::string miscText;

        if (!(file >>
            layer >>
            worldX >>
            worldY >>
            std::quoted(edgeIds[0]) >>
            std::quoted(edgeIds[1]) >>
            std::quoted(edgeIds[2]) >>
            std::quoted(edgeIds[3]) >>
            std::quoted(edgeColorIds[0]) >>
            std::quoted(edgeColorIds[1]) >>
            std::quoted(edgeColorIds[2]) >>
            std::quoted(edgeColorIds[3]) >>
            std::quoted(miscId) >>
            std::quoted(miscColorId)))
        {
            return false;
        }

        if (version >= 8 &&
            !(file >> std::quoted(miscText)))
        {
            return false;
        }

        for (std::size_t index = 0;
            index < edgeIds.size();
            ++index)
        {
            if (!edgeIds[index].empty() &&
                (
                    edgeColorIds[index].empty() ||
                    loadedPalette.find(
                        edgeColorIds[index]
                    ) == nullptr
                    ))
            {
                return false;
            }
        }

        if (!miscId.empty() &&
            (
                miscColorId.empty() ||
                loadedPalette.find(
                    miscColorId
                ) == nullptr
                ))
        {
            return false;
        }

        savedCells.emplace_back(
            layer,
            worldX,
            worldY,
            std::move(edgeIds),
            std::move(edgeColorIds),
            std::move(miscId),
            std::move(miscColorId),
            std::move(miscText)
        );
    }

    chunkManager.clear();

    chunkManager.colorPalette() =
        std::move(loadedPalette);

    for (const auto& [edgeId, colorId] :
        loadedEdgeColorAssignments)
    {
        chunkManager.setEdgeColorAssignment(
            edgeId,
            colorId
        );
    }

    for (const auto& [miscId, colorId] :
        loadedMiscColorAssignments)
    {
        chunkManager.setMiscColorAssignment(
            miscId,
            colorId
        );
    }
    constexpr std::array<EdgeDirection, 4>
        directions =
    {
        EdgeDirection::North,
        EdgeDirection::East,
        EdgeDirection::South,
        EdgeDirection::West
    };

    for (const auto& [
        layer,
        worldX,
        worldY,
        edgeIds,
        edgeColorIds,
        miscId,
        miscColorId,
        miscText
    ] : savedCells)
    {
        chunkManager.setActiveLayer(
            layer
        );

        Cell& cell =
            chunkManager.cell(
                worldX,
                worldY
            );

        for (std::size_t index = 0;
            index < directions.size();
            ++index)
        {
            if (!edgeIds[index].empty())
            {
                cell.setEdge(
                    directions[index],
                    edgeIds[index],
                    edgeColorIds[index]
                );
            }
        }

        if (!miscId.empty())
        {
            cell.setMisc(
                miscId,
                miscColorId
            );
        }

        if (!miscText.empty())
        {
            cell.setMiscText(
                miscText
            );
        }
    }

    chunkManager.setActiveLayer(
        savedActiveLayer
    );

    return true;
}
    