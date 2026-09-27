#pragma once
#include "Shader.h"
#include <filesystem>
#include <memory>
#include <glm/glm.hpp>
namespace ow3d
{
class CharacterOutfit
{
    struct Data;
    std::unique_ptr<Data> data;

  public:
    CharacterOutfit();
    ~CharacterOutfit();
    void load(const std::filesystem::path &folder);
    void destroy();
    void setWorn(unsigned mask);
    unsigned worn() const;
    int pick(glm::vec3 origin, glm::vec3 direction) const;
    void draw(Shader &shader, int lod, int clip, float phase, int oldClip, float oldPhase, float blend,bool recordPicking=true);
};
} // namespace ow3d
