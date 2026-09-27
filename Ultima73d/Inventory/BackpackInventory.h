#pragma once
#include "EquipmentInventory.h"
#include <imgui.h>
#include <functional>
#include <imgui_internal.h>
#include <glad/gl.h>
#include <SDL3/SDL.h>

struct BackpackInventory {
 std::array<unsigned,Equipment::itemCount> fittedIcons{};std::array<ImVec2,Equipment::itemCount> fittedSizes{};std::function<void(int)> preparePreview;
 void clearFitted(){for(auto id:fittedIcons)if(id)glDeleteTextures(1,&id);fittedIcons={};fittedSizes={};}

 ImVec2 panelPosition{1,1},panelStart{},panelMouse{},panelOrigin{};bool movingPanel=false;std::filesystem::path panelState;
 bool restorePanel(){std::ifstream in(panelState);std::string version;ImVec2 p;if(!(in>>version>>p.x>>p.y)||version!="backpack-panel-v1"||!std::isfinite(p.x)||!std::isfinite(p.y)||p.x<0||p.x>1||p.y<0||p.y>1)return false;panelPosition=p;return true;}
 bool savePanel(){std::ofstream out(panelState);out<<"backpack-panel-v1\n"<<std::setprecision(9)<<panelPosition.x<<' '<<panelPosition.y<<'\n';out.flush();return bool(out);}
 ImVec2 placePanel(ImVec2 a,ImVec2 b,float extent,bool enabled,std::string& error){
  float roomX=std::max(0.f,b.x-a.x-extent-24),roomY=std::max(0.f,b.y-a.y-extent-24);
  ImVec2 p(a.x+12+panelPosition.x*roomX,a.y+12+panelPosition.y*roomY);auto& io=ImGui::GetIO();
  bool flap=enabled&&io.MousePos.x>=p.x+extent*.12f&&io.MousePos.x<=p.x+extent*.88f&&io.MousePos.y>=p.y+extent*.03f&&io.MousePos.y<=p.y+extent*.25f;
  if(flap&&!ImGui::GetDragDropPayload()&&!movingPanel){ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);ImGui::SetTooltip("Rucksack hier greifen und verschieben");if(ImGui::IsMouseClicked(0)){movingPanel=true;panelStart=panelPosition;panelMouse=io.MousePos;panelOrigin=p;}}
  if(movingPanel){
   if(ImGui::IsKeyPressed(ImGuiKey_Escape)){panelPosition=panelStart;movingPanel=false;}
   else{
    panelPosition=ImVec2(roomX>0?std::clamp((panelOrigin.x+io.MousePos.x-panelMouse.x-a.x-12)/roomX,0.f,1.f):0,roomY>0?std::clamp((panelOrigin.y+io.MousePos.y-panelMouse.y-a.y-12)/roomY,0.f,1.f):0);
    if(!ImGui::IsMouseDown(0)){movingPanel=false;if(!savePanel())error="Rucksackposition konnte nicht gespeichert werden.";}
   }
   p=ImVec2(a.x+12+panelPosition.x*roomX,a.y+12+panelPosition.y*roomY);
  }return p;
 }

 std::array<ImVec2,Equipment::itemCount> groundRotation{};int rotatingGround=-1;bool groundRightHandled=false;
 std::array<bool,Equipment::itemCount> falling{};std::array<float,Equipment::itemCount> fallY{};
 std::array<bool,Equipment::itemCount> ground{};std::array<ImVec2,Equipment::itemCount> groundPositions{};std::filesystem::path groundState;
 void loadGround(const std::filesystem::path& path){groundState=path;ground={};groundRotation={};rotatingGround=-1;std::ifstream in(path);std::string version;if(!(in>>version)||(version!="ground-v1"&&version!="ground-v2"&&version!="ground-v3"))return;auto flags=ground;auto pos=groundPositions;auto rotations=groundRotation;for(int i=0;i<(version=="ground-v3"?Equipment::itemCount:5);++i){int flag;if(!(in>>flag>>pos[i].x>>pos[i].y)||(flag!=0&&flag!=1)||!std::isfinite(pos[i].x)||!std::isfinite(pos[i].y)||pos[i].x<0||pos[i].x>1||pos[i].y<0||pos[i].y>1)return;flags[i]=flag!=0;if((version=="ground-v2"||version=="ground-v3")&&(!(in>>rotations[i].x>>rotations[i].y)||!std::isfinite(rotations[i].x)||!std::isfinite(rotations[i].y)||std::abs(rotations[i].x)>3.142f||std::abs(rotations[i].y)>1.45f))return;}if(flags[4]&&!flags[3])pos[3]=pos[4];flags[3]=flags[3]||flags[4];flags[4]=false;ground=flags;groundPositions=pos;groundRotation=rotations;}
 bool saveGround(){std::ofstream out(groundState);out<<"ground-v3\n"<<std::setprecision(9);for(int i=0;i<Equipment::itemCount;++i)out<<ground[i]<<' '<<groundPositions[i].x<<' '<<groundPositions[i].y<<' '<<groundRotation[i].x<<' '<<groundRotation[i].y<<'\n';out.flush();return bool(out);}
 void drawIcon(int i,ImVec2 a,ImVec2 b){if(preparePreview)preparePreview(i);auto* d=ImGui::GetWindowDrawList();if(fittedIcons[i]){auto sz=fittedSizes[i];float factor=std::min((b.x-a.x)/sz.x,(b.y-a.y)/sz.y);ImVec2 mid((a.x+b.x)*.5f,(a.y+b.y)*.5f),half(sz.x*factor*.5f,sz.y*factor*.5f);d->AddImage((ImTextureID)(intptr_t)fittedIcons[i],ImVec2(mid.x-half.x,mid.y-half.y),ImVec2(mid.x+half.x,mid.y+half.y),ImVec2(0,1),ImVec2(1,0));return;}if(Equipment::boots(i)){float w=b.x-a.x;d->AddImage((ImTextureID)(intptr_t)icons[3],a,ImVec2(a.x+w*.68f,b.y));d->AddImage((ImTextureID)(intptr_t)icons[4],ImVec2(a.x+w*.32f,a.y),b);}else d->AddImage((ImTextureID)(intptr_t)icons[i],a,b);}
 bool previewOverBag=false;
 ImVec2 previewSize(int i)const{auto sz=fittedIcons[i]?fittedSizes[i]:ImVec2(56,56);if(previewOverBag){float factor=std::min(size*.17f/sz.x,size*.17f/sz.y);sz=ImVec2(sz.x*factor,sz.y*factor);}return sz;}
 void preview(int i){if(preparePreview)preparePreview(i);auto a=ImGui::GetCursorScreenPos();auto sz=previewSize(i);ImGui::Dummy(sz);drawIcon(i,a,ImVec2(a.x+sz.x,a.y+sz.y));}
 void drawGround(Equipment::Inventory& inventory,int& selected,ImVec2 a,ImVec2 b){
  groundRightHandled=rotatingGround>=0;auto& io=ImGui::GetIO();if(rotatingGround>=0){if(ImGui::IsMouseDown(1)){auto& r=groundRotation[rotatingGround];r.x=std::remainder(r.x+io.MouseDelta.x*.01f,6.2831853f);r.y=std::clamp(r.y+io.MouseDelta.y*.01f,-1.45f,1.45f);}else{saveGround();rotatingGround=-1;}}
  auto cursor=ImGui::GetCursorScreenPos();float width=b.x-a.x,height=b.y-a.y;
  for(int i=0;i<Equipment::itemCount;++i)if(i!=4)if(ground[i]&&!inventory.fits[i].worn){ImGui::PushID(i+100);if(falling[i]){fallY[i]=std::min(groundPositions[i].y,fallY[i]+ImGui::GetIO().DeltaTime*1.8f);falling[i]=fallY[i]<groundPositions[i].y;}ImVec2 center(a.x+groundPositions[i].x*width,a.y+(falling[i]?fallY[i]:groundPositions[i].y)*height);if(preparePreview)preparePreview(i);auto sz=fittedIcons[i]?fittedSizes[i]:ImVec2(54,54);ImVec2 start(center.x-sz.x*.5f,center.y-sz.y+2);
   ImGui::SetCursorScreenPos(start);if(ImGui::InvisibleButton("ground-item",sz))selected=i;
   if(ImGui::IsItemHovered()&&ImGui::IsMouseClicked(1)&&!ImGui::GetDragDropPayload()){rotatingGround=i;selected=i;groundRightHandled=true;}
   auto* d=ImGui::GetWindowDrawList();d->AddEllipseFilled(ImVec2(center.x,center.y+20),ImVec2(23,6),IM_COL32(0,0,0,90));
   drawIcon(i,start,ImVec2(start.x+sz.x,start.y+sz.y));
   if(ImGui::IsItemHovered())ImGui::SetTooltip("%s: links ziehen zum Aufheben, rechts ziehen zum Drehen",inventory.items[i].label);
   if(ImGui::BeginDragDropSource()){dragOffset={0,0};ImGui::SetDragDropPayload("CHARACTER_EQUIPMENT",&i,sizeof(i));selected=i;if(icons[i]||i>=5)preview(i);ImGui::TextUnformatted(inventory.items[i].label);ImGui::EndDragDropSource();}ImGui::PopID();
  }ImGui::SetCursorScreenPos(cursor);ImGui::Dummy(ImVec2(0,0));
 }
 unsigned background=0;std::array<unsigned,Equipment::itemCount> icons{};
 std::array<ImVec2,Equipment::itemCount> positions{{{.27f,.42f},{.50f,.42f},{.73f,.42f},{.27f,.55f},{.62f,.65f},{.50f,.55f},{.73f,.55f},{.27f,.68f},{.50f,.68f},{.73f,.68f}}};
 ImVec2 dragOffset{};ImVec2 origin{};float size=0;int moves=0,pendingUnequip=-1,pendingEquip=-1;
 std::filesystem::path state;
 static unsigned texture(const std::filesystem::path& path){
  auto* src=SDL_LoadBMP(path.string().c_str());if(!src)return 0;auto* image=SDL_ConvertSurface(src,SDL_PIXELFORMAT_RGBA32);SDL_DestroySurface(src);if(!image)return 0;
  unsigned id;glGenTextures(1,&id);glBindTexture(GL_TEXTURE_2D,id);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,image->w,image->h,0,GL_RGBA,GL_UNSIGNED_BYTE,image->pixels);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);SDL_DestroySurface(image);return id;
 }
 void load(const std::filesystem::path& assets,const std::filesystem::path& settings,const Equipment::Inventory& inventory){
  state=settings;panelState=settings;panelState+=".panel";restorePanel();restore();background=texture(assets/"backpack-inventory-v1.bmp");
  for(int i=0;i<Equipment::itemCount;++i)icons[i]=texture(assets/"equipment"/(std::string(inventory.items[i].id)+".bmp"));
 }
 bool restore(){std::ifstream in(state);std::string version;if(!(in>>version)||(version!="backpack-v1"&&version!="backpack-v2"))return false;auto candidate=positions;
  for(int i=0;i<(version=="backpack-v2"?Equipment::itemCount:5);++i){auto& p=candidate[i];if(!(in>>p.x>>p.y)||!std::isfinite(p.x)||!std::isfinite(p.y)||p.x<.23f||p.x>.77f||p.y<.40f||p.y>.70f)return false;
  }if(version=="backpack-v1")candidate=BackpackInventory{}.positions;positions=candidate;return true;
 }
 bool save(){std::ofstream out(state);out<<"backpack-v2\n"<<std::setprecision(9);for(auto p:positions)out<<p.x<<' '<<p.y<<'\n';out.flush();return bool(out);}
 void shutdown(){clearFitted();if(background)glDeleteTextures(1,&background);for(auto id:icons)if(id)glDeleteTextures(1,&id);background=0;icons={};}
 void draw(Equipment::Inventory& inventory,int& selected,std::array<ImVec2,Equipment::itemCount>& centers,std::string& error,float requestedSize=0){
  origin=ImGui::GetCursorScreenPos();size=requestedSize>0?requestedSize:std::max(140.f,std::min(420.f,ImGui::GetContentRegionAvail().x));
  auto* draw=ImGui::GetWindowDrawList();ImVec2 end(origin.x+size,origin.y+size);
  if(background)draw->AddImage((ImTextureID)(intptr_t)background,origin,end);
  else draw->AddRectFilled(origin,end,IM_COL32(65,37,17,255));
  const float half=size*.065f;
  for(int i=0;i<Equipment::itemCount;++i)if(i!=4){if(inventory.fits[i].worn||ground[i])continue;ImGui::PushID(inventory.items[i].id);centers[i]=ImVec2(origin.x+positions[i].x*size,origin.y+positions[i].y*size);
   ImVec2 a(centers[i].x-half,centers[i].y-half),b(centers[i].x+half,centers[i].y+half);
   ImGui::SetCursorScreenPos(a);ImGui::BeginDisabled(!inventory.items[i].available);
   if(ImGui::InvisibleButton("item",ImVec2(half*2,half*2)))selected=i;
   if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))pendingEquip=i;
   if(icons[i]||i>=5)drawIcon(i,a,b);else draw->AddText(a,IM_COL32_WHITE,inventory.items[i].label);
   if(inventory.fits[i].worn)draw->AddCircleFilled(ImVec2(b.x-4,a.y+4),3,IM_COL32(80,230,120,255));
   if(ImGui::IsItemHovered())ImGui::SetTooltip("%s%s\nDoppelklick: anziehen. Oder auf Sir Canegm ziehen.",inventory.items[i].label,inventory.fits[i].worn?" (angelegt)":"");
   if(ImGui::BeginDragDropSource()){
    if(!ImGui::GetDragDropPayload()){auto click=ImGui::GetIO().MouseClickedPos[0];dragOffset=ImVec2((click.x-centers[i].x)/size,(click.y-centers[i].y)/size);}
    ImGui::SetDragDropPayload("CHARACTER_EQUIPMENT",&i,sizeof(i));selected=i;
    if(icons[i]||i>=5)preview(i);ImGui::TextUnformatted(inventory.items[i].label);ImGui::EndDragDropSource();
   }ImGui::EndDisabled();ImGui::PopID();
  }
  ImRect interior(ImVec2(origin.x+.15f*size,origin.y+.32f*size),ImVec2(origin.x+.85f*size,origin.y+.78f*size));
  if(ImGui::BeginDragDropTargetCustom(interior,ImGui::GetID("backpack-drop"))){
   auto* payload=ImGui::AcceptDragDropPayload("CHARACTER_EQUIPMENT",ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
   bool fromCharacter=false;
   if(!payload){payload=ImGui::AcceptDragDropPayload("WORN_EQUIPMENT",ImGuiDragDropFlags_AcceptNoDrawDefaultRect);fromCharacter=payload!=nullptr;}
   if(payload&&payload->DataSize==sizeof(int)){
    int i=*static_cast<const int*>(payload->Data);if(i>=0&&i<Equipment::itemCount){if(fromCharacter){pendingUnequip=i;dragOffset={0,0};}ground[i]=false;if(!saveGround())error="Bodenablage konnte nicht gespeichert werden.";auto m=ImGui::GetIO().MousePos;
     positions[i]=ImVec2(std::clamp((m.x-origin.x)/size-dragOffset.x,.23f,.77f),std::clamp((m.y-origin.y)/size-dragOffset.y,.40f,.70f));selected=i;++moves;
     if(!save())error="Rucksackposition konnte nicht gespeichert werden.";
    }
   }ImGui::EndDragDropTarget();
  }
  ImGui::SetCursorScreenPos(origin);ImGui::Dummy(ImVec2(size,size));
 }
};
