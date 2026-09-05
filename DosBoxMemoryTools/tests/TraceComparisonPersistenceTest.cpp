// Standalone regression tests; compile with TraceComparisonPersistence.cpp.
#include "TraceComparisonPersistence.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <windows.h>

using namespace DosBoxMemoryTools;
namespace Persistence = TraceComparisonPersistence;

static void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

static bool equal(const RuntimeInstruction& a, const RuntimeInstruction& b)
{
    const auto& x = a.registers;
    const auto& y = b.registers;
    return a.address == b.address && a.cs == b.cs && a.ip == b.ip &&
        x.ax == y.ax && x.bx == y.bx && x.cx == y.cx && x.dx == y.dx &&
        x.si == y.si && x.di == y.di && x.bp == y.bp && x.sp == y.sp &&
        x.ds == y.ds && x.es == y.es && x.ss == y.ss &&
        a.bytes == b.bytes && a.stackBytes == b.stackBytes && a.writeValue == b.writeValue;
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Provide an isolated test output directory");
        const auto directory = std::filesystem::path(argv[1]) / std::to_string(GetCurrentProcessId());
        const auto a = directory / "A.snapshot";
        const auto b = directory / "B.snapshot";
        std::vector<RuntimeInstruction> restored(1);
        restored[0].address = 42;
        std::string name = "unchanged";
        require(Persistence::restore(a, restored, name) == Persistence::RestoreResult::Missing, "missing file");
        require(restored[0].address == 42 && name == "unchanged", "missing file changed outputs");

        std::vector<RuntimeInstruction> original(1000000);
        for (size_t i = 0; i < original.size(); ++i)
        {
            auto& instruction = original[i];
            instruction.address = i * 13;
            instruction.cs = static_cast<uint16_t>(i);
            instruction.ip = static_cast<uint16_t>(i + 1);
            auto& r = instruction.registers;
            size_t value = i;
            for (auto field : {&r.ax, &r.bx, &r.cx, &r.dx, &r.si, &r.di, &r.bp, &r.sp, &r.ds, &r.es, &r.ss})
                *field = static_cast<uint16_t>(++value);
            for (auto& byte : instruction.bytes) byte = static_cast<uint8_t>(++value);
            for (auto& byte : instruction.stackBytes) byte = static_cast<uint8_t>(++value);
            instruction.writeValue = static_cast<uint8_t>(i);
        }
        const std::string source = "C:\\traces with spaces\\original A.trace";
        require(Persistence::save(a, original, source), "large snapshot save");
        require(Persistence::restore(a, restored, name) == Persistence::RestoreResult::Loaded, "large snapshot restore");
        require(name == source && restored.size() == original.size(), "large snapshot metadata");
        for (size_t i = 0; i < original.size(); ++i)
            require(equal(original[i], restored[i]), "record round trip");
        require(Persistence::save(b, {}, "empty B.trace"), "empty B save");
        require(Persistence::restore(b, restored, name) == Persistence::RestoreResult::Loaded && restored.empty(), "empty B restore");
        require(Persistence::restore(a, restored, name) == Persistence::RestoreResult::Loaded && restored.size() == original.size(), "B changed A");

        // Deny replacement of A, then verify its last good snapshot survives.
        HANDLE locked = CreateFileW(a.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        require(locked != INVALID_HANDLE_VALUE, "lock snapshot");
        const bool saved = Persistence::save(a, {}, "replacement");
        CloseHandle(locked);
        require(!saved, "locked replacement must fail");
        require(Persistence::restore(a, restored, name) == Persistence::RestoreResult::Loaded && restored.size() == original.size(), "failed write destroyed A");
        require(Persistence::save(a, {original[123]}, "new A"), "replacement save");
        require(Persistence::restore(a, restored, name) == Persistence::RestoreResult::Loaded && restored.size() == 1 && equal(restored[0], original[123]), "replacement restore");

        std::ifstream input(a, std::ios::binary);
        const std::string valid((std::istreambuf_iterator<char>(input)), {});
        const auto corrupt = directory / "corrupt.snapshot";
        auto reject = [&](const std::string& data)
        {
            { std::ofstream file(corrupt, std::ios::binary); file.write(data.data(), data.size()); }
            restored.assign(1, original[456]);
            name = "unchanged";
            require(Persistence::restore(corrupt, restored, name) == Persistence::RestoreResult::Invalid, "corruption accepted");
            require(restored.size() == 1 && equal(restored[0], original[456]) && name == "unchanged", "corruption changed outputs");
        };
        for (size_t length = 0; length < valid.size(); ++length)
            reject(valid.substr(0, length));
        for (size_t byte = 0; byte < valid.size(); ++byte)
        {
            auto altered = valid;
            altered[byte] ^= 1;
            reject(altered);
        }
        auto oversized = valid;
        for (size_t i = 8; i < 16; ++i) oversized[i] = '\xff';
        reject(oversized);
        reject(valid + "extra");
        std::cout << "PASS: 1,000,000 records, all fields, independent A/B, empty/missing, replacement, write failure, truncation, corruption, oversized count\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
