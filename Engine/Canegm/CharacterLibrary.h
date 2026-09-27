#pragma once
#include "GltfModel.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <string>

namespace CharacterLibrary {
inline std::string utf8(const std::filesystem::path& path){auto s=path.u8string();return {s.begin(),s.end()};}
inline bool glb(const std::filesystem::path& path){auto ext=utf8(path.extension());std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return (char)std::tolower(c);});return ext==".glb";}
inline bool valid(const GltfModel& model){
 if(model.meshes.empty())return false;
 for(const auto& m:model.meshes){
  if(m.vertices.empty()||m.indices.empty()||m.indices.size()%3)return false;
  for(auto i:m.indices)if(i>=m.vertices.size())return false;
  for(auto& v:m.vertices)if(!std::isfinite(v.px)||!std::isfinite(v.py)||!std::isfinite(v.pz))return false;
  for(auto f:m.transform)if(!std::isfinite(f))return false;
  if(m.materialIndex>=int(model.materials.size()))return false;
 }
 for(auto& mat:model.materials)if(mat.baseColorImage>=0&&(mat.baseColorImage>=int(model.images.size())||model.images[mat.baseColorImage].bytes.empty()))return false;
 return true;
}
inline std::vector<std::filesystem::path> list(const std::filesystem::path& root){
 std::vector<std::filesystem::path> files;std::error_code ec;
 for(std::filesystem::directory_iterator it(root,ec),end;!ec&&it!=end;it.increment(ec))if(it->is_regular_file(ec)&&glb(it->path()))files.push_back(it->path());
 std::sort(files.begin(),files.end());return files;
}
inline bool read(const std::filesystem::path& path,GltfModel& output,std::string& error){
 try{GltfModel candidate;
  if(!glb(path)){error="Bitte eine GLB-Datei waehlen.";return false;}
  if(!loadGltf(path,candidate)||!valid(candidate)){error="Das Modell konnte nicht geladen werden. Bitte als GLB mit eingebetteten Texturen exportieren.";return false;}
  output=std::move(candidate);error.clear();return true;
 }catch(const std::exception&){error="Die Modelldatei konnte nicht gelesen werden.";return false;}
}
inline bool importModel(const std::filesystem::path& source,const std::filesystem::path& root,std::filesystem::path& imported,GltfModel& model,std::string& error){
 GltfModel candidate;if(!read(source,candidate,error))return false;
 try{
  std::filesystem::create_directories(root);
  auto target=root/(source.stem().native()+std::filesystem::path(".glb").native());
  if(std::filesystem::weakly_canonical(source.parent_path())==std::filesystem::weakly_canonical(root))target=source;
  else{
   for(int n=2;std::filesystem::exists(target);++n)target=root/(source.stem().native()+std::filesystem::path("_"+std::to_string(n)+".glb").native());
   std::filesystem::copy_file(source,target); // Never overwrite an existing character.
  }
  imported=target;model=std::move(candidate);error.clear();return true;
 }catch(const std::exception&){error="Der Charakter konnte nicht in die Bibliothek kopiert werden.";return false;}
}
inline bool save(const std::filesystem::path& settings,const std::filesystem::path& selected){
 std::ofstream out(settings,std::ios::trunc);if(!out)return false;
 out<<"character-library-v1\n"<<std::quoted(utf8(selected.filename()))<<'\n';out.flush();return bool(out);
}
inline std::filesystem::path restore(const std::filesystem::path& settings,const std::filesystem::path& root){
 std::ifstream in(settings);std::string version,name,extra;
 if(!(in>>version>>std::quoted(name))||version!="character-library-v1"||in>>extra)return {};
 auto file=std::filesystem::path(std::u8string(name.begin(),name.end()));
 if(file.empty()||file!=file.filename()||!glb(file))return {};
 std::error_code ec;auto path=root/file;return std::filesystem::is_regular_file(path,ec)?path:std::filesystem::path{};
}
}
