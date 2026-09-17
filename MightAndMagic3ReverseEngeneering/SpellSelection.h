#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <algorithm>

namespace MightAndMagic3
{
    struct SpellSelection
    {
        std::vector<int> rows;
        int readyRow = -1;
    };

    inline double spellTextSimilarity(const std::vector<int>& a, const std::vector<int>& b)
    {
        if (a.empty() || b.empty()) return 0;
        const auto glyphs = [](const std::vector<int>& mask) {
            std::vector<std::vector<int>> result;
            const int width = mask[0], height = mask[1];
            int start = -1;
            for (int x = 0; x <= width; ++x)
            {
                bool occupied = false;
                if (x < width)
                    for (int y = 0; y < height; ++y)
                        occupied = occupied || mask[2 + y * width + x] != 0;
                if (occupied && start < 0) start = x;
                if (!occupied && start >= 0)
                {
                    std::vector<int> glyph;
                    for (int gy = 0; gy < 16; ++gy)
                        for (int gx = 0; gx < 16; ++gx)
                            glyph.push_back(mask[2 + (gy * height / 16) * width +
                                start + gx * (x - start) / 16]);
                    result.push_back(glyph);
                    start = -1;
                }
            }
            return result;
        };
        const auto left = glyphs(a), right = glyphs(b);
        if (left.empty() || left.size() != right.size()) return 0;
        double score = 0;
        for (std::size_t i = 0; i < left.size(); ++i)
        {
            int common = 0, total = 0;
            for (std::size_t p = 0; p < left[i].size(); ++p)
            {
                common += left[i][p] && right[i][p];
                total += left[i][p] + right[i][p];
            }
            score += total ? 2.0 * common / total : 0;
        }
        return score / left.size();
    }

    // Compare text silhouettes, ignoring the yellow/green selection color.
    inline SpellSelection readSpellSelection(const uint8_t* pixels,
        uint32_t width, uint32_t height, uint32_t pitch)
    {
        SpellSelection result;
        if (!pixels || width < 640 || height < 272 || pitch < width * 3)
            return result;
        const auto bpp = pitch / width;
        const auto ink = [&](int x, int y) {
            const auto* p = pixels + static_cast<std::size_t>(y) * pitch + x * bpp;
            return (p[2] > 200 && p[1] > 200 && p[0] < 100) ||
                (p[1] > 180 && p[2] < 140 && p[0] < 140) ||
                (p[2] > 200 && p[1] > 200 && p[0] > 200) ||
                (p[2] > 180 && p[1] < 80 && p[0] < 80);
        };
        const auto text = [&](int left, int top, int right, int bottom) {
            int minX = right, minY = bottom, maxX = -1, maxY = -1;
            for (int y = top; y < bottom; ++y)
                for (int x = left; x < right; ++x)
                    if (ink(x, y))
                    {
                        if (x < minX) minX = x;
                        if (y < minY) minY = y;
                        if (x > maxX) maxX = x;
                        if (y > maxY) maxY = y;
                    }
            std::vector<int> mask;
            if (maxX < minX) return mask;
            mask.push_back(maxX - minX + 1);
            mask.push_back(maxY - minY + 1);
            for (int y = minY; y <= maxY; ++y)
                for (int x = minX; x <= maxX; ++x)
                    mask.push_back(ink(x, y) ? 1 : 0);
            return mask;
        };
        const auto ready = text(466, 132, 626, 154);
        double best = 0, second = 0;
        int bestRow = -1;
        for (int row = 0; row < 10; ++row)
        {
            const auto name = text(88, 56 + row * 18, 320, 72 + row * 18);
            if (name.empty()) continue;
            result.rows.push_back(row);
            const double score = spellTextSimilarity(name, ready);
            if (score > best)
            {
                second = best;
                best = score;
                bestRow = row;
            }
            else if (score > second) second = score;
        }
        // The list and Cast panel use different fonts. Reject ambiguous matches.
        if (best >= 0.80 && best - second >= 0.08) result.readyRow = bestRow;
        return result;
    }
}
