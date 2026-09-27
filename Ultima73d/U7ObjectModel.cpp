#include "U7ObjectModel.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace {

constexpr float LiftMetres = 0.5f;
constexpr int P = U7::TilePixels;

// Share of the opaque frame pixels that the object's box covers in the U7 projection.
float boxCoverage(const U7::Frame& frame, const U7::Data::Size& size) {
    const float h = size.z * LiftMetres, tolerance = 0.75f / P;
    int opaque = 0, inside = 0;
    for (int v = 0; v < frame.height; ++v)
        for (int u = 0; u < frame.width; ++u) {
            if (!frame.rgba[(size_t(v) * frame.width + u) * 4 + 3]) continue;
            ++opaque;
            // Pixel centre relative to the anchor corner: a = dx - lift, b = dy - lift (tiles).
            const float a = (u + 0.5f - frame.hotX - 1) / P, b = (v + 0.5f - frame.hotY - 1) / P;
            const float low = std::max({0.f, -size.x - a, -size.y - b}), high = std::min({h, -a, -b});
            if (low <= high + tolerance) ++inside;
        }
    return opaque ? float(inside) / opaque : 1.f;
}

void addQuad(std::vector<U7ObjectModel::Vertex>& out, const glm::vec3 (&p)[4], const glm::vec2 (&uv)[4], const glm::vec3& normal) {
    for (int i : {0, 1, 2, 0, 2, 3}) out.push_back({p[i], normal, uv[i]});
}

std::string folderName(const std::string& name) {
    std::string result;
    for (unsigned char c : name) result += std::isalnum(c) ? char(std::tolower(c)) : '-';
    while (!result.empty() && result.back() == '-') result.pop_back();
    return result.empty() ? "unnamed" : result;
}

}

std::filesystem::path U7ObjectModel::file(const std::filesystem::path& assetDirectory, const U7::Data& data, int shape, int frame) {
    char folder[16], base[16];
    std::snprintf(folder, sizeof(folder), "%04d_", shape);
    std::snprintf(base, sizeof(base), "frame%02d.obj", frame);
    return assetDirectory / "Objects" / (folder + folderName(data.name(shape))) / base;
}

