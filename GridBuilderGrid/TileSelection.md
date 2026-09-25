# Generic tile selection and placement

`GridBuilderGrid` owns one selection through its `WorldViewWindow`. `GridPaintSelection`
contains category, opaque host index and a default edge span. None, Ground and Edge
are implemented; Misc and Furniture are reserved and never dispatch placement.

Host integration:

```cpp
grid.setGroundDropCallback(placeGround);
grid.setWallDropCallback(placeEdge);
// Optional: resolve a context-dependent span without exposing host metadata.
grid.setEdgeSpanResolver(resolveEdgeSpan);
// Inside any host catalog's ImGui window:
gridTileEntry(&grid, GridPaintKind::Ground, index, texture, name);
gridTileEntry(&grid, GridPaintKind::Edge, index, texture, name, span);
```

Include `GridTileEntry.h`. The helper handles the image button, exclusive selection
outline, drag source and payload preview. Hosts retain names, textures, grouping,
loading, placement and persistence. Pass nullptr for a drag-only catalog.

The API also exposes setActiveGroundTile, setActiveEdgeTile, clearActiveTile,
activeTile, isActiveGroundTile and isActiveEdgeTile. Choosing a catalog tile activates
Pencil. Pan and Eraser remain explicit tools. Graphic selection is retained but inactive
while schematic tools are used. Selection is transient and per grid, not process-global.

`gridPaintTarget` is the single finite-layer hit test for click placement, drag
preview and delivery, including edge distance (min of 12px and 25% of the cell),
N/E/S/W order and host-supplied spans. Normal left press starts a stroke. Holding and moving paints each newly crossed
cell/edge once, including intermediate targets between frames. A drag delivery does not also become a click. Ctrl gestures,
popups, blocked input and other tools do not paint. Double-click retains chunk fit;
the first press of that gesture is still an ordinary paint click.

No paths, game names, layer/material codes or persistence formats belong here.

## Editor styles and strokes

`GridEditorStyle::GraphicTiles` is the default. `setEditorStyle` / `editorStyle`
select or query it. `drawEditorStyleMenu` adds Editor > Darstellungsstil to a host's
menu bar. `setGraphicCatalogsRenderer` registers a host callback;
`drawGraphicCatalogs` executes it only in GraphicTiles. No catalog data is owned by
GridBuilderGrid. The host may still honor individual catalog close buttons.

Schematic shows the existing Editor Edge/Misc palettes, uses the existing Pencil
and underlying ChunkManager, suppresses graphical ground textures and draws existing
graphic edges schematically. Changing style never reloads or clears data. The tile
selection remains available on return. Style is transient and starts GraphicTiles
on each application launch.

`GridStrokeGesture` is shared with the legacy schematic Pencil: press starts,
hold continues, release/blocking ends. Legacy orientation, turning, backtracking
and removal semantics remain in WorldViewWallPainter. `GridPaintStroke` adds host
callback deduplication: ground cell coordinates, or canonical physical edge keys.
All edges covered by a wide tile are reserved for that stroke. Returning to any
reserved cell/edge or its opposite-cell alias does not dispatch again. A later
stroke may replace it. Invalid spans and grid targets never enter the visited set.

Graphical strokes sample paths every 1/8 cell and use the shared gridPaintTarget.
Interpolated edge samples follow the endpoint orientation to avoid perpendicular
corner spurs. Leaving the canvas pauses sampling; re-entry continues without
bridging the outside path. Selecting another tile, changing tools/styles, closing
the grid, Ctrl input or drag-and-drop cancels a stroke. Drag delivery is separate
from stroke lifetime, but shares the same hit test and placement callbacks.

No generic undo/persistence batching, new object formats or MM3 import are included.
