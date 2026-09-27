#pragma once
#include "BuildingCollision.h"
#include "Mesh.h"
#include "OpeningMotion.h"
#include "Shader.h"
#include <filesystem>
#include <fstream>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iomanip>
#include <limits>
#include <string>
#include <vector>
namespace ow3d
{
class HouseOpenings
{
    struct Opening
    {
        std::string label;
        Mesh mesh, glass;
        std::vector<glm::vec3> vertices;
        glm::vec3 pivot, axis;
        float angle = 0;
        OpeningMotion motion;
    };
    std::vector<Opening> items;
    glm::mat4 transform(const Opening &item, float progress) const
    {
        float smooth = progress * progress * (3 - 2 * progress);
        return glm::translate(glm::mat4(1), item.pivot) *
               glm::rotate(glm::mat4(1), item.angle * smooth, item.axis) *
               glm::translate(glm::mat4(1), -item.pivot);
    }
    std::vector<glm::vec3> triangles(const Opening &item, float progress) const
    {
        auto matrix = transform(item, progress);
        std::vector<glm::vec3> out;
        out.reserve(item.vertices.size());
        for (auto v : item.vertices)
            out.push_back(glm::vec3(matrix * glm::vec4(v, 1)));
        return out;
    }

  public:
    void clear()
    {
        items.clear();
    }
    size_t size() const
    {
        return items.size();
    }
    float progress(size_t i) const
    {
        return items.at(i).motion.progress();
    }
    bool blocked(size_t i) const
    {
        return items.at(i).motion.blocked();
    }
    glm::vec3 center(size_t i) const
    {
        auto t = triangles(items.at(i), items.at(i).motion.progress());
        glm::vec3 c(0);
        for (auto p : t)
            c += p;
        return c / float(t.size());
    }
    void load(const std::filesystem::path &path)
    {
        clear();
        std::ifstream f(path);
        std::string magic;
        int version, count;
        if (!(f >> magic >> version >> count) || magic != "OWOPENINGS" || version != 1 || count < 0 ||
            count > 1000)
            throw std::runtime_error("Invalid house opening asset");
        for (int i = 0; i < count; ++i)
        {
            Opening item;
            std::string mesh;
            float legacyProgress = 0;
            f >> std::quoted(item.label) >> mesh >> item.pivot.x >> item.pivot.y >> item.pivot.z >>
                item.axis.x >> item.axis.y >> item.axis.z >> item.angle >> legacyProgress;
            // Legacy asset pose is consumed, but new instances always start open/unlocked.
            std::ifstream input(path.parent_path() / mesh);
            int mv;
            unsigned n;
            std::string mm;
            if (!(input >> mm >> mv >> n) || mm != "OW3D_MESH" || mv != 1 || !n || n > 100000 || n % 3)
                throw std::runtime_error("Invalid opening mesh");
            std::vector<float> rows(n * 9);
            for (auto &v : rows)
                input >> v;
            if (!input || !f)
                throw std::runtime_error("Truncated opening mesh");
            for (size_t k = 0; k < rows.size(); k += 9)
                item.vertices.push_back({rows[k], rows[k + 1], rows[k + 2]});
            std::vector<float> opaque,transparent;
            for(size_t k=0;k<rows.size();k+=27){
                auto& target=int(rows[k+8])==12?transparent:opaque;
                target.insert(target.end(),rows.begin()+k,rows.begin()+k+27);
            }
            if (!opaque.empty() && !item.mesh.create(opaque.data(), unsigned(opaque.size()/9), 9))
                throw std::runtime_error("Cannot create opening mesh");
            if (!transparent.empty() && !item.glass.create(transparent.data(), unsigned(transparent.size()/9), 9))
                throw std::runtime_error("Cannot create window glass mesh");
            items.push_back(std::move(item));
        }
    }
    bool toggle(size_t i) { return items.at(i).motion.toggle(); }
    bool setOpen(size_t i, bool open) { return items.at(i).motion.setOpen(open); }
    bool setLocked(size_t i, bool locked) { return items.at(i).motion.setLocked(locked); }
    bool locked(size_t i) const { return items.at(i).motion.locked(); }
    OpeningState state(size_t i) const { return items.at(i).motion.state(); }
    const char *hint(size_t i) const
    {
        const auto &item = items.at(i);
        if (item.motion.locked())
            return "Abgeschlossen";
        if (item.motion.blocked())
            return "Durch Sir Canegm blockiert - bitte etwas zur Seite gehen.";
        return item.label.rfind("door", 0) == 0
                   ? (item.motion.target() > .5f ? "Tuer schliessen (Klick)" : "Tuer oeffnen (Klick)")
                   : (item.motion.target() > .5f ? "Fenster schliessen (Klick)" : "Fenster oeffnen (Klick)");
    }
    void sync(BuildingCollision &collision) const
    {
        std::vector<glm::vec3> all;
        for (const auto &item : items)
        {
            auto part = triangles(item, item.motion.progress());
            all.insert(all.end(), part.begin(), part.end());
        }
        collision.setMovingGeometry(all);
    }
    void update(float dt, BuildingCollision &collision, const glm::vec3 *character)
    {
        for (auto &item : items)
        {
            item.motion.update(dt, [&](float next) {
                return !character || !collision.touchesCharacter(triangles(item, next), *character);
            });
        }
        sync(collision);
    }
    void draw(Shader &shader)
    {
        for (auto &item : items)
        {
            shader.setMat4("objectTransform", transform(item, item.motion.progress()));
            item.mesh.bind();
            glDrawArrays(GL_TRIANGLES, 0, item.mesh.vertexCount());
        }
        shader.setMat4("objectTransform", glm::mat4(1));
    }
    void drawGlass(Shader& shader,const glm::mat4& view) {
        std::vector<size_t> order;
        for(size_t i=0;i<items.size();++i)if(items[i].glass.vertexCount())order.push_back(i);
        std::sort(order.begin(),order.end(),[&](size_t a,size_t b){return (view*glm::vec4(center(a),1)).z<(view*glm::vec4(center(b),1)).z;});
        for(auto i:order){auto& item=items[i];shader.setMat4("objectTransform",transform(item,item.motion.progress()));item.glass.bind();glDrawArrays(GL_TRIANGLES,0,item.glass.vertexCount());}
        shader.setMat4("objectTransform",glm::mat4(1));
    }
    static float hit(glm::vec3 origin, glm::vec3 direction, const std::vector<glm::vec3> &vertices,
                     size_t count = std::numeric_limits<size_t>::max())
    {
        float best = std::numeric_limits<float>::max();
        count = std::min(count, vertices.size());
        for (size_t i = 0; i + 2 < count; i += 3)
        {
            auto a = vertices[i], e1 = vertices[i + 1] - a, e2 = vertices[i + 2] - a,
                 p = glm::cross(direction, e2);
            float det = glm::dot(e1, p);
            if (std::abs(det) < 1e-10f)
                continue;
            auto t = origin - a;
            float u = glm::dot(t, p) / det;
            if (u < 0 || u > 1)
                continue;
            auto q = glm::cross(t, e1);
            float v = glm::dot(direction, q) / det;
            if (v < 0 || u + v > 1)
                continue;
            float distance = glm::dot(e2, q) / det;
            if (distance > 0)
                best = std::min(best, distance);
        }
        return best;
    }
    int pick(glm::vec3 origin, glm::vec3 direction, float limit) const
    {
        int selected = -1;
        for (size_t i = 0; i < items.size(); ++i)
        {
            float distance = hit(origin, direction, triangles(items[i], items[i].motion.progress()));
            if (distance < limit)
            {
                limit = distance;
                selected = int(i);
            }
        }
        return selected;
    }
};
} // namespace ow3d
