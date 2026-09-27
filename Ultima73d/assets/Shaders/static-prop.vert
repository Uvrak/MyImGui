#version 460 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 textureCoordinate;
layout(location=3) in float material;
layout(location=4) in vec4 boneIndices;
layout(location=5) in vec4 boneWeights;
uniform mat4 viewProjection,model;
// Corpse mesh lies head at -z: flipZ=-1 turns its object space head-up for the patterns.
uniform float flipZ=1.;
uniform float time=0.;
uniform float joloPhase=0.;
uniform int joloAnimated=0,joloMotion=0;
uniform int rigged=0;
uniform mat4 joints[22];
out vec3 worldNormal,worldPosition,modelPosition;
out vec2 uv;
flat out int surface;
void rotateYZ(inout vec3 p,inout vec3 n,vec3 pivot,float angle){
    float c=cos(angle),s=sin(angle);vec3 q=p-pivot;
    p=pivot+vec3(q.x,c*q.y-s*q.z,s*q.y+c*q.z);
    n=normalize(vec3(n.x,c*n.y-s*n.z,s*n.y+c*n.z));
}
void animateJolo(inout vec3 p,inout vec3 n){
    if(joloAnimated==0 || joloMotion==0)return;
    float running=float(joloMotion==2);
    float phase=joloPhase*6.2831853,wave=sin(phase),side=p.x<0.?-1.:1.;
    float stride=wave*side*mix(.43,.72,running);
    float counter=-wave*side*mix(.38,.70,running);
    // Canegm-style retarget: thigh and calf bend separately, while upper arm
    // and forearm retain the source clip's visible elbow flexion.
    if(p.y<.080 && abs(p.x)>.010){
        rotateYZ(p,n,vec3(side*.026,.080,0.),stride*.58-max(0.,-wave*side)*mix(.28,.62,running));
    }else if(p.y<.148 && abs(p.x)>.008){
        rotateYZ(p,n,vec3(side*.026,.148,0.),stride);
    }else if(p.y<.174 && abs(p.x)>.052){
        rotateYZ(p,n,vec3(side*.057,.174,0.),counter*.55-max(0.,wave*side)*mix(.18,.48,running));
    }else if(p.y<.224 && abs(p.x)>.040){
        rotateYZ(p,n,vec3(side*.044,.216,0.),counter);
    }
    // Hip rise and opposing shoulder/hip twist reproduce Canegm's weight shift.
    float step=abs(sin(phase));
    float twist=sin(phase*2.)*mix(.025,.050,running);
    if(p.y>.145){float c=cos(twist),s=sin(twist);p.xz=mat2(c,-s,s,c)*p.xz;n.xz=mat2(c,-s,s,c)*n.xz;}
    p.y+=step*mix(.0022,.0044,running);
}
void main(){
    vec3 p=position,n=normal;
    if(rigged!=0){
        mat4 skin=mat4(0.);
        skin+=joints[clamp(int(boneIndices.x+.5),0,21)]*boneWeights.x;
        skin+=joints[clamp(int(boneIndices.y+.5),0,21)]*boneWeights.y;
        skin+=joints[clamp(int(boneIndices.z+.5),0,21)]*boneWeights.z;
        skin+=joints[clamp(int(boneIndices.w+.5),0,21)]*boneWeights.w;
        p=(skin*vec4(p,1.)).xyz;n=normalize(mat3(skin)*n);
    }
    animateJolo(p,n);
    worldPosition=(model*vec4(p,1)).xyz;
    modelPosition=p*vec3(1,1,flipZ);
    worldNormal=normalize(transpose(inverse(mat3(model)))*n);
    surface=int(material+.5);
    uv=textureCoordinate;
    gl_Position=viewProjection*vec4(worldPosition,1);
}
