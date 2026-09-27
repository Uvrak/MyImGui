#include "U7Data.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace U7 {
namespace {

std::filesystem::path original(const std::filesystem::path& directory, const std::string& name) {
    auto untouched = directory / (name + ".u7org");
    if (std::filesystem::exists(untouched)) return untouched;
    auto path = directory / name;
    if (!std::filesystem::exists(path)) throw std::runtime_error("U7 file missing: " + path.string());
    return path;
}

std::vector<std::uint8_t> readFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open " + path.string());
    return {std::istreambuf_iterator<char>(in), {}};
}

std::uint32_t u32(const std::vector<std::uint8_t>& data, size_t at) {
    if (at + 4 > data.size()) throw std::runtime_error("Truncated U7 file");
    return data[at] | data[at + 1] << 8 | data[at + 2] << 16 | std::uint32_t(data[at + 3]) << 24;
}
int s16(const std::uint8_t* p) { return std::int16_t(p[0] | p[1] << 8); }

// Flex archive: 80-byte title, magic 0xFFFF1A00 and entry count at 0x50, entry table at 0x80.
struct Flex {
    std::vector<std::uint8_t> data;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> entries;   // offset, size
    explicit Flex(const std::filesystem::path& path) : data(readFile(path)) {
        if (u32(data, 0x50) != 0xFFFF1A00u) throw std::runtime_error("Not a U7 flex file: " + path.string());
        const auto count = u32(data, 0x54);
        if (count > 100000) throw std::runtime_error("Invalid flex entry count: " + path.string());
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto offset = u32(data, 0x80 + 8 * i), size = u32(data, 0x84 + 8 * i);
            if (size && std::uint64_t(offset) + size > data.size()) throw std::runtime_error("Flex entry out of range: " + path.string());
            entries.emplace_back(offset, size);
        }
    }
};

}

void Data::load(const std::filesystem::path& directory) {
    paletteFile = original(directory, "PALETTES.FLX");
    shapesFile = original(directory, "SHAPES.VGA");
    chunksFile = original(directory, "U7CHUNKS");
    mapFile = original(directory, "U7MAP");
    textFile = original(directory, "TEXT.FLX");
    tfaFile = original(directory, "TFA.DAT");
    const auto tfa = readFile(tfaFile);
    m_classes.assign(tfa.size() / 3, 0);
    m_sizes.assign(m_classes.size(), Size{});
    for (size_t i = 0; i < m_classes.size(); ++i) {
        m_classes[i] = tfa[i * 3 + 1] & 15;
        m_sizes[i] = {1 + (tfa[i * 3 + 2] & 7), 1 + (tfa[i * 3 + 2] >> 3 & 7), tfa[i * 3] >> 5};
    }

    Flex palettes(paletteFile);
    if (palettes.entries.empty() || palettes.entries[0].second < 768) throw std::runtime_error("Palette missing");
    const auto* p = palettes.data.data() + palettes.entries[0].first;
    auto expand = [](std::uint8_t v) { return std::uint8_t((v & 63) << 2 | (v & 63) >> 4); };   // 6 -> 8 bit
    for (int i = 0; i < 256; ++i) m_palette[i] = {expand(p[3 * i]), expand(p[3 * i + 1]), expand(p[3 * i + 2])};

    Flex shapes(shapesFile);
    if (shapes.entries.size() < FlatShapes) throw std::runtime_error("SHAPES.VGA has too few shapes");
    m_shapes = std::move(shapes.data);
    m_shapeEntries = std::move(shapes.entries);

    Flex text(textFile);
    m_names.assign(m_shapeEntries.size(), {});
    for (size_t i = 0; i < m_names.size() && i < text.entries.size(); ++i) {
        const auto [offset, size] = text.entries[i];
        std::string name(reinterpret_cast<const char*>(text.data.data() + offset), size);
        m_names[i] = name.substr(0, name.find('\0'));
    }

    m_chunks = readFile(chunksFile);
    if (m_chunks.size() % 512) throw std::runtime_error("U7CHUNKS size is not a multiple of 512");

    const auto map = readFile(mapFile);
    if (map.size() != 144 * 256 * 2) throw std::runtime_error("U7MAP has an unexpected size");
    m_map.assign(WorldChunks * WorldChunks, 0);
    for (int super = 0; super < 144; ++super)
        for (int i = 0; i < 256; ++i) {
            const int cx = super % 12 * 16 + i % 16, cy = super / 12 * 16 + i / 16;
            const size_t at = (size_t(super) * 256 + i) * 2;
            m_map[size_t(cy) * WorldChunks + cx] = std::uint16_t(map[at] | map[at + 1] << 8);
        }
    readObjects(directory);
}

