#pragma once

namespace GridBuilder3D
{
    class Camera3D
    {
    public:
        void setPosition(float x, float y, float z);

        float x() const;
        float y() const;
        float z() const;

        void setYaw(float yaw);
        float yaw() const;

    private:
        float m_x = 0.0f;
        float m_y = 0.0f;
        float m_z = 0.0f;

        float m_yaw = 0.0f;
    };
}