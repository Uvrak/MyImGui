#pragma once

namespace ow3d
{
    class Renderer;

    class IWorld
    {
    public:
        virtual ~IWorld() = default;

        virtual void update(float deltaTime) = 0;
        virtual void render(Renderer& renderer) = 0;
    };
}