void Data::readObjects(const std::filesystem::path& directory) {
    m_objects.clear();
    // Terrain objects standing directly in the chunks.
    for (int y = 0; y < WorldTiles; ++y)
        for (int x = 0; x < WorldTiles; ++x) {
            const auto t = tile(x, y);
            if (t.shape >= FlatShapes) m_objects.push_back({x, y, 0, t.shape, t.frame, Source::Chunk});
        }
    // Fixed objects, one flex per superchunk.
    for (int super = 0; super < 144; ++super) {
        char name[16];
        std::snprintf(name, sizeof name, "U7IFIX%02X", super);
        const auto path = directory / name;
        if (!std::filesystem::exists(path)) continue;
        Flex fixed(path);
        for (size_t chunk = 0; chunk < fixed.entries.size() && chunk < 256; ++chunk) {
            const auto [offset, size] = fixed.entries[chunk];
            for (std::uint32_t k = 0; k + 4 <= size; k += 4) {
                const auto* e = fixed.data.data() + offset + k;
                const int x = super % 12 * 256 + int(chunk % 16) * 16 + (e[0] >> 4 & 15);
                const int y = super / 12 * 256 + int(chunk / 16) * 16 + (e[0] & 15);
                m_objects.push_back({x, y, e[1] & 15, std::uint16_t(e[2] | (e[3] & 3) << 8), std::uint8_t(e[3] >> 2), Source::Fixed});
            }
        }
    }
    // Movable objects of the new game: u7iregnn entries of INITGAME.DAT (13-byte name first).
    const auto initPath = directory / "INITGAME.DAT";
    if (!std::filesystem::exists(initPath)) return;
    Flex init(initPath);
    for (const auto& [offset, size] : init.entries) {
        if (size < 13) continue;
        std::string entryName(reinterpret_cast<const char*>(init.data.data() + offset), 13);
        entryName = entryName.substr(0, entryName.find('\0'));
        for (auto& c : entryName) c = char(std::tolower(static_cast<unsigned char>(c)));
        if (entryName.rfind("u7ireg", 0) != 0) continue;
        const int super = std::stoi(entryName.substr(6), nullptr, 16);
        size_t p = offset + 13;
        const size_t end = offset + size;
        int depth = 0;                                   // > 0 inside a container's contents
        while (p < end) {
            const int length = init.data[p++];
            if (length == 0) continue;
            if (length == 1) { if (depth > 0) --depth; continue; }
            if (length >= 254 || p + length > end) break;
            const auto* e = init.data.data() + p;
            p += length;
            if (length < 6) continue;
            const int x = super % 12 * 256 + (e[0] >> 4) * 16 + (e[0] & 15);
            const int y = super / 12 * 256 + (e[1] >> 4) * 16 + (e[1] & 15);
            const int lift = length == 6 ? e[4] >> 4 : (length >= 10 ? e[9] >> 4 : 0);
            if (depth == 0)
                m_objects.push_back({x, y, lift, std::uint16_t(e[2] | (e[3] & 3) << 8), std::uint8_t(e[3] >> 2), Source::Movable});
            // A container (12 bytes, class 6) with a type word lists its contents next, up to a 1.
            if (length == 12 && shapeClass(e[2] | (e[3] & 3) << 8) == 6 && (e[4] | e[5] << 8) != 0) ++depth;
        }
    }
}

int Data::flatFrameCount(int shape) const {
    return shape >= 0 && shape < FlatShapes && shape < int(m_shapeEntries.size()) ? int(m_shapeEntries[shape].second / 64) : 0;
}

const std::uint8_t* Data::flatFrame(int shape, int frame) const {
    if (frame < 0 || frame >= flatFrameCount(shape)) return nullptr;
    return m_shapes.data() + m_shapeEntries[shape].first + size_t(frame) * 64;
}

