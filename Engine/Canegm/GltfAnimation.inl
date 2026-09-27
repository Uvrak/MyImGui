// glTF animation math uses column vectors. Skin palettes produce world-space vertices:
// globalJoint * inverseBind. The renderer therefore uses identity for skinned meshes.
namespace {
using namespace fastgltf::math;
GltfMatrix storeMatrix(const fmat4x4& m){GltfMatrix out;std::copy(m.data(),m.data()+16,out.begin());return out;}
fmat4x4 matrixOf(const GltfMatrix& a){fmat4x4 m;std::copy(a.begin(),a.end(),m.data());return m;}
std::shared_ptr<GltfAnimationData> readGltfAnimation(const fastgltf::Asset& asset){
 auto out=std::make_shared<GltfAnimationData>();out->nodes.resize(asset.nodes.size());
 for(size_t i=0;i<asset.nodes.size();++i){const auto& n=asset.nodes[i];auto& d=out->nodes[i];
  auto m=fastgltf::getTransformMatrix(n);d.local=storeMatrix(m);fvec3 t,s;fquat r;decomposeTransformMatrix(m,s,r,t);
  for(int k=0;k<3;++k){d.translation[k]=t[k];d.scale[k]=s[k];}for(int k=0;k<4;++k)d.rotation[k]=r[k];
  d.weights.assign(n.weights.begin(),n.weights.end());
  for(auto child:n.children){if(child>=out->nodes.size()||out->nodes[child].parent!=-1)throw std::runtime_error("Invalid node hierarchy");out->nodes[child].parent=int(i);}
 }
 // Validate cycles even outside the active scene.
 for(size_t i=0;i<out->nodes.size();++i){int p=int(i);size_t depth=0;while(p>=0){if(++depth>out->nodes.size())throw std::runtime_error("Node cycle");p=out->nodes[p].parent;}}
 for(const auto& skin:asset.skins){GltfSkin d;for(auto joint:skin.joints)d.joints.push_back(unsigned(joint));d.inverseBind.assign(d.joints.size(),storeMatrix(fmat4x4()));
  if(skin.inverseBindMatrices){const auto& a=asset.accessors[*skin.inverseBindMatrices];if(a.count!=d.joints.size())throw std::runtime_error("Invalid inverse bind count");fastgltf::iterateAccessorWithIndex<fmat4x4>(asset,a,[&](auto m,size_t i){d.inverseBind[i]=storeMatrix(m);});}out->skins.push_back(std::move(d));
 }
 for(const auto& a:asset.animations){GltfClip clip;clip.name=a.name.empty()?"Clip "+std::to_string(out->clips.size()+1):std::string(a.name);
  for(const auto& channel:a.channels){if(!channel.nodeIndex)continue;GltfChannel c;c.node=unsigned(*channel.nodeIndex);
   switch(channel.path){case fastgltf::AnimationPath::Translation:c.path=0;c.width=3;break;case fastgltf::AnimationPath::Rotation:c.path=1;c.width=4;break;case fastgltf::AnimationPath::Scale:c.path=2;c.width=3;break;case fastgltf::AnimationPath::Weights:c.path=3;break;default:continue;}
   const auto& sampler=a.samplers[channel.samplerIndex];c.interpolation=sampler.interpolation==fastgltf::AnimationInterpolation::Step?1:sampler.interpolation==fastgltf::AnimationInterpolation::CubicSpline?2:0;
   const auto& input=asset.accessors[sampler.inputAccessor];const auto& output=asset.accessors[sampler.outputAccessor];
   fastgltf::iterateAccessor<float>(asset,input,[&](float t){if(!std::isfinite(t)||t<0||(!c.times.empty()&&t<=c.times.back()))throw std::runtime_error("Invalid animation times");c.times.push_back(t);});
   if(c.times.empty())throw std::runtime_error("Empty animation");
   if(c.path==3){c.width=int(output.count/(c.times.size()*(c.interpolation==2?3:1)));fastgltf::iterateAccessor<float>(asset,output,[&](float v){c.values.push_back(v);});}
   else if(c.width==4)fastgltf::iterateAccessor<fvec4>(asset,output,[&](auto v){for(int k=0;k<4;++k)c.values.push_back(v[k]);});
   else fastgltf::iterateAccessor<fvec3>(asset,output,[&](auto v){for(int k=0;k<3;++k)c.values.push_back(v[k]);});
   if(c.width<=0||c.values.size()!=c.times.size()*c.width*(c.interpolation==2?3:1))throw std::runtime_error("Invalid animation output count");
   for(auto v:c.values)if(!std::isfinite(v))throw std::runtime_error("Nonfinite animation");
   clip.duration=std::max(clip.duration,c.times.back());clip.channels.push_back(std::move(c));
  }out->clips.push_back(std::move(clip));
 }return out;
}
void readGltfDeformation(const fastgltf::Asset& asset,const fastgltf::Primitive& p,const fastgltf::Mesh& source,int node,GltfMesh& mesh){
 mesh.node=node;const auto& n=asset.nodes[node];mesh.skin=n.skinIndex?int(*n.skinIndex):-1;
 mesh.morphWeights.assign(source.weights.begin(),source.weights.end());if(!n.weights.empty())mesh.morphWeights.assign(n.weights.begin(),n.weights.end());
 if(mesh.skin>=0){auto j=p.findAttribute("JOINTS_0"),w=p.findAttribute("WEIGHTS_0");if(j==p.attributes.end()||w==p.attributes.end())throw std::runtime_error("Skin missing joints/weights");
  if(p.findAttribute("JOINTS_1")!=p.attributes.end())throw std::runtime_error("More than four skin influences are not supported");
  if(asset.accessors[j->accessorIndex].count!=mesh.vertices.size()||asset.accessors[w->accessorIndex].count!=mesh.vertices.size())throw std::runtime_error("Invalid skin vertex count");
  mesh.influences.resize(mesh.vertices.size());
  fastgltf::iterateAccessorWithIndex<fastgltf::math::uvec4>(asset,asset.accessors[j->accessorIndex],[&](auto v,size_t i){for(int k=0;k<4;++k){if(v[k]>=asset.skins[mesh.skin].joints.size())throw std::runtime_error("Invalid joint index");mesh.influences[i].joints[k]=v[k];}});
  fastgltf::iterateAccessorWithIndex<fvec4>(asset,asset.accessors[w->accessorIndex],[&](auto v,size_t i){float total=0;for(int k=0;k<4;++k){if(!std::isfinite(v[k])||v[k]<0)throw std::runtime_error("Invalid skin weight");total+=v[k];}if(total<=0)throw std::runtime_error("Zero skin weights");for(int k=0;k<4;++k)mesh.influences[i].weights[k]=v[k]/total;});
 }
 for(const auto& target:p.targets){GltfMorph morph;for(const auto& attr:target){auto* dst=attr.name=="POSITION"?&morph.positions:attr.name=="NORMAL"?&morph.normals:nullptr;if(!dst)continue;const auto& accessor=asset.accessors[attr.accessorIndex];if(accessor.count!=mesh.vertices.size())throw std::runtime_error("Invalid morph count");dst->resize(accessor.count);fastgltf::iterateAccessorWithIndex<fvec3>(asset,accessor,[&](auto v,size_t i){(*dst)[i]={v.x(),v.y(),v.z()};});}mesh.morphs.push_back(std::move(morph));}
}
}
GltfPose evaluateGltfPose(const GltfAnimationData& data,int clip,float time){
 using namespace fastgltf::math;GltfPose pose;pose.globals.resize(data.nodes.size());pose.weights.resize(data.nodes.size());auto nodes=data.nodes;std::vector<bool> changed(nodes.size(),false);
 if(clip>=0&&clip<int(data.clips.size()))for(const auto& c:data.clips[clip].channels){
  size_t a=0,b=0;float u=0,dt=0;if(time>=c.times.back())a=b=c.times.size()-1;else if(time>c.times.front()){b=std::upper_bound(c.times.begin(),c.times.end(),time)-c.times.begin();a=b-1;dt=c.times[b]-c.times[a];u=(time-c.times[a])/dt;}
  const int stride=c.width*(c.interpolation==2?3:1),offset=c.interpolation==2?c.width:0;std::vector<float> v(c.width);
  for(int k=0;k<c.width;++k){float x=c.values[a*stride+offset+k],y=c.values[b*stride+offset+k];
   if(c.interpolation==1||a==b)v[k]=x;else if(c.interpolation==2){float u2=u*u,u3=u2*u;v[k]=(2*u3-3*u2+1)*x+(u3-2*u2+u)*dt*c.values[a*stride+2*c.width+k]+(-2*u3+3*u2)*y+(u3-u2)*dt*c.values[b*stride+k];}else v[k]=x+(y-x)*u;
  }
  if(c.path==1){if(c.interpolation==0&&a!=b){auto i=a*stride,j=b*stride;auto q=slerp(fquat(c.values[i],c.values[i+1],c.values[i+2],c.values[i+3]),fquat(c.values[j],c.values[j+1],c.values[j+2],c.values[j+3]),u);for(int k=0;k<4;++k)v[k]=q[k];}float length=0;for(float x:v)length+=x*x;if(length>1e-12f)for(auto& x:v)x/=std::sqrt(length);else v={0,0,0,1};}
  auto& n=nodes[c.node];if(c.path==3)n.weights=v;else {changed[c.node]=true;if(c.path==0)std::copy(v.begin(),v.end(),n.translation.begin());if(c.path==1)std::copy(v.begin(),v.end(),n.rotation.begin());if(c.path==2)std::copy(v.begin(),v.end(),n.scale.begin());}
 }
 std::vector<bool> done(nodes.size(),false);std::function<void(size_t)> visit=[&](size_t i){if(done[i])return;const auto& n=nodes[i];auto m=matrixOf(n.local);if(changed[i])m=scale(rotate(translate(fmat4x4(),fvec3(n.translation[0],n.translation[1],n.translation[2])),fquat(n.rotation[0],n.rotation[1],n.rotation[2],n.rotation[3])),fvec3(n.scale[0],n.scale[1],n.scale[2]));if(n.parent>=0){visit(n.parent);m=matrixOf(pose.globals[n.parent])*m;}pose.globals[i]=storeMatrix(m);pose.weights[i]=n.weights;done[i]=true;};for(size_t i=0;i<nodes.size();++i)visit(i);return pose;
}
std::vector<GltfMatrix> gltfSkinMatrices(const GltfAnimationData& data,const GltfPose& pose,int skin){std::vector<GltfMatrix> out;if(skin<0||skin>=int(data.skins.size()))return out;auto& s=data.skins[skin];for(size_t i=0;i<s.joints.size();++i)out.push_back(storeMatrix(matrixOf(pose.globals[s.joints[i]])*matrixOf(s.inverseBind[i])));return out;}
void deformGltfVertices(const GltfMesh& mesh,const GltfPose& pose,const std::vector<GltfMatrix>& palette,const std::vector<GltfVertex>& source,std::vector<GltfVertex>& output){
 output=source;const auto& weights=mesh.node>=0&&!pose.weights[mesh.node].empty()?pose.weights[mesh.node]:mesh.morphWeights;
 for(size_t i=0;i<output.size();++i){auto& v=output[i];for(size_t t=0;t<mesh.morphs.size()&&t<weights.size();++t){auto& m=mesh.morphs[t];if(!m.positions.empty()){v.px+=weights[t]*m.positions[i][0];v.py+=weights[t]*m.positions[i][1];v.pz+=weights[t]*m.positions[i][2];}if(!m.normals.empty()){v.nx+=weights[t]*m.normals[i][0];v.ny+=weights[t]*m.normals[i][1];v.nz+=weights[t]*m.normals[i][2];}}
  if(!palette.empty()&&i<mesh.influences.size()){
   // Affine skinning: only the 3x4 coefficients affect positions/normals.
   // Raw scalar arithmetic avoids constructing/inverting generic matrices per vertex.
   const auto& influence=mesh.influences[i];float m[12]{};
   const auto* iw=influence.weights.data();const auto* ij=influence.joints.data();
   for(int j=0;j<4;++j){
    if(iw[j]==0.f)continue;
    const float* joint=palette[ij[j]].data();
    for(int c=0;c<4;++c)for(int r=0;r<3;++r)m[c*3+r]+=iw[j]*joint[c*4+r];
   }
   const float x=v.px,y=v.py,z=v.pz;
   v.px=m[0]*x+m[3]*y+m[6]*z+m[9];
   v.py=m[1]*x+m[4]*y+m[7]*z+m[10];
   v.pz=m[2]*x+m[5]*y+m[8]*z+m[11];
   // Cofactor matrix is inverse-transpose times determinant. Normalization
   // removes its magnitude; preserve the determinant sign for mirrored bones.
   const float a=m[4]*m[8]-m[5]*m[7],b=m[5]*m[6]-m[3]*m[8],c=m[3]*m[7]-m[4]*m[6];
   const float d=m[7]*m[2]-m[8]*m[1],e=m[8]*m[0]-m[6]*m[2],f=m[6]*m[1]-m[7]*m[0];
   const float g=m[1]*m[5]-m[2]*m[4],h=m[2]*m[3]-m[0]*m[5],k=m[0]*m[4]-m[1]*m[3];
   const float determinant=m[0]*a+m[1]*b+m[2]*c;
   const float nx=a*v.nx+d*v.ny+g*v.nz,ny=b*v.nx+e*v.ny+h*v.nz,nz=c*v.nx+f*v.ny+k*v.nz;
   const float length=std::sqrt(nx*nx+ny*ny+nz*nz);
   if(length>0.f && std::abs(determinant)>0.f && length/std::abs(determinant)>1e-12f){
    const float factor=(determinant<0.f?-1.f:1.f)/length;
    v.nx=nx*factor;v.ny=ny*factor;v.nz=nz*factor;
   }
  }
 }
}
