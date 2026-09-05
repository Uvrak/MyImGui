#include "TraceComparisonPersistence.h"

#include <fstream>
#include <limits>
#include <windows.h>

namespace DosBoxMemoryTools::TraceComparisonPersistence
{
    namespace
    {
        constexpr char magic[] = "DBTCMP01";
        constexpr size_t recordSize = 83; // u64 address, 13 u16, 16+32+1 bytes
        constexpr uint64_t hashSeed = 14695981039346656037ull;
        constexpr uint64_t hashPrime = 1099511628211ull;

        void hashBytes(uint64_t& hash, const char* bytes, size_t size)
        {
            for (size_t i = 0; i < size; ++i)
                hash = (hash ^ static_cast<unsigned char>(bytes[i])) * hashPrime;
        }

        void put(char*& out, uint64_t value, size_t width)
        {
            for (size_t i = 0; i < width; ++i)
            {
                *out++ = static_cast<char>(value & 0xff);
                value >>= 8;
            }
        }

        uint64_t get(const char*& in, size_t width)
        {
            uint64_t value = 0;
            for (size_t i = 0; i < width; ++i)
                value |= uint64_t(static_cast<unsigned char>(*in++)) << (8 * i);
            return value;
        }

        void encode(const RuntimeInstruction& instruction, char* out)
        {
            put(out, instruction.address, 8);
            put(out, instruction.cs, 2);
            put(out, instruction.ip, 2);
            const auto& r = instruction.registers;
            for (auto value : {r.ax, r.bx, r.cx, r.dx, r.si, r.di, r.bp, r.sp, r.ds, r.es, r.ss})
                put(out, value, 2);
            for (auto value : instruction.bytes) put(out, value, 1);
            for (auto value : instruction.stackBytes) put(out, value, 1);
            put(out, instruction.writeValue, 1);
        }

        bool decode(const char* in, RuntimeInstruction& instruction)
        {
            const auto address = get(in, 8);
            if (address > (std::numeric_limits<size_t>::max)()) return false;
            instruction.address = static_cast<size_t>(address);
            instruction.cs = static_cast<uint16_t>(get(in, 2));
            instruction.ip = static_cast<uint16_t>(get(in, 2));
            auto& r = instruction.registers;
            for (auto value : {&r.ax, &r.bx, &r.cx, &r.dx, &r.si, &r.di, &r.bp, &r.sp, &r.ds, &r.es, &r.ss})
                *value = static_cast<uint16_t>(get(in, 2));
            for (auto& value : instruction.bytes) value = static_cast<uint8_t>(get(in, 1));
            for (auto& value : instruction.stackBytes) value = static_cast<uint8_t>(get(in, 1));
            instruction.writeValue = static_cast<uint8_t>(get(in, 1));
            return true;
        }
    }

    bool save(const std::filesystem::path& path,
        const std::vector<RuntimeInstruction>& trace,
        const std::string& sourceFilename) noexcept
    {
        std::filesystem::path temporary;
        try
        {
            if (sourceFilename.size() > 4095 || sourceFilename.find('\0') != std::string::npos)
                return false;
            if (!path.parent_path().empty())
                std::filesystem::create_directories(path.parent_path());
            temporary = path;
            temporary += "." + std::to_string(GetCurrentProcessId()) + ".tmp";
            std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
            file.exceptions(std::ios::failbit | std::ios::badbit);
            file.write(magic, 8);
            char header[12];
            char* out = header;
            put(out, trace.size(), 8);
            put(out, sourceFilename.size(), 4);
            file.write(header, sizeof(header));
            file.write(sourceFilename.data(), static_cast<std::streamsize>(sourceFilename.size()));
            uint64_t hash = hashSeed;
            hashBytes(hash, header, sizeof(header));
            hashBytes(hash, sourceFilename.data(), sourceFilename.size());
            for (const auto& instruction : trace)
            {
                char record[recordSize];
                encode(instruction, record);
                file.write(record, sizeof(record));
                hashBytes(hash, record, sizeof(record));
            }
            char checksum[8];
            out = checksum;
            put(out, hash, 8);
            file.write(checksum, sizeof(checksum));
            file.flush();
            file.close();
            // Same-directory replacement keeps the previous snapshot on write failure.
            if (MoveFileExW(temporary.c_str(), path.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
                return true;
        }
        catch (const std::exception&)
        {
            // I/O and allocation failures must not terminate the application.
        }
        std::error_code error;
        if (!temporary.empty()) std::filesystem::remove(temporary, error);
        return false;
    }

    RestoreResult restore(const std::filesystem::path& path,
        std::vector<RuntimeInstruction>& trace,
        std::string& sourceFilename) noexcept
    {
        try
        {
            if (!std::filesystem::exists(path)) return RestoreResult::Missing;
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            file.exceptions(std::ios::failbit | std::ios::badbit);
            const auto length = file.tellg();
            if (length < 28) return RestoreResult::Invalid;
            file.seekg(0);
            char signature[8];
            file.read(signature, sizeof(signature));
            if (std::string(signature, 8) != magic) return RestoreResult::Invalid;
            char header[12];
            file.read(header, sizeof(header));
            const char* in = header;
            const auto count = get(in, 8);
            const auto nameSize = get(in, 4);
            const auto payloadSize = static_cast<uint64_t>(length) - 28;
            // Validate against the actual file size BEFORE allocating any trace storage.
            if (nameSize > 4095 || nameSize > payloadSize ||
                (payloadSize - nameSize) % recordSize != 0 ||
                count != (payloadSize - nameSize) / recordSize)
                return RestoreResult::Invalid;
            std::vector<RuntimeInstruction> loaded;
            if (count > loaded.max_size()) return RestoreResult::Invalid;
            std::string filename(static_cast<size_t>(nameSize), '\0');
            file.read(filename.data(), static_cast<std::streamsize>(nameSize));
            if (filename.find('\0') != std::string::npos) return RestoreResult::Invalid;
            uint64_t hash = hashSeed;
            hashBytes(hash, header, sizeof(header));
            hashBytes(hash, filename.data(), filename.size());
            loaded.reserve(static_cast<size_t>(count));
            for (uint64_t i = 0; i < count; ++i)
            {
                char record[recordSize];
                file.read(record, sizeof(record));
                hashBytes(hash, record, sizeof(record));
                RuntimeInstruction instruction{};
                if (!decode(record, instruction)) return RestoreResult::Invalid;
                loaded.push_back(instruction);
            }
            char checksum[8];
            file.read(checksum, sizeof(checksum));
            in = checksum;
            if (get(in, 8) != hash) return RestoreResult::Invalid;
            trace.swap(loaded);
            sourceFilename.swap(filename);
            return RestoreResult::Loaded;
        }
        catch (const std::exception&)
        {
            return RestoreResult::Invalid;
        }
    }
}
