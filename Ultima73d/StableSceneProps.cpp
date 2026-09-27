// Ported from Ultima7Remake/StaticSceneProps.cpp (the stable scene); positions are given by a
// ground function instead of Ultima7Remake's cube sphere. Keep changes in step with the original.
#include "StableScene.h"
#include <Canegm/GltfModel.h>
#include <glad/gl.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <stdexcept>
#include <cmath>

namespace {
std::string shaderText(const std::filesystem::path& path){
    std::ifstream in(path);if(!in)throw std::runtime_error("Static prop shader missing: "+path.string());
    return {std::istreambuf_iterator<char>(in),{}};
}
// The supplied Tripo corpse has one untextured material and no UVs. Preserve
// its geometry/material intent, but classify broad clothing regions per face.
// This avoids inventing a replacement texture and keeps the low-poly facets.
// The mesh lies with its head at -z; classify along the body with h=-z (head up).
float corpseMaterial(glm::vec3 p){
    p.z=-p.z;
    if(p.z<-.285f)return 3.f;                                      // boots
    if(p.z<.075f){
        if(std::abs(p.x)>.245f && p.z>-.20f)return 0.f;            // hands
        return 2.f;                                                // trousers
    }
    // Head and neck: the narrow column above the collar (neck at h=.33-.37, head
    // centred slightly off x=0). The face is on the high-Y side; the back of the
    // skull rests on the floor (low Y) and carries the hair with the crown.
    if(p.z>.325f && std::abs(p.x-.012f)<.10f){
        if(p.z<.37f)return 0.f;                                    // neck skin
        if(p.y<.05f || p.z>.475f || std::abs(p.x-.012f)>.055f)return 4.f; // hair: back, crown and sides
        if(p.z<.40f && p.y>.085f)return 8.f;                       // stubble on the chin
        return 7.f;                                                 // face
    }
    if(std::abs(p.x)>.265f)return p.z>.18f?1.f:0.f;                // shirt shoulders; bare forearms/hands
    if(p.z<.105f)return 5.f;                                      // belt
    if(p.z>.19f && p.z<.285f && std::abs(p.x)<.105f && p.y>.105f)return 6.f; // wound
    return 1.f;                                                    // linen shirt
}
// ToterGargoyle.glb carries its own materials in this order: skin, horn-claw,
// loincloth, blood, wing membrane (shader surfaces 9-13).
float gargoyleMaterial(int materialIndex){
    constexpr float surfaces[]={9.f,10.f,11.f,12.f,13.f};
    return materialIndex>=0 && materialIndex<5?surfaces[materialIndex]:9.f;
}
void corpseXPose(glm::vec3& p,glm::vec3& n){
    // Christopher's U7 silhouette is spread-eagle: hands reach towards the
    // northern forks and feet towards the southern forks.  The Tripo source
    // uses -Z from pelvis to head, so rotate limbs around shoulder/hip pivots
    // while leaving torso, pelvis and head fixed on their map registration.
    float h=-p.z;
    // The generated source exaggerates the boots. Restore human proportions
    // around the ankles before spreading the legs, without changing leg length.
    if(h<-.285f){
        const glm::vec3 ankle(p.x>0.f?.055f:-.055f,p.y,.285f);
        const glm::vec3 q=p-ankle;
        p=ankle+glm::vec3(q.x*.80f,q.y*.80f,q.z);
        h=-p.z;
    }
    float angle=0.f;glm::vec3 pivot(0);
    const float side=std::abs(p.x);
    // Outer forearms/hands sit as low as the hips in the source mesh.  Claim
    // them for the arm before testing the legs, otherwise the hand receives a
    // leg rotation and creates an apparent bent elbow.
    if(h<.37f&&(side>.20f||(h>.075f&&side>.135f))){               // complete straight arms
        angle=glm::radians(p.x>0.f?63.f:-63.f);
        pivot=glm::vec3(p.x>0.f?.135f:-.135f,p.y,-.235f);
    }else if(h<.07f&&side>.035f){                                 // legs
        angle=glm::radians(p.x>0.f?25.f:-25.f);
        pivot=glm::vec3(p.x>0.f?.055f:-.055f,p.y,-.055f);
    }else return;
    const float c=std::cos(angle),s=std::sin(angle);const glm::vec3 q=p-pivot;
    p=pivot+glm::vec3(c*q.x+s*q.z,q.y,-s*q.x+c*q.z);
    n=glm::normalize(glm::vec3(c*n.x+s*n.z,n.y,-s*n.x+c*n.z));
}
GltfMesh simplifyStatic(const GltfMesh& source,float cell){
    if(cell<=0)return source;
    GltfMesh result=source;result.vertices.clear();result.indices.clear();
    using Key=std::array<int,3>;std::map<Key,unsigned> clusters;
    std::vector<unsigned> remap,count;
    for(const auto& v:source.vertices){
        Key key{int(std::floor(v.px/cell)),int(std::floor(v.py/cell)),int(std::floor(v.pz/cell))};
        auto [where,added]=clusters.emplace(key,unsigned(result.vertices.size()));const unsigned target=where->second;
        remap.push_back(target);if(added){result.vertices.push_back({});count.push_back(0);}
        auto& out=result.vertices[target];out.px+=v.px;out.py+=v.py;out.pz+=v.pz;out.u=v.u;
        out.nx+=v.nx;out.ny+=v.ny;out.nz+=v.nz;++count[target];
    }
    for(size_t i=0;i<result.vertices.size();++i){auto& v=result.vertices[i];const float n=float(count[i]);
        v.px/=n;v.py/=n;v.pz/=n;const auto normal=glm::normalize(glm::vec3(v.nx,v.ny,v.nz));v.nx=normal.x;v.ny=normal.y;v.nz=normal.z;}
    for(size_t i=0;i+2<source.indices.size();i+=3){const auto a=remap[source.indices[i]],b=remap[source.indices[i+1]],c=remap[source.indices[i+2]];
        if(a!=b&&a!=c&&b!=c)result.indices.insert(result.indices.end(),{a,b,c});}
    return result;
}
// Style 2 (stable furnishings, garden tools): shader surface by GLB material name.
float furnishingSurface(const std::string& material){
    if(material=="hay")return 14.f;
    if(material=="table-wood")return 15.f;
    if(material=="iron")return 16.f;
    if(material=="handle-wood")return 17.f;
    // Props/GardenTools (Tripo, reference-matched material names)
    if(material=="worn-iron")return 16.f;
    if(material=="ash-handle")return 17.f;
    if(material=="linen")return 18.f;
    if(material=="red-stripe")return 19.f;
    if(material=="golden-straw")return 14.f;
    if(material=="straw-highlight")return 20.f;
    if(material=="binding")return 21.f;
    if(material=="oak")return 22.f;
    if(material=="tablecloth")return 28.f;
    if(material=="trough-stone")return 32.f;
    if(material=="water")return 33.f;
    throw std::runtime_error("Unknown furnishing material: "+material);
}
float joloSurface(const std::string& material){
    if(material=="skin")return 37.f;
    if(material=="white-hair")return 34.f;
    if(material=="loincloth")return 18.f;
    if(material=="cord")return 21.f;
    if(material=="jolo-leather"||material=="jolo-trousers")return 35.f;
    if(material=="jolo-boots")return 3.f;
    if(material=="jolo-hardware")return 36.f;
    throw std::runtime_error("Unknown Jolo material: "+material);
}
float joloLeatherSurface(const std::string& material){
    if(material=="jolo-leather")return 35.f;
    if(material=="jolo-hardware")return 36.f;
    throw std::runtime_error("Unknown Jolo leather material: "+material);
}
std::vector<float> drawVertices(const GltfMesh& mesh,int style,const std::vector<float>& surfaces){
    std::vector<float> result;result.reserve(mesh.indices.size()*17);
    for(size_t triangle=0;triangle+2<mesh.indices.size();triangle+=3){
        glm::vec3 center(0);for(int corner=0;corner<3;++corner){const auto& v=mesh.vertices[mesh.indices[triangle+corner]];center+=glm::vec3(v.px,v.py,v.pz)/3.f;}
        // u carries the GLB material index of the source vertex.
        const int index=int(mesh.vertices[mesh.indices[triangle]].u);
        const float material=style==3&&surfaces.front()==38.f?38.f:style>=2?surfaces.at(size_t(index)):style==1?gargoyleMaterial(index):corpseMaterial(center);
        for(int corner=0;corner<3;++corner){const auto vertexIndex=mesh.indices[triangle+corner];const auto& v=mesh.vertices[vertexIndex];
            glm::vec3 p(v.px,v.py,v.pz),n(v.nx,v.ny,v.nz);if(style==0)corpseXPose(p,n);
            result.insert(result.end(),{p.x,p.y,p.z,n.x,n.y,n.z,style==3?v.u:v.px,style==3?v.v:v.pz,material});
            GltfInfluence influence;if(vertexIndex<mesh.influences.size())influence=mesh.influences[vertexIndex];else influence.weights[0]=1;
            for(auto joint:influence.joints)result.push_back(float(joint));for(auto weight:influence.weights)result.push_back(weight);}
    }
    return result;
}
}

