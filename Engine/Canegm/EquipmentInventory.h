#pragma once
#include "CharacterLibrary.h"
#include "EquipmentCatalog.h"
#include "ArmorFitting.h"
#include "HelmetFitting.h"
#include "MeshNormals.h"
#include "GarmentCoverage.h"
#include <memory>
#include <array>
#include <cfloat>
#include <limits>
#include <sstream>

// All fitting operates in world space (glTF: Y up, character looking toward +Z).
namespace CanegmEquipment {
struct Bounds {
 std::array<float,3> lo{FLT_MAX,FLT_MAX,FLT_MAX},hi{-FLT_MAX,-FLT_MAX,-FLT_MAX};
 void add(const std::array<float,3>& p){for(int i=0;i<3;++i){lo[i]=std::min(lo[i],p[i]);hi[i]=std::max(hi[i],p[i]);}}
 float size(int i)const{return hi[i]-lo[i];} float center(int i)const{return (lo[i]+hi[i])*.5f;}
 bool valid()const{return size(0)>1e-5f&&size(1)>1e-5f&&size(2)>1e-5f;}
};
inline std::array<float,3> world(const GltfMesh& m,const GltfVertex& v){return {m.transform[0]*v.px+m.transform[4]*v.py+m.transform[8]*v.pz+m.transform[12],m.transform[1]*v.px+m.transform[5]*v.py+m.transform[9]*v.pz+m.transform[13],m.transform[2]*v.px+m.transform[6]*v.py+m.transform[10]*v.pz+m.transform[14]};}
inline Bounds bounds(const GltfModel& m){Bounds b;for(auto& mesh:m.meshes)for(auto& v:mesh.vertices)b.add(world(mesh,v));return b;}
inline Bounds region(const GltfModel& m,int slot){
 slot=category(slot);auto all=bounds(m);Bounds b;float h=all.size(1);
 for(auto& mesh:m.meshes)for(auto& v:mesh.vertices){auto p=world(mesh,v);float y=(p[1]-all.lo[1])/h,x=(p[0]-all.center(0))/all.size(0);
  bool use=slot==0?y>.84f:slot==1?(y>.55f&&y<.81f&&std::abs(x)<.3875f):slot==2?(y>.06f&&y<.55f&&std::abs(x)<.375f):(y<.26f&&(slot==3?x<0:x>0));
  if(use)b.add(p);
 }
 return b.valid()?b:all;
}
struct Fit {bool worn=false; std::array<float,3> scale{1,1,1},offset{0,0,0};std::array<float,2> legShape{1,1};float thighScale=1;std::array<float,2> innerCalf{1,1};float hemRaise=0;std::array<float,5> regionHeight{};std::array<float,2> innerThigh{1,1},innerThighHeight{};std::array<float,6> bootShape{1,1,1,1,1,1};ArmorFitting::Pulls armorPull{};float bootMiddle=1;bool closeBootGaps=false;std::array<float,2> bootCalfDepth{};bool trousersTucked=false;std::array<float,4> calfPull{},calfPullHeight{};HelmetFitting::Pulls helmetPull{};float yaw=0;bool operator==(const Fit&)const=default;};
struct Item {const char* id;const char* label;GltfModel model;bool available=false;std::string error;};
class Inventory {
 struct TrouserBootCache{uint64_t key=0;std::vector<GltfMesh> meshes;};
 mutable std::shared_ptr<TrouserBootCache> trouserBootCache;
public:
 std::array<Item,itemCount> items{{{"helmet","Lederhelm"},{"armor","Lederruestung"},{"trousers","Lederhose"},{"boot-right","Stiefelpaar"},{"boot-left","Stiefelpaar"},{"trousers-1","Dunkle Schnuerhose"},{"trousers-2","Leinenhose"},{"trousers-3","Gruene Bundhose"},{"trousers-4","Verstaerkte Lederhose"},{"trousers-5","Blaue Kniehose"}}};
 std::array<Fit,itemCount> fits{};GltfModel reference;bool ready=false;
 size_t assetRevision=0;
 bool load(const std::filesystem::path& dir){
  ++assetRevision;
  const auto assets=std::filesystem::exists(dir/"fitted-v2/fit.json")?dir/"fitted-v2":
   (std::filesystem::exists(dir/"textured-v1/textures.json")?dir/"textured-v1":dir);
  std::string error;ready=CharacterLibrary::read(assets/"body.glb",reference,error);
  for(auto& item:items)item.available=CharacterLibrary::read(assets/(std::string(item.id)+".glb"),item.model,item.error);
  return ready&&std::all_of(items.begin(),items.end(),[](auto& i){return i.available;});
 }
 bool equip(int slot,bool worn){if(slot<0||slot>=itemCount||!ready||!items[slot].available)return false;if(boots(slot)){if(!items[3].available||!items[4].available)return false;fits[3].worn=fits[4].worn=worn;}else {if(worn&&trousers(slot))for(int i=0;i<itemCount;++i)if(trousers(i))fits[i].worn=false;fits[slot].worn=worn;}return true;}
 bool resetFit(int item){if(item<0||item>=itemCount||!items[item].available)return false;bool worn=fits[item].worn;fits[item]=Fit{};fits[item].worn=worn;return true;}
 bool fitToBody(const GltfModel& body,int item){if(item<0||item>=itemCount||!fits[item].worn||!items[item].available)return false;
  // The prepared reference fit is mapped to this body's anatomical regions by compose.
  fits[item].scale={1,1,1};fits[item].offset={0,0,0};
  if(item==1)snapArmor(body);else if(boots(item))fits[item].closeBootGaps=true;
  return true;
 }
 void clear(){fits={};}
 bool save(const std::filesystem::path& path)const{
  auto temporary=path;temporary+=".tmp";
  std::ofstream f(temporary);if(!f)return false;f<<"equipment-v20\n"<<std::setprecision(9);
  for(int i=0;i<itemCount;++i){f<<items[i].id<<' '<<fits[i].worn;for(auto x:fits[i].scale)f<<' '<<x;for(auto x:fits[i].offset)f<<' '<<x;for(auto x:fits[i].legShape)f<<' '<<x;f<<' '<<fits[i].thighScale;for(auto x:fits[i].innerCalf)f<<' '<<x;f<<' '<<fits[i].hemRaise;for(auto x:fits[i].regionHeight)f<<' '<<x;for(auto x:fits[i].innerThigh)f<<' '<<x;for(auto x:fits[i].innerThighHeight)f<<' '<<x;for(auto x:fits[i].bootShape)f<<' '<<x;for(auto& p:fits[i].armorPull)for(auto x:p)f<<' '<<x;f<<' '<<fits[i].bootMiddle<<' '<<fits[i].closeBootGaps;for(auto x:fits[i].bootCalfDepth)f<<' '<<x;f<<' '<<fits[i].trousersTucked;for(auto x:fits[i].calfPull)f<<' '<<x;for(auto x:fits[i].calfPullHeight)f<<' '<<x;for(auto& p:fits[i].helmetPull)for(auto x:p)f<<' '<<x;f<<' '<<fits[i].yaw;f<<'\n';}f.flush();if(!f)return false;f.close();
  // Windows rename cannot replace an existing file. Keep the old settings until
  // a complete new file has been written, then copy and remove only our temp.
  std::error_code ec;std::filesystem::copy_file(temporary,path,std::filesystem::copy_options::overwrite_existing,ec);if(ec)return false;std::filesystem::remove(temporary,ec);return true;
 }
 bool restore(const std::filesystem::path& path){
  clear();std::ifstream f(path);std::string version;if(!(f>>version))return false;int format=0;for(int i=1;i<=20;++i)if(version=="equipment-v"+std::to_string(i))format=i;if(!format)return false;auto candidate=fits;
  for(int i=0;i<(format>=8?itemCount:5);++i){std::string id;int worn;if(!(f>>id>>worn)||id!=items[i].id||(worn!=0&&worn!=1))return false;candidate[i].worn=worn!=0;
   for(auto& x:candidate[i].scale)if(!(f>>x)||!std::isfinite(x)||x<.25f||x>3.f)return false;
   for(auto& x:candidate[i].offset)if(!(f>>x)||!std::isfinite(x)||std::abs(x)>.5f)return false;
   if(format>=2)for(auto& x:candidate[i].legShape)if(!(f>>x)||!std::isfinite(x)||x<.6f||x>2.f)return false;
   if(format>=3&&(!(f>>candidate[i].thighScale)||!std::isfinite(candidate[i].thighScale)||candidate[i].thighScale<.6f||candidate[i].thighScale>2.f))return false;
   if(format>=4)for(auto& x:candidate[i].innerCalf)if(!(f>>x)||!std::isfinite(x)||x<.6f||x>2.f)return false;
   if(format>=5&&(!(f>>candidate[i].hemRaise)||!std::isfinite(candidate[i].hemRaise)||candidate[i].hemRaise<0||candidate[i].hemRaise>.35f))return false;
   if(format>=6)for(auto& x:candidate[i].regionHeight)if(!(f>>x)||!std::isfinite(x)||std::abs(x)>.15f)return false;
   if(format>=7){for(auto& x:candidate[i].innerThigh)if(!(f>>x)||!std::isfinite(x)||x<.6f||x>2.f)return false;for(auto& x:candidate[i].innerThighHeight)if(!(f>>x)||!std::isfinite(x)||std::abs(x)>.15f)return false;}
   if(format>=9)for(auto& x:candidate[i].bootShape)if(!(f>>x)||!std::isfinite(x)||x<.5f||x>2.f)return false;
   if(format>=10)for(int j=0;j<(format>=18?ArmorFitting::count:format>=16?16:14);++j)for(auto& x:candidate[i].armorPull[j])if(!(f>>x)||!std::isfinite(x)||std::abs(x)>.35f)return false;
   if(format>=11){auto& x=candidate[i].bootMiddle;if(!(f>>x)||!std::isfinite(x)||x<.5f||x>2.f)return false;}
   if(format>=12){int enabled;if(!(f>>enabled)||(enabled!=0&&enabled!=1))return false;candidate[i].closeBootGaps=enabled!=0;}
   if(format>=13)for(auto& x:candidate[i].bootCalfDepth)if(!(f>>x)||!std::isfinite(x)||x<-.35f||x>.5f)return false;
   if(format>=14){int enabled;if(!(f>>enabled)||(enabled!=0&&enabled!=1))return false;candidate[i].trousersTucked=enabled!=0;}
   if(format>=15){for(auto& x:candidate[i].calfPull)if(!(f>>x)||!std::isfinite(x)||x<-.2f||x>.4f)return false;for(auto& x:candidate[i].calfPullHeight)if(!(f>>x)||!std::isfinite(x)||std::abs(x)>.15f)return false;}
   if(format>=17)for(int j=0;j<(format>=20?HelmetFitting::count:7);++j)for(auto& x:candidate[i].helmetPull[j])if(!(f>>x)||!std::isfinite(x)||std::abs(x)>.35f)return false;
   if(format>=19&&(!(f>>candidate[i].yaw)||!std::isfinite(candidate[i].yaw)||std::abs(candidate[i].yaw)>180))return false;
  }std::string extra;if(f>>extra)return false;candidate[3].worn=candidate[4].worn=candidate[3].worn||candidate[4].worn;if(format<8)for(int i=5;i<itemCount;++i){candidate[i]=candidate[2];candidate[i].worn=false;}int wornTrousers=0;for(int i=0;i<itemCount;++i)if(trousers(i)&&candidate[i].worn)++wornTrousers;if(wornTrousers>1)return false;fits=candidate;return true;
 }
 // Fit the torso with the same smooth local controls used by the green grips.
 // Radial queries start inside the torso; missing surface hits leave a region alone.
 bool snapArmor(const GltfModel& body){
  if(!ready||!items[1].available||!fits[1].worn)return false;
  auto b=fittedBounds(body,1),torso=region(body,1);float h=bounds(body).size(1);
  if(!b.valid()||h<=0)return false;
  GarmentCoverage::Surface surface(body);auto before=fits[1];
  for(int iteration=0;iteration<12;++iteration){auto next=fits[1].armorPull;
   for(int j=0;j<ArmorFitting::count;++j){
    auto p=ArmorFitting::deform(ArmorFitting::anchors[j],fits[1].armorPull);
    for(int a=0;a<3;++a)p[a]=b.lo[a]+p[a]*b.size(a);
    GarmentCoverage::V origin{torso.center(0),p[1],torso.center(2)},d=GarmentCoverage::sub(p,origin);
    float r=std::sqrt(GarmentCoverage::dot(d,d));if(r<1e-6f)continue;d=GarmentCoverage::mul(d,1/r);
    float hit=surface.radius(origin,d);if(hit==FLT_MAX||hit>h*.3f)continue;
    float correction=std::clamp(hit+h*.004f-r,-h*.025f,h*.025f);
    for(int a:{0,2})next[j][a]=std::clamp(next[j][a]+.6f*correction*d[a]/b.size(a),-.35f,.35f);
   }fits[1].armorPull=next;
  }return fits[1]!=before;
 }
 Bounds fittedBounds(const GltfModel& body,int slot)const{
  auto source=region(reference,slot),target=region(body,slot),b=bounds(items[slot].model);float h=bounds(body).size(1);
  for(int a=0;a<3;++a){float scale=target.size(a)/source.size(a)*fits[slot].scale[a];float shift=target.center(a)-source.center(a)*scale+fits[slot].offset[a]*h;b.lo[a]=b.lo[a]*scale+shift;b.hi[a]=b.hi[a]*scale+shift;}return b;
 }
 std::array<float,3> rotatePoint(const GltfModel& body,int slot,std::array<float,3> p)const{auto b=fittedBounds(body,slot);float angle=fits[slot].yaw*.01745329252f,c=std::cos(angle),s=std::sin(angle),x=p[0]-b.center(0),z=p[2]-b.center(2);return {b.center(0)+c*x+s*z,p[1],b.center(2)-s*x+c*z};}
 GltfModel compose(const GltfModel& body,bool includeImages=true,bool bindEquipment=true)const{
  GltfModel out;out.animation=body.animation;out.meshes=body.meshes;out.materials=body.materials;if(includeImages)out.images=body.images;int nextImage=int(body.images.size());if(!ready||!bounds(body).valid())return out;
  unsigned mask=0,coverageMask=0;for(int i=0;i<itemCount;++i)if(fits[i].worn&&items[i].available){
   mask|=1u<<category(i);
   // Prepared coverage is valid only at the automatic fit. If a user moves or
   // resizes a garment, keep the complete body to avoid exposing missing faces.
   if(fits[i].scale==std::array<float,3>{1,1,1}&&fits[i].offset==std::array<float,3>{0,0,0}&&fits[i].legShape==std::array<float,2>{1,1}&&fits[i].thighScale==1&&fits[i].innerCalf==std::array<float,2>{1,1}&&fits[i].hemRaise==0&&fits[i].regionHeight==std::array<float,5>{}&&fits[i].innerThigh==std::array<float,2>{1,1}&&fits[i].innerThighHeight==std::array<float,2>{}&&fits[i].bootShape==std::array<float,6>{1,1,1,1,1,1}&&fits[i].armorPull==ArmorFitting::Pulls{}&&fits[i].bootMiddle==1&&fits[i].bootCalfDepth==std::array<float,2>{}&&fits[i].calfPull==std::array<float,4>{}&&fits[i].calfPullHeight==std::array<float,4>{}&&i<5)coverageMask|=1u<<i;
  }
  // Keep hair and neck geometry intact; fit covered hair beneath the actual cap.
  coverageMask&=~3u;
  // Only source-authored coverage groups are hidden automatically; unknown
  // imported bodies remain intact, so differing poses never acquire holes.
  std::erase_if(out.meshes,[&](const auto& m){if(m.materialIndex<0||m.materialIndex>=int(out.materials.size()))return false;auto n=out.materials[m.materialIndex].name;
   if(n.rfind("body-mask-",0)!=0)return false;unsigned covered=0;std::istringstream(n.substr(10))>>covered;return (covered&coverageMask)!=0;});
  auto bodyBounds=bounds(body);float height=bodyBounds.size(1);
  std::unique_ptr<GarmentCoverage::Surface> bootBodySurface;
  std::array<std::array<Bounds,32>,2> legRings;
  if((fits[3].worn&&fits[3].closeBootGaps)||(fits[4].worn&&fits[4].closeBootGaps)){
   bootBodySurface=std::make_unique<GarmentCoverage::Surface>(body);
   for(const auto& part:body.meshes)for(const auto& v:part.vertices){auto p=world(part,v);float y=(p[1]-bodyBounds.lo[1])/height;if(y<0||y>.30f)continue;int side=p[0]<bodyBounds.center(0)?0:1;int ring=std::clamp(int(y/.30f*32),0,31);legRings[side][ring].add(p);}
  }


  for(int slot=0;slot<itemCount;++slot){if(!fits[slot].worn||!items[slot].available)continue;
   auto source=region(reference,slot),target=region(body,slot);const auto& fit=fits[slot];
   std::array<float,3> scale,shift;
   for(int a=0;a<3;++a){scale[a]=target.size(a)/source.size(a)*fit.scale[a];shift[a]=target.center(a)-source.center(a)*scale[a]+fit.offset[a]*height;}
   const auto& item=items[slot].model;int imageOffset=nextImage,materialOffset=int(out.materials.size());nextImage+=int(item.images.size());
   if(includeImages)out.images.insert(out.images.end(),item.images.begin(),item.images.end());
   for(auto material:item.materials){if(material.baseColorImage>=0)material.baseColorImage+=imageOffset;else{material.baseColorFactor[0]=.25f;material.baseColorFactor[1]=.105f;material.baseColorFactor[2]=.038f;}out.materials.push_back(material);}
   for(auto mesh:item.meshes){
    if(slot==0&&fit.helmetPull!=HelmetFitting::Pulls{}){
     auto b=bounds(item);for(auto& v:mesh.vertices){auto p=world(mesh,v);for(int a=0;a<3;++a)p[a]=(p[a]-b.lo[a])/b.size(a);p=HelmetFitting::deform(p,fit.helmetPull);v.px=b.lo[0]+p[0]*b.size(0);v.py=b.lo[1]+p[1]*b.size(1);v.pz=b.lo[2]+p[2]*b.size(2);}
     const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};std::copy(identity,identity+16,mesh.transform);updateSmoothNormals(mesh.vertices,mesh.indices,makeNormalGroups(mesh.vertices));
    }
    if(slot==1&&fit.armorPull!=ArmorFitting::Pulls{}){
     auto b=bounds(item);for(auto& v:mesh.vertices){auto p=world(mesh,v);for(int a=0;a<3;++a)p[a]=(p[a]-b.lo[a])/b.size(a);p=ArmorFitting::deform(p,fit.armorPull);v.px=b.lo[0]+p[0]*b.size(0);v.py=b.lo[1]+p[1]*b.size(1);v.pz=b.lo[2]+p[2]*b.size(2);}
     const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};std::copy(identity,identity+16,mesh.transform);updateSmoothNormals(mesh.vertices,mesh.indices,makeNormalGroups(mesh.vertices));
    }
    if(boots(slot)){
     const auto b=bounds(item);Bounds shaft;for(auto& part:item.meshes)for(auto& v:part.vertices){auto p=world(part,v);if(p[1]>b.lo[1]+b.size(1)*.55f)shaft.add(p);}
     float cx=shaft.center(0),cz=shaft.center(2);auto smooth=[](float a,float b,float t){t=std::clamp((t-a)/(b-a),0.f,1.f);return t*t*(3-2*t);};
     for(auto& v:mesh.vertices){auto p=world(mesh,v);float y=(p[1]-b.lo[1])/b.size(1),z=(p[2]-b.lo[2])/b.size(2);
      float upper=smooth(.35f,.65f,y),ankle=smooth(.10f,.23f,y)*(1-smooth(.35f,.52f,y)),foot=1-smooth(.20f,.38f,y);
      float middle=smooth(.32f,.48f,y)*(1-smooth(.58f,.75f,y));
      p[0]=cx+(p[0]-cx)*(1+(fit.bootMiddle-1)*middle+(fit.bootShape[0]-1)*upper+(fit.bootShape[1]-1)*ankle+(fit.bootShape[2]-1)*foot);
      p[2]=cz+(p[2]-cz)*(1+(fit.bootMiddle-1)*middle+(fit.bootShape[0]-1)*upper+(fit.bootShape[1]-1)*ankle);
      p[2]+=b.size(2)*((fit.bootShape[4]-1)*smooth(.45f,.9f,z)-(fit.bootShape[3]-1)*(1-smooth(.1f,.4f,z)))*foot;
      float calf=smooth(.40f,.62f,y)*(1-smooth(.88f,1.f,y));
      float front=smooth(cz-b.size(2)*.05f,cz+b.size(2)*.18f,p[2]);
      float rear=1-smooth(cz-b.size(2)*.18f,cz+b.size(2)*.05f,p[2]);
      p[2]+=b.size(2)*calf*(fit.bootCalfDepth[0]*front-fit.bootCalfDepth[1]*rear);
      p[1]+=b.size(1)*(fit.bootShape[5]-1)*std::max(0.f,y-.35f);
      v.px=p[0];v.py=p[1];v.pz=p[2];
     }
     const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};std::copy(identity,identity+16,mesh.transform);
     updateSmoothNormals(mesh.vertices,mesh.indices,makeNormalGroups(mesh.vertices));
    }
    if(trousers(slot)){
     // Derive the two lower-leg centerlines from the garment, independent of
     // character size, source coordinates and trouser proportions.
     auto garment=bounds(item);float h=garment.size(1);std::array<std::array<Bounds,16>,2> rings;
     for(const auto& part:item.meshes)for(const auto& vertex:part.vertices){auto p=world(part,vertex);float y=(p[1]-garment.lo[1])/h;int side=p[0]<garment.center(0)?0:1;int band=std::clamp(int(y*16),0,15);rings[side][band].add(p);}
     auto centerAt=[&](int side,float y,int coordinate){float t=std::clamp(y*16-.5f,0.f,15.f);int a=int(t),b=std::min(a+1,15);auto center=[&](int band){for(int n=0;n<16;++n){int i=std::clamp(band+(n%2?-1:1)*((n+1)/2),0,15);if(rings[side][i].lo[coordinate]<=rings[side][i].hi[coordinate])return rings[side][i].center(coordinate);}return garment.center(coordinate);};return center(a)+(center(b)-center(a))*(t-a);};
     auto smooth=[](float a,float b,float y){float t=std::clamp((y-a)/(b-a),0.f,1.f);return t*t*(3-2*t);};
     for(auto& v:mesh.vertices){auto p=world(mesh,v);float y=(p[1]-garment.lo[1])/h;int side=p[0]<garment.center(0)?0:1;
      float cy=y,ay=y,iy=y;
      float protect=1-smooth(.82f,.98f,y);
      float calf=smooth(.08f,.25f,cy)*(1-smooth(.42f,.60f,cy))*protect,ankle=(1-smooth(.08f,.30f,ay))*protect;
      float innerWeight=smooth(.08f,.25f,iy)*(1-smooth(.42f,.60f,iy))*protect;
      {
       float cx=centerAt(side,y,0),cz=centerAt(side,y,2);
       // +Z is the shin/front. Calf adjustment affects only the rear half,
       // fading with zero slope at the centerline; the shin stays fixed.
       float inward=side==0?1.f:-1.f;
       float medial=std::max(0.f,(p[0]-cx)*inward);
       float innerDelta=inward*(fit.innerCalf[side]-1)*innerWeight*medial*smooth(0.f,garment.size(0)*.08f,medial);
       float rear=std::max(0.f,cz-p[2]);
       float rearWeight=smooth(0.f,garment.size(2)*.20f,rear);
       float ankleFactor=1+(fit.legShape[1]-1)*ankle;
       p[0]=cx+(p[0]-cx)*(ankleFactor+(fit.legShape[0]-1)*calf*rearWeight);
       p[2]=cz+(p[2]-cz)*ankleFactor-(fit.legShape[0]-1)*calf*rear*rearWeight;
       p[0]+=innerDelta;
      }
      float ty=y;
      float thigh=smooth(.45f,.65f,ty)*(1-smooth(.85f,.98f,ty));
      float thighFactor=1+(fit.thighScale-1)*thigh;
      float tx=centerAt(side,y,0),tz=centerAt(side,y,2);
      p[0]=tx+(p[0]-tx)*thighFactor;p[2]=tz+(p[2]-tz)*thighFactor;
      float centerX=centerAt(side,y,0),centerZ=centerAt(side,y,2);
      float medialDistance=std::max(0.f,(p[0]-centerX)*(side==0?1.f:-1.f));
      float innerThighWeight=smooth(.48f,.65f,y)*(1-smooth(.83f,.95f,y));
      float innerThighMask=smooth(0.f,garment.size(0)*.08f,medialDistance)*smooth(0.f,garment.size(0)*.03f,std::abs(p[0]-garment.center(0)));
      p[0]+=(side==0?1.f:-1.f)*(fit.innerThigh[side]-1)*innerThighWeight*medialDistance*innerThighMask;
      p[1]+=h*fit.innerThighHeight[side]*innerThighWeight*innerThighMask;
      float innerMask=smooth(0.f,garment.size(0)*.08f,medialDistance);
      float rearMask=smooth(0.f,garment.size(2)*.20f,std::max(0.f,centerZ-p[2]));
      p[1]+=h*(fit.regionHeight[0]*calf*rearMask+fit.regionHeight[1]*ankle+fit.regionHeight[2]*thigh+fit.regionHeight[3+side]*innerWeight*innerMask);
      float frontMask=smooth(0.f,garment.size(2)*.16f,std::max(0.f,p[2]-centerZ));
      float outerMask=smooth(0.f,garment.size(0)*.06f,std::max(0.f,(p[0]-centerX)*(side==0?-1.f:1.f)));
      p[2]+=garment.size(2)*fit.calfPull[side]*calf*frontMask;
      p[0]+=(side==0?-1.f:1.f)*garment.size(0)*fit.calfPull[2+side]*calf*outerMask;
      p[1]+=h*calf*(fit.calfPullHeight[side]*frontMask+fit.calfPullHeight[2+side]*outerMask);
      // Monotone shortening of lower legs, tapering to zero below the hips.
      p[1]+=h*fit.hemRaise*(1-smooth(0.f,.65f,y));
      v.px=p[0];v.py=p[1];v.pz=p[2];
     }
     const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};std::copy(identity,identity+16,mesh.transform);
     updateSmoothNormals(mesh.vertices,mesh.indices,makeNormalGroups(mesh.vertices));
    }
    mesh.name="equipment/"+std::string(items[slot].id)+"/"+mesh.name;if(mesh.materialIndex>=0)mesh.materialIndex+=materialOffset;
    // Left-multiply the node transform by the world-space fit. Renderer applies
    // the inverse transpose to normals, preserving nonuniform scale lighting.
    for(int col=0;col<4;++col)for(int row=0;row<3;++row)mesh.transform[col*4+row]*=scale[row];
    for(int row=0;row<3;++row)mesh.transform[12+row]+=shift[row];
    if(boots(slot)&&fit.closeBootGaps&&bootBodySurface){
     // Keep the fitted silhouette where it already clears the skin. Push only
     // penetrating leather radially outward around the corresponding leg.
     int side=slot-3;
     auto ringCenter=[&](int ring,int axis){for(int n=0;n<32;++n){int k=std::clamp(ring+(n%2?-1:1)*((n+1)/2),0,31);auto& r=legRings[side][k];if(r.lo[axis]<=r.hi[axis])return r.center(axis);}return region(body,slot).center(axis);};
     for(auto& v:mesh.vertices){auto p=world(mesh,v);float y=(p[1]-bodyBounds.lo[1])/height;
      if(y>.008f&&y<.30f){float band=std::clamp(y/.30f*32-.5f,0.f,31.f);int low=int(band),high=std::min(low+1,31);float blend=band-low;GarmentCoverage::V origin{0,p[1],0};
       for(int a:{0,2})origin[a]=ringCenter(low,a)*(1-blend)+ringCenter(high,a)*blend;
       auto d=GarmentCoverage::sub(p,origin);float radius=std::sqrt(GarmentCoverage::dot(d,d));if(radius>1e-6f){d=GarmentCoverage::mul(d,1/radius);float hit=bootBodySurface->radius(origin,d);if(hit!=FLT_MAX&&hit<height*.14f&&radius<hit+height*.003f)p=GarmentCoverage::add(origin,GarmentCoverage::mul(d,hit+height*.003f));}
      }v.px=p[0];v.py=p[1];v.pz=p[2];
     }
     const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};std::copy(identity,identity+16,mesh.transform);updateSmoothNormals(mesh.vertices,mesh.indices,makeNormalGroups(mesh.vertices));
    }
    if(slot==0&&fit.yaw!=0){
     // Rotate the node matrix once, not the vertices. Recomputing fittedBounds
     // inside the vertex loop previously made a single turn quadratic in mesh size.
     auto pivot=fittedBounds(body,slot);float angle=fit.yaw*.01745329252f,c=std::cos(angle),s=std::sin(angle);
     for(int col=0;col<4;++col){float x=mesh.transform[col*4],z=mesh.transform[col*4+2];if(col==3){x-=pivot.center(0);z-=pivot.center(2);}mesh.transform[col*4]=c*x+s*z;mesh.transform[col*4+2]=-s*x+c*z;if(col==3){mesh.transform[col*4]+=pivot.center(0);mesh.transform[col*4+2]+=pivot.center(2);}}
    }
    if(!mesh.indices.empty())out.meshes.push_back(std::move(mesh));
   }
  }
  // Preserve outer trousers, or fit their lower legs inside the boots when tucked.
  if(fits[3].worn&&fits[4].worn)for(int trouser=0;trouser<itemCount;++trouser)if(trousers(trouser)&&fits[trouser].worn){
   if(!fits[trouser].trousersTucked){
    // Keep every boot face and UV. Fit only the covered shaft inside the
    // trouser surface; never cut away the exposed foot at a global hem plane.
    GltfModel cloth;auto pantsPrefix="equipment/"+std::string(items[trouser].id)+"/";
    for(auto& m:out.meshes)if(m.name.rfind(pantsPrefix,0)==0)cloth.meshes.push_back(m);
    GarmentCoverage::Surface surface(cloth);
    for(int boot=3;boot<=4;++boot){auto bootPrefix="equipment/"+std::string(items[boot].id)+"/";Bounds bb,shaft;
     for(auto& m:out.meshes)if(m.name.rfind(bootPrefix,0)==0)for(auto& v:m.vertices)bb.add(world(m,v));if(!bb.valid())continue;
     for(auto& m:out.meshes)if(m.name.rfind(bootPrefix,0)==0)for(auto& v:m.vertices){auto p=world(m,v);if(p[1]>bb.lo[1]+bb.size(1)*.5f)shaft.add(p);}if(!shaft.valid())continue;
     for(auto& m:out.meshes)if(m.name.rfind(bootPrefix,0)==0){for(auto& v:m.vertices){auto p=world(m,v);
       if(p[1]>bb.lo[1]+bb.size(1)*.35f){GarmentCoverage::V o{shaft.center(0),p[1],shaft.center(2)},d=GarmentCoverage::sub(p,o);float r=std::sqrt(GarmentCoverage::dot(d,d));if(r>1e-6f){d=GarmentCoverage::mul(d,1/r);float hit=surface.radius(o,d);if(hit!=FLT_MAX&&hit>height*.005f&&hit<height*.14f&&r>hit-height*.003f)p=GarmentCoverage::add(o,GarmentCoverage::mul(d,std::max(height*.002f,hit-height*.003f)));}}
       v.px=p[0];v.py=p[1];v.pz=p[2];
      }const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};std::copy(identity,identity+16,m.transform);updateSmoothNormals(m.vertices,m.indices,makeNormalGroups(m.vertices));
     }
    }continue;
   }
   uint64_t fitKey=1469598103934665603ull;auto prefix="equipment/"+std::string(items[trouser].id)+"/";
   for(auto& m:out.meshes)if(m.name.rfind(prefix,0)==0||m.name.rfind("equipment/boot-",0)==0){fitKey=GarmentCoverage::geometryKey(m,fitKey);GarmentCoverage::hashValue(fitKey,uint32_t(m.materialIndex));}
   for(auto p:{bodyBounds.lo,bodyBounds.hi})for(auto value:p)GarmentCoverage::hashValue(fitKey,std::bit_cast<uint32_t>(value));GarmentCoverage::hashValue(fitKey,fits[trouser].trousersTucked);GarmentCoverage::hashValue(fitKey,trouser);
   if(trouserBootCache&&trouserBootCache->key==fitKey){for(auto& m:out.meshes)if(m.name.rfind(prefix,0)==0)for(auto& cached:trouserBootCache->meshes)if(cached.name==m.name){m=cached;break;}continue;}
   for(int boot=3;boot<=4;++boot){GltfModel shell;for(auto& m:out.meshes)if(m.name.rfind("equipment/"+std::string(items[boot].id)+"/",0)==0)shell.meshes.push_back(m);auto bb=bounds(shell);if(!bb.valid())continue;Bounds shaft;for(auto& m:shell.meshes)for(auto& v:m.vertices){auto p=world(m,v);if(p[1]>bb.lo[1]+bb.size(1)*.5f)shaft.add(p);}if(!shaft.valid())continue;GarmentCoverage::Surface surface(shell);
    auto shellRadius=[&](GarmentCoverage::V o,GarmentCoverage::V d){o[1]=std::min(o[1],bb.hi[1]-height*.001f);float distance=height*.3f;float hit=surface.radius(GarmentCoverage::add(o,GarmentCoverage::mul(d,distance)),GarmentCoverage::mul(d,-1));return hit==FLT_MAX?FLT_MAX:distance-hit;};
    for(auto& m:out.meshes)if(m.name.rfind("equipment/"+std::string(items[trouser].id)+"/",0)==0){
     for(auto& v:m.vertices){auto p=world(m,v);bool side=boot==3?p[0]<bodyBounds.center(0):p[0]>=bodyBounds.center(0);
      if(side&&p[1]>bb.lo[1]&&p[1]<bb.hi[1]+height*.035f){GarmentCoverage::V o{shaft.center(0),p[1],shaft.center(2)},d=GarmentCoverage::sub(p,o);float radius=std::sqrt(GarmentCoverage::dot(d,d));if(radius>1e-6f){d=GarmentCoverage::mul(d,1/radius);float hit=shellRadius(o,d);if(hit!=FLT_MAX&&hit>0&&hit<height*.15f){bool tuck=fits[trouser].trousersTucked;float target=std::max(height*.002f,hit+(tuck?-1.f:1.f)*height*.006f);if(tuck?radius>target:radius<target){float w=std::clamp((bb.hi[1]+height*.035f-p[1])/(height*.035f),0.f,1.f);w=w*w*(3-2*w);p=GarmentCoverage::add(o,GarmentCoverage::mul(d,radius+(target-radius)*w));}}}}
      v.px=p[0];v.py=p[1];v.pz=p[2];
     }
     const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};std::copy(identity,identity+16,m.transform);updateSmoothNormals(m.vertices,m.indices,makeNormalGroups(m.vertices));
    }
   }
   auto cached=std::make_shared<TrouserBootCache>();cached->key=fitKey;for(auto& m:out.meshes)if(m.name.rfind(prefix,0)==0)cached->meshes.push_back(m);trouserBootCache=std::move(cached);
  }
  // The new trouser cuts cover different leg lengths. Hide only authored
  // trouser-covered skin inside the currently fitted garment's vertical span.
  for(int slot=5;slot<itemCount;++slot)if(fits[slot].worn){auto b=fittedBounds(body,slot);for(auto& m:out.meshes){if(m.name.rfind("equipment/",0)==0||m.materialIndex<0)continue;auto name=out.materials[m.materialIndex].name;unsigned group=0;if(name.rfind("body-mask-",0)==0)std::istringstream(name.substr(10))>>group;if(!(group&4))continue;std::vector<unsigned> visible;
   for(size_t t=0;t<m.indices.size();t+=3){bool covered=true;for(int j=0;j<3;++j){auto p=world(m,m.vertices[m.indices[t+j]]);covered&=p[1]>b.lo[1]+height*.018f&&p[1]<b.hi[1]-height*.005f&&p[0]>b.lo[0]&&p[0]<b.hi[0];}if(!covered)visible.insert(visible.end(),m.indices.begin()+t,m.indices.begin()+t+3);}m.indices=std::move(visible);
  }std::erase_if(out.meshes,[](auto& m){return m.indices.empty();});}
  if(mask&1u){
   GltfModel shell;for(auto& m:out.meshes)if(m.name.rfind("equipment/helmet/",0)==0)shell.meshes.push_back(m);
   GarmentCoverage::Surface surface(shell);auto head=region(body,0);
   GarmentCoverage::V origin{head.center(0),bodyBounds.lo[1]+height*.90f,head.center(2)};
   for(auto& m:out.meshes)if(m.name.rfind("equipment/",0)!=0){
    for(auto& v:m.vertices){auto p=world(m,v);if(p[1]>bodyBounds.lo[1]+height*.84f){auto d=GarmentCoverage::sub(p,origin);float r=std::sqrt(GarmentCoverage::dot(d,d));if(r>1e-6f){d=GarmentCoverage::mul(d,1/r);float hit=surface.radius(origin,d);if(hit!=FLT_MAX&&hit>height*.015f&&hit<height*.2f&&r>hit-height*.0015f){float target=hit-height*.0025f;p=GarmentCoverage::add(origin,GarmentCoverage::mul(d,target));}}}v.px=p[0];v.py=p[1];v.pz=p[2];}
    const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};std::copy(identity,identity+16,m.transform);updateSmoothNormals(m.vertices,m.indices,makeNormalGroups(m.vertices));
   }
  }
  // Bind fitted garments to the same skin using nearby body-surface weights.
  // Bake their fitted transform before skinning; skin palettes operate in world space.
  if(body.animation&&!body.animation->skins.empty()){
   auto restPose=evaluateGltfPose(*body.animation,-1,0);
   const auto& primarySkin=body.animation->skins.front();
   unsigned pelvis=primarySkin.joints.front();
   std::vector<bool> legJoint(primarySkin.joints.size(),false);
   for(size_t j=0;j<primarySkin.joints.size();++j){int node=int(primarySkin.joints[j]);if(node==int(pelvis)){legJoint[j]=true;continue;}
    for(size_t depth=0;depth<body.animation->nodes.size()&&node>=0;++depth){int parent=body.animation->nodes[node].parent;if(parent==int(pelvis)){legJoint[j]=restPose.globals[node][13]<restPose.globals[pelvis][13];break;}node=parent;}
   }
   // Rest-pose leg alignment must precede skin transfer: changing only weights
   // leaves trousers/boots hanging beside the avatar's corrected, narrower legs.
   constexpr int rings=24;
   std::array<std::array<Bounds,rings>,2> bodyRings;
   auto ringIndex=[&](float y){return std::clamp(int((y-bodyBounds.lo[1])/height/.48f*rings),0,rings-1);};
   for(const auto& part:body.meshes)for(size_t vi=0;vi<part.vertices.size();++vi){auto p=world(part,part.vertices[vi]);if(p[1]>bodyBounds.lo[1]+height*.40f||vi>=part.influences.size())continue;float legWeight=0;const auto& inf=part.influences[vi];for(int k=0;k<4;++k)if(inf.joints[k]<legJoint.size()&&legJoint[inf.joints[k]])legWeight+=inf.weights[k];if(legWeight<.8f)continue;bodyRings[p[0]<bodyBounds.center(0)?0:1][ringIndex(p[1])].add(p);}
   for(int slot=2;slot<itemCount;++slot)if(fits[slot].worn&&items[slot].available){
    const std::string prefix="equipment/"+std::string(items[slot].id)+"/";
    std::array<std::array<Bounds,rings>,2> garmentRings;
    for(const auto& part:out.meshes)if(part.name.rfind(prefix,0)==0)for(const auto& v:part.vertices){auto p=world(part,v);garmentRings[p[0]<bodyBounds.center(0)?0:1][ringIndex(p[1])].add(p);}
    auto correction=[&](int side,int ring){for(int d=0;d<rings;++d)for(int direction:{-1,1}){int k=ring+d*direction;if(k<0||k>=rings)continue;auto& b=bodyRings[side][k];auto& g=garmentRings[side][k];if(b.valid()&&g.valid())return b.center(0)+fits[slot].offset[0]*height-g.center(0);}return 0.f;};
    for(auto& part:out.meshes)if(part.name.rfind(prefix,0)==0){
     for(auto& v:part.vertices){auto p=world(part,v);float y=(p[1]-bodyBounds.lo[1])/height,t=std::clamp((.48f-y)/.12f,0.f,1.f);t=t*t*(3-2*t);
      int side=p[0]<bodyBounds.center(0)?0:1;float r=std::clamp(y/.48f*rings-.5f,0.f,float(rings-1));int lo=int(r),hi=std::min(lo+1,rings-1);
      p[0]+=t*((1-(r-lo))*correction(side,lo)+(r-lo)*correction(side,hi));v.px=p[0];v.py=p[1];v.pz=p[2];
     }
     const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};std::copy(identity,identity+16,part.transform);
    }
   }
   if(!bindEquipment){for(auto& mesh:out.meshes)if(mesh.name.rfind("equipment/",0)==0){
    // Node indices belong to the garment's source GLB, not the avatar skeleton.
    // In rest-pose editing its fitted transform is authoritative.
    mesh.node=-1;mesh.skin=-1;mesh.influences.clear();
    updateSmoothNormals(mesh.vertices,mesh.indices,makeNormalGroups(mesh.vertices));
   }return out;}
   struct Sample{std::array<float,3> p;GltfInfluence influence;bool leg;};
   int skin=-1;std::vector<Sample> samples;
   for(const auto& m:body.meshes)if(m.skin>=0&&m.influences.size()==m.vertices.size()){
    if(skin<0)skin=m.skin;if(m.skin!=skin)continue;
    for(size_t i=0;i<m.vertices.size();++i){float legWeight=0;const auto& inf=m.influences[i];for(int k=0;k<4;++k)if(inf.joints[k]<legJoint.size()&&legJoint[inf.joints[k]])legWeight+=inf.weights[k];samples.push_back({world(m,m.vertices[i]),inf,legWeight>=.8f});}
   }
   std::sort(samples.begin(),samples.end(),[](const auto& a,const auto& b){return a.p[1]<b.p[1];});
   if(!samples.empty())for(auto& m:out.meshes)if(m.name.rfind("equipment/",0)==0){
    bool lowerBody=false;for(int slot=2;slot<itemCount;++slot)if(m.name.rfind("equipment/"+std::string(items[slot].id)+"/",0)==0)lowerBody=true;
    m.skin=skin;m.node=-1;m.influences.clear();
    for(auto& v:m.vertices){auto p=world(m,v);std::array<float,4> distances{FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX};std::array<size_t,4> nearest{};
     int right=int(std::lower_bound(samples.begin(),samples.end(),p[1],[](const auto& sample,float y){return sample.p[1]<y;})-samples.begin()),left=right-1;
     while(left>=0||right<int(samples.size())){float dl=left>=0?std::abs(samples[left].p[1]-p[1]):FLT_MAX,dr=right<int(samples.size())?std::abs(samples[right].p[1]-p[1]):FLT_MAX;float dy=std::min(dl,dr);if(dy*dy>distances[3])break;size_t i=dl<dr?size_t(left--):size_t(right++);if(lowerBody&&!samples[i].leg)continue;if(p[1]<bodyBounds.lo[1]+height*.42f&&(p[0]-bodyBounds.center(0))*(samples[i].p[0]-bodyBounds.center(0))<0)continue;float d=0;for(int a=0;a<3;++a)d+=(p[a]-samples[i].p[a])*(p[a]-samples[i].p[a]);
      for(int k=0;k<4;++k)if(d<distances[k]){for(int j=3;j>k;--j){distances[j]=distances[j-1];nearest[j]=nearest[j-1];}distances[k]=d;nearest[k]=i;break;}
     }
     std::vector<float> weights(body.animation->skins[skin].joints.size(),0);
     for(int k=0;k<4;++k){const auto& inf=samples[nearest[k]].influence;float w=1/std::max(distances[k],1e-8f);for(int j=0;j<4;++j)if(inf.joints[j]<weights.size()&&(!lowerBody||(inf.joints[j]<legJoint.size()&&legJoint[inf.joints[j]])))weights[inf.joints[j]]+=w*inf.weights[j];}
     GltfInfluence inf;float sum=0;for(int k=0;k<4;++k){auto best=std::max_element(weights.begin(),weights.end());inf.joints[k]=unsigned(best-weights.begin());inf.weights[k]=*best;sum+=*best;*best=0;}for(auto& w:inf.weights)w/=std::max(sum,1e-12f);m.influences.push_back(inf);
     v.px=p[0];v.py=p[1];v.pz=p[2];
    }
    const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};std::copy(identity,identity+16,m.transform);updateSmoothNormals(m.vertices,m.indices,makeNormalGroups(m.vertices));
   }
  }
  // The cuirass must never cut away skin visible through its collar or armholes.
  return out;
 }
};
}
