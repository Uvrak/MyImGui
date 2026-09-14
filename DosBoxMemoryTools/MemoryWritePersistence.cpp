#include "pch.h"
#include "MemoryWritePersistence.h"

#include <fstream>

namespace DosBoxMemoryTools::MemoryWritePersistence
{
    bool save(
        const std::filesystem::path& path,
        const Snapshot& snapshot
    )
    {
        if (!path.parent_path().empty())
        {
            std::filesystem::create_directories(
                path.parent_path()
            );
        }

        std::ofstream file(
            path,
            std::ios::binary | std::ios::trunc
        );

        if (!file)
        {
            return false;
        }

        const size_t count =
            snapshot.captures.size();

        file.write(
            reinterpret_cast<const char*>(&count),
            sizeof(count)
        );

        file.write(
            reinterpret_cast<const char*>(&snapshot.rangeStart),
            sizeof(snapshot.rangeStart)
        );

        file.write(
            reinterpret_cast<const char*>(&snapshot.rangeEnd),
            sizeof(snapshot.rangeEnd)
        );

        for (const RuntimeInstruction& capture : snapshot.captures)
        {
            file.write(
                reinterpret_cast<const char*>(&capture),
                sizeof(capture)
            );
        }

        return file.good();
    }

    bool load(
        const std::filesystem::path& path,
        Snapshot& snapshot
    )
    {
        std::ifstream file(
            path,
            std::ios::binary
        );

        if (!file)
        {
            return false;
        }

        size_t count = 0;

        file.read(
            reinterpret_cast<char*>(&count),
            sizeof(count)
        );

        file.read(
            reinterpret_cast<char*>(&snapshot.rangeStart),
            sizeof(snapshot.rangeStart)
        );

        file.read(
            reinterpret_cast<char*>(&snapshot.rangeEnd),
            sizeof(snapshot.rangeEnd)
        );

        if (!file)
        {
            return false;
        }

        if (!file)
        {
            return false;
        }

        std::vector<RuntimeInstruction> loaded(
            count
        );

        for (RuntimeInstruction& capture : loaded)
        {
            file.read(
                reinterpret_cast<char*>(&capture),
                sizeof(capture)
            );

            if (!file)
            {
                return false;
            }
        }

        snapshot.captures =
            std::move(loaded);

        return true;
    }
}