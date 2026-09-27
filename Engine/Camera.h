#pragma once
#include "WorldSettings.h"

#include <glm/glm.hpp>

namespace ow3d
{
    class Camera
    {
    public:
        Camera();

        void setPosition(const glm::vec3& position);
        void setPerspective(
            float fovDegrees,
            float aspectRatio,
            float nearPlane,
            float farPlane
        );

        glm::mat4 viewMatrix() const;
        const glm::mat4& projectionMatrix() const;

        void panPixels(float dx, float dy, float viewportHeight);
        void moveForward(float amount);
        void walkForward(float amount);
        void moveRight(float amount);
        void moveUp(float amount);
        void rotateYaw(float degrees);
        void rotatePitch(float degrees);

        void lookAt(const glm::vec3& target);
        void lookAt(const glm::vec3& target, const glm::vec3& up);
        void setTopView(float tiltDegrees = 5.0f);
        void showCharacterTopView(float heightAboveGround);
        const glm::vec3& characterPosition() const { return m_characterPosition; }
        void zoom(float wheelSteps);
        void zoomFree(float wheelSteps);
        float minimumFieldOfView = 20.f;
        void setCharacterPosition(const glm::vec3& p) { m_characterPosition = p; }
        float fieldOfView() const { return m_fovDegrees; }
        void setFieldOfView(float degrees) { setPerspective(glm::clamp(degrees,minimumFieldOfView,90.f),m_aspectRatio,m_nearPlane,m_farPlane); }

        const glm::vec3& position() const;

        void wrapPosition(float worldSize);

    private:
        void updateForward();
        float m_pitch = 0.0f;

        float m_yaw = -90.0f;
        glm::vec3 m_characterPosition{0.0f, 0.0f, PlanetRadius};
        bool m_characterOverview = false;
        glm::vec3 m_position{
            0.0f,
            0.0f,
            PlanetRadius + 2.0f
        };
        glm::vec3 m_forward{ 0.0f, 0.0f, -1.0f };
        glm::vec3 m_up{ 0.0f, 1.0f, 0.0f };

        glm::mat4 m_projection{ 1.0f };
        float m_fovDegrees = 60.0f;
        float m_aspectRatio = 1280.0f / 720.0f;
        float m_nearPlane = 0.1f;
        float m_farPlane = 1000.0f;

    };
}
