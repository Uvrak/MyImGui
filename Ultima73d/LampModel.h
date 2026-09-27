#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

// Street lamp for Britannia3d in the style of U7's lamp post (shape 889): a turned, tapered
// post on a flared foot, a crossbar under the head, and a lantern with an iron frame, four
// glowing panes and a rounded cap. Metres, foot at the origin, y up.
namespace LampModel {

struct Vertex { glm::vec3 position, normal; glm::vec2 uv; };

struct Parts {
    std::vector<Vertex> post;      // wood-brown post, foot, crossbar and cap (U7's pinkish brown)
    std::vector<Vertex> iron;      // lantern frame and ring
    std::vector<Vertex> glass;     // glowing panes
    float height = 0;
};

Parts build();

// Textures (RGBA, square): the post's worn paint along its length; the lit lantern pane.
std::vector<std::uint8_t> postTexture(int size);
std::vector<std::uint8_t> glassTexture(int size);

}