U7ObjectModel U7ObjectModel::generate(const U7::Data& data, int shape, int frameIndex) {
    U7ObjectModel model;
    const auto frame = data.frame(shape, frameIndex);
    if (frame.rgba.empty()) return model;
    model.textureWidth = frame.width;
    model.textureHeight = frame.height;
    model.texture = frame.rgba;

    const auto size = data.shapeSize(shape);
    const float w = float(frame.width), hgt = float(frame.height);
    // Frame coordinates of a point dx, dy (tiles, <= 0) and h (metres above the lift).
    auto uv = [&](float dx, float dy, float h) {
        return glm::vec2((frame.hotX + 1 + P * (dx - h)) / w, (frame.hotY + 1 + P * (dy - h)) / hgt);
    };
    auto at = [](float dx, float dy, float h) { return glm::vec3(dx, h, dy); };
    const float sx = float(size.x), sy = float(size.y), sz = size.z * LiftMetres;
    auto& out = model.vertices;

    if (size.z == 0) {
        // Flat object: the whole frame lies on the plane at its lift.
        model.kind = Kind::Flat;
        const float x0 = -(frame.hotX + 1) / float(P), x1 = (w - frame.hotX - 1) / P;
        const float y0 = -(frame.hotY + 1) / float(P), y1 = (hgt - frame.hotY - 1) / P;
        const float lift = 0.02f;
        addQuad(out, {at(x0, y0, lift), at(x1, y0, lift), at(x1, y1, lift), at(x0, y1, lift)},
                {uv(x0, y0, 0), uv(x1, y0, 0), uv(x1, y1, 0), uv(x0, y1, 0)}, {0, 1, 0});
        return model;
    }
    if (boxCoverage(frame, size) >= 0.85f) {
        // Top, south and east faces as U7 shows them; north and west repeat south and east.
        model.kind = Kind::Box;
        addQuad(out, {at(-sx, -sy, sz), at(0, -sy, sz), at(0, 0, sz), at(-sx, 0, sz)},
                {uv(-sx, -sy, sz), uv(0, -sy, sz), uv(0, 0, sz), uv(-sx, 0, sz)}, {0, 1, 0});
        addQuad(out, {at(-sx, 0, sz), at(0, 0, sz), at(0, 0, 0), at(-sx, 0, 0)},
                {uv(-sx, 0, sz), uv(0, 0, sz), uv(0, 0, 0), uv(-sx, 0, 0)}, {0, 0, 1});
        addQuad(out, {at(0, 0, sz), at(0, -sy, sz), at(0, -sy, 0), at(0, 0, 0)},
                {uv(0, 0, sz), uv(0, -sy, sz), uv(0, -sy, 0), uv(0, 0, 0)}, {1, 0, 0});
        addQuad(out, {at(0, -sy, sz), at(-sx, -sy, sz), at(-sx, -sy, 0), at(0, -sy, 0)},
                {uv(0, 0, sz), uv(-sx, 0, sz), uv(-sx, 0, 0), uv(0, 0, 0)}, {0, 0, -1});
        addQuad(out, {at(-sx, -sy, sz), at(-sx, 0, sz), at(-sx, 0, 0), at(-sx, -sy, 0)},
                {uv(0, -sy, sz), uv(0, 0, sz), uv(0, 0, 0), uv(0, -sy, 0)}, {-1, 0, 0});
        return model;
    }
    // Two crossed upright quads through the footprint centre. The first faces the U7 viewer
    // (south east), so the frame is unprojected onto it exactly: a frame offset (du, dv) from
    // the centre pixel is s = (du - dv) * sqrt(2) / 16 along the quad and h = -(du + dv) / 16 up.
    // The second quad, turned by 90 degrees, repeats it.
    model.kind = Kind::Upright;
    const float uc = frame.hotX + 1 - P * sx / 2, vc = frame.hotY + 1 - P * sy / 2;
    const glm::vec3 centre(-sx / 2, 0, -sy / 2);
    const glm::vec2 corners[4] = {{0, 0}, {w, 0}, {w, hgt}, {0, hgt}};
    const glm::vec2 uvs[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    for (const glm::vec3 axis : {glm::vec3(0.7071f, 0, -0.7071f), glm::vec3(0.7071f, 0, 0.7071f)}) {
        glm::vec3 p[4];
        for (int i = 0; i < 4; ++i) {
            const float du = corners[i].x - uc, dv = corners[i].y - vc;
            p[i] = centre + axis * ((du - dv) * 1.41421356f / 16.f) + glm::vec3(0, -(du + dv) / 16.f, 0);
        }
        addQuad(out, p, uvs, glm::cross(axis, glm::vec3(0, 1, 0)));
    }
    return model;
}

bool U7ObjectModel::save(const std::filesystem::path& objFile) const {
    if (empty()) return false;
    std::error_code error;
    std::filesystem::create_directories(objFile.parent_path(), error);
    auto png = objFile, mtl = objFile;
    png.replace_extension(".png");
    mtl.replace_extension(".mtl");

    auto* image = SDL_CreateSurfaceFrom(textureWidth, textureHeight, SDL_PIXELFORMAT_RGBA32,
                                        const_cast<std::uint8_t*>(texture.data()), textureWidth * 4);
    const bool saved = image && IMG_SavePNG(image, png.string().c_str());
    SDL_DestroySurface(image);
    if (!saved) return false;

    std::ofstream material(mtl);
    material << "newmtl u7\nKd 1 1 1\nmap_Kd " << png.filename().string() << '\n';
    std::ofstream obj(objFile);
    const char* kinds[] = {"flat", "box", "upright", "file"};
    obj << "# Ultima73d object model, kind " << kinds[int(kind)] << "\n"
        << "# metres, Y up, Z south; origin = south east bottom corner of the footprint\n"
        << "mtllib " << mtl.filename().string() << "\nusemtl u7\n";
    for (const auto& v : vertices) obj << "v " << v.position.x << ' ' << v.position.y << ' ' << v.position.z << '\n';
    for (const auto& v : vertices) obj << "vt " << v.uv.x << ' ' << 1.f - v.uv.y << '\n';
    for (const auto& v : vertices) obj << "vn " << v.normal.x << ' ' << v.normal.y << ' ' << v.normal.z << '\n';
    for (size_t i = 0; i < vertices.size(); i += 3)
        obj << "f " << i + 1 << '/' << i + 1 << '/' << i + 1 << ' ' << i + 2 << '/' << i + 2 << '/' << i + 2 << ' '
            << i + 3 << '/' << i + 3 << '/' << i + 3 << '\n';
    return bool(obj);
}

U7ObjectModel U7ObjectModel::load(const std::filesystem::path& objFile) {
    U7ObjectModel model;
    std::ifstream in(objFile);
    if (!in) return model;
    std::vector<glm::vec3> positions, normals;
    std::vector<glm::vec2> uvs;
    std::filesystem::path textureFile = objFile;
    textureFile.replace_extension(".png");
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream words(line);
        std::string key;
        words >> key;
        if (key == "#" && line.find("kind ") != std::string::npos) {
            const auto kind = line.substr(line.find("kind ") + 5);
            model.kind = kind == "flat" ? Kind::Flat : kind == "box" ? Kind::Box : kind == "upright" ? Kind::Upright : Kind::File;
        } else if (key == "v") {
            glm::vec3 p; words >> p.x >> p.y >> p.z; positions.push_back(p);
        } else if (key == "vt") {
            glm::vec2 t; words >> t.x >> t.y; uvs.push_back({t.x, 1.f - t.y});
        } else if (key == "vn") {
            glm::vec3 n; words >> n.x >> n.y >> n.z; normals.push_back(n);
        } else if (key == "mtllib") {
            std::string name; std::getline(words >> std::ws, name);
            std::ifstream material(objFile.parent_path() / name);
            for (std::string m; std::getline(material, m);)
                if (m.rfind("map_Kd ", 0) == 0) textureFile = objFile.parent_path() / m.substr(7);
        } else if (key == "f") {
            // Polygons as triangle fans; indices "p", "p/t", "p//n" or "p/t/n", negative = relative.
            std::vector<Vertex> polygon;
            for (std::string corner; words >> corner;) {
                int index[3] = {0, 0, 0};
                for (int part = 0, start = 0; part < 3; ++part) {
                    const auto slash = corner.find('/', size_t(start));
                    const auto text = corner.substr(size_t(start), slash == std::string::npos ? std::string::npos : slash - start);
                    if (!text.empty()) index[part] = std::stoi(text);
                    if (slash == std::string::npos) break;
                    start = int(slash) + 1;
                }
                auto pick = [](int i, auto& list) { return i > 0 ? size_t(i - 1) : i < 0 ? list.size() + size_t(i) : list.size(); };
                Vertex v{};
                if (const auto i = pick(index[0], positions); i < positions.size()) v.position = positions[i];
                if (const auto i = pick(index[1], uvs); i < uvs.size()) v.uv = uvs[i];
                if (const auto i = pick(index[2], normals); i < normals.size()) v.normal = normals[i];
                polygon.push_back(v);
            }
            for (size_t i = 2; i < polygon.size(); ++i) {
                Vertex t[3] = {polygon[0], polygon[i - 1], polygon[i]};
                if (glm::length(t[0].normal) < 0.5f) {
                    const auto n = glm::cross(t[1].position - t[0].position, t[2].position - t[0].position);
                    for (auto& v : t) v.normal = glm::length(n) > 0 ? glm::normalize(n) : glm::vec3(0, 1, 0);
                }
                model.vertices.insert(model.vertices.end(), t, t + 3);
            }
        }
    }
    if (auto* image = IMG_Load(textureFile.string().c_str())) {
        if (auto* rgba = SDL_ConvertSurface(image, SDL_PIXELFORMAT_RGBA32)) {
            model.textureWidth = rgba->w;
            model.textureHeight = rgba->h;
            model.texture.resize(size_t(rgba->w) * rgba->h * 4);
            for (int y = 0; y < rgba->h; ++y)
                std::copy_n(static_cast<const std::uint8_t*>(rgba->pixels) + size_t(y) * rgba->pitch, size_t(rgba->w) * 4,
                            model.texture.begin() + size_t(y) * rgba->w * 4);
            SDL_DestroySurface(rgba);
        }
        SDL_DestroySurface(image);
    }
    return model;
}

U7ObjectModel U7ObjectModel::loadOrCreate(const std::filesystem::path& assetDirectory, const U7::Data& data, int shape, int frame,
                                          bool* created) {
    const auto path = file(assetDirectory, data, shape, frame);
    if (created) *created = false;
    if (std::filesystem::exists(path)) {
        auto model = load(path);
        if (!model.empty()) return model;
    }
    auto model = generate(data, shape, frame);
    if (!model.empty() && model.save(path) && created) *created = true;
    return model;
}
