#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <cmath>
struct GltfVertex { float px,py,pz; float nx,ny,nz; float u=0,v=0; };
using GltfMatrix = std::array<float,16>;
struct GltfInfluence { std::array<unsigned,4> joints{}; std::array<float,4> weights{}; };
struct GltfMorph { std::vector<std::array<float,3>> positions,normals; };
struct GltfNode { int parent=-1; GltfMatrix local{}; std::array<float,3> translation{},scale{1,1,1}; std::array<float,4> rotation{0,0,0,1}; std::vector<float> weights; };
struct GltfSkin { std::vector<unsigned> joints; std::vector<GltfMatrix> inverseBind; };
struct GltfChannel { unsigned node=0; int path=0,interpolation=0,width=0; std::vector<float> times,values; };
struct GltfClip { std::string name; float duration=0; std::vector<GltfChannel> channels; };
struct GltfAnimationData { std::vector<GltfNode> nodes; std::vector<GltfSkin> skins; std::vector<GltfClip> clips; };
struct GltfPose { std::vector<GltfMatrix> globals; std::vector<std::vector<float>> weights; };
struct GltfMesh {
 int node=-1,skin=-1; std::vector<GltfInfluence> influences; std::vector<GltfMorph> morphs; std::vector<float> morphWeights;
 std::string name; std::vector<GltfVertex> vertices; std::vector<std::uint32_t> indices; int materialIndex=-1;
 float transform[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
};
struct GltfImage { std::string name; std::vector<std::uint8_t> bytes; };
struct GltfMaterial { std::string name; int baseColorImage=-1; float baseColorFactor[4]={1,1,1,1}; float alphaCutoff=-1.0f; };
struct GltfModel { std::shared_ptr<const GltfAnimationData> animation; std::vector<GltfMesh> meshes; std::vector<GltfImage> images; std::vector<GltfMaterial> materials; };
bool loadGltf(const std::filesystem::path& path,GltfModel& model);

GltfPose evaluateGltfPose(const GltfAnimationData& data,int clip,float time);
std::vector<GltfMatrix> gltfSkinMatrices(const GltfAnimationData& data,const GltfPose& pose,int skin);
void deformGltfVertices(const GltfMesh& mesh,const GltfPose& pose,const std::vector<GltfMatrix>& palette,const std::vector<GltfVertex>& source,std::vector<GltfVertex>& output);

struct GltfPlayback {
 int clip=-1;float time=0,speed=1;bool playing=false;
 void reset(const GltfAnimationData* data){clip=-1;time=0;speed=1;playing=false;if(!data||data->clips.empty())return;clip=0;for(size_t i=0;i<data->clips.size();++i)if(data->clips[i].name=="Walk"){clip=int(i);break;}playing=true;}
 void advance(const GltfAnimationData& data,float dt){if(!playing||clip<0||clip>=int(data.clips.size())||!std::isfinite(dt)||dt<0)return;float duration=data.clips[clip].duration;if(duration>0)time=std::fmod(time+dt*speed,duration);else time=0;}
};
