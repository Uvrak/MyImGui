#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <algorithm>

namespace ow3d
{
    Camera::Camera() = default;

    void Camera::setPosition(const glm::vec3& position)
    {
        m_position = position;
        m_characterPosition = glm::normalize(position) * PlanetRadius;
        m_characterOverview = false;
    }

    void Camera::setPerspective(
        float fovDegrees,
        float aspectRatio,
        float nearPlane,
        float farPlane)
    {
        m_fovDegrees = fovDegrees;
        m_aspectRatio = aspectRatio;
        m_nearPlane = nearPlane;
        m_farPlane = farPlane;
        m_projection = glm::perspective(
            glm::radians(fovDegrees),
            aspectRatio,
            nearPlane,
            farPlane
        );
    }

    void Camera::zoom(float wheelSteps)
    {
        if (!std::isfinite(wheelSteps)) return;
        const float fov = glm::clamp(m_fovDegrees - wheelSteps * 3.0f, minimumFieldOfView, 90.0f);
        setPerspective(fov, m_aspectRatio, m_nearPlane, m_farPlane);
    }

    void Camera::zoomFree(float wheelSteps)
    {
        if(!std::isfinite(wheelSteps) || wheelSteps==0)return;
        const float radius=glm::length(m_position);
        const float b=glm::dot(m_position,m_forward);
        const float discriminant=b*b-radius*radius+PlanetRadius*PlanetRadius;
        float distance=discriminant>=0?-b-std::sqrt(discriminant):-1.f;
        if(distance<=.001f)distance=std::max(.02f,radius-PlanetRadius);
        const float next=std::clamp(distance*std::exp(-std::clamp(wheelSteps,-20.f,20.f)*.16f),.02f,PlanetRadius*6.f);
        auto position=m_position+m_forward*(distance-next);
        if(glm::length(position)<PlanetRadius+.003f)position=glm::normalize(position)*(PlanetRadius+.003f);
        m_position=position;
        m_characterOverview=false;
    }

    glm::mat4 Camera::viewMatrix() const
    {
        return glm::lookAt(
            m_position,
            m_position + m_forward,
            m_up
        );
    }

    const glm::mat4& Camera::projectionMatrix() const
    {
        return m_projection;
    }

    void Camera::panPixels(float dx, float dy, float viewportHeight)
    {
        if (!std::isfinite(dx) || !std::isfinite(dy) || !std::isfinite(viewportHeight) || viewportHeight<=0) return;
        const float radius=glm::length(m_position);
        if(radius<.001f)return;
        const glm::vec3 radial=m_position/radius;
        glm::vec3 right=glm::cross(m_forward,m_up);
        if(glm::length(right)<.00001f)return;
        right=glm::normalize(right);
        const auto screenUp=glm::normalize(glm::cross(right,m_forward));
        // Scale the grab to the visible ground distance, including a 90-degree view.
        const float incidence=std::max(.15f,-glm::dot(m_forward,radial));
        const float distance=std::max(.001f,radius-PlanetRadius)/incidence;
        const float scale=2.f*distance*std::tan(glm::radians(m_fovDegrees)*.5f)/viewportHeight;
        glm::vec3 shift=(-dx*right+dy*screenUp)*scale;
        shift-=radial*glm::dot(shift,radial);
        const float length=glm::length(shift);
        if(length<.0000001f)return;
        const auto rotation=glm::mat3(glm::rotate(glm::mat4(1),std::min(length/radius,.15f),glm::normalize(glm::cross(radial,shift))));
        m_position=glm::normalize(rotation*m_position)*radius;
        m_forward=glm::normalize(rotation*m_forward);
        m_up=glm::normalize(rotation*m_up);
        m_characterPosition=rotation*m_characterPosition;
    }

    void Camera::moveForward(float amount)
    {
        const float cameraRadius =
            glm::length(m_position);

        const glm::vec3 up =
            glm::normalize(m_position);

        glm::vec3 forward =
            m_forward - up * glm::dot(m_forward, up);

        if (glm::dot(forward, forward) < 1e-10f) return;
        forward = glm::normalize(forward);

        const glm::vec3 rotationAxis =
            glm::normalize(
                glm::cross(up, forward)
            );

        const float angle =
            amount / cameraRadius;

        const glm::mat4 rotation =
            glm::rotate(
                glm::mat4(1.0f),
                angle,
                rotationAxis
            );

        m_position =
            glm::vec3(
                rotation * glm::vec4(m_position, 1.0f)
            );
        m_characterPosition = glm::normalize(glm::vec3(rotation * glm::vec4(m_characterPosition, 1.0f))) * PlanetRadius;

        m_forward =
            glm::normalize(
                glm::vec3(
                    rotation * glm::vec4(m_forward, 0.0f)
                )
            );

        m_up =
            glm::normalize(m_position);
    }

    void Camera::walkForward(float amount)
    {
        if (m_characterOverview)
        {
            // Keep the overview following the grounded character while walking.
            moveForward(amount * glm::length(m_position) / PlanetRadius);
            return;
        }
        // Walking follows the spherical ground, regardless of viewing pitch.
        const float radius = PlanetRadius + WalkingEyeHeight;
        m_position = glm::normalize(m_position) * radius;
        moveForward(amount);
        // Prevent cumulative float drift during long walks around the planet.
        m_position = glm::normalize(m_position) * radius;
        m_up = glm::normalize(m_position);
    }