StableSceneProps::~StableSceneProps(){for(auto& prop:m_props)if(prop.texture)glDeleteTextures(1,&prop.texture);}

void StableSceneProps::load(const std::filesystem::path& assets,const std::filesystem::path& data,const Ground& ground){
    if(!m_shader.create(shaderText(data/"Shaders/static-prop.vert").c_str(),
                        shaderText(data/"Shaders/static-prop.frag").c_str()))
        throw std::runtime_error("Static prop shader failed");
    std::ifstream placements(data/"Maps/static-scene-props.txt");
    std::string magic;unsigned count=0;
    if(!(placements>>magic>>count)||magic!="U7_STATIC_PROPS_2"||count>256)
        throw std::runtime_error("Invalid static prop catalog");
    for(unsigned entry=0;entry<count;++entry){
        std::string file;float x,y,height,pitch,yaw,roll,scale;int style;
        if(!(placements>>std::quoted(file)>>x>>y>>height>>pitch>>yaw>>roll>>scale>>style) ||
           !std::isfinite(x+y+height+pitch+yaw+roll+scale) || x<0||x>=1792||y<0||y>=1792 ||
           height<-.5f||height>20.f||scale<=0||scale>20.f||style<0||style>4)
            throw std::runtime_error("Invalid static prop placement");
        bool skipped=false;for(const auto& part:skipFiles)skipped=skipped||file.find(part)!=std::string::npos;
        if(skipped)continue;
        GltfModel source;
        if(!loadGltf(assets/file,source)||source.meshes.empty())
            throw std::runtime_error("Static prop GLB missing or invalid: "+file);
        std::vector<float> surfaces;
        if(style==2)for(const auto& material:source.materials)surfaces.push_back(furnishingSurface(material.name));
        if(style==3)for(const auto& material:source.materials)surfaces.push_back(material.baseColorImage>=0?38.f:joloSurface(material.name));
        if(style==4)for(const auto& material:source.materials)surfaces.push_back(joloLeatherSurface(material.name));
        GltfMesh combined;
        glm::vec3 low(std::numeric_limits<float>::max()),high(-std::numeric_limits<float>::max());
        for(const auto& part:source.meshes){
            const glm::mat4 node=glm::make_mat4(part.transform);
            const glm::mat3 normalMatrix=glm::transpose(glm::inverse(glm::mat3(node)));
            for(size_t triangle=0;triangle+2<part.indices.size();triangle+=3){
                glm::vec3 center(0);glm::vec3 points[3],normals[3];
                for(int corner=0;corner<3;++corner){
                    const auto index=part.indices[triangle+corner];if(index>=part.vertices.size())throw std::runtime_error("Static prop index out of range");
                    const auto& v=part.vertices[index];
                    points[corner]=glm::vec3(node*glm::vec4(v.px,v.py,v.pz,1));
                    normals[corner]=glm::normalize(normalMatrix*glm::vec3(v.nx,v.ny,v.nz));
                    center+=points[corner]/3.f;low=glm::min(low,points[corner]);high=glm::max(high,points[corner]);
                }
                for(int corner=0;corner<3;++corner){const auto p=points[corner],n=normals[corner];const auto& vertex=part.vertices[part.indices[triangle+corner]];
                    combined.vertices.push_back({p.x,p.y,p.z,n.x,n.y,n.z,style==3?vertex.u:float(part.materialIndex),style==3?vertex.v:0.f});
                    combined.influences.push_back(part.indices[triangle+corner]<part.influences.size()?part.influences[part.indices[triangle+corner]]:GltfInfluence{});
                    combined.indices.push_back(unsigned(combined.indices.size()));}
            }
        }
        if(combined.indices.empty()||combined.indices.size()>300000)throw std::runtime_error("Invalid static prop geometry");
        const auto root=ground(x,y),up=glm::normalize(root);
        const auto east=glm::normalize(glm::vec3(1,0,0)-up*up.x),south=glm::normalize(glm::cross(up,east));
        glm::mat4 tangent(1);tangent[0]=glm::vec4(east,0);tangent[1]=glm::vec4(up,0);tangent[2]=glm::vec4(south,0);tangent[3]=glm::vec4(root+up*(height*Metre),1);
        glm::mat4 local(1);local=glm::rotate(local,glm::radians(yaw),glm::vec3(0,1,0));
        local=glm::rotate(local,glm::radians(pitch),glm::vec3(1,0,0));local=glm::rotate(local,glm::radians(roll),glm::vec3(0,0,1));
        local=glm::scale(local,glm::vec3(scale*Metre));
        Prop prop;prop.model=tangent*local;prop.restModel=prop.model;prop.metres=scale;prop.flipZ=style==0?-1.f:1.f;prop.style=style;prop.localCenter=(low+high)*.5f;
        if((style==3||style==4)&&source.animation&&!source.animation->skins.empty()){
            prop.animation=source.animation;prop.skin=source.meshes.front().skin;
            const char* names[]={"Idle","Walk","Run"};
            for(int wanted=0;wanted<3;++wanted)for(size_t clip=0;clip<source.animation->clips.size();++clip)
                if(source.animation->clips[clip].name==names[wanted])prop.clips[wanted]=int(clip);
            if(prop.skin<0||prop.clips[0]<0||prop.clips[1]<0||prop.clips[2]<0)throw std::runtime_error("Incomplete Jolo humanoid rig");
        }
        if(style==3&&!source.materials.empty()&&source.materials.front().baseColorImage>=0){
            const int imageIndex=source.materials.front().baseColorImage;
            if(imageIndex>=int(source.images.size()))throw std::runtime_error("Invalid Jolo texture index");
            const auto& bytes=source.images[size_t(imageIndex)].bytes;
            auto* image=IMG_Load_IO(SDL_IOFromConstMem(bytes.data(),bytes.size()),true);
            if(!image)throw std::runtime_error("Cannot decode Jolo texture");
            auto* rgba=SDL_ConvertSurface(image,SDL_PIXELFORMAT_RGBA32);SDL_DestroySurface(image);
            if(!rgba)throw std::runtime_error(SDL_GetError());
            glGenTextures(1,&prop.texture);glBindTexture(GL_TEXTURE_2D,prop.texture);glPixelStorei(GL_UNPACK_ROW_LENGTH,rgba->pitch/4);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,rgba->w,rgba->h,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba->pixels);
            glPixelStorei(GL_UNPACK_ROW_LENGTH,0);SDL_DestroySurface(rgba);glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        }
        {   // Extent in metres (east, up, north) for placement checks.
            Extent extent{file,x,y,glm::vec3(1e9f),glm::vec3(-1e9f)};
            for(const auto& v:combined.vertices){const auto q=glm::vec3(local*glm::vec4(v.px,v.py,v.pz,1))/Metre;extent.low=glm::min(extent.low,q);extent.high=glm::max(extent.high,q);}
            m_extents.push_back(extent);
        }
        prop.center=glm::vec3(prop.model*glm::vec4(prop.localCenter,1));
        prop.radius=glm::length(high-low)*.5f*scale*Metre;
        // Thin tool handles would collapse in the coarse LODs; furnishings keep full detail.
        const float cells[]={0.f,.018f,.042f};
        for(int lod=0;lod<3;++lod){const auto simplified=simplifyStatic(combined,(style==2||style==3)?0.f:cells[lod]);const auto vertices=drawVertices(simplified,style,surfaces);
            if(vertices.empty()||!prop.lod[lod].create(vertices.data(),unsigned(vertices.size()/17),17))throw std::runtime_error("Static prop LOD upload failed");}
        m_props.push_back(std::move(prop));
    }
}

