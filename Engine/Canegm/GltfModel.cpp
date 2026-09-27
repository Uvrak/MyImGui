#include "GltfModel.h"
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <iostream>
#include <cstring>
#include <variant>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <functional>
#include "GltfAnimation.inl"

static bool copySourceBytes(const fastgltf::Asset& asset,const fastgltf::DataSource& src,std::vector<std::uint8_t>& out){
 if(auto a=std::get_if<fastgltf::sources::Array>(&src)){out.resize(a->bytes.size());std::memcpy(out.data(),a->bytes.data(),a->bytes.size());return true;}
 if(auto b=std::get_if<fastgltf::sources::ByteView>(&src)){out.resize(b->bytes.size());std::memcpy(out.data(),b->bytes.data(),b->bytes.size());return true;}
 if(auto v=std::get_if<fastgltf::sources::BufferView>(&src)){
  const auto& bv=asset.bufferViews[v->bufferViewIndex]; const auto& buf=asset.buffers[bv.bufferIndex];
  std::vector<std::uint8_t> all;if(!copySourceBytes(asset,buf.data,all)||bv.byteOffset+bv.byteLength>all.size())return false;
  out.assign(all.begin()+bv.byteOffset,all.begin()+bv.byteOffset+bv.byteLength);return true;
 }
 return false;
}
static bool loadGltfImpl(const std::filesystem::path& path,GltfModel& model){
 auto file=fastgltf::MappedGltfFile::FromPath(path);if(!file){std::cerr<<"GLB open failed\n";return false;}
 fastgltf::Parser parser;auto result=parser.loadGltf(file.get(),path.parent_path(),fastgltf::Options::LoadExternalBuffers|fastgltf::Options::GenerateMeshIndices);
 if(result.error()!=fastgltf::Error::None){std::cerr<<"GLB parse failed: "<<fastgltf::getErrorMessage(result.error())<<"\n";return false;}
 auto& asset=result.get();
 if(fastgltf::validate(asset)!=fastgltf::Error::None)return false;
 model={};
 auto animation=readGltfAnimation(asset);model.animation=animation;
 for(const auto&i:asset.images){GltfImage gi;gi.name=i.name;copySourceBytes(asset,i.data,gi.bytes);model.images.push_back(std::move(gi));}
 for(const auto&m:asset.materials){GltfMaterial gm;gm.name=m.name;if(m.alphaMode==fastgltf::AlphaMode::Mask)gm.alphaCutoff=(float)m.alphaCutoff;auto f=m.pbrData.baseColorFactor;gm.baseColorFactor[0]=f.x();gm.baseColorFactor[1]=f.y();gm.baseColorFactor[2]=f.z();gm.baseColorFactor[3]=f.w();if(m.pbrData.baseColorTexture){auto ti=m.pbrData.baseColorTexture->textureIndex;if(ti<asset.textures.size()&&asset.textures[ti].imageIndex)gm.baseColorImage=(int)*asset.textures[ti].imageIndex;}model.materials.push_back(std::move(gm));}
 auto addMesh=[&](std::size_t meshIndex,const fastgltf::math::fmat4x4& matrix,int node){const auto&source=asset.meshes[meshIndex];for(const auto&primitive:source.primitives){const auto*pos=primitive.findAttribute("POSITION");if(pos==primitive.attributes.end()||!primitive.indicesAccessor)continue;const auto*normal=primitive.findAttribute("NORMAL");const auto&pa=asset.accessors[pos->accessorIndex];GltfMesh mesh;mesh.name=source.name;std::memcpy(mesh.transform,matrix.data(),sizeof(mesh.transform));mesh.vertices.resize(pa.count);fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset,pa,[&](auto v,std::size_t i){mesh.vertices[i]={v.x(),v.y(),v.z(),0,0,1,0,0};});const auto*uv=primitive.findAttribute("TEXCOORD_0");if(uv!=primitive.attributes.end()){const auto&ua=asset.accessors[uv->accessorIndex];fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset,ua,[&](auto v,std::size_t i){mesh.vertices[i].u=v.x();mesh.vertices[i].v=v.y();});}mesh.materialIndex=primitive.materialIndex?(int)*primitive.materialIndex:-1;if(normal!=primitive.attributes.end()){const auto&na=asset.accessors[normal->accessorIndex];fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset,na,[&](auto v,std::size_t i){mesh.vertices[i].nx=v.x();mesh.vertices[i].ny=v.y();mesh.vertices[i].nz=v.z();});}const auto&ia=asset.accessors[*primitive.indicesAccessor];mesh.indices.resize(ia.count);fastgltf::copyFromAccessor<std::uint32_t>(asset,ia,mesh.indices.data());readGltfDeformation(asset,primitive,source,node,mesh);std::cout<<"GLB mesh: "<<mesh.name<<" vertices="<<mesh.vertices.size()<<" material="<<mesh.materialIndex<<"\n";model.meshes.push_back(std::move(mesh));}};
 if(asset.defaultScene)fastgltf::iterateSceneNodes(asset,*asset.defaultScene,fastgltf::math::fmat4x4(),[&](fastgltf::Node&n,const fastgltf::math::fmat4x4&m){if(n.meshIndex)addMesh(*n.meshIndex,m,int(&n-asset.nodes.data()));});else for(std::size_t s=0;s<asset.scenes.size();++s)fastgltf::iterateSceneNodes(asset,s,fastgltf::math::fmat4x4(),[&](fastgltf::Node&n,const fastgltf::math::fmat4x4&m){if(n.meshIndex)addMesh(*n.meshIndex,m,int(&n-asset.nodes.data()));});
 std::cout<<"GLB images="<<model.images.size()<<" materials="<<model.materials.size()<<"\n";return !model.meshes.empty();
}

bool loadGltf(const std::filesystem::path& path,GltfModel& model){
 try {GltfModel candidate;if(!loadGltfImpl(path,candidate))return false;model=std::move(candidate);return true;}
 catch(const std::exception& e){std::cerr<<"GLB load failed: "<<e.what()<<"\n";return false;}
}
