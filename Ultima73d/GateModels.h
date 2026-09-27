#pragma once
#include <glm/glm.hpp>
#include <vector>

// High resolution models for Trinsic's town gates and wooden stairs, in U7's style:
//
//   portcullis   an iron grid (U7 271 / 272): square vertical bars ending in spikes, riveted
//                cross bars, a heavy top beam; runs along x from 0 to length, y up.
//   winch        the windlass beside the gate (U7 949 / 950): a wooden drum wound with chain on
//                two trestles, iron crank handles at both ends; drum along x, centre at 0.
//   stairStep    one wooden step block of a staircase: a plank body with a thicker tread that
//                overhangs a little, and dark nosing; footprint low .. high (x, z), y up.
//
// Metres, y up.
namespace GateModels {

struct Vertex { glm::vec3 position, normal; glm::vec2 uv; };

struct Parts {
    std::vector<Vertex> iron, wood, darkWood;
};

Parts portcullis(float length, float height);
// The standing trestles (drum = false) or the turning part (drum = true: drum, bands, chain,
// axle and cranks, turning about the x axis at WinchAxle metres).
inline constexpr float WinchAxle = 0.72f, WinchDrumRadius = 0.2f;
Parts winch(bool drum);
void stairStep(Parts& out, glm::vec3 low, glm::vec3 high);

}
