#pragma once
#include <cstdint>
#include <functional>
#include <algorithm>
#include <cmath>
enum class GridEditorStyle { GraphicTiles, Schematic };
enum class GridPaintKind { None, Ground, Edge, Misc, Furniture };
struct GridPaintSelection { GridPaintKind kind=GridPaintKind::None;std::uint32_t index=0;int edgeSpan=1; };
using GridEdgeSpanResolver=std::function<int(std::uint32_t,int,int,int)>;
struct GridPaintTarget { int x=0,y=0,side=-1,span=1;bool valid=false; };
// Shared by click painting, drag previews and drag delivery. No host data is interpreted here.
inline GridPaintTarget gridPaintTarget(float tileX,float tileY,float cellSize,int width,int height,
    GridPaintSelection selection,const GridEdgeSpanResolver& edgeSpan={}){
    GridPaintTarget target;target.x=int(std::floor(tileX));target.y=int(std::floor(tileY));
    if(target.x<0 || target.y<0 || target.x>=width || target.y>=height)return target;
    if(selection.kind==GridPaintKind::Ground){target.valid=true;return target;}
    if(selection.kind!=GridPaintKind::Edge)return target;
    const float x=tileX-target.x,y=tileY-target.y;
    const float distances[]={y,1-x,1-y,x};int side=0;
    for(int i=1;i<4;++i)if(distances[i]<distances[side])side=i;
    target.side=side;
    if(distances[side]*cellSize>(std::min)(12.f,cellSize*.25f))return target;
    target.span=edgeSpan?edgeSpan(selection.index,target.x,target.y,side):selection.edgeSpan;
    const bool vertical=side==1 || side==3;
    target.valid=target.span>=1 && target.span<=(vertical?height-target.y:width-target.x);
    return target;
}

inline constexpr const char* GroundTilePayloadType = "GRID_GROUND_TILE";
inline constexpr const char* WallTilePayloadType = "GRID_WALL_TILE";
struct WallTilePayload {std::uint32_t index;int span=1;};

enum class PlacementAction { None, Place, Replace, Remove };
struct GridPlacementState { bool occupied=false,same=false;GridPaintTarget instance; };
inline PlacementAction evaluatePlacement(const GridPlacementState& state,bool toggle=true){
    return state.occupied?(state.same && toggle?PlacementAction::Remove:PlacementAction::Replace):PlacementAction::Place;
}
using GridPlacementQuery=std::function<GridPlacementState(GridPaintSelection,GridPaintTarget)>;
using GridPlacementCallback=std::function<void(GridPaintSelection,GridPaintTarget,PlacementAction)>;