void StableSceneProps::render(const glm::mat4& viewProjection,const glm::vec3& eye,float time) {
    if(m_props.empty())return;m_shader.bind();m_shader.setMat4("viewProjection",viewProjection);m_shader.setInt("baseColorTexture",12);
    m_shader.setFloat("time",time);m_shader.setFloat("joloPhase",m_joloPhase);m_shader.setInt("joloMotion",m_joloMotion);
    m_shader.setVec3("eye",eye);const bool culling=glIsEnabled(GL_CULL_FACE),blending=glIsEnabled(GL_BLEND);GLboolean depthWrite=GL_TRUE;glGetBooleanv(GL_DEPTH_WRITEMASK,&depthWrite);
    glDisable(GL_CULL_FACE);glDisable(GL_BLEND);glDepthMask(GL_TRUE);
    for(const auto& prop:m_props){if(prop.style==4&&!m_joloLeatherEquipped)continue;
        const float distance=glm::distance(eye,prop.center);if(distance>2.f+prop.radius)continue;
        const int lod=distance<.45f?0:distance<1.f?1:2;m_shader.setMat4("model",prop.model);m_shader.setInt("joloAnimated",(prop.style==3||prop.style==4)&&!prop.animation);
        m_shader.setInt("rigged",prop.animation?1:0);
        if(prop.animation){const int clip=prop.clips[size_t(m_joloMotion)];const auto& animation=*prop.animation;
            const auto pose=evaluateGltfPose(animation,clip,m_joloPhase*animation.clips[size_t(clip)].duration);const auto palette=gltfSkinMatrices(animation,pose,prop.skin);
            for(size_t joint=0;joint<std::min<size_t>(palette.size(),22);++joint){const auto matrix=glm::make_mat4(palette[joint].data());const auto name="joints["+std::to_string(joint)+"]";m_shader.setMat4(name.c_str(),matrix);}}
        m_shader.setInt("hasBaseColorTexture",prop.texture!=0);glActiveTexture(GL_TEXTURE12);glBindTexture(GL_TEXTURE_2D,prop.texture);glActiveTexture(GL_TEXTURE0);
        m_shader.setFloat("unitMetres",prop.metres);m_shader.setFloat("flipZ",prop.flipZ);prop.lod[lod].bind();glDrawArrays(GL_TRIANGLES,0,prop.lod[lod].vertexCount());}
    if(culling)glEnable(GL_CULL_FACE);if(blending)glEnable(GL_BLEND);glDepthMask(depthWrite);glBindVertexArray(0);
}
