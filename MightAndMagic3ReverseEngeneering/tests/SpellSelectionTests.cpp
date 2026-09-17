#include "../SpellSelection.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>

int main(int argc, char** argv)
{
    using MightAndMagic3::readSpellSelection;
    std::vector<uint8_t> frame(4096 * 1024);
    const auto paint = [&](int x, int y, bool green = false) {
        const auto offset = y * 4096 + x * 4;
        frame[offset] = 0;
        frame[offset + 1] = 255;
        frame[offset + 2] = green ? 65 : 255;
    };
    assert(readSpellSelection(nullptr, 1024, 1024, 4096).rows.empty());
    assert(readSpellSelection(frame.data(), 10, 10, 40).rows.empty());
    assert(readSpellSelection(frame.data(), 1024, 1024, 4096).readyRow == -1);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 6; ++x)
            if (x < 2 || y >= 6)
            {
                paint(520 + x, 136 + y);
                paint(92 + x, 94 + y, true);
            }
    auto result = readSpellSelection(frame.data(), 1024, 1024, 4096);
    assert(result.rows.size() == 1 && result.readyRow == 2);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 6; ++x)
            if (x < 2 || y >= 6) paint(92 + x, 58 + y);
    assert(readSpellSelection(frame.data(), 1024, 1024, 4096).readyRow == -1);

    // Optional real 1024x1024 BGRA capture: four spells, ready spell Licht.
    if (argc > 1)
    {
        std::ifstream input(argv[1], std::ios::binary);
        frame.assign(std::istreambuf_iterator<char>(input), {});
        assert(frame.size() == 4096 * 1024);
        result = readSpellSelection(frame.data(), 1024, 1024, 4096);
        assert(result.rows.size() == 4 && result.readyRow == 0);
        // Move the ready spell to another row, then make the match ambiguous.
        for (int y = 0; y < 16; ++y)
            for (int x = 88; x < 320; ++x)
                for (int c = 0; c < 4; ++c)
                    std::swap(frame[(56 + y) * 4096 + x * 4 + c],
                        frame[(92 + y) * 4096 + x * 4 + c]);
        assert(readSpellSelection(frame.data(), 1024, 1024, 4096).readyRow == 2);
        for (int y = 0; y < 16; ++y)
            for (int x = 88; x < 320; ++x)
                for (int c = 0; c < 4; ++c)
                    frame[(56 + y) * 4096 + x * 4 + c] =
                        frame[(92 + y) * 4096 + x * 4 + c];
        assert(readSpellSelection(frame.data(), 1024, 1024, 4096).readyRow == -1);
    }
    std::cout << "Spell selection tests passed\n";
}
