#pragma once
#include "BackpackInventory.h"
#include "CompassIcon.h"
#include "CharacterPortrait.h"
#include <stdexcept>
#include "BackpackScene.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>

class BackpackPanel {
public:
    std::function<void(ImVec2,float)> extraItems;
    // The open bag on screen (top left, size); size 0 when closed.
    std::pair<ImVec2,float> panel{{0,0},0};
    bool isVisible() const { return visible; }
    // false: equipment lying on the ground is drawn by the scene itself (as 3D things).
    bool groundIcons=true;
    glm::vec3 groundPoint(int item) const { return groundWorld.at(item); }
    bool equipmentOnGround(int item) const { return bag.ground.at(item)&&!inventory.fits[item].worn; }
    // The scene moved equipment: back into the bag, or onto the ground at point (world).
    void putEquipmentInBag(int item){ if(Equipment::boots(item))item=3; bag.ground[item]=false; saveWorld(true); }
    void dropEquipment(int item,glm::vec3 point){ if(Equipment::boots(item))item=3; bag.ground[item]=true; groundWorld[item]=point; saveWorld(true); }
    // Things of the world that are clothing (a U73D_ITEM payload): the equipment they are, or -1;
    // wornFromWorld: the thing has been put on (it leaves the world).
    std::function<int(int)> wearableEquipment;
    std::function<void(int)> wornFromWorld;
    // Whether a screen point lies on Sir Canegm (where clothing dropped is put on).
    bool overCharacterAt(ImVec2 mouse) const {
        if(!scene||lastB.x<=lastA.x)return false;
        auto& actor=scene->character();auto vp=scene->camera().projectionMatrix()*scene->camera().viewMatrix();
        auto project=[&](glm::vec3 point){auto q=vp*glm::vec4(point,1);return ImVec2(lastA.x+(q.x/q.w+1)*.5f*(lastB.x-lastA.x),lastA.y+(1-q.y/q.w)*.5f*(lastB.y-lastA.y));};
        auto up=glm::normalize(actor.position());auto foot=project(actor.position());auto head=project(actor.position()+up*actor.height());
        auto mid=project(actor.position()+up*actor.height()*.5f);float radius=std::max(25.f,std::abs(foot.y-head.y)*.4f);
        return ImRect({mid.x-radius,std::min(head.y,foot.y)-15},{mid.x+radius,std::max(head.y,foot.y)+15}).Contains(mouse);
    }
    // What the bag holds of the equipment (neither worn nor lying on the ground) and the compass, kg.
    float carriedKg() const {
        float kg=compass.onGround?0.f:Equipment::compassKg;
        for(int i=0;i<Equipment::itemCount;++i)if(!inventory.fits[i].worn&&!bag.ground[i])kg+=Equipment::itemKg[i];
        return kg;
    }
    ~BackpackPanel() { bag.shutdown(); }
    bool saveNow() { saveWorld(true); return bag.save() && bag.saveGround() && bag.savePanel() && error.empty(); }
    void load(const std::filesystem::path& assets, const std::filesystem::path& settings, bool fresh=false) {
        std::filesystem::create_directories(settings);
        outfitState=settings / "outfit.cfg";
        std::ifstream worn(outfitState);std::string version;unsigned mask=0;
        if(!fresh&&(worn>>version>>mask)&&version=="outfit-v1"&&mask<1024)for(int i=0;i<Equipment::itemCount;++i)if(mask&(1u<<i))inventory.equip(i,true);
        const auto state = settings / "backpack.cfg";
        if (!std::filesystem::exists(state) && std::filesystem::exists(assets / "defaults/backpack.cfg"))
            std::filesystem::copy_file(assets / "defaults/backpack.cfg", state);
        bag.load(assets / "UI", state, inventory);
        compass.load(assets / "UI/navigation",settings,fresh);
        if(fresh){bag.positions=BackpackInventory{}.positions;bag.panelPosition={1,1};}
        if (!bag.background) throw std::runtime_error("Missing backpack background in " + assets.string());
        for (auto icon : bag.icons) if (!icon) throw std::runtime_error("Missing backpack item icon.");
        bag.groundState = settings / "backpack-ground.cfg";
        worldState=settings/"ground-items.cfg";
        if(!fresh){std::ifstream f(worldState);std::string version;if((f>>version)&&version=="world-items-v1"){
            auto flags=bag.ground;auto points=groundWorld;bool valid=true;
            for(int i=0;i<Equipment::itemCount;++i){int flag;auto& q=points[i];if(!(f>>flag>>q.x>>q.y>>q.z)||(flag!=0&&flag!=1)||!std::isfinite(q.x)||!std::isfinite(q.y)||!std::isfinite(q.z)||glm::length(q)>1e6f) {valid=false;break;}flags[i]=flag&&!inventory.fits[i].worn;}
            if(valid){bag.ground=flags;groundWorld=points;}
        }}
        savedGround=bag.ground;
    }
    ImVec2 itemScreenPosition(int item) const { return centers.at(item); }
    ImVec2 wornScreenPosition(int slot) const { return wornCenters.at(slot); }
    ImVec2 compassItemPosition() const{return compass.iconCenter;}
    ImVec2 compassWindowPosition() const{return compass.windowCenter;}
    ImVec2 compassClosePosition() const{return compass.closeCenter;}
    bool compassOpen()const{return compass.opened;}
    unsigned portraitEquipment()const{return portrait.equippedMask();}
    bool savePortrait(const char* path){return portrait.save(path);}
    bool compassOnGround()const{return compass.onGround;}
    ImVec2 compassGroundPosition()const{return compass.groundCenter;}
    unsigned wornMask() const { return inventory.mask(); }
    bool onGround(int item) const { return bag.ground.at(item); }
    ImVec2 groundScreenPosition(int item) const { return groundCenters.at(item); }
    void attach(BackpackScene& renderer) { scene=&renderer;scene->character().outfit.setWorn(inventory.mask()); }
    void setEquipment(int item,bool worn) {
        if(!scene)return;
        const auto before=inventory.fits;
        if(!inventory.equip(item,worn))return;
        try {scene->character().outfit.setWorn(inventory.mask());}
        catch(const std::exception& e){inventory.fits=before;error=e.what();return;}
        bag.ground[item]=false;
        if(Equipment::boots(item))bag.ground[3]=bag.ground[4]=false;
        saveWorld();
        auto tmp=outfitState;tmp+=".tmp";std::ofstream f(tmp);f<<"outfit-v1 "<<inventory.mask()<<'\n';f.close();
        std::error_code ec;if(f)std::filesystem::copy_file(tmp,outfitState,std::filesystem::copy_options::overwrite_existing,ec);
        if(!f||ec)error="Ausruestung konnte nicht gespeichert werden.";
        else {std::filesystem::remove(tmp,ec);error.clear();}
    }
    void menu() {
        if (ImGui::BeginMenu("Fenster")) {
            ImGui::MenuItem("Rucksack", "B", &visible);
            ImGui::EndMenu();
        }
    }
    bool draw(ImVec2 a, ImVec2 b) {
        lastA=a;lastB=b;
        if (!ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_B, false)) visible = !visible;
        if (b.x-a.x<80 || b.y-a.y<80) return false;
        bool portraitHovered=scene&&portrait.draw(a,b,scene->character());
        // Clothing dropped on the portrait is put on too (Sir Canegm may stand behind the bag).
        if(scene&&portrait.lastMax.x>portrait.lastMin.x&&ImGui::GetDragDropPayload()){
            const auto* dragged=ImGui::GetDragDropPayload();
            const bool wearable=dragged->IsDataType("CHARACTER_EQUIPMENT")||(dragged->IsDataType("U73D_ITEM")&&dragged->DataSize==sizeof(int)&&wearableEquipment&&wearableEquipment(*static_cast<const int*>(dragged->Data))>=0);
            if(wearable){
                const ImRect target(portrait.lastMin,portrait.lastMax);
                if(ImGui::BeginDragDropTargetCustom(target,ImGui::GetID("SirCanegm-portrait-equip"))){
                    if(auto payload=ImGui::AcceptDragDropPayload("CHARACTER_EQUIPMENT",ImGuiDragDropFlags_AcceptNoDrawDefaultRect))if(payload->DataSize==sizeof(int))setEquipment(*static_cast<const int*>(payload->Data),true);
                    if(auto payload=ImGui::AcceptDragDropPayload("U73D_ITEM",ImGuiDragDropFlags_AcceptNoDrawDefaultRect))if(payload->DataSize==sizeof(int)&&wearableEquipment){
                        const int id=*static_cast<const int*>(payload->Data),item=wearableEquipment(id);
                        if(item>=0){setEquipment(item,true);if(wornFromWorld)wornFromWorld(id);}
                    }
                    ImGui::EndDragDropTarget();
                }
            }
        }
        bool compassHovered=compass.blocks(a,b)||portraitHovered;
        bool groundHovered=compassHovered?false:drawWorldItems(a,b);
        if (!visible) { panel.second=0; return (scene&&compass.drawWindow(a,b,scene->camera(),scene->character().position()))||groundHovered||portraitHovered; }
        ImGui::BeginDisabled(compassHovered);
        const auto cursor=ImGui::GetCursorScreenPos();
        const float size=std::min(420.f,std::min(b.x-a.x,b.y-a.y)-24.f);
        const bool enabled=!compassHovered && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        const auto mouse=ImGui::GetIO().MousePos;
        const ImVec2 candidate(a.x+12+bag.panelPosition.x*std::max(0.f,b.x-a.x-size-24),
            a.y+12+bag.panelPosition.y*std::max(0.f,b.y-a.y-size-24));
        const float closeSize=std::min(30.f,size*.10f);
        const ImVec2 closeCenter(candidate.x+size*.80f,candidate.y+size*.10f);
        const bool overClose=std::abs(mouse.x-closeCenter.x)<closeSize*.6f && std::abs(mouse.y-closeCenter.y)<closeSize*.6f;
        const ImVec2 p=bag.placePanel(a,b,size,enabled&&!overClose,error);
        const bool hovered=enabled && mouse.x>=p.x && mouse.x<=p.x+size && mouse.y>=p.y && mouse.y<=p.y+size;
        ImGui::PushID("MainViewBackpack");
        ImGui::SetCursorScreenPos(p);
        panel={p,size};
        bag.draw(inventory,selected,centers,error,size);
        if(scene)compass.drawIcon(p,size,scene->camera(),scene->character().position());
        if(extraItems)extraItems(p,size);
        if(ImGui::BeginDragDropTargetCustom(ImRect({p.x+size*.15f,p.y+size*.26f},{p.x+size*.85f,p.y+size*.78f}),ImGui::GetID("Compass-to-backpack"))){
            if(ImGui::AcceptDragDropPayload("INVENTORY_COMPASS",ImGuiDragDropFlags_AcceptNoDrawDefaultRect))compass.putInBag({(mouse.x-p.x)/size,(mouse.y-p.y)/size});
            ImGui::EndDragDropTarget();
        }
        // Claim the flap drag so ImGui cannot move/undock the underlying viewport.
        ImGui::SetCursorScreenPos({p.x+size*.12f,p.y+size*.03f});
        ImGui::InvisibleButton("Rucksack verschieben",{size*.62f,size*.22f});
        const ImVec2 center(p.x+size*.80f,p.y+size*.10f);
        if(inventoryCloseButton("Rucksack schliessen",center,closeSize,"Rucksack schliessen (B)"))visible=false;
        auto* ink=ImGui::GetWindowDrawList();
        if(scene) {
            if(bag.pendingEquip>=0){setEquipment(bag.pendingEquip,true);bag.pendingEquip=-1;}
            if(bag.pendingUnequip>=0){setEquipment(bag.pendingUnequip,false);bag.pendingUnequip=-1;}
            ImGui::SetCursorScreenPos({p.x+size*.24f,p.y+size*.80f});
            if(selected>=0&&selected<Equipment::itemCount){
                if(ImGui::Button(inventory.fits[selected].worn ? "Ausziehen" : "Anziehen",{size*.5f,25}))setEquipment(selected,!inventory.fits[selected].worn);
            }
            int slots[4]={0,1,2,3};for(int i=5;i<Equipment::itemCount;++i)if(inventory.fits[i].worn)slots[2]=i;
            for(int k=0;k<4;++k){int item=slots[k];ImGui::PushID(200+item);ImVec2 pos(p.x+size*(.22f+k*.145f),p.y+size*.88f);float side=size*.12f;wornCenters[k]={pos.x+side*.5f,pos.y+side*.5f};
                ImGui::SetCursorScreenPos(pos);
                if(inventory.fits[item].worn){
                    if(ImGui::InvisibleButton("Angelegt",{side,side}))setEquipment(item,false);
                    ImGui::GetWindowDrawList()->AddRectFilled(pos,{pos.x+side,pos.y+side},IM_COL32(30,55,27,230),4);
                    bag.drawIcon(item,pos,{pos.x+side,pos.y+side});
                    if(ImGui::IsItemHovered())ImGui::SetTooltip("%s: klicken zum Ausziehen oder in den Rucksack ziehen",inventory.items[item].label);
                    if(ImGui::BeginDragDropSource()){ImGui::SetDragDropPayload("WORN_EQUIPMENT",&item,sizeof(item));bag.preview(item);ImGui::EndDragDropSource();}
                } else {ImGui::GetWindowDrawList()->AddRect(pos,{pos.x+side,pos.y+side},IM_COL32(190,155,90,150),4);}
                ImGui::PopID();
            }
        }
        bool overCharacter=false;
        if(scene && enabled && !ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            int hit=-1;
            if(!hovered && mouse.x>=a.x && mouse.x<=b.x && mouse.y>=a.y && mouse.y<=b.y && !groundHovered)
                hit=scene->character().pickEquipment(scene->camera(),2*(mouse.x-a.x)/(b.x-a.x)-1,1-2*(mouse.y-a.y)/(b.y-a.y));
            if(characterDrag<0 && hit>=0 && !ImGui::GetDragDropPayload())characterDrag=hit;
            if(characterDrag>=0){
                ImGui::SetCursorScreenPos(a);
                ImGui::InvisibleButton("Worn-on-character",{b.x-a.x,b.y-a.y});
                if(ImGui::IsItemHovered() && hit>=0)ImGui::SetTooltip("%s: in den Rucksack oder auf den Boden ziehen",inventory.items[hit].label);
                if(ImGui::BeginDragDropSource()){
                    ImGui::SetDragDropPayload("WORN_EQUIPMENT",&characterDrag,sizeof(characterDrag));bag.preview(characterDrag);ImGui::EndDragDropSource();
                }
                if(!ImGui::IsMouseDown(0))characterDrag=-1;
            }
        }
        const auto* dragged=ImGui::GetDragDropPayload();
        const bool clothing=dragged&&dragged->IsDataType("U73D_ITEM")&&dragged->DataSize==sizeof(int)&&wearableEquipment&&wearableEquipment(*static_cast<const int*>(dragged->Data))>=0;
        if(scene && dragged && (dragged->IsDataType("CHARACTER_EQUIPMENT") || dragged->IsDataType("WORN_EQUIPMENT") || clothing)) {
            auto& actor=scene->character();auto vp=scene->camera().projectionMatrix()*scene->camera().viewMatrix();
            auto project=[&](glm::vec3 point){auto q=vp*glm::vec4(point,1);return ImVec2(a.x+(q.x/q.w+1)*.5f*(b.x-a.x),a.y+(1-q.y/q.w)*.5f*(b.y-a.y));};
            auto up=glm::normalize(actor.position());auto foot=project(actor.position());auto head=project(actor.position()+up*actor.height());
            auto mid=project(actor.position()+up*actor.height()*.5f);float radius=std::max(25.f,std::abs(foot.y-head.y)*.4f);
            ImRect target({mid.x-radius,std::min(head.y,foot.y)-15},{mid.x+radius,std::max(head.y,foot.y)+15});
            overCharacter=target.Contains(mouse)&&enabled&&!hovered;
            if(overCharacter){
                if(ImGui::BeginDragDropTargetCustom(target,ImGui::GetID("SirCanegm-equip"))){
                    if(auto payload=ImGui::AcceptDragDropPayload("CHARACTER_EQUIPMENT",ImGuiDragDropFlags_AcceptNoDrawDefaultRect))if(payload->DataSize==sizeof(int))setEquipment(*static_cast<const int*>(payload->Data),true);
                    if(auto payload=ImGui::AcceptDragDropPayload("U73D_ITEM",ImGuiDragDropFlags_AcceptNoDrawDefaultRect))if(payload->DataSize==sizeof(int)&&wearableEquipment){
                        const int id=*static_cast<const int*>(payload->Data),item=wearableEquipment(id);
                        if(item>=0){setEquipment(item,true);if(wornFromWorld)wornFromWorld(id);}
                    }
                    ImGui::EndDragDropTarget();
                }
            }
        }
        if(scene && enabled && !hovered && !overCharacter && ImGui::GetDragDropPayload() && !ImGui::GetDragDropPayload()->IsDataType("BRITANNIA_STONE") && !ImGui::GetDragDropPayload()->IsDataType("U73D_ITEM")) {
            glm::vec3 point;
            if(scene->pickGround(2*(mouse.x-a.x)/(b.x-a.x)-1,1-2*(mouse.y-a.y)/(b.y-a.y),point)){
                if(ImGui::BeginDragDropTargetCustom(ImRect(a,b),ImGui::GetID("world-ground-drop"))){
                    if(ImGui::AcceptDragDropPayload("INVENTORY_COMPASS",ImGuiDragDropFlags_AcceptNoDrawDefaultRect))compass.drop(point);
                    auto payload=ImGui::AcceptDragDropPayload("CHARACTER_EQUIPMENT",ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
                    if(!payload)payload=ImGui::AcceptDragDropPayload("WORN_EQUIPMENT",ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
                    if(payload && payload->DataSize==sizeof(int)){
                        int item=*static_cast<const int*>(payload->Data);if(item>=0&&item<Equipment::itemCount){
                            if(Equipment::boots(item))item=3;
                            setEquipment(item,false);
                            if(!inventory.fits[item].worn){bag.ground[item]=true;groundWorld[item]=point;saveWorld(true);}
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
            }
        }
        saveWorld();
        if (hovered && !error.empty()) ImGui::SetTooltip("%s",error.c_str());
        ImGui::PopID();
        ImGui::SetCursorScreenPos(cursor);
        ImGui::Dummy({0,0});
        ImGui::EndDisabled();
        if(scene)compassHovered=compass.drawWindow(a,b,scene->camera(),scene->character().position())||compassHovered;
        return hovered || bag.movingPanel || overCharacter || groundHovered || characterDrag>=0 || compassHovered;
    }
private:
    CharacterPortrait portrait;
    CompassIcon compass;
    int characterDrag=-1;
    std::filesystem::path worldState;
    std::array<glm::vec3,Equipment::itemCount> groundWorld{};
    std::array<ImVec2,Equipment::itemCount> groundCenters{};
    std::array<bool,Equipment::itemCount> savedGround{};
    void saveWorld(bool force=false){
        if(worldState.empty()||(!force&&savedGround==bag.ground))return;
        std::ofstream f(worldState);f<<"world-items-v1\n"<<std::setprecision(9);
        for(int i=0;i<Equipment::itemCount;++i){auto p=groundWorld[i];f<<bag.ground[i]<<' '<<p.x<<' '<<p.y<<' '<<p.z<<'\n';}
        if(!f)error="Bodenablage konnte nicht gespeichert werden.";
        savedGround=bag.ground;
    }
    bool drawWorldItems(ImVec2 a,ImVec2 b){
        if(!scene)return false;bool hovered=false;auto cursor=ImGui::GetCursorScreenPos();
        auto vp=scene->camera().projectionMatrix()*scene->camera().viewMatrix();
        if(compass.onGround){
            auto q=vp*glm::vec4(compass.groundPosition,1);
            if(q.w>0){auto ndc=glm::vec3(q)/q.w;glm::vec3 ground;
                if(std::abs(ndc.x)<=1&&std::abs(ndc.y)<=1&&ndc.z<=1&&scene->pickGround(ndc.x,ndc.y,ground)&&glm::distance(ground,compass.groundPosition)<.035f){
                    ImVec2 center(a.x+(ndc.x+1)*.5f*(b.x-a.x),a.y+(1-ndc.y)*.5f*(b.y-a.y));
                    compass.groundCenter={center.x,center.y-20};
                    ImGui::GetWindowDrawList()->AddEllipseFilled(center,{22,6},IM_COL32(0,0,0,100));
                    hovered|=compass.drawSource({center.x-22,center.y-42},44,scene->camera(),scene->character().position(),false);
                }
            }
        }
        for(int i=0;i<Equipment::itemCount;++i)if(groundIcons&&bag.ground[i]&&!inventory.fits[i].worn){
            auto q=vp*glm::vec4(groundWorld[i],1);if(q.w<=0)continue;
            auto ndc=glm::vec3(q)/q.w;if(std::abs(ndc.x)>1||std::abs(ndc.y)>1||ndc.z>1)continue;
            glm::vec3 visibleGround;if(!scene->pickGround(ndc.x,ndc.y,visibleGround)||glm::distance(visibleGround,groundWorld[i])>.035f)continue;
            ImVec2 center(a.x+(ndc.x+1)*.5f*(b.x-a.x),a.y+(1-ndc.y)*.5f*(b.y-a.y));groundCenters[i]={center.x,center.y-20};
            ImVec2 pos(center.x-22,center.y-42);ImGui::PushID(400+i);ImGui::SetCursorScreenPos(pos);
            ImGui::InvisibleButton("Ground equipment",{44,44});hovered|=ImGui::IsItemHovered();
            auto d=ImGui::GetWindowDrawList();d->AddEllipseFilled(center,{22,6},IM_COL32(0,0,0,100));bag.drawIcon(i,pos,{pos.x+44,pos.y+44});
            if(ImGui::IsItemHovered())ImGui::SetTooltip("%s: in den Rucksack oder auf Sir Canegm ziehen",inventory.items[i].label);
            if(ImGui::BeginDragDropSource()){bag.dragOffset={0,0};ImGui::SetDragDropPayload("CHARACTER_EQUIPMENT",&i,sizeof(i));bag.preview(i);ImGui::EndDragDropSource();}
            ImGui::PopID();
        }
        ImGui::SetCursorScreenPos(cursor);return hovered;
    }
    std::array<ImVec2,4> wornCenters{};
    BackpackScene* scene=nullptr;
    ImVec2 lastA{0,0},lastB{0,0};
    std::filesystem::path outfitState;
    Equipment::Inventory inventory;
    BackpackInventory bag;
    std::array<ImVec2,Equipment::itemCount> centers{};
    int selected=-1;
    bool visible=true;
    std::string error;
};
