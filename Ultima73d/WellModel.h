#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

// Village well for Britannia3d in the style of U7's well (shapes 470 and 740): a round wall of
// rubble stone with a dressed rim, dark water deep inside, and over it a wooden windlass on two
// posts with a crank, a rope and a bucket. Metres, centre of the well at the origin, y up; the
// windlass runs along x.
namespace WellModel {

struct Vertex { glm::vec3 position, normal; glm::vec2 uv; };

struct Parts {
    std::vector<Vertex> stone;     // the round wall (stone material, mapped in metres)
    std::vector<Vertex> rim;       // dressed rim stones
    std::vector<Vertex> wood;      // posts, windlass, bucket
    std::vector<Vertex> iron;      // crank, bucket bands
    std::vector<Vertex> rope;
    std::vector<Vertex> water;
};

Parts build();

}
