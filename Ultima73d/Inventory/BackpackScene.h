#pragma once
#include <AnimatedCharacter.h>
#include <Camera.h>
#include <glm/glm.hpp>

// What Ultima7Remake's backpack panel needs of the scene it lives in (in Ultima7Remake the
// ow3d::Renderer): the character wearing the equipment, the camera, and the ground under a
// screen point (normalised device coordinates) in world coordinates.
struct BackpackScene {
    virtual ~BackpackScene() = default;
    virtual ow3d::AnimatedCharacter& character() = 0;
    virtual ow3d::Camera& camera() = 0;
    virtual bool pickGround(float x, float y, glm::vec3& ground) const = 0;
};
