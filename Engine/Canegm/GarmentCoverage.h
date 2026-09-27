#pragma once
#include "GltfModel.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <numeric>
#include <bit>
#include <unordered_map>

// Conservative torso coverage queries in world space. Source meshes stay intact.
namespace GarmentCoverage {
using V=std::array<float,3>;
inline V add(V a,V b){for(int i=0;i<3;++i)a[i]+=b[i];return a;}
inline V sub(V a,V b){for(int i=0;i<3;++i)a[i]-=b[i];return a;}
inline V mul(V a,float s){for(auto& x:a)x*=s;return a;}
inline float dot(V a,V b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
inline V world(const GltfMesh& m,const GltfVertex& v){return {m.transform[0]*v.px+m.transform[4]*v.py+m.transform[8]*v.pz+m.transform[12],m.transform[1]*v.px+m.transform[5]*v.py+m.transform[9]*v.pz+m.transform[13],m.transform[2]*v.px+m.transform[6]*v.py+m.transform[10]*v.pz+m.transform[14]};}
struct Triangle {V a,b,c,n;};
class Surface {
 struct Node{V lo{FLT_MAX,FLT_MAX,FLT_MAX},hi{-FLT_MAX,-FLT_MAX,-FLT_MAX};int begin=0,end=0,left=-1,right=-1;};
 std::vector<Triangle> triangles;std::vector<int> order;std::vector<Node> nodes;
 int build(int begin,int end){int id=int(nodes.size());nodes.emplace_back();Node n;n.begin=begin;n.end=end;
  for(int j=begin;j<end;++j){auto& t=triangles[order[j]];for(auto p:{t.a,t.b,t.c})for(int i=0;i<3;++i){n.lo[i]=std::min(n.lo[i],p[i]);n.hi[i]=std::max(n.hi[i],p[i]);}}
  if(end-begin>12){int axis=0;for(int i=1;i<3;++i)if(n.hi[i]-n.lo[i]>n.hi[axis]-n.lo[axis])axis=i;int mid=(begin+end)/2;std::nth_element(order.begin()+begin,order.begin()+mid,order.begin()+end,[&](int a,int b){auto& x=triangles[a];auto& y=triangles[b];return x.a[axis]+x.b[axis]+x.c[axis]<y.a[axis]+y.b[axis]+y.c[axis];});n.left=build(begin,mid);n.right=build(mid,end);}nodes[id]=n;return id;
 }
 static V cross(V a,V b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
 void ray(int id,V p,V d,float& best)const{
  auto& n=nodes[id];float lo=0,hi=best;for(int i=0;i<3;++i){if(std::abs(d[i])<1e-9f){if(p[i]<n.lo[i]||p[i]>n.hi[i])return;}else{float a=(n.lo[i]-p[i])/d[i],b=(n.hi[i]-p[i])/d[i];if(a>b)std::swap(a,b);lo=std::max(lo,a);hi=std::min(hi,b);if(lo>hi)return;}}
  if(n.left>=0){ray(n.left,p,d,best);ray(n.right,p,d,best);return;}
  for(int j=n.begin;j<n.end;++j){auto& t=triangles[order[j]];auto e1=sub(t.b,t.a),e2=sub(t.c,t.a),h=cross(d,e2);float det=dot(e1,h);if(std::abs(det)<1e-10f)continue;auto s=sub(p,t.a);float u=dot(s,h)/det;if(u<-.00001f||u>1.00001f)continue;auto q=cross(s,e1);float v=dot(d,q)/det;if(v<-.00001f||u+v>1.00001f)continue;float hit=dot(e2,q)/det;if(hit>0&&hit<best)best=hit;}
 }
public:
 explicit Surface(const GltfModel& body){for(auto& m:body.meshes)for(size_t j=0;j+2<m.indices.size();j+=3){Triangle t;t.a=world(m,m.vertices[m.indices[j]]);t.b=world(m,m.vertices[m.indices[j+1]]);t.c=world(m,m.vertices[m.indices[j+2]]);auto a=sub(t.b,t.a),b=sub(t.c,t.a);t.n={a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};float len=std::sqrt(dot(t.n,t.n));if(len>1e-10f){t.n=mul(t.n,1/len);triangles.push_back(t);}}
  order.resize(triangles.size());std::iota(order.begin(),order.end(),0);if(!order.empty())build(0,int(order.size()));
 }
 float radius(V p,V direction)const{float best=FLT_MAX;if(!nodes.empty())ray(0,p,direction,best);return best;}
};

inline void hashValue(uint64_t& key,uint32_t value){key^=value;key*=1099511628211ull;}
inline uint64_t geometryKey(const GltfMesh& mesh,uint64_t key=1469598103934665603ull){
 for(auto x:mesh.transform)hashValue(key,std::bit_cast<uint32_t>(x));
 for(auto& v:mesh.vertices)for(auto x:{v.px,v.py,v.pz})hashValue(key,std::bit_cast<uint32_t>(x));
 for(auto i:mesh.indices)hashValue(key,i);return key;
}
// Cache only the current fitted shell. Changing fit/body invalidates the cache;
// repeated picking, previews and trouser edits reuse the same coverage results.
class Mask {
 Surface surface;V center;float height,width,bottom;
 std::unordered_map<uint64_t,std::vector<unsigned>> indices;
public:
 uint64_t key;
 Mask(const GltfModel& shell,V torsoCenter,float bodyHeight,float bodyWidth,float bodyBottom,uint64_t cacheKey):surface(shell),center(torsoCenter),height(bodyHeight),width(bodyWidth),bottom(bodyBottom),key(cacheKey){}
 bool covered(V p)const{
  float y=(p[1]-bottom)/height;
  // Keep head, limbs and hip region. Only upright humanoid torso coverage.
  if(y<.53f||y>.83f||std::abs(p[0]-center[0])>width*.29f)return false;
  V origin{center[0],p[1],center[2]},d=sub(p,origin);float r=std::sqrt(dot(d,d));if(r<1e-6f)return false;
  float hit=surface.radius(origin,mul(d,1/r));return hit!=FLT_MAX&&std::abs(r-hit)<height*.06f;
 }
 const std::vector<unsigned>& visible(const GltfMesh& mesh){
  auto id=geometryKey(mesh);auto existing=indices.find(id);if(existing!=indices.end())return existing->second;
  std::vector<unsigned> result;std::vector<V> points;std::vector<bool> hidden;
  for(auto& v:mesh.vertices){points.push_back(world(mesh,v));hidden.push_back(covered(points.back()));}
  for(size_t i=0;i<mesh.indices.size();i+=3){auto a=mesh.indices[i],b=mesh.indices[i+1],c=mesh.indices[i+2];
   bool hide=hidden[a]&&hidden[b]&&hidden[c]&&covered(mul(add(points[a],add(points[b],points[c])),1.f/3));
   if(!hide)result.insert(result.end(),mesh.indices.begin()+i,mesh.indices.begin()+i+3);
  }return indices.emplace(id,std::move(result)).first->second;
 }
};
}
