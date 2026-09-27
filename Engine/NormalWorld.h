#pragma once

#include "IWorld.h"

namespace ow3d
{
    class NormalWorld : public IWorld
    {
    public:
        void update(float deltaTime) override;
        void render(Renderer& renderer) override;
    };
}