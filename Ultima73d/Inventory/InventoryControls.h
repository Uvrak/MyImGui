#pragma once
#include <imgui.h>

inline bool inventoryCloseButton(const char* id, ImVec2 center, float size, const char* hint) {
    ImGui::SetCursorScreenPos({center.x-size*.5f,center.y-size*.5f});
    const bool clicked=ImGui::InvisibleButton(id,{size,size});
    const bool hot=ImGui::IsItemHovered();auto* ink=ImGui::GetWindowDrawList();
    const float radius=size*.46f;
    ink->AddCircleFilled({center.x+1.5f,center.y+2.f},radius+1.f,IM_COL32(0,0,0,130),24);
    ink->AddCircleFilled(center,radius,hot?IM_COL32(102,57,26,255):IM_COL32(55,29,15,255),24);
    ink->AddCircle(center,radius,IM_COL32(183,134,64,255),24,1.5f);
    ink->AddCircle(center,radius-3.f,IM_COL32(119,76,35,255),24,1.f);
    const float arm=size*.18f;
    const ImU32 brass=hot?IM_COL32(255,227,149,255):IM_COL32(231,188,106,255);
    ink->AddLine({center.x-arm,center.y-arm},{center.x+arm,center.y+arm},brass,2.5f);
    ink->AddLine({center.x-arm,center.y+arm},{center.x+arm,center.y-arm},brass,2.5f);
    for(int sx:{-1,1})for(int sy:{-1,1})ink->AddCircleFilled({center.x+sx*arm,center.y+sy*arm},1.7f,brass,8);
    if(hot){ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);ImGui::SetTooltip("%s",hint);}
    return clicked;
}
