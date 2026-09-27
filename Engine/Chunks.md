# Tile-Chunks

Die bestehende Cube-Sphere bleibt bei 256 x 256 Tiles je Face. TileWorld teilt jede der sechs Seiten in 16 x 16 Chunks mit jeweils 16 x 16 Tiles: insgesamt 1536 Chunks.

Jeder TileChunk besitzt seine Tiles, Material-Meshes und eine Bounding Box. Nur geaenderte Chunks werden beim naechsten Rendern neu aufgebaut. Leere Chunks halten keine GPU-Meshes. Material, UVs, Surface-Werte und Tile-Hoehen bleiben erhalten. Bounding-Box/Frustum-Culling spart Draw Calls ausserhalb des Kamerabildes; es ist kein Horizont- oder Occlusion-Culling.

## Zugriff

- tile(face, x, y): bisherige globale Tile-Koordinaten bleiben gueltig. Nicht-konstanter Zugriff markiert den zugehoerigen Chunk als geaendert. Fuer reines Lesen eine const TileWorld-Referenz verwenden.
- chunkForTile(face, x, y): globale Tile-Koordinaten in ChunkCoord umrechnen.
- chunk(face, chunkX, chunkY): schreibgeschuetzte Chunk-Informationen. Meshes, Bounds und visibleTiles spiegeln den letzten erfolgreichen Rebuild wider.
- invalidateChunk(face, chunkX, chunkY): nach spaeteren Aenderungen ueber eine aufbewahrte Tile&-Referenz aufrufen, falls dazwischen bereits gerendert wurde.
- renderStats(): Rebuilds, gezeichnete/ausgesparte Chunks, Draw Calls und gezeichnete Tiles des letzten Frames.

Ungueltige Koordinaten werfen std::out_of_range. GPU-Meshes werden wie bisher auf dem Render-Thread erzeugt und muessen vor dem OpenGL-Kontext zerstoert werden. Tile-Hoehen verwenden weiterhin unabhaengige Ecken pro Tile; unterschiedliche Hoehen erzeugen keine automatischen Seitenwaende.

Alle Chunks bleiben im RAM. Hintergrundladen, Festplatten-Streaming und LOD sind noch nicht enthalten. Benutzerdefinierte Vertex-Shader duerfen die Geometrie fuer dieses Bounding-Box-Culling nicht ueber die berechneten Tile-Ecken hinaus verschieben.

## Engine-Test ohne Spiel-Build

In einer Visual-Studio-Entwicklerkonsole vom Repository-Hauptordner aus:

    msbuild tests\tile_chunks.vcxproj /p:Configuration=Debug /p:Platform=x64
    work\chunks\bin\ChunkTests.exe

Baut nur Engine, deren MyImGui-Abhaengigkeit und den separaten Test. Ultima7Remake und Game werden nicht gebaut. Der GPU-Test benoetigt den von der Engine verwendeten OpenGL-Kontext und verbirgt sein Testfenster.
