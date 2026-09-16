#include "Camera3D.h"

namespace GridBuilder3D
{
    void Camera3D::setPosition(
        float x,
        float y,
        float z
    )
    {
        m_x = x;
        m_y = y;
        m_z = z;
    }

    float Camera3D::x() const
    {
        return m_x;
    }

    float Camera3D::y() const
    {
        return m_y;
    }

    float Camera3D::z() const
    {
        return m_z;
    }

    void Camera3D::setYaw(float yaw)
    {
        m_yaw = yaw;
    }

    float Camera3D::yaw() const
    {
        return m_yaw;
    }
}