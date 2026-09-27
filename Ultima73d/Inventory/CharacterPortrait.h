#pragma once
#include <AnimatedCharacter.h>
#include <imgui.h>
#include <glad/gl.h>
#include <SDL3/SDL.h>
#include <algorithm>
#include <limits>
#include <vector>

class CharacterPortrait {
    unsigned framebuffer=0,color=0,depth=0;
    unsigned worn=std::numeric_limits<unsigned>::max();
    float renderedHeight=0;
    static constexpr int resolution=256;
    void refresh(ow3d::AnimatedCharacter& actor){
        if(!actor.loaded()||(color&&worn==actor.outfit.worn()&&renderedHeight==actor.height()))return;
        GLint drawFbo,readFbo,viewport[4],program,vao,active,texture,renderbuffer;
        GLfloat clear[4];GLboolean depthWrite;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&drawFbo);glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&readFbo);
        glGetIntegerv(GL_VIEWPORT,viewport);glGetIntegerv(GL_CURRENT_PROGRAM,&program);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);
        glGetIntegerv(GL_ACTIVE_TEXTURE,&active);glActiveTexture(GL_TEXTURE0);glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture);
        glGetIntegerv(GL_RENDERBUFFER_BINDING,&renderbuffer);glGetFloatv(GL_COLOR_CLEAR_VALUE,clear);glGetBooleanv(GL_DEPTH_WRITEMASK,&depthWrite);
        bool blend=glIsEnabled(GL_BLEND),depthTest=glIsEnabled(GL_DEPTH_TEST),scissor=glIsEnabled(GL_SCISSOR_TEST);
        if(!color){
            glGenFramebuffers(1,&framebuffer);glGenTextures(1,&color);glGenRenderbuffers(1,&depth);
            glBindTexture(GL_TEXTURE_2D,color);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,resolution,resolution,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
            glBindRenderbuffer(GL_RENDERBUFFER,depth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,resolution,resolution);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
            if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("Portrait framebuffer incomplete");
        }
        glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);glViewport(0,0,resolution,resolution);
        glDisable(GL_SCISSOR_TEST);glDisable(GL_BLEND);glEnable(GL_DEPTH_TEST);glDepthMask(GL_TRUE);
        glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        auto up=glm::normalize(actor.position());auto front=glm::normalize(actor.heading()-up*glm::dot(actor.heading(),up));
        auto target=actor.position()+up*(actor.height()*.91f);
        const float relativeHeight=actor.height()/actor.Height;
        ow3d::Camera camera;camera.setPosition(target+(front*.105f+up*.002f)*relativeHeight);camera.lookAt(target);camera.setPerspective(18,1,.001f,1.f);
        actor.draw(camera,resolution,true);worn=actor.outfit.worn();renderedHeight=actor.height();
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,drawFbo);glBindFramebuffer(GL_READ_FRAMEBUFFER,readFbo);
        glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);glClearColor(clear[0],clear[1],clear[2],clear[3]);
        glDepthMask(depthWrite);if(!depthTest)glDisable(GL_DEPTH_TEST);if(blend)glEnable(GL_BLEND);if(scissor)glEnable(GL_SCISSOR_TEST);
        glUseProgram(program);glBindVertexArray(vao);glBindRenderbuffer(GL_RENDERBUFFER,renderbuffer);
        glBindTexture(GL_TEXTURE_2D,texture);glActiveTexture(active);
    }
public:
    ~CharacterPortrait(){if(framebuffer)glDeleteFramebuffers(1,&framebuffer);if(color)glDeleteTextures(1,&color);if(depth)glDeleteRenderbuffers(1,&depth);}
    unsigned equippedMask()const{return worn;}
    bool save(const char* path){
        if(!color)return false;GLint previous;glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&previous);glBindFramebuffer(GL_READ_FRAMEBUFFER,framebuffer);
        std::vector<unsigned char> pixels(resolution*resolution*4);glReadPixels(0,0,resolution,resolution,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());glBindFramebuffer(GL_READ_FRAMEBUFFER,previous);
        for(int y=0;y<resolution/2;++y)for(int x=0;x<resolution*4;++x)std::swap(pixels[y*resolution*4+x],pixels[(resolution-y-1)*resolution*4+x]);
        auto surface=SDL_CreateSurfaceFrom(resolution,resolution,SDL_PIXELFORMAT_RGBA32,pixels.data(),resolution*4);
        if(!surface)return false;bool ok=SDL_SaveBMP(surface,path);SDL_DestroySurface(surface);return ok;
    }
    bool draw(ImVec2 a,ImVec2 b,ow3d::AnimatedCharacter& actor){
        if(!actor.loaded())return false;refresh(actor);
        auto cursor=ImGui::GetCursorScreenPos();auto* d=ImGui::GetWindowDrawList();
        float size=std::min(106.f,std::min(b.x-a.x,b.y-a.y)*.24f);float bar=12,space=5;
        ImVec2 p(a.x+14,b.y-size-2*(bar+space)-14),end(p.x+size,p.y+size+2*(bar+space));
        ImGui::SetCursorScreenPos(p);ImGui::InvisibleButton("Sir Canegm portrait",{size,end.y-p.y});bool hover=ImGui::IsItemHovered();
        if(hover)ImGui::SetTooltip("Sir Canegm\nLeben: %.0f / %.0f\nMana: %.0f / %.0f",actor.health,actor.maxHealth,actor.mana,actor.maxMana);
        d->AddRectFilled({p.x-4,p.y-4},{end.x+4,end.y+4},IM_COL32(37,29,20,235),7);
        d->AddRectFilled(p,{p.x+size,p.y+size},IM_COL32(43,52,39,255),5);
        d->AddImage((ImTextureID)(intptr_t)color,p,{p.x+size,p.y+size},{0,1},{1,0});
        d->AddRect(p,{p.x+size,p.y+size},IM_COL32(171,134,74,255),5,0,2.f);
        auto meter=[&](float y,float value,float maximum,ImU32 fill){
            float ratio=maximum>0?std::clamp(value/maximum,0.f,1.f):0;
            d->AddRectFilled({p.x,y},{p.x+size,y+bar},IM_COL32(22,20,19,255),3);
            if(ratio>0)d->AddRectFilled({p.x+2,y+2},{p.x+2+(size-4)*ratio,y+bar-2},fill,2);
            d->AddRect({p.x,y},{p.x+size,y+bar},IM_COL32(160,126,70,255),3);
        };
        meter(p.y+size+space,actor.health,actor.maxHealth,IM_COL32(184,36,38,255));
        meter(p.y+size+bar+2*space,actor.mana,actor.maxMana,IM_COL32(43,95,201,255));
        ImGui::SetCursorScreenPos(cursor);return hover;
    }
};
