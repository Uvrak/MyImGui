#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif

#include <cstdint>
#include <vector>

#include "DosBoxMemorySnapshot.h"

namespace MyImGui
{
    class DosBoxMemoryReader
    {
    public:
        DosBoxMemoryReader();
        ~DosBoxMemoryReader();

        DosBoxMemoryReader(
            const DosBoxMemoryReader&
        ) = delete;

        DosBoxMemoryReader& operator=(
            const DosBoxMemoryReader&
            ) = delete;

        bool tryOpen();
        bool readSnapshot();

        bool isOpen() const;

        uint64_t snapshotId() const;

        const std::vector<uint8_t>&
            memory() const;

        void close();

    private:
#ifdef _WIN32
        HANDLE m_mapping =
            nullptr;

        const uint8_t* m_sharedMemory =
            nullptr;
#endif

        uint64_t m_snapshotId = 0;

        std::vector<uint8_t>
            m_memory;
    };
}
