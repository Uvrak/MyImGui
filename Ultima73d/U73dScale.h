#pragma once

// Metric system of Ultima73d: U7 measures in tiles and lifts; these are the real sizes.
//
//   Horizontal: 1 tile = 0.5 m, as in Ultima7Remake (2 U7 tiles = 1 m there): a U7 door
//               opening of 2 tiles is 1 m, a bed of 3 x 5 tiles 1.5 x 2.5 m.
//   Vertical:   U7 uses one lift for everything, but its buildings and its furniture do not
//               share a scale: a wall or door object is 5 lifts, a table 2, a bed 1. Each
//               group gets its own real lift height:
//                 structures (walls, doors, windows, roofs, stairs and everything at lift 4
//                 and above, i.e. upper floors): a 5 lift wall is one storey of 3.125 m, 1.25 x
//                 U7's 2.50 m (1 lift = 0.5 m), so a window (3 lifts at lift 1) has its sill
//                 at 0.625 m and its top at 2.50 m;
//                 furniture and things at ground level: a 2 lift table is 0.75 m, so what
//                 lies on it (lift 2) lies on its top; a bed or chest is 0.375 m, a fence 0.75 m.
//               Trees and bushes keep the proportions of their frame (8 pixels = 1 tile).
//   People:     Sir Canegm is 1.80 m tall.
namespace U73dScale {

inline constexpr float TileMetres = 0.5f;
inline constexpr float StoreyHeight = 3.125f;                // a 5 lift wall / door object
inline constexpr float StructureLift = StoreyHeight / 5;     // 0.625 m
inline constexpr float TableHeight = 0.75f;                  // a 2 lift table
inline constexpr float FurnitureLift = TableHeight / 2;      // 0.375 m
inline constexpr float CharacterHeight = 1.80f;
// Walls, doors and windows are this share of U7's one tile thickness (centred on the tile);
// posts and the thick town wall blocks keep their size.
inline constexpr float WallThickness = 0.75f;
// The shed doors (DoorModel): leaf height; above it the wall goes on up to StoreyHeight.
inline constexpr float DoorHeight = 2.0f;
// Lift at which a thing counts as standing on a building (upper floors, walkways, roofs).
inline constexpr int UpperFloorLift = 4;

}
