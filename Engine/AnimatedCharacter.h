#pragma once
#include <functional>
#include "BuildingCollision.h"
#include "Camera.h"
#include "Shader.h"
#include "CharacterOutfit.h"
#include <filesystem>
#include <string>
#include <vector>
namespace ow3d
{
// Portable, sampled skeletal poses. The original rigged GLB remains in assets/source.
class AnimatedCharacter
{
  public:
    bool load(const std::filesystem::path &folder);
    void destroy();
    bool loaded() const
    {
        return m_loaded;
    }
    BuildingCollision collision;
    std::function<void(BuildingCollision&, glm::vec3, float)> prepareCollision;
    CharacterOutfit outfit;
    void setPosition(glm::vec3 position)
    {
        m_position = position;
    }
    bool follow = true;
    bool topDown = true;
    void update(float dt, bool forward, float mouseX, float mouseY);
    void updateCamera(Camera &camera,float dt=0);
    int pickEquipment(const Camera &camera, float x, float y) const;
    void resetCameraTilt() { m_targetPitch = topDown ? m_topDownPitch : .55f; }
    void setTopDownAngle(float degrees);
    void setTopDownView(bool enabled,Camera& camera);
    float topDownAngle() const { return glm::degrees(m_topDownPitch); }
    void zoom(float steps);
    void setCameraDistance(float distance);
    float cameraDistance() const { return m_distance*renderedScale(); }
    float renderedScale() const { return glm::length(glm::vec3(sceneModelTransform[0])); }
    float minimumCameraDistance = .28f;
    // Frontal view when zoomed in: starts at frontalStart x the closest distance and is
    // complete at the closest distance, looking frontalPitch degrees down.
    float frontalStart = 2.2f, frontalPitch = 6.f;
    float frontalAmount() const;
    // Zooming past the closest distance moves the camera through the head to just in front
    // of the face, looking straight ahead (first person): complete at firstPersonReach x the
    // closest distance, so whoever stands in front is seen face to face.
    float firstPersonReach = .35f, firstPersonPitch = 3.f;
    float firstPersonAmount() const;
    // Zooming out tilts the camera to a straight-down view: it starts at overheadStart and is
    // vertical at overheadFull (model units); each zoom-out step multiplies the distance by
    // 1/zoomOutStep.
    float overheadStart = 1.f, overheadFull = 1.8f, zoomOutStep = .8f;
    float overheadAmount() const;
    bool preferOutwardCamera = false;
    glm::mat4 sceneModelTransform{1.f};
    void setSceneModelTransform(const glm::mat4& model,Camera& camera);
    void draw(const Camera &camera, int viewportHeight,bool portrait=false);
    glm::vec3 heading() const { return m_heading; }
    glm::vec2 cameraOrbit() const { return {m_pitch,m_yaw}; }
    void restoreCameraOrbit(glm::vec3 heading,glm::vec2 orbit){
        const auto up=glm::normalize(m_position);
        const auto tangent=heading-up*glm::dot(heading,up);
        if(glm::length(tangent)>.001f)m_heading=glm::normalize(tangent);
        m_pitch=m_targetPitch=orbit.x;m_yaw=orbit.y;
    }
    float health=100.f,maxHealth=100.f,mana=100.f,maxMana=100.f;
    void preview(int gait, float distance, float time);
    void advancePreview(float time)
    {
        m_phase = time / m_levels[0].clips[m_clip].duration;
        m_phase -= int(m_phase);
    }
    int lod() const
    {
        return m_lod;
    }
    int gait() const
    {
        return m_gait;
    }
    float speed() const
    {
        return m_speed;
    }
    glm::vec3 position() const
    {
        return m_position;
    }
    static constexpr float Height = .157f;
    float height() const { return m_height; }
    void setHeight(float height);
    glm::vec2 collisionRadii() const { return m_collisionRadii*m_height; }
    void configureCollisionFootprint(BuildingCollision& target) const { target.setFootprint(m_heading,collisionRadii()); }

  private:
    struct Clip
    {
        float duration = 1;
        unsigned frames = 0;
        std::vector<float> poses;
    };
    struct Level
    {
        unsigned vertices = 0;
        std::vector<float> uv;
        std::vector<unsigned> indices;
        Clip clips[3];
        unsigned vao = 0, vbo = 0, ebo = 0;
    };
    Level m_levels[3];
    glm::vec2 m_collisionRadii{.26f,.38f};
    Shader m_shader;
    unsigned m_texture = 0, m_arrowVao = 0, m_arrowVbo = 0;
    bool m_loaded = false;
    float m_height = Height;
    float m_scale = Height, m_walk = .6f, m_run = 2.2f, m_travel = 0, m_speed = 0, m_phase = 0,
          m_oldPhase = 0, m_blend = 1;
    float m_distance = .66f, m_pitch = glm::radians(55.95f), m_yaw = 0;
    float m_targetPitch = glm::radians(55.95f);
    float m_topDownPitch = glm::radians(55.95f);
    float m_cameraDistance = -1.f;
    float m_cameraLift = 0.f;
    bool m_lastTopDown = true;
    // Gait 0 is retained for a future sneak mode, but is not selectable in gameplay.
    int m_gait = 1, m_clip = 0, m_oldClip = 0, m_lod = 0;
    glm::vec3 m_position{0}, m_heading{0, 1, 0};
    std::vector<float> m_stream;
    void sample(const Level &, int clip, float phase, std::vector<float> &out) const;
};
} // namespace ow3d
