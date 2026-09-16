#include "../mm3_cursor_patch.h"
#include <cassert>
#include <cstdio>
#include <vector>

int main()
{
    // Captured MM3 code, relocated away from the discovery address 0x9C741.
    const uint8_t code[] = {
        0x8C,0xC8,0x8E,0xD8,0x8B,0x1E,0x3E,0x0B,0x8B,0x0E,
        0x3C,0x0B,0x2B,0xFF,0x8E,0x06,0xF6,0x09,0xC5,0x36,
        0x4C,0x0B,0xE8,0x42,0x08,0x5F,0x5E,0x1F,0x5D,0xC3
    };
    std::vector<uint8_t> memory(0x100000, 0);
    const std::size_t start = 0x81234;
    std::copy(code, code + sizeof(code), memory.begin() + start);
    const auto original = memory;
    unsigned writes = 0;
    auto write = [&](std::size_t address, uint8_t value) {
        ++writes;
        memory.at(address) = value;
    };
    MM3CursorPatch patch;
    patch.update(memory.data(), memory.size(), false, true, write);
    assert(memory == original && writes == 0);
    patch.update(memory.data(), memory.size(), true, true, write);
    assert(patch.status() == MM3CursorPatch::Status::Hidden);
    assert(patch.address() == start + 22 && writes == 1);
    assert(memory[start + 22] == 0xA9);
    for (std::size_t i = 0; i < memory.size(); ++i)
        assert(i == start + 22 || memory[i] == original[i]);
    patch.update(memory.data(), memory.size(), true, true, write);
    assert(writes == 1); // Repeated updates do not rewrite code.
    patch.update(memory.data(), memory.size(), true, false, write);
    assert(memory == original && patch.status() == MM3CursorPatch::Status::Visible);
    patch.update(memory.data(), memory.size(), true, true, write);
    const auto beforeExit = memory;
    patch.update(memory.data(), memory.size(), false, true, write);
    assert(memory == beforeExit && patch.address() == 0);

    // Refuse ambiguous matches and mismatched executable bytes.
    memory = original;
    std::copy(code, code + sizeof(code), memory.begin() + 0x91234);
    const auto ambiguous = memory;
    patch.update(memory.data(), memory.size(), true, true, write);
    assert(memory == ambiguous && patch.status() == MM3CursorPatch::Status::Ambiguous);
    memory = original;
    memory[start + 23] ^= 1;
    const auto unsupported = memory;
    patch.update(memory.data(), memory.size(), true, true, write);
    assert(memory == unsupported && patch.status() == MM3CursorPatch::Status::Searching);

    // Never restore a byte into a replaced overlay or a subsequent program.
    memory = original;
    patch.update(memory.data(), memory.size(), true, true, write);
    memory[start] = 0;
    const auto replaced = memory;
    patch.update(memory.data(), memory.size(), false, true, write);
    assert(memory == replaced);

    // A saved state containing the verified hidden form can be restored.
    memory = original;
    memory[start + 22] = 0xA9;
    MM3CursorPatch fromState;
    fromState.update(memory.data(), memory.size(), true, false, write);
    assert(memory == original);
    fromState.update(nullptr, 0, true, true, write);
    assert(fromState.status() == MM3CursorPatch::Status::Searching);
    fromState.update(memory.data(), 12, true, true, write);
    assert(fromState.status() == MM3CursorPatch::Status::Searching);
    std::puts("MM3 cursor patch: all safety and relocation checks passed.");
}