Frame Data::frame(int shape, int frameIndex) const {
    Frame result;
    if (shape < 0 || shape >= int(m_shapeEntries.size())) return result;
    if (shape < FlatShapes) {
        const auto* flat = flatFrame(shape, frameIndex);
        if (!flat) return result;
        result.width = result.height = TilePixels;
        result.hotX = result.hotY = TilePixels - 1;
        result.rgba.resize(TilePixels * TilePixels * 4);
        for (int i = 0; i < TilePixels * TilePixels; ++i) {
            const auto c = m_palette[flat[i]];
            result.rgba[i * 4] = c.r; result.rgba[i * 4 + 1] = c.g; result.rgba[i * 4 + 2] = c.b; result.rgba[i * 4 + 3] = 255;
        }
        return result;
    }
    const auto [offset, size] = m_shapeEntries[shape];
    if (size < 8) return result;
    const auto* base = m_shapes.data() + offset;
    auto read32 = [&](size_t at) { return at + 4 <= size ? std::uint32_t(base[at] | base[at + 1] << 8 | base[at + 2] << 16 | base[at + 3] << 24) : 0u; };
    const auto first = read32(4);
    const int frames = first >= 8 ? int((first - 4) / 4) : 0;
    if (frameIndex < 0 || frameIndex >= frames) return result;
    const auto at = read32(4 + 4 * size_t(frameIndex));
    if (at + 8 > size) return result;
    const int right = s16(base + at), left = s16(base + at + 2), above = s16(base + at + 4), below = s16(base + at + 6);
    // The hot spot may lie outside the image (negative right or below, e.g. fences).
    if (left + right + 1 <= 0 || above + below + 1 <= 0 || left + right > 2048 || above + below > 2048) return result;
    result.width = left + right + 1;
    result.height = above + below + 1;
    result.hotX = left;
    result.hotY = above;
    result.rgba.assign(size_t(result.width) * result.height * 4, 0);
    auto put = [&](int px, int py, std::uint8_t index) {
        if (px < 0 || py < 0 || px >= result.width || py >= result.height) return;
        const auto c = m_palette[index];
        auto* out = result.rgba.data() + (size_t(py) * result.width + px) * 4;
        out[0] = c.r; out[1] = c.g; out[2] = c.b; out[3] = 255;
    };
    size_t p = at + 8;
    while (p + 2 <= size) {
        int length = base[p] | base[p + 1] << 8;
        p += 2;
        if (length == 0) break;
        const bool encoded = length & 1;
        length >>= 1;
        if (p + 4 > size) break;
        const int x = s16(base + p) + left, y = s16(base + p + 2) + above;
        p += 4;
        if (!encoded) {
            for (int k = 0; k < length && p < size; ++k) put(x + k, y, base[p++]);
            continue;
        }
        for (int done = 0; done < length && p < size;) {
            int count = base[p++];
            const bool repeat = count & 1;
            count >>= 1;
            if (repeat) {
                if (p >= size) break;
                const auto index = base[p++];
                for (int k = 0; k < count; ++k) put(x + done + k, y, index);
            } else {
                for (int k = 0; k < count && p < size; ++k) put(x + done + k, y, base[p++]);
            }
            done += count;
        }
    }
    return result;
}

const std::string& Data::name(int shape) const {
    static const std::string none;
    return shape >= 0 && shape < int(m_names.size()) ? m_names[shape] : none;
}

TileRef Data::tile(int x, int y) const {
    if (x < 0 || y < 0 || x >= WorldTiles || y >= WorldTiles) throw std::out_of_range("U7 tile position");
    const auto chunk = m_map[size_t(y / ChunkTiles) * WorldChunks + x / ChunkTiles];
    const size_t at = size_t(chunk) * 512 + ((y % ChunkTiles) * ChunkTiles + x % ChunkTiles) * 2;
    if (at + 1 >= m_chunks.size()) return {};
    const auto b0 = m_chunks[at], b1 = m_chunks[at + 1];
    return {std::uint16_t(b0 | (b1 & 3) << 8), std::uint8_t(b1 >> 2 & 31)};
}

}
