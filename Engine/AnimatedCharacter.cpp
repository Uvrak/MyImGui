#include "AnimatedCharacter.h"
#include "WorldSettings.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
namespace ow3d
{
namespace
{
template <class T> void read(std::ifstream &f, T &value)
{
    if (!f.read(reinterpret_cast<char *>(&value), sizeof(T)))
        throw std::runtime_error("Truncated character cache");
}
template <class T> void array(std::ifstream &f, std::vector<T> &value, size_t count)
{
    value.resize(count);
    if (!f.read(reinterpret_cast<char *>(value.data()), count * sizeof(T)))
        throw std::runtime_error("Truncated character poses");
}
constexpr const char *vs = R"(#version 460 core
layout(location=0) in vec3 p; layout(location=1) in vec3 n; layout(location=2) in vec2 uv;
uniform mat4 model,view,projection; out vec3 normal; out vec2 texcoord;
void main(){gl_Position=projection*view*model*vec4(p,1);normal=mat3(model)*n;texcoord=uv;})";
constexpr const char *fs = R"(#version 460 core
in vec3 normal; in vec2 texcoord; uniform sampler2D albedo; uniform int arrow; uniform vec3 colorFactor; out vec4 color;
void main(){if(arrow!=0){color=vec4(.045,.24,.09,1);return;}
vec3 c=texture(albedo,texcoord).rgb;float light=.66+.34*max(dot(normalize(normal),normalize(vec3(-.3,-.5,1))),0);
color=vec4(c*colorFactor*light,1);})";
void layout()
{
    for (int i = 0; i < 3; ++i)
        glEnableVertexAttribArray(i);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
}
} // namespace
bool AnimatedCharacter::load(const std::filesystem::path &folder)
{
    destroy();
    try
    {
        float height = 1;
        for (int l = 0; l < 3; ++l)
        {
            auto &level = m_levels[l];
            std::ifstream f(folder / ("lod" + std::to_string(l) + ".owanim"), std::ios::binary);
            char magic[8];
            if (!f.read(magic, 8) || std::memcmp(magic, "OWANIM1\0", 8))
                throw std::runtime_error("Missing/invalid Canegm LOD cache");
            unsigned ni, nc;
            read(f, level.vertices);
            read(f, ni);
            read(f, nc);
            read(f, height);
            if (!level.vertices || level.vertices > 200000 || ni > 1200000 || ni % 3 || nc != 3 ||
                !std::isfinite(height) || height <= 0)
                throw std::runtime_error("Invalid character cache dimensions");
            array(f, level.uv, size_t(level.vertices) * 2);
            array(f, level.indices, ni);
            for (auto i : level.indices)
                if (i >= level.vertices)
                    throw std::runtime_error("Invalid character index");
            const char *names[] = {"Idle", "Walk", "Run"};
            for (int c = 0; c < 3; ++c)
            {
                char name[16];
                f.read(name, 16);
                auto &clip = level.clips[c];
                read(f, clip.duration);
                read(f, clip.frames);
                if (std::strncmp(name, names[c], 16) || clip.frames < 2 || clip.frames > 1000 ||
                    !std::isfinite(clip.duration) || clip.duration <= 0)
                    throw std::runtime_error("Invalid character clip");
                array(f, clip.poses, size_t(clip.frames) * level.vertices * 6);
            }
            glGenVertexArrays(1, &level.vao);
            glGenBuffers(1, &level.vbo);
            glGenBuffers(1, &level.ebo);
            glBindVertexArray(level.vao);
            glBindBuffer(GL_ARRAY_BUFFER, level.vbo);
            glBufferData(GL_ARRAY_BUFFER, size_t(level.vertices) * 8 * sizeof(float), nullptr,
                         GL_STREAM_DRAW);
            layout();
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, level.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, ni * sizeof(unsigned), level.indices.data(),
                         GL_STATIC_DRAW);
        }
        // Enclose the visible body throughout idle/walk/run, independently of render LOD.
        glm::vec2 radii(0);
        for(const auto& clip:m_levels[0].clips)for(size_t i=0;i<clip.poses.size();i+=6)
            radii=glm::max(radii,glm::abs(glm::vec2(clip.poses[i],clip.poses[i+2])));
        float envelope=1;
        for(const auto& clip:m_levels[0].clips)for(size_t i=0;i<clip.poses.size();i+=6)
            envelope=std::max(envelope,glm::length(glm::vec2(clip.poses[i],clip.poses[i+2])/radii));
        m_collisionRadii=radii*(envelope*1.02f/height);
        m_scale = m_height / height;
        std::ifstream cfg(folder / "locomotion.cfg");
        std::string version;
        cfg >> version >> m_walk >> m_run;
        if (version != "locomotion-v1" || !(m_walk > 0) || !(m_run > m_walk))
            throw std::runtime_error("Invalid locomotion speeds");
        SDL_Surface *image = SDL_LoadBMP((folder / "albedo.bmp").string().c_str());
        if (!image)
            throw std::runtime_error("Missing Canegm texture");
        SDL_Surface *rgba = SDL_ConvertSurface(image, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(image);
        if (!rgba)
            throw std::runtime_error("Cannot decode Canegm texture");
        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgba->w, rgba->h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     rgba->pixels);
        SDL_DestroySurface(rgba);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        if (!m_shader.create(vs, fs))
            throw std::runtime_error("Canegm shader failed");
        glGenVertexArrays(1, &m_arrowVao);
        glGenBuffers(1, &m_arrowVbo);
        glBindVertexArray(m_arrowVao);
        glBindBuffer(GL_ARRAY_BUFFER, m_arrowVbo);
        layout();
        glBindVertexArray(0);
        // Beside the east entrance, outside the house footprint. Y-up model becomes radial-up.
        m_position = glm::normalize(glm::vec3(-.48f, 1.0f, 19.978f)) * PlanetRadius;
        auto up = glm::normalize(m_position);
        m_heading = glm::normalize(glm::vec3(-1, 0, 0) - up * glm::dot(up, glm::vec3(-1, 0, 0)));
        m_loaded = true;
        return true;
    }
    catch (const std::exception &e)
    {
        SDL_Log("Canegm: %s", e.what());
        destroy();
        return false;
    }
}
void AnimatedCharacter::setHeight(float height)
{
    if(!std::isfinite(height) || height<=0)throw std::invalid_argument("Invalid character height");
    const float ratio=height/m_height;
    m_scale*=ratio;m_speed*=ratio;m_distance*=ratio;
    m_height=height;m_cameraDistance=-1.f;
}
void AnimatedCharacter::destroy()
{
    outfit.destroy();
    for (auto &l : m_levels)
    {
        if (l.vao)
            glDeleteVertexArrays(1, &l.vao);
        if (l.vbo)
            glDeleteBuffers(1, &l.vbo);
        if (l.ebo)
            glDeleteBuffers(1, &l.ebo);
        l = Level{};
    }
    if (m_texture)
        glDeleteTextures(1, &m_texture);
    if (m_arrowVao)
        glDeleteVertexArrays(1, &m_arrowVao);
    if (m_arrowVbo)
        glDeleteBuffers(1, &m_arrowVbo);
    m_texture = m_arrowVao = m_arrowVbo = 0;
    m_shader.destroy();
    m_loaded = false;
}
float AnimatedCharacter::frontalAmount() const
{
    const float close=std::clamp((frontalStart*minimumCameraDistance-m_distance)/
                                 std::max((frontalStart-1.f)*minimumCameraDistance,1e-6f),0.f,1.f);
    return close*close*(3-2*close);
}
float AnimatedCharacter::firstPersonAmount() const
{
    const float inside=std::clamp((minimumCameraDistance-m_distance)/
                                  std::max((1.f-firstPersonReach)*minimumCameraDistance,1e-6f),0.f,1.f);
    return inside*inside*(3-2*inside);
}
float AnimatedCharacter::overheadAmount() const
{
    const float out=std::clamp((m_distance-overheadStart)/std::max(overheadFull-overheadStart,1e-6f),0.f,1.f);
    return out*out*(3-2*out);
}
void AnimatedCharacter::zoom(float steps)
{
    // Zooming out takes bigger steps, so the overhead view is reached quickly.
    m_distance = std::clamp(m_distance * std::pow(steps < 0 ? zoomOutStep : .86f, steps), minimumCameraDistance*firstPersonReach, 8.f);
}
void AnimatedCharacter::update(float dt, bool forward, float dx, float dy)
{
    if (!m_loaded)
        return;
    dt = std::clamp(dt, 0.f, .05f);
    if (topDown != m_lastTopDown)
    {
        m_targetPitch = topDown ? m_topDownPitch : .55f;
        m_lastTopDown = topDown;
    }
    auto up = glm::normalize(m_position);
    m_yaw -= dx * .0026f;
    if (forward)
    {
        m_gait = std::max(1, m_gait);
        m_travel = std::clamp(m_travel - dy, 0.f, 440.f);
        // Enter running with half the original mouse travel; retain a separate return
        // threshold so small hand movements do not toggle the gait repeatedly.
        constexpr float faster[] = {80.f, 100.f, 320.f};
        constexpr float slower[] = {35.f, 70.f, 260.f};
        while (m_gait < 3 && m_travel >= faster[m_gait])
            ++m_gait;
        while (m_gait > 1 && m_travel <= slower[m_gait - 1])
            --m_gait;
    }
    else
    {
        m_travel = 0;
        m_gait = 1;
        // Positive orbit pitch looks downward: mouse down increases it.
        // Smooth the target,
        // including preset changes, instead of snapping the rendered camera.
        m_targetPitch = std::clamp(m_targetPitch + dy * .003f,
                                  glm::radians(-70.f), glm::radians(90.f));
    }
    m_pitch += (m_targetPitch - m_pitch) * (1.f - std::exp(-12.f * dt));
    if (forward && std::abs(m_yaw) > 1e-6f)
    {
        m_heading = glm::normalize(glm::vec3(glm::rotate(glm::mat4(1), m_yaw, up) * glm::vec4(m_heading, 0)));
        m_yaw = 0;
    }
    if (prepareCollision) {
        prepareCollision(collision, m_position, m_height);
        configureCollisionFootprint(collision);
        // Re-evaluate support even while idle (e.g. a stone is picked up or moved).
        m_position=collision.groundPosition(collision.resolve(m_position,m_position));
    }
    const float target = forward ? m_scale * (m_gait >= 2 ? m_run * (m_gait == 3 ? 1.5f : 1.f)
                                                          : m_walk * (m_gait == 1 ? 1.8f : 1.f))
                                 : 0;
    const float change = m_walk * m_scale * 12 * dt;
    m_speed += std::clamp(target - m_speed, -change, change);
    if (m_speed > 0 && dt > 0)
    {
        auto rotation =
            glm::rotate(glm::mat4(1), m_speed * dt / PlanetRadius, glm::normalize(glm::cross(up, m_heading)));
        auto candidate = glm::normalize(glm::vec3(rotation * glm::vec4(m_position, 1))) * PlanetRadius;
        auto moved = collision.resolve(m_position, candidate);
        auto displacement = moved - m_position;
        displacement -= up * glm::dot(displacement, up);
        float distance = glm::length(displacement);
        m_speed = distance < 1e-6f ? 0.f : std::min(m_speed, distance / dt);
        m_position = moved;
        auto nextUp = glm::normalize(m_position);
        m_heading = glm::normalize(m_heading - nextUp * glm::dot(m_heading, nextUp));
    }
    const int clip = m_speed < .0001f ? 0 : (m_gait >= 2 && forward ? 2 : 1);
    if (clip != m_clip)
    {
        m_oldClip = m_clip;
        m_oldPhase = m_phase;
        m_clip = clip;
        m_blend = 0;
    }
    m_blend = std::min(1.f, m_blend + dt / .18f);
    const float native = m_scale * (m_clip == 2 ? m_run : m_walk);
    m_phase += dt * (m_clip == 0 ? 1.f : m_speed / native) / m_levels[0].clips[m_clip].duration;
    m_phase -= std::floor(m_phase);
}
void AnimatedCharacter::setTopDownAngle(float degrees)
{
    if(!std::isfinite(degrees))return;
    m_topDownPitch=glm::radians(std::clamp(degrees,10.f,90.f));
    if(topDown)m_pitch=m_targetPitch=m_topDownPitch;
}
void AnimatedCharacter::setTopDownView(bool enabled,Camera& camera)
{
    topDown=m_lastTopDown=enabled;
    follow=true;
    m_pitch=m_targetPitch=topDown?m_topDownPitch:.55f;
    updateCamera(camera);
}
void AnimatedCharacter::setCameraDistance(float distance)
{
    if(!std::isfinite(distance))return;
    m_distance=std::clamp(distance/renderedScale(),minimumCameraDistance*firstPersonReach,8.f);
    m_cameraDistance=-1.f;
}
void AnimatedCharacter::setSceneModelTransform(const glm::mat4& model,Camera& camera)
{
    const auto before=glm::vec3(sceneModelTransform*glm::vec4(m_position,1));
    const auto after=glm::vec3(model*glm::vec4(m_position,1));
    const float ratio=glm::length(glm::vec3(model[0]))/renderedScale();
    sceneModelTransform=model;
    // Preserve the camera offset in model units, including height and sideways
    // offset. The orbit distance remains stored in unscaled model coordinates.
    camera.setPosition(after+(camera.position()-before)*ratio);
    camera.setCharacterPosition(after);
}
void AnimatedCharacter::updateCamera(Camera &camera,float dt)
{
    if (!m_loaded || !follow)
        return;
    auto up = glm::normalize(m_position);
    auto tangentHeading=glm::normalize(m_heading-up*glm::dot(m_heading,up));
    auto viewHeading = glm::vec3(glm::rotate(glm::mat4(1), m_yaw, up) * glm::vec4(tangentHeading, 0));
    // Zooming in turns the view frontal: from frontalStart down to the closest distance the
    // camera sinks behind the character to eye level and looks ahead instead of down.
    const float frontal=frontalAmount();
    auto target = m_position + up * (m_height * (.45f+.45f*frontal));
    float pitch = m_pitch+(glm::radians(frontalPitch)-m_pitch)*frontal;
    // Zooming out tilts the view to straight down (90 degrees).
    pitch += (glm::radians(90.f)-pitch)*overheadAmount();
    // The orbit keeps at least the closest distance; first person takes over below it.
    const float orbit = std::max(m_distance, minimumCameraDistance);
    auto eye = target + up * (orbit * std::sin(pitch)) -
               viewHeading * (orbit * std::cos(pitch));
    // Keep the requested viewing direction when the orbit reaches the ground.
    // Raising both the eye and its look target prevents the floor clamp from
    // limiting upward looking or causing a snap at the transition.
    const auto ground = collision.groundPosition(eye);
    const auto groundUp = glm::normalize(ground);
    constexpr float clearance = .025f;
    const float lift = std::max(0.f, clearance - glm::dot(eye - ground, groundUp));
    const auto correction = groundUp * lift;
    const auto desiredEye=eye+correction;
    const auto offset=desiredEye-target;
    const float desiredDistance=glm::length(offset);
    glm::vec3 resolvedEye;
    if(preferOutwardCamera){
        // U7's small world scale requires a camera probe smaller than the body.
        // Never pull the camera onto the avatar because a trunk crosses its ray.
        const float probe=std::min(.015f,m_height*.12f);
        float requiredLift=0;
        if(collision.cameraFraction(target,desiredEye,probe)<.999f){
            requiredLift=desiredDistance;
            for(int step=1;step<=24;++step){
                const float lift=desiredDistance*float(step)*.125f;
                if(collision.cameraFraction(target,desiredEye+up*lift,probe*1.2f)>=.999f){requiredLift=lift;break;}
            }
        }
        if(m_cameraDistance<0)m_cameraLift=requiredLift;
        else if(requiredLift>m_cameraLift)m_cameraLift=requiredLift;
        else {
            const float candidate=m_cameraLift+(requiredLift-m_cameraLift)*(1-std::exp(-2.f*std::clamp(dt>0?dt:1.f/60.f,0.f,.1f)));
            // Keep the last safe height at obstacle boundaries instead of pumping.
            if(collision.cameraFraction(target,desiredEye+up*candidate,probe*1.2f)>=.999f)m_cameraLift=candidate;
        }
        resolvedEye=desiredEye+up*m_cameraLift;
        // Upward avoidance must also preserve distance when looking below the horizon.
        if(glm::distance(target,resolvedEye)<desiredDistance)
            resolvedEye=target+glm::normalize(resolvedEye-target)*desiredDistance;
        m_cameraDistance=glm::distance(target,resolvedEye);
    }else{
    const float safeDistance=desiredDistance*collision.cameraFraction(target,desiredEye);
    // Move inward immediately to avoid crossing a wall; ease back outward.
    if(m_cameraDistance<0 || safeDistance<m_cameraDistance)m_cameraDistance=safeDistance;
    else m_cameraDistance+=(safeDistance-m_cameraDistance)*(1-std::exp(-6.f*std::clamp(dt,0.f,.1f)));
    resolvedEye=target+offset*(m_cameraDistance/std::max(desiredDistance,1e-6f));
    auto support=collision.groundPosition(resolvedEye);auto supportUp=glm::normalize(support);
    resolvedEye+=supportUp*std::max(0.f,clearance-glm::dot(resolvedEye-support,supportUp));
    resolvedEye=target+(resolvedEye-target)*collision.cameraFraction(target,resolvedEye);
    }
    const auto renderedPosition=glm::vec3(sceneModelTransform*glm::vec4(m_position,1));
    auto direction=preferOutwardCamera?target-resolvedEye:target-eye;
    if(const float first=firstPersonAmount();first>0){
        // First person: from behind the head to just in front of the face, eyes level,
        // so whoever stands in front is seen face to face.
        const auto face=m_position+up*(m_height*.93f)+viewHeading*(m_height*.10f);
        // Looking up and down (right mouse): the orbit pitch moves the view about its default.
        const float lookDown=std::clamp(firstPersonPitch+glm::degrees(m_pitch-m_topDownPitch),-75.f,75.f);
        const auto ahead=glm::normalize(viewHeading*std::cos(glm::radians(lookDown))-up*std::sin(glm::radians(lookDown)));
        const auto look=glm::normalize(glm::normalize(direction)*(1-first)+ahead*first);
        resolvedEye+=(face-resolvedEye)*first;
        direction=look*std::max(glm::length(direction),m_height);
    }
    resolvedEye=glm::vec3(sceneModelTransform*glm::vec4(resolvedEye,1));
    const auto renderedDirection=glm::vec3(sceneModelTransform*glm::vec4(direction,0));
    camera.setPosition(resolvedEye);
    if(std::abs(glm::dot(glm::normalize(direction),up))>.999f)
        camera.lookAt(resolvedEye+renderedDirection,viewHeading);
    else camera.lookAt(resolvedEye+renderedDirection,up);
    camera.setCharacterPosition(renderedPosition);
}
void AnimatedCharacter::preview(int gait, float distance, float time)
{
    m_cameraDistance=-1.f;
    m_lastTopDown = topDown;
    m_pitch = m_targetPitch = topDown ? m_topDownPitch : .55f;
    m_gait = std::clamp(gait, 0, 3);
    m_clip = m_gait >= 2 ? 2 : 1;
    m_phase = std::fmod(time / m_levels[0].clips[m_clip].duration, 1.f);
    m_blend = 1;
    m_speed =
        m_scale * (m_gait >= 2 ? m_run * (m_gait == 3 ? 1.5f : 1.f) : m_walk * (m_gait == 1 ? 1.8f : 1));
    m_distance = distance;
}
int AnimatedCharacter::pickEquipment(const Camera &camera, float x, float y) const
{
    if(!m_loaded || !outfit.worn())return -1;
    auto up=glm::normalize(m_position),right=glm::normalize(glm::cross(up,m_heading));
    glm::mat4 model(1);
    model[0]=glm::vec4(right*m_scale,0);model[1]=glm::vec4(up*m_scale,0);
    model[2]=glm::vec4(m_heading*m_scale,0);model[3]=glm::vec4(m_position,1);
    auto inverse=glm::inverse(camera.projectionMatrix()*camera.viewMatrix()*sceneModelTransform*model);
    auto a=inverse*glm::vec4(x,y,-1,1),b=inverse*glm::vec4(x,y,1,1);
    return outfit.pick(glm::vec3(a)/a.w,glm::normalize(glm::vec3(b)/b.w-glm::vec3(a)/a.w));
}
void AnimatedCharacter::sample(const Level &l, int clip, float phase, std::vector<float> &out) const
{
    const auto &c = l.clips[clip];
    float t = phase * (c.frames - 1);
    unsigned a = static_cast<unsigned>(t), b = std::min(a + 1, c.frames - 1);
    float blend = t - a;
    size_t count = size_t(l.vertices) * 6;
    out.resize(count);
    for (size_t k = 0; k < count; ++k)
        out[k] = c.poses[a * count + k] * (1 - blend) + c.poses[b * count + k] * blend;
}
void AnimatedCharacter::draw(const Camera &camera, int viewportHeight,bool portrait)
{
    if (!m_loaded)
        return;
    const int clip=portrait?0:m_clip,oldClip=portrait?0:m_oldClip;
    const float phase=portrait?0:m_phase,oldPhase=portrait?0:m_oldPhase,blend=portrait?1:m_blend;
    const float pixels = m_height * (portrait?1.f:glm::length(glm::vec3(sceneModelTransform[0]))) * viewportHeight * camera.projectionMatrix()[1][1] /
                         (2 * std::max(.01f, glm::distance(camera.position(), m_position)));
    // Hysteresis keeps detail stable at the two switch distances.
    if (!portrait && m_lod == 0 && pixels < 130)
        m_lod = 1;
    if (!portrait && m_lod == 1 && pixels > 155)
        m_lod = 0;
    if (!portrait && m_lod == 1 && pixels < 48)
        m_lod = 2;
    if (!portrait && m_lod == 2 && pixels > 60)
        m_lod = 1;
    const int lod=portrait?0:m_lod;
    auto &l = m_levels[lod];
    if (!outfit.worn())
    {
        std::vector<float> pose;
        sample(l, clip, phase, pose);
        if (blend < 1)
        {
            std::vector<float> old;
            sample(l, oldClip, oldPhase, old);
            for (size_t i = 0; i < pose.size(); ++i)
                pose[i] = old[i] * (1 - blend) + pose[i] * blend;
        }
        m_stream.resize(size_t(l.vertices) * 8);
        for (unsigned i = 0; i < l.vertices; ++i)
        {
            std::copy_n(pose.data() + i * 6, 6, m_stream.data() + i * 8);
            m_stream[i * 8 + 6] = l.uv[i * 2];
            m_stream[i * 8 + 7] = l.uv[i * 2 + 1];
        }
    }
    auto up = glm::normalize(m_position), right = glm::normalize(glm::cross(up, m_heading));
    glm::mat4 model(1);
    model[0] = glm::vec4(right * m_scale, 0);
    model[1] = glm::vec4(up * m_scale, 0);
    model[2] = glm::vec4(m_heading * m_scale, 0);
    model[3] = glm::vec4(m_position, 1);
    if(!portrait)model=sceneModelTransform*model;
    const bool culling = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);
    m_shader.bind();
    m_shader.setMat4("model", model);
    m_shader.setMat4("view", camera.viewMatrix());
    m_shader.setMat4("projection", camera.projectionMatrix());
    m_shader.setInt("albedo", 0);
    m_shader.setInt("arrow", 0);
    m_shader.setVec3("colorFactor", {1, 1, 1});
    glActiveTexture(GL_TEXTURE0);
    if (outfit.worn())
        outfit.draw(m_shader, lod, clip, phase, oldClip, oldPhase, blend,!portrait);
    else
    {
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glBindVertexArray(l.vao);
        glBindBuffer(GL_ARRAY_BUFFER, l.vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_stream.size() * sizeof(float), m_stream.data());
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(l.indices.size()), GL_UNSIGNED_INT, nullptr);
    }
    if (!portrait && m_speed > .0001f)
    {
        // Same width, four shaft lengths. +Z points away from the character.
        float start = .85f,
              len = m_gait == 0   ? .20f
                    : m_gait == 1 ? .43f
                    : m_gait == 2 ? .68f
                                  : .95f,
              w = .075f, shaft = .028f, tip = start + len;
        glm::vec2 p[] = {{-shaft, start}, {shaft, start},   {shaft, tip - .12f}, {w, tip - .12f},
                         {0, tip},        {-w, tip - .12f}, {-shaft, tip - .12f}};
        int triangles[] = {0, 1, 2, 0, 2, 6, 6, 2, 3, 6, 3, 4, 6, 4, 5};
        std::vector<float> arrow;
        for (auto i : triangles)
        {
            arrow.insert(arrow.end(), {p[i].x, .012f, p[i].y, 0, 1, 0, 0, 0});
        }
        m_shader.setInt("arrow", 1);
        glBindVertexArray(m_arrowVao);
        glBindBuffer(GL_ARRAY_BUFFER, m_arrowVbo);
        glBufferData(GL_ARRAY_BUFFER, arrow.size() * sizeof(float), arrow.data(), GL_STREAM_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, 15);
    }
    glBindVertexArray(0);
    if (culling)
        glEnable(GL_CULL_FACE);
}
} // namespace ow3d
