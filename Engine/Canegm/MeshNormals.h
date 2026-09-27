#pragma once
#include "GltfModel.h"
#include <array>
#include <cmath>
#include <map>

// Attribute seams duplicate positions in glTF. Share only shading normals,
// within each mesh; never weld or reorder the actual vertices/indices.
inline std::vector<unsigned> makeNormalGroups(const std::vector<GltfVertex>& vertices) {
 std::map<std::array<float,3>,unsigned> groups;
 std::vector<unsigned> result;
 for(const auto& v:vertices) {
  auto [it,inserted]=groups.emplace(std::array<float,3>{v.px,v.py,v.pz},unsigned(groups.size()));
  result.push_back(it->second);
 }
 return result;
}

inline void updateSmoothNormals(std::vector<GltfVertex>& vertices,
 const std::vector<unsigned>& indices,const std::vector<unsigned>& groups) {
 std::vector<std::array<double,3>> sums(vertices.size(),{0,0,0});
 for(size_t i=0;i+2<indices.size();i+=3) {
  const auto &a=vertices[indices[i]], &b=vertices[indices[i+1]], &c=vertices[indices[i+2]];
  double ux=b.px-a.px,uy=b.py-a.py,uz=b.pz-a.pz;
  double vx=c.px-a.px,vy=c.py-a.py,vz=c.pz-a.pz;
  std::array<double,3> n={uy*vz-uz*vy,uz*vx-ux*vz,ux*vy-uy*vx};
  for(int j=0;j<3;++j)for(int k=0;k<3;++k)sums[groups[indices[i+j]]][k]+=n[k];
 }
 for(size_t i=0;i<vertices.size();++i) {
  const auto& n=sums[groups[i]];
  double length=std::sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);
  if(length>1e-20) {vertices[i].nx=float(n[0]/length);vertices[i].ny=float(n[1]/length);vertices[i].nz=float(n[2]/length);}
 }
}
