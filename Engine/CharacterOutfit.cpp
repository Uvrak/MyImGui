#include "CharacterOutfit.h"
#include "Canegm/EquipmentInventory.h"
#include <SDL3/SDL.h>
#include <glad/gl.h>
#include <map>
#include <stdexcept>
#include <tuple>
namespace ow3d
{
namespace
{
struct Part
{
    int item = -1;
    std::vector<GltfVertex> drawn;
    GltfMesh mesh;
    unsigned vao = 0, vbo = 0, ebo = 0, texture = 0;
    glm::vec3 color{1};
};
void release(std::vector<Part> &parts)
{
    for (auto &p : parts)
    {
        glDeleteVertexArrays(1, &p.vao);
        glDeleteBuffers(1, &p.vbo);
        glDeleteBuffers(1, &p.ebo);
    }
    parts.clear();
}
uint64_t hash(const GltfImage &image)
{
    uint64_t h = 14695981039346656037ull;
    for (auto b : image.bytes)
        h = (h ^ b) * 1099511628211ull;
    return h;
}
// Cluster vertices by spatial cell, dominant bone and atlas region. Merge actual
// skin influences; dropping triangles alone would leave holes in garments.
GltfMesh simplify(const GltfMesh &src, float cell)
{
    if (cell == 0)
        return src;
    GltfMesh out = src;
    out.vertices.clear();
    out.indices.clear();
    out.influences.clear();
    using Key = std::array<int, 6>;
    std::map<Key, unsigned> clusters;
    std::vector<unsigned> remap, count;
    std::vector<std::map<unsigned, float>> weights;
    for (size_t i = 0; i < src.vertices.size(); ++i)
    {
        const auto &v = src.vertices[i];
        const auto &inf = src.influences[i];
        int dominant =
            int(inf.joints[std::max_element(inf.weights.begin(), inf.weights.end()) - inf.weights.begin()]);
        Key key{int(std::floor(v.px / cell)), int(std::floor(v.py / cell)),
                int(std::floor(v.pz / cell)), dominant,
                int(std::floor(v.u / .045f)), int(std::floor(v.v / .045f))};
        auto [it, added] = clusters.emplace(key, unsigned(out.vertices.size()));
        unsigned k = it->second;
        remap.push_back(k);
        if (added)
        {
            out.vertices.push_back({});
            count.push_back(0);
            weights.emplace_back();
        }
        auto &dst = out.vertices[k];
        dst.px += v.px;
        dst.py += v.py;
        dst.pz += v.pz;
        dst.nx += v.nx;
        dst.ny += v.ny;
        dst.nz += v.nz;
        dst.u += v.u;
        dst.v += v.v;
        ++count[k];
        for (int b = 0; b < 4; ++b)
            weights[k][inf.joints[b]] += inf.weights[b];
    }
    for (size_t k = 0; k < out.vertices.size(); ++k)
    {
        auto &v = out.vertices[k];
        float n = float(count[k]);
        v.px /= n;
        v.py /= n;
        v.pz /= n;
        v.nx /= n;
        v.ny /= n;
        v.nz /= n;
        v.u /= n;
        v.v /= n;
        std::vector<std::pair<float, unsigned>> ranked;
        for (auto [joint, w] : weights[k])
            ranked.push_back({w, joint});
        std::sort(ranked.rbegin(), ranked.rend());
        GltfInfluence inf;
        float total = 0;
        for (size_t b = 0; b < std::min(size_t(4), ranked.size()); ++b)
        {
            inf.weights[b] = ranked[b].first;
            inf.joints[b] = ranked[b].second;
            total += inf.weights[b];
        }
        for (auto &w : inf.weights)
            w /= total;
        out.influences.push_back(inf);
    }
    for (size_t i = 0; i < src.indices.size(); i += 3)
    {
        auto a = remap[src.indices[i]], b = remap[src.indices[i + 1]], c = remap[src.indices[i + 2]];
        if (a != b && a != c && b != c)
            out.indices.insert(out.indices.end(), {a, b, c});
    }
    return out;
}
} // namespace
struct CharacterOutfit::Data
{
    CanegmEquipment::Inventory inventory;
    GltfModel body;
    std::vector<Part> parts[3];
    std::map<uint64_t, unsigned> textures;
    std::filesystem::path folder;
    unsigned mask = 0;
    int clips[3]{};
    int drawnLod = 0;
    unsigned texture(const GltfImage &image)
    {
        auto h = hash(image);
        auto found = textures.find(h);
        if (found != textures.end())
            return found->second;
        auto path = folder / "equipment/textures" / (std::to_string(h) + ".bmp");
        auto bmp = SDL_LoadBMP(path.string().c_str());
        if (!bmp)
            throw std::runtime_error("Missing outfit texture: " + path.string());
        auto rgba = SDL_ConvertSurface(bmp, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(bmp);
        if (!rgba)
            throw std::runtime_error("Outfit texture conversion failed");
        unsigned t;
        glGenTextures(1, &t);
        glBindTexture(GL_TEXTURE_2D, t);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgba->w, rgba->h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     rgba->pixels);
        SDL_DestroySurface(rgba);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        textures[h] = t;
        return t;
    }
    ~Data()
    {
        for (auto &level : parts)
            release(level);
        for (auto [_, t] : textures)
            glDeleteTextures(1, &t);
    }
};
CharacterOutfit::CharacterOutfit() = default;
CharacterOutfit::~CharacterOutfit() = default;
void CharacterOutfit::destroy()
{
    data.reset();
}
void CharacterOutfit::load(const std::filesystem::path &folder)
{
    auto next = std::make_unique<Data>();
    next->folder = folder;
    if (!loadGltf(folder / "source/SirCanegm_Walk_Run.glb", next->body) ||
        !next->inventory.load(folder / "equipment"))
        throw std::runtime_error("Cannot load Sir Canegm equipment");
    if (!next->inventory.restore(folder / "equipment/fit.cfg"))
        throw std::runtime_error("Cannot load saved garment fits");
    for (auto &f : next->inventory.fits)
        f.worn = false;
    const char *names[] = {"Idle", "Walk", "Run"};
    for (int k = 0; k < 3; ++k)
    {
        next->clips[k] = -1;
        for (size_t i = 0; i < next->body.animation->clips.size(); ++i)
            if (next->body.animation->clips[i].name == names[k])
                next->clips[k] = int(i);
        if (next->clips[k] < 0)
            throw std::runtime_error("Missing outfit animation");
    }
    data = std::move(next);
}
unsigned CharacterOutfit::worn() const
{
    return data ? data->mask : 0;
}
void CharacterOutfit::setWorn(unsigned mask)
{
    if (!data)
        throw std::runtime_error("Outfit is not loaded");
    mask &= 1023u;
    if (mask == data->mask)
        return;
    auto &d = *data;
    for (int i = 0; i < 10; ++i)
        d.inventory.fits[i].worn = (mask & (1u << i)) != 0;
    if (mask)
    {
        auto model = d.inventory.compose(d.body);
        std::vector<unsigned> textures;
        for (const auto &image : model.images)
            textures.push_back(d.texture(image));
        std::vector<Part> levels[3];
        try
        {
            for (int lod = 0; lod < 3; ++lod)
                for (const auto &mesh : model.meshes)
                {
                    if (mesh.indices.empty())
                        continue;
                    Part part;
                    for(int item=0;item<CanegmEquipment::itemCount;++item)
                        if(mesh.name.rfind("equipment/"+std::string(d.inventory.items[item].id)+"/",0)==0)part.item=item;
                    part.mesh = simplify(mesh, lod == 0 ? 0.f : lod == 1 ? .018f : .038f);
                    if (part.mesh.indices.empty())
                        continue;
                    const auto &mat = model.materials.at(mesh.materialIndex);
                    part.color = {mat.baseColorFactor[0], mat.baseColorFactor[1], mat.baseColorFactor[2]};
                    if (mat.baseColorImage < 0)
                        throw std::runtime_error("Outfit material has no texture");
                    part.texture = textures.at(mat.baseColorImage);
                    glGenVertexArrays(1, &part.vao);
                    glGenBuffers(1, &part.vbo);
                    glGenBuffers(1, &part.ebo);
                    glBindVertexArray(part.vao);
                    glBindBuffer(GL_ARRAY_BUFFER, part.vbo);
                    glBufferData(GL_ARRAY_BUFFER, part.mesh.vertices.size() * sizeof(GltfVertex), nullptr,
                                 GL_STREAM_DRAW);
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, part.ebo);
                    glBufferData(GL_ELEMENT_ARRAY_BUFFER, part.mesh.indices.size() * sizeof(unsigned),
                                 part.mesh.indices.data(), GL_STATIC_DRAW);
                    for (int k = 0; k < 3; ++k)
                        glEnableVertexAttribArray(k);
                    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GltfVertex), nullptr);
                    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GltfVertex),
                                          (void *)(3 * sizeof(float)));
                    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GltfVertex),
                                          (void *)(6 * sizeof(float)));
                    levels[lod].push_back(std::move(part));
                }
        }
        catch (...)
        {
            for (auto &level : levels)
                release(level);
            throw;
        }
        for (int lod = 0; lod < 3; ++lod)
        {
            release(d.parts[lod]);
            d.parts[lod] = std::move(levels[lod]);
        }
    }
    else
        for (auto &level : d.parts)
            release(level);
    d.mask = mask;
    glBindVertexArray(0);
}
void CharacterOutfit::draw(Shader &shader, int lod, int clip, float phase, int oldClip, float oldPhase,
                           float blend,bool recordPicking)
{
    if (!data || !data->mask)
        return;
    auto &d = *data;
    const auto &animation = *d.body.animation;
    if(recordPicking)d.drawnLod=std::clamp(lod,0,2);
    int current = d.clips[clip], previous = d.clips[oldClip];
    auto pose = evaluateGltfPose(animation, current, phase * animation.clips[current].duration);
    auto palette = gltfSkinMatrices(animation, pose, 0);
    GltfPose oldPose;
    std::vector<GltfMatrix> oldPalette;
    if (blend < 1)
    {
        oldPose = evaluateGltfPose(animation, previous, oldPhase * animation.clips[previous].duration);
        oldPalette = gltfSkinMatrices(animation, oldPose, 0);
    }
    std::vector<GltfVertex> vertices, old;
    for (auto &part : d.parts[std::clamp(lod, 0, 2)])
    {
        deformGltfVertices(part.mesh, pose, palette, part.mesh.vertices, vertices);
        if (blend < 1)
        {
            deformGltfVertices(part.mesh, oldPose, oldPalette, part.mesh.vertices, old);
            for (size_t i = 0; i < vertices.size(); ++i)
            {
                auto &a = vertices[i];
                auto &b = old[i];
                a.px = b.px + (a.px - b.px) * blend;
                a.py = b.py + (a.py - b.py) * blend;
                a.pz = b.pz + (a.pz - b.pz) * blend;
                a.nx = b.nx + (a.nx - b.nx) * blend;
                a.ny = b.ny + (a.ny - b.ny) * blend;
                a.nz = b.nz + (a.nz - b.nz) * blend;
            }
        }
        shader.setVec3("colorFactor", part.color);
        glBindTexture(GL_TEXTURE_2D, part.texture);
        glBindVertexArray(part.vao);
        glBindBuffer(GL_ARRAY_BUFFER, part.vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(GltfVertex), vertices.data());
        glDrawElements(GL_TRIANGLES, GLsizei(part.mesh.indices.size()), GL_UNSIGNED_INT, nullptr);
        if(recordPicking)part.drawn.swap(vertices);
    }
    shader.setVec3("colorFactor", {1, 1, 1});
}
int CharacterOutfit::pick(glm::vec3 origin, glm::vec3 direction) const
{
    if(!data || !data->mask)return -1;
    float nearest=1e30f;int result=-1;
    for(const auto& part:data->parts[data->drawnLod]) {
        if(part.drawn.empty())continue;
        auto point=[&](unsigned i){const auto& v=part.drawn[i];return glm::vec3(v.px,v.py,v.pz);};
        // Slab test against the animated part's bounds before testing its triangles.
        glm::vec3 low(1e30f),high(-1e30f);
        for(const auto& v:part.drawn){low=glm::min(low,glm::vec3(v.px,v.py,v.pz));high=glm::max(high,glm::vec3(v.px,v.py,v.pz));}
        float enter=0.f,leave=nearest;
        for(int axis=0;axis<3 && enter<=leave;++axis){
            if(std::abs(direction[axis])<1e-12f){if(origin[axis]<low[axis] || origin[axis]>high[axis])enter=leave+1.f;continue;}
            float t0=(low[axis]-origin[axis])/direction[axis],t1=(high[axis]-origin[axis])/direction[axis];
            if(t0>t1)std::swap(t0,t1);
            enter=(std::max)(enter,t0);leave=(std::min)(leave,t1);
        }
        if(enter>leave)continue;
        for(size_t i=0;i+2<part.mesh.indices.size();i+=3) {
            auto a=point(part.mesh.indices[i]);auto e1=point(part.mesh.indices[i+1])-a;
            auto e2=point(part.mesh.indices[i+2])-a;auto h=glm::cross(direction,e2);
            float det=glm::dot(e1,h);if(std::abs(det)<1e-9f)continue;
            auto s=origin-a;float u=glm::dot(s,h)/det;if(u<0 || u>1)continue;
            auto q=glm::cross(s,e1);float v=glm::dot(direction,q)/det;if(v<0 || u+v>1)continue;
            float t=glm::dot(e2,q)/det;
            if(t>0 && t<nearest){nearest=t;result=part.item;}
        }
    }
    return result;
}
} // namespace ow3d
