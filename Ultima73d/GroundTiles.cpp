#include "GroundTiles.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace GroundTiles {
namespace {

float hash(int x, int y, unsigned seed) {
    std::uint32_t h = std::uint32_t(x) * 374761393u + std::uint32_t(y) * 668265263u + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return float((h ^ (h >> 16)) & 0xffff) / 65535.f;
}

// Value noise with the period of the tile, so neighbouring tiles join.
float noise(float x, float y, int period, unsigned seed) {
    const int ix = int(std::floor(x)), iy = int(std::floor(y));
    const float fx = x - ix, fy = y - iy, sx = fx * fx * (3 - 2 * fx), sy = fy * fy * (3 - 2 * fy);
    auto h = [&](int i, int j) { return hash(((i % period) + period) % period, ((j % period) + period) % period, seed); };
    return (h(ix, iy) * (1 - sx) + h(ix + 1, iy) * sx) * (1 - sy) + (h(ix, iy + 1) * (1 - sx) + h(ix + 1, iy + 1) * sx) * sy;
}

}

std::map<Key, Variant> loadVariants(const std::filesystem::path& file) {
    std::map<Key, Variant> variants;
    std::ifstream in(file);
    for (std::string line; std::getline(in, line);) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream fields(line);
        int shape, frame;
        Variant variant;
        if (fields >> shape >> frame >> variant.outside >> variant.inside) variants[{shape, frame}] = variant;
    }
    return variants;
}

void saveVariants(const std::filesystem::path& file, const std::map<Key, Variant>& variants) {
    std::error_code error;
    std::filesystem::create_directories(file.parent_path(), error);
    std::ofstream out(file);
    out << "# U7 ground tile -> its 3D variant in Britannia3d, and its high resolution tile.\n"
           "# outside: cobble, cobble-dirt, grass, grass-mud, tile   inside (under roofs): boards, flagstones, bricks, carpet, tile\n"
           "# shape frame outside inside file\n";
    for (const auto& [key, variant] : variants)
        out << key.first << ' ' << key.second << ' ' << variant.outside << ' ' << variant.inside << ' ' << tileFile(key) << '\n';
}

std::string tileFile(Key key) {
    return "Materials/Ground/" + std::to_string(key.first) + "-" + std::to_string(key.second) + ".png";
}

std::vector<std::uint8_t> loadOrCreate(const std::filesystem::path& assetDirectory, Key key, const std::vector<std::uint8_t>& upscaled, int size) {
    const auto path = assetDirectory / tileFile(key);
    if (auto* image = IMG_Load(path.string().c_str())) {
        auto* converted = SDL_ConvertSurface(image, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(image);
        if (converted) {
            // Any size: sampled to size x size.
            std::vector<std::uint8_t> out(size_t(size) * size * 4);
            for (int y = 0; y < size; ++y)
                for (int x = 0; x < size; ++x) {
                    const int sx = x * converted->w / size, sy = y * converted->h / size;
                    std::copy_n(static_cast<const std::uint8_t*>(converted->pixels) + size_t(sy) * converted->pitch + size_t(sx) * 4, 4,
                                out.begin() + (size_t(y) * size + x) * 4);
                }
            SDL_DestroySurface(converted);
            return out;
        }
    }
    // Made from U7's tile: its shapes stay, their surfaces get grain, mottling and speckles.
    const unsigned seed = unsigned(key.first * 32 + key.second);
    std::vector<std::uint8_t> out(upscaled);
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x) {
            auto* p = out.data() + (size_t(y) * size + x) * 4;
            const float mottle = noise(x / 16.f, y / 16.f, size / 16, seed) - 0.5f, fine = noise(x / 4.f, y / 4.f, size / 4, seed + 7) - 0.5f;
            const float grain = hash(x, y, seed + 13) - 0.5f;
            float shade = 1.f + 0.1f * mottle + 0.07f * fine + 0.05f * grain;
            if (hash(x, y, seed + 21) > 0.985f) shade *= 0.82f;       // dark specks
            else if (hash(x, y, seed + 29) > 0.99f) shade *= 1.12f;  // light specks
            for (int k = 0; k < 3; ++k) p[k] = std::uint8_t(std::clamp(p[k] * shade, 0.f, 255.f));
        }
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (auto* surface = SDL_CreateSurfaceFrom(size, size, SDL_PIXELFORMAT_RGBA32, out.data(), size * 4)) {
        IMG_SavePNG(surface, path.string().c_str());
        SDL_DestroySurface(surface);
    }
    return out;
}


std::map<Key, std::string> loadStructureVariants(const std::filesystem::path& file) {
    std::map<Key, std::string> variants;
    std::ifstream in(file);
    for (std::string line; std::getline(in, line);) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream fields(line);
        int shape, frame;
        std::string variant;
        if (fields >> shape >> frame >> variant) {
            std::replace(variant.begin(), variant.end(), '_', ' ');   // (spaces are written as _)
            variants[{shape, frame}] = variant;
        }
    }
    return variants;
}

void saveStructureVariants(const std::filesystem::path& file, const std::map<Key, std::string>& variants, const std::string& what,
                           const std::string& choices) {
    std::error_code error;
    std::filesystem::create_directories(file.parent_path(), error);
    std::ofstream out(file);
    out << "# U7 " << what << " -> how Britannia3d draws it.\n# variant: " << choices << "\n# shape frame variant file\n";
    for (const auto& [key, variant] : variants) {
        std::string word = variant;
        std::replace(word.begin(), word.end(), ' ', '_');
        out << key.first << ' ' << key.second << ' ' << word << ' ' << objectFile(key) << '\n';
    }
}

std::string objectFile(Key key) {
    return "Materials/Objects/" + std::to_string(key.first) + "-" + std::to_string(key.second) + ".png";
}

std::vector<std::uint8_t> loadOrCreatePicture(const std::filesystem::path& file, std::vector<std::uint8_t> out, int width, int height, unsigned seed) {
    if (auto* image = IMG_Load(file.string().c_str())) {
        auto* converted = SDL_ConvertSurface(image, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(image);
        if (converted) {
            std::vector<std::uint8_t> loaded(size_t(width) * height * 4);
            for (int y = 0; y < height; ++y)
                for (int x = 0; x < width; ++x) {
                    const int sx = x * converted->w / width, sy = y * converted->h / height;
                    std::copy_n(static_cast<const std::uint8_t*>(converted->pixels) + size_t(sy) * converted->pitch + size_t(sx) * 4, 4,
                                loaded.begin() + (size_t(y) * width + x) * 4);
                }
            SDL_DestroySurface(converted);
            return loaded;
        }
    }
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            auto* p = out.data() + (size_t(y) * width + x) * 4;
            if (!p[3]) continue;
            const float mottle = noise(x / 16.f, y / 16.f, 1 << 20, seed) - 0.5f, fine = noise(x / 4.f, y / 4.f, 1 << 20, seed + 7) - 0.5f;
            float shade = 1.f + 0.08f * mottle + 0.06f * fine + 0.05f * (hash(x, y, seed + 13) - 0.5f);
            if (hash(x, y, seed + 21) > 0.988f) shade *= 0.85f;
            for (int k = 0; k < 3; ++k) p[k] = std::uint8_t(std::clamp(p[k] * shade, 0.f, 255.f));
        }
    std::error_code error;
    std::filesystem::create_directories(file.parent_path(), error);
    if (auto* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, out.data(), width * 4)) {
        IMG_SavePNG(surface, file.string().c_str());
        SDL_DestroySurface(surface);
    }
    return out;
}

}
