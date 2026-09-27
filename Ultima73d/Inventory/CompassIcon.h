#pragma once
#include <Camera.h>
#include "InventoryControls.h"
#include <imgui_internal.h>
#include <imgui.h>
#include <glad/gl.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <algorithm>

class CompassIcon {
    unsigned compass=0, needle=0;
    int angleFrame=-1;
    bool windowMoving=false;
    ImVec2 windowStart{},mouseStart{};
    std::filesystem::path state;
    void save(){std::ofstream f(state);f.precision(9);f<<"compass-v3 "<<iconPosition.x<<' '<<iconPosition.y<<' '<<windowPosition.x<<' '<<windowPosition.y<<' '<<onGround<<' '<<groundPosition.x<<' '<<groundPosition.y<<' '<<groundPosition.z<<' '<<opened; }
    float angle=0;
    bool initialized=false;
    static unsigned texture(const std::filesystem::path& path) {
        std::ifstream f(path,std::ios::binary);unsigned dimensions[2]{};
        f.read(reinterpret_cast<char*>(dimensions),sizeof(dimensions));
        if(!f||!dimensions[0]||!dimensions[1]||dimensions[0]>4096||dimensions[1]>4096)
            throw std::runtime_error("Invalid navigation icon: "+path.string());
        std::vector<unsigned char> rgba(size_t(dimensions[0])*dimensions[1]*4);
        f.read(reinterpret_cast<char*>(rgba.data()),rgba.size());
        if(!f)throw std::runtime_error("Truncated navigation icon");
        unsigned t;glGenTextures(1,&t);glBindTexture(GL_TEXTURE_2D,t);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,dimensions[0],dimensions[1],0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        return t;
    }
public:
    ~CompassIcon(){if(compass)glDeleteTextures(1,&compass);if(needle)glDeleteTextures(1,&needle);}
    bool opened=false, onGround=false;
    glm::vec3 groundPosition{};
    ImVec2 groundCenter{};
    void drop(glm::vec3 point){onGround=true;opened=false;windowMoving=false;groundPosition=point;save();}
    void putInBag(ImVec2 position){onGround=false;iconPosition={std::clamp(position.x,.2f,.8f),std::clamp(position.y,.305f,.72f)};save();}
    ImVec2 iconPosition{.5f,.305f},windowPosition{1,0},iconCenter{},windowCenter{},closeCenter{};
    void load(const std::filesystem::path& folder,const std::filesystem::path& settings,bool fresh=false){
        compass=texture(folder/"compass-parchment.rgba");needle=texture(folder/"compass-needle.rgba");state=settings/"compass.cfg";
        if(!fresh){std::ifstream f(state);std::string version;ImVec2 i,w;
            if((f>>version>>i.x>>i.y>>w.x>>w.y)&&(version=="compass-v1"||version=="compass-v2"||version=="compass-v3")&&std::isfinite(i.x)&&std::isfinite(i.y)&&std::isfinite(w.x)&&std::isfinite(w.y)&&i.x>=.2f&&i.x<=.8f&&i.y>=.305f&&i.y<=.72f&&w.x>=0&&w.x<=1&&w.y>=0&&w.y<=1){iconPosition=i;windowPosition=w;
                if(version=="compass-v2"||version=="compass-v3"){int ground;glm::vec3 point;if((f>>ground>>point.x>>point.y>>point.z)&&(ground==0||ground==1)&&std::isfinite(point.x)&&std::isfinite(point.y)&&std::isfinite(point.z)&&(!ground||(glm::length(point)>19&&glm::length(point)<22))){onGround=ground!=0;groundPosition=point;if(version=="compass-v3"){int active;if((f>>active)&&(active==0||active==1))opened=active&&!onGround;}}}
            }}
    }
    static float bearing(glm::vec3 up,glm::vec3 forward) {
        up=glm::normalize(up);
        auto north=glm::vec3(0,1,0)-up*up.y;
        if(glm::length(north)<.0001f)north=glm::vec3(0,0,1)-up*up.z;
        north=glm::normalize(north);auto east=glm::normalize(glm::cross(north,up));
        forward-=up*glm::dot(forward,up);
        if(glm::length(forward)<1e-6f)return 0;
        return std::atan2(glm::dot(forward,east),glm::dot(forward,north));
    }
    static float smooth(float current,float target,float dt){return current+std::remainder(target-current,6.283185307f)*(1-std::exp(-12.f*std::clamp(dt,0.f,.1f)));}
    void visual(ImVec2 p,float size,const ow3d::Camera& camera,glm::vec3 position) {
        auto* d=ImGui::GetWindowDrawList();ImVec2 center(p.x+size*.5f,p.y+size*.5f);
        d->AddImage((ImTextureID)(intptr_t)compass,p,{p.x+size,p.y+size});
        if(angleFrame!=ImGui::GetFrameCount()){
            auto view=camera.viewMatrix();auto forward=-glm::vec3(view[0][2],view[1][2],view[2][2]);
            float target=bearing(glm::normalize(position),forward);
            if(!initialized){angle=target;initialized=true;}else angle=smooth(angle,target,ImGui::GetIO().DeltaTime);
            angleFrame=ImGui::GetFrameCount();
        }
        const float half=size*.29f;
        auto point=[&](float x,float y){return ImVec2(center.x+x*std::cos(angle)-y*std::sin(angle),center.y+x*std::sin(angle)+y*std::cos(angle));};
        d->AddImageQuad((ImTextureID)(intptr_t)needle,point(-half,-half),point(half,-half),point(half,half),point(-half,half));
    }
    bool drawIcon(ImVec2 bag,float bagSize,const ow3d::Camera& camera,glm::vec3 position) {
        if(onGround)return false;
        const float size=bagSize*.072f;
        iconCenter={bag.x+iconPosition.x*bagSize,bag.y+iconPosition.y*bagSize};
        ImVec2 p(iconCenter.x-size*.5f,iconCenter.y-size*.5f);
        return drawSource(p,size,camera,position);
    }
    bool drawSource(ImVec2 p,float size,const ow3d::Camera& camera,glm::vec3 position,bool allowOpen=true){
        auto cursor=ImGui::GetCursorScreenPos();
        ImVec2 center(p.x+size*.5f,p.y+size*.5f);
        ImGui::PushID("CompassItem");ImGui::SetCursorScreenPos(p);
        ImGui::InvisibleButton("Kompass",{size,size});bool hovered=ImGui::IsItemHovered();
        if(allowOpen&&hovered&&ImGui::IsMouseDoubleClicked(0)){opened=!opened;if(!opened)windowMoving=false;save();}

        visual(p,size,camera,position);
        if(opened){auto* d=ImGui::GetWindowDrawList();float pad=3,arm=size*.24f;
            for(int sx:{-1,1})for(int sy:{-1,1}){
                ImVec2 corner(center.x+sx*(size*.5f+pad),center.y+sy*(size*.5f+pad));
                ImVec2 points[]={{corner.x-sx*arm,corner.y},corner,{corner.x,corner.y-sy*arm}};
                d->AddPolyline(points,3,IM_COL32(227,190,108,255),ImDrawFlags_None,2.f);
            }
        }
        if(hovered&&!ImGui::GetDragDropPayload()){ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);ImGui::SetTooltip("%s",allowOpen?(opened?"Kompass: Doppelklick zum Schliessen\nIn den Rucksack oder auf den Boden ziehen":"Kompass: Doppelklick zum Oeffnen\nIn den Rucksack oder auf den Boden ziehen"):"Kompass: in den Rucksack ziehen zum Aufnehmen\nOder auf dem Boden verschieben");}
        if(ImGui::BeginDragDropSource()){
            const int item=0;ImGui::SetDragDropPayload("INVENTORY_COMPASS",&item,sizeof(item));
            auto p=ImGui::GetCursorScreenPos();ImGui::Dummy({44,44});visual(p,44,camera,position);ImGui::EndDragDropSource();
        }
        ImGui::PopID();ImGui::SetCursorScreenPos(cursor);return hovered;

    }
    ImRect windowRect(ImVec2 a,ImVec2 b) const {
        const float side=std::min(143.f,std::min(b.x-a.x,b.y-a.y)-24.f);
        ImVec2 p(a.x+12+windowPosition.x*std::max(0.f,b.x-a.x-side-24),a.y+43+windowPosition.y*std::max(0.f,b.y-a.y-side-55));
        return {p,{p.x+side,p.y+side}};
    }
    bool blocks(ImVec2 a,ImVec2 b)const{auto rect=windowRect(a,b);rect.Expand(8);return opened&&(windowMoving||rect.Contains(ImGui::GetIO().MousePos));}
    bool drawWindow(ImVec2 a,ImVec2 b,const ow3d::Camera& camera,glm::vec3 position){
        if(!opened)return false;
        auto cursor=ImGui::GetCursorScreenPos();auto mouse=ImGui::GetIO().MousePos;
        auto rect=windowRect(a,b);float side=rect.GetWidth();
        if(windowMoving){
            windowPosition={std::clamp(windowStart.x+(mouse.x-mouseStart.x)/std::max(1.f,b.x-a.x-side-24),0.f,1.f),std::clamp(windowStart.y+(mouse.y-mouseStart.y)/std::max(1.f,b.y-a.y-side-55),0.f,1.f)};
            if(!ImGui::IsMouseDown(0)){windowMoving=false;save();}
            rect=windowRect(a,b);
        }
        windowCenter=rect.GetCenter();closeCenter={rect.Max.x-4,rect.Min.y+5};
        ImGui::PushID("CompassWindow");visual(rect.Min,side,camera,position);
        if(inventoryCloseButton("Kompass schliessen",closeCenter,16,"Kompass schliessen")){opened=false;windowMoving=false;save();}
        bool overClose=ImGui::IsItemHovered();
        if(!overClose||windowMoving){
            ImGui::SetCursorScreenPos(rect.Min);ImGui::InvisibleButton("Kompass verschieben",rect.GetSize());
            if(ImGui::IsItemActivated()){windowStart=windowPosition;mouseStart=mouse;}
            if(ImGui::IsItemActive()&&ImGui::IsMouseDragging(0))windowMoving=true;
            if(ImGui::IsItemHovered()){ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);ImGui::SetTooltip("Kompass verschieben");}
        }
        ImGui::PopID();ImGui::SetCursorScreenPos(cursor);
        return rect.Contains(mouse)||overClose||windowMoving;
    }
};
