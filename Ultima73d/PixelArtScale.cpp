#include "PixelArtScale.h"
#include <algorithm>
#include <cstring>

namespace PixelArtScale {

std::vector<std::uint8_t> scale2x(const std::uint8_t* rgba, int width, int height) {
    auto at = [&](int x, int y) {
        std::uint32_t value;
        std::memcpy(&value, rgba + (size_t(std::clamp(y, 0, height - 1)) * width + std::clamp(x, 0, width - 1)) * 4, 4);
        return value;
    };
    std::vector<std::uint8_t> out(size_t(width) * height * 16);
    auto put = [&](int x, int y, std::uint32_t value) { std::memcpy(out.data() + (size_t(y) * width * 2 + x) * 4, &value, 4); };
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            const auto b = at(x, y - 1), d = at(x - 1, y), e = at(x, y), f = at(x + 1, y), h = at(x, y + 1);
            const bool edge = b != h && d != f;
            put(2 * x, 2 * y, edge && d == b ? d : e);
            put(2 * x + 1, 2 * y, edge && b == f ? f : e);
            put(2 * x, 2 * y + 1, edge && d == h ? d : e);
            put(2 * x + 1, 2 * y + 1, edge && h == f ? f : e);
        }
    return out;
}

std::vector<std::uint8_t> scale(const std::uint8_t* rgba, int width, int height, int factor) {
    std::vector<std::uint8_t> image(rgba, rgba + size_t(width) * height * 4);
    for (; factor > 1; factor /= 2, width *= 2, height *= 2) image = scale2x(image.data(), width, height);
    return image;
}

void smoothEdges(std::vector<std::uint8_t>& rgba, int width, int height, int radius) {
    const size_t count = size_t(width) * height;
    bool transparent = false;
    for (size_t i = 0; i < count && !transparent; ++i) transparent = rgba[i * 4 + 3] < 255;
    if (!transparent || radius <= 0) return;

    // Colour bleeding: each pass gives transparent texels next to coloured ones their mean colour.
    std::vector<std::uint8_t> coloured(count);
    for (size_t i = 0; i < count; ++i) coloured[i] = rgba[i * 4 + 3] >= 128;
    for (int pass = 0; pass < radius + 1; ++pass) {
        auto next = coloured;
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x) {
                const size_t i = size_t(y) * width + x;
                if (coloured[i]) continue;
                int sum[3] = {0, 0, 0}, n = 0;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx) {
                        const int nx = x + dx, ny = y + dy;
                        if (nx < 0 || ny < 0 || nx >= width || ny >= height) continue;
                        const size_t j = size_t(ny) * width + nx;
                        if (!coloured[j]) continue;
                        for (int k = 0; k < 3; ++k) sum[k] += rgba[j * 4 + k];
                        ++n;
                    }
                if (!n) continue;
                for (int k = 0; k < 3; ++k) rgba[i * 4 + k] = std::uint8_t(sum[k] / n);
                next[i] = 1;
            }
        coloured.swap(next);
    }

    // Alpha: separable box blur of the given radius.
    std::vector<int> alpha(count), blurred(count);
    for (size_t i = 0; i < count; ++i) alpha[i] = rgba[i * 4 + 3];
    const int span = 2 * radius + 1;
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            int sum = 0;
            for (int d = -radius; d <= radius; ++d) sum += alpha[size_t(y) * width + std::clamp(x + d, 0, width - 1)];
            blurred[size_t(y) * width + x] = sum;
        }
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            int sum = 0;
            for (int d = -radius; d <= radius; ++d) sum += blurred[size_t(std::clamp(y + d, 0, height - 1)) * width + x];
            rgba[(size_t(y) * width + x) * 4 + 3] = std::uint8_t(sum / (span * span));
        }
}

}
