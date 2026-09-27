#pragma once
#include "WorldSettings.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <stdexcept>
#include <vector>
namespace ow3d
{
// Grounded circular character against the rendered wall surfaces, in the house's tangent plane.
class BuildingCollision
{
    struct Face
    {
        glm::vec2 a, b;
        float low, high;
    };
    glm::vec3 origin{0};
    glm::mat3 basis{1};
    float floor = 0;
    float bodyRadius = .024f, bodyHeight = .157f;
    glm::mat2 footprintMetric{1}, footprintInverse{1};
    std::vector<glm::vec2> outline;
    std::vector<Face> faces;
    std::vector<glm::vec3> supportTriangles;
    size_t fixedFaces = 0;
    bool inside(glm::vec2 p) const
    {
        bool result = false;
        for (size_t i = 0, j = outline.size() - 1; i < outline.size(); j = i++)
        {
            auto a = outline[i], b = outline[j];
            if ((a.y > p.y) != (b.y > p.y) && p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x)
                result = !result;
        }
        return result;
    }

  public:
    static constexpr float Radius = .024f;
    // A tangent frame for spherical terrain, built from the same visible mesh as rendering.
    void setSurfaceGeometry(glm::vec3 position, float height, const std::vector<glm::vec3>& triangles)
    {
        footprintMetric=footprintInverse=glm::mat2(1);
        origin = position;
        const auto up = glm::normalize(position);
        const auto reference = std::abs(up.z) < .9f ? glm::vec3(0,0,1) : glm::vec3(0,1,0);
        const auto right = glm::normalize(glm::cross(reference, up));
        basis = glm::mat3(right, glm::cross(up, right), up);
        outline.clear();
        supportTriangles.clear();
        bodyHeight = height;
        bodyRadius = Radius * height / .157f;
        faces = meshFaces(triangles);
        fixedFaces = faces.size();
    }
    void setFootprint(glm::vec3 heading, glm::vec2 radii)
    {
        const auto localHeading=glm::transpose(basis)*heading;
        const auto forward=glm::normalize(glm::vec2(localHeading));
        const glm::vec2 right(forward.y,-forward.x);
        footprintInverse=glm::mat2(right*radii.x/bodyRadius,forward*radii.y/bodyRadius);
        footprintMetric=glm::inverse(footprintInverse);
    }
    void load(const std::filesystem::path &path)
    {
        std::ifstream f(path);
        std::string magic;
        int version;
        size_t np = 0, nb = 0;
        if (!(f >> magic >> version) || magic != "OWCOLLISION" || version != 2)
            throw std::runtime_error("Missing house collision asset");
        for (int k = 0; k < 3; ++k)
            f >> origin[k];
        for (int c = 0; c < 3; ++c)
            for (int k = 0; k < 3; ++k)
                f >> basis[c][k];
        f >> floor >> np >> nb;
        if (np < 3 || np > 1000 || nb > 10000)
            throw std::runtime_error("Invalid house collision dimensions");
        outline.resize(np);
        faces.resize(nb);
        for (auto &p : outline)
            f >> p.x >> p.y;
        for (auto &face : faces)
            f >> face.a.x >> face.a.y >> face.b.x >> face.b.y >> face.low >> face.high;
        if (!f)
            throw std::runtime_error("Truncated house collision asset");
        fixedFaces = faces.size();
    }
    glm::vec3 local(glm::vec3 p) const
    {
        return glm::transpose(basis) * (p - origin);
    }
    glm::vec3 world(glm::vec3 p) const
    {
        return origin + basis * p;
    }
    bool contains(glm::vec3 p) const
    {
        auto q = local(p);
        return !outline.empty() && std::abs(q.z - floor) < .3f && inside(glm::vec2(q));
    }
    std::vector<Face> meshFaces(const std::vector<glm::vec3> &triangles) const
    {
        std::vector<Face> result;
        for (size_t i = 0; i + 2 < triangles.size(); i += 3)
        {
            glm::vec3 p[3] = {local(triangles[i]), local(triangles[i + 1]), local(triangles[i + 2])};
            auto n = glm::cross(p[1] - p[0], p[2] - p[0]);
            float length = glm::length(n);
            if (length < 1e-10f || std::abs(n.z / length) > .15f)
                continue;
            int a = 0, b = 1;
            float longest = 0;
            for (int x = 0; x < 3; ++x)
                for (int y = x + 1; y < 3; ++y)
                {
                    float d = glm::length(glm::vec2(p[x] - p[y]));
                    if (d > longest)
                    {
                        longest = d;
                        a = x;
                        b = y;
                    }
                }
            if (longest > 1e-6f)
                result.push_back({glm::vec2(p[a]), glm::vec2(p[b]), std::min({p[0].z, p[1].z, p[2].z}),
                                  std::max({p[0].z, p[1].z, p[2].z})});
        }
        return result;
    }
    void setMovingGeometry(const std::vector<glm::vec3> &triangles)
    {
        faces.resize(fixedFaces);
        auto moving = meshFaces(triangles);
        faces.insert(faces.end(), moving.begin(), moving.end());
    }
    void appendBlockingGeometry(const std::vector<glm::vec3>& triangles)
    {
        const auto added=meshFaces(triangles);faces.insert(faces.end(),added.begin(),added.end());
    }
    void appendSupportGeometry(const std::vector<glm::vec3>& triangles)
    {
        supportTriangles.insert(supportTriangles.end(),triangles.begin(),triangles.end());
    }
    glm::vec3 supportedPosition(glm::vec3 base) const
    {
        const glm::dvec3 start(base),up=glm::normalize(start);double highest=0;
        for(size_t i=0;i+2<supportTriangles.size();i+=3){
            const glm::dvec3 a(supportTriangles[i]),b(supportTriangles[i+1]),c(supportTriangles[i+2]);
            const auto e1=b-a,e2=c-a,h=glm::cross(up,e2);const double determinant=glm::dot(e1,h);
            if(std::abs(determinant)<1e-14)continue;
            const auto s=start-a;const double u=glm::dot(s,h)/determinant;
            if(u< -1e-6||u>1.000001)continue;
            const auto q=glm::cross(s,e1);const double v=glm::dot(up,q)/determinant;
            if(v< -1e-6||u+v>1.000001)continue;
            highest=std::max(highest,glm::dot(e2,q)/determinant);
        }
        return glm::vec3(start+up*highest);
    }
    bool touchesCharacter(const std::vector<glm::vec3> &triangles, glm::vec3 position) const
    {
        auto p = local(position);
        const auto center=footprintMetric*glm::vec2(p);
        for (auto face : meshFaces(triangles))
        {
            if (face.high <= p.z + bodyHeight * (.012f/.157f) || face.low >= p.z + bodyHeight)
                continue;
            face.a=footprintMetric*face.a;face.b=footprintMetric*face.b;
            auto edge = face.b - face.a;
            float length = glm::dot(edge, edge);
            if (length < 1e-12f)
                continue;
            auto nearest =
                face.a + edge * std::clamp(glm::dot(center - face.a, edge) / length, 0.f, 1.f);
            if (glm::distance(center, nearest) < bodyRadius - .00001f)
                return true;
        }
        return false;
    }
    glm::vec3 resolve(glm::vec3 from, glm::vec3 to) const
    {
        if (faces.empty())
            return supportTriangles.empty()?to:groundPosition(to);
        auto a = local(from), b = local(to);
        if (std::abs(a.z) > 1.f)
            return to;
        const auto start=footprintMetric*glm::vec2(a);
        glm::vec2 p=start, delta=footprintMetric*glm::vec2(b-a);
        int steps = std::max(1, int(std::ceil(glm::length(delta) / (bodyRadius * .35f))));
        for (int step = 0; step < steps; ++step)
        {
            p += delta / float(steps);
            for (int iteration = 0; iteration < 6; ++iteration)
            {
                bool changed = false;
                // The outline is NOT a collider. Only mesh faces overlapping the
                // character's actual height can block a step (never overhead lintels).
                const float foot = !outline.empty() && inside(footprintInverse*p) ? floor : a.z;
                for (auto face : faces)
                {
                    if (face.high <= foot + bodyHeight * (.012f/.157f) || face.low >= foot + bodyHeight)
                        continue;
                    face.a=footprintMetric*face.a;face.b=footprintMetric*face.b;
                    auto edge = face.b - face.a;
                    float edgeLength = glm::dot(edge, edge);
                    if (edgeLength < 1e-12f)
                        continue;
                    float t = std::clamp(glm::dot(p - face.a, edge) / edgeLength, 0.f, 1.f);
                    auto closest = face.a + edge * t;
                    auto d = p - closest;
                    float distance = glm::length(d);
                    if (distance >= bodyRadius)
                        continue;
                    if (distance > 1e-7f)
                        p += d * ((bodyRadius - distance) / distance);
                    else
                    {
                        glm::vec2 normal = glm::normalize(glm::vec2(-edge.y, edge.x));
                        if (glm::dot(start - closest, normal) < 0)
                            normal = -normal;
                        p += normal * bodyRadius;
                    }
                    changed = true;
                }
                if (!changed)
                    break;
            }
        }
        p=footprintInverse*p;
        return groundPosition(world({p.x, p.y, 0}));
    }
    // Sweep a small camera volume along the sight line. Use actual wall height,
    // so doorways and space above a wall do not become invisible camera barriers.
    float cameraFraction(glm::vec3 target,glm::vec3 eye,float radius=.015f) const
    {
        const auto a=local(target),delta=local(eye)-a;
        float limit=1.f;
        for(const auto& face:faces){
            auto edge=face.b-face.a;float length=glm::length(edge);if(length<1e-6f)continue;
            auto along=edge/length;auto normal=glm::vec2(-along.y,along.x);
            auto relative=glm::vec2(a)-face.a;
            glm::vec3 start(glm::dot(relative,along),glm::dot(relative,normal),a.z);
            glm::vec3 ray(glm::dot(glm::vec2(delta),along),glm::dot(glm::vec2(delta),normal),delta.z);
            glm::vec3 low(-radius,-radius,face.low-radius),high(length+radius,radius,face.high+radius);
            float enter=0,leave=limit;bool hit=true;
            for(int k=0;k<3;++k){
                if(std::abs(ray[k])<1e-8f){if(start[k]<low[k]||start[k]>high[k]){hit=false;break;}}
                else{float t0=(low[k]-start[k])/ray[k],t1=(high[k]-start[k])/ray[k];if(t0>t1)std::swap(t0,t1);enter=std::max(enter,t0);leave=std::min(leave,t1);if(enter>leave){hit=false;break;}}
            }
            if(hit)limit=std::min(limit,std::max(0.f,enter-.001f));
        }
        return limit;
    }
    // Query support height without applying wall collision (also used by the camera).
    glm::vec3 groundPosition(glm::vec3 position) const
    {
        if (outline.empty())
            return supportedPosition(glm::normalize(position) * PlanetRadius);
        glm::vec2 p(local(position));
        // Floor and doorstep are walkable, not wall colliders. Outside returns to spherical terrain.
        auto horizontal = world({p.x, p.y, 0});
        float radius = PlanetRadius;
        float along = glm::dot(horizontal, basis[2]);
        float ground =
            -along +
            std::sqrt(std::max(0.f, along * along + radius * radius - glm::dot(horizontal, horizontal)));
        float z = ground;
        if (inside(p))
            z = std::max(z, floor);
        // Two stone steps in the existing east entrance mesh.
        if (p.x > .0285f && p.x < .0885f && p.y > .381f && p.y < .534f)
            z = std::max(z, .012f);
        if (p.x > .0825f && p.x < .1335f && p.y > .372f && p.y < .543f)
            z = std::max(z, -.009f);
        return supportedPosition(world({p.x, p.y, z}));
    }
};
} // namespace ow3d
