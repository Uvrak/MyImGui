#pragma once
#include "GridBuilderGrid.h"
#include <imgui.h>
// A host supplies the icon/name/index only. Selection, highlight and drag payloads are shared.
inline bool gridTileEntry(GridBuilderGrid* grid,GridPaintKind kind,std::uint32_t index,
    ImTextureID texture,const char* name,int span=1,ImU32 tint=IM_COL32_WHITE,ImVec2 size={64,64}){
    ImGui::PushID(int(kind));ImGui::PushID(int(index));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{0,0});
    const bool clicked=ImGui::ImageButton("##tile",texture,size,{0,0},{1,1},{0,0,0,0},ImGui::ColorConvertU32ToFloat4(tint));
    ImGui::PopStyleVar();
    const bool hovered=ImGui::IsItemHovered();
    auto select=[&]{if(!grid)return;if(kind==GridPaintKind::Ground)grid->setActiveGroundTile(index);else if(kind==GridPaintKind::Edge)grid->setActiveEdgeTile(index,span);};
    if(clicked)select();
    if(grid && ((kind==GridPaintKind::Ground && grid->isActiveGroundTile(index)) || (kind==GridPaintKind::Edge && grid->isActiveEdgeTile(index))))
        ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),IM_COL32(255,220,80,255),0.f,3.f);
    if((kind==GridPaintKind::Ground || kind==GridPaintKind::Edge) && ImGui::BeginDragDropSource()){
        select();
        if(kind==GridPaintKind::Ground)ImGui::SetDragDropPayload(GroundTilePayloadType,&index,sizeof(index));
        else {const WallTilePayload payload{index,span};ImGui::SetDragDropPayload(WallTilePayloadType,&payload,sizeof(payload));}
        ImGui::Image(texture,size);ImGui::TextUnformatted(name);ImGui::EndDragDropSource();
    }
    ImGui::PopID();ImGui::PopID();return hovered;
}