    void Camera::moveRight(float amount)
    {
        const float cameraRadius =
            PlanetRadius + 2.0f;

        const glm::vec3 up =
            glm::normalize(m_position);

        const glm::vec3 right =
            glm::normalize(
                glm::cross(m_forward, up)
            );

        const glm::vec3 rotationAxis =
            glm::normalize(
                glm::cross(up, right)
            );

        const float angle =
            amount / cameraRadius;

        const glm::mat4 rotation =
            glm::rotate(
                glm::mat4(1.0f),
                angle,
                rotationAxis
            );

        m_position =
            glm::vec3(
                rotation * glm::vec4(m_position, 1.0f)
            );
        m_characterPosition = glm::normalize(glm::vec3(rotation * glm::vec4(m_characterPosition, 1.0f))) * PlanetRadius;

        m_forward =
            glm::normalize(
                glm::vec3(
                    rotation * glm::vec4(m_forward, 0.0f)
                )
            );

        m_up =
            glm::normalize(m_position);
    }

    void Camera::moveUp(float amount)
    {
        float radius = glm::length(m_position);

        radius += amount;

        constexpr float minimumHeight = 0.06f;

        if (radius < PlanetRadius + minimumHeight)
            radius = PlanetRadius + minimumHeight;

        m_position =
            glm::normalize(m_position) * radius;

        m_up =
            glm::normalize(m_position);
    }

    void Camera::rotateYaw(float degrees)
    {
        const glm::mat4 rotation =
            glm::rotate(
                glm::mat4(1.0f),
                glm::radians(-degrees),
                m_up
            );

        m_forward =
            glm::normalize(
                glm::vec3(
                    rotation * glm::vec4(m_forward, 0.0f)
                )
            );
    }

    void Camera::rotatePitch(float degrees)
    {
        // Derive pitch from the actual orientation, including presets and lookAt.
        const float pitch = glm::degrees(std::asin(glm::clamp(glm::dot(m_forward, m_up), -1.0f, 1.0f)));
        const float newPitch = glm::clamp(pitch + degrees, -89.0f, 89.0f);
        degrees = newPitch - pitch;
        m_pitch = newPitch;

        const glm::vec3 right =
            glm::normalize(
                glm::cross(m_forward, m_up)
            );

        const glm::mat4 rotation =
            glm::rotate(
                glm::mat4(1.0f),
                glm::radians(degrees),
                right
            );

        m_forward =
            glm::normalize(
                glm::vec3(
                    rotation * glm::vec4(m_forward, 0.0f)
                )
            );
    }

    void Camera::lookAt(const glm::vec3& target)
{
    m_forward =
        glm::normalize(
            target - m_position
        );

    m_up =
        glm::normalize(m_position);
}

    const glm::vec3& Camera::position() const
    {
        return m_position;
    }

    void Camera::lookAt(const glm::vec3& target, const glm::vec3& up)
    {
        m_forward = glm::normalize(target - m_position);
        m_up = glm::normalize(up - m_forward * glm::dot(up, m_forward));
    }

    void Camera::setTopView(float tiltDegrees)
    {
        m_up = glm::normalize(m_position);
        glm::vec3 heading = m_forward - m_up * glm::dot(m_forward, m_up);
        if (glm::length(heading) < 0.0001f)
        {
            const glm::vec3 axis = std::abs(m_up.y) < 0.9f ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
            heading = axis - m_up * glm::dot(axis, m_up);
        }
        const float tilt = glm::radians(glm::clamp(tiltDegrees, 1.0f, 89.0f));
        m_forward = glm::normalize(-m_up * std::cos(tilt) + glm::normalize(heading) * std::sin(tilt));
        m_pitch = glm::degrees(tilt) - 90.0f;
    }

    void Camera::showCharacterTopView(float heightAboveGround)
    {
        const glm::vec3 up = glm::normalize(m_characterPosition);
        glm::vec3 heading = m_forward - up * glm::dot(m_forward, up);
        if (glm::length(heading) < 0.0001f)
        {
            const glm::vec3 axis = std::abs(up.y) < 0.9f ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
            heading = axis - up * glm::dot(axis, up);
        }
        const float height = std::max(heightAboveGround, 0.1f);
        m_position = m_characterPosition + up * height
            - glm::normalize(heading) * (height * std::tan(glm::radians(4.05f)));
        lookAt(m_characterPosition);
        m_characterOverview = true;
    }

    void Camera::wrapPosition(float worldSize)
    {
        if (m_position.x < 0.0f)
            m_position.x += worldSize;

        if (m_position.x >= worldSize)
            m_position.x -= worldSize;

        if (m_position.z < 0.0f)
            m_position.z += worldSize;

        if (m_position.z >= worldSize)
            m_position.z -= worldSize;
    }

    void Camera::updateForward()
    {
        const float yawRadians = glm::radians(m_yaw);
        const float pitchRadians = glm::radians(m_pitch);

        m_forward.x = std::cos(yawRadians) * std::cos(pitchRadians);
        m_forward.y = std::sin(pitchRadians);
        m_forward.z = std::sin(yawRadians) * std::cos(pitchRadians);

        m_forward = glm::normalize(m_forward);
    }
}
