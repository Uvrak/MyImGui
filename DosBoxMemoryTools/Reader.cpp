#include "MemoryReader.h"

#include <cstring>

namespace
{
    constexpr const char*
        DosBoxMemorySnapshotMappingName =
        "DosBoxMemorySnapshot";

    constexpr size_t SharedMemorySize =
        sizeof(
            DosBoxMemoryTools::
            DosBoxMemorySnapshotHeader
            ) +
        DosBoxMemoryTools::
        MemorySnapshotCapacity;
}

namespace DosBoxMemoryTools
{
    MemoryReader::
        MemoryReader()
    {}
    
    MemoryReader::
        ~MemoryReader()
    {
        close();
    }

    bool MemoryReader::tryOpen()
    {
#ifdef _WIN32
        if (isOpen())
        {
            return true;
        }

        m_mapping =
            OpenFileMappingA(
                FILE_MAP_READ,
                FALSE,
                DosBoxMemorySnapshotMappingName
            );

        if (m_mapping == nullptr)
        {
            return false;
        }

        m_sharedMemory =
            static_cast<const uint8_t*>(
                MapViewOfFile(
                    m_mapping,
                    FILE_MAP_READ,
                    0,
                    0,
                    SharedMemorySize
                )
                );

        if (m_sharedMemory == nullptr)
        {
            CloseHandle(
                m_mapping
            );

            m_mapping =
                nullptr;

            return false;
        }

        return true;
#else
        return false;
#endif
    }

    bool MemoryReader::readSnapshot()
    {
        if (!tryOpen())
        {
            return false;
        }

        const auto* header =
            reinterpret_cast<
            const DosBoxMemorySnapshotHeader*
            >(
                m_sharedMemory
                );

        if (header->version !=
            MemorySnapshotVersion ||
            header->memorySize == 0 ||
            header->memorySize >
            MemorySnapshotCapacity)
        {
            return false;
        }

        const uint8_t* source =
            m_sharedMemory +
            sizeof(
                DosBoxMemorySnapshotHeader
                );

        m_memory.assign(
            source,
            source +
            header->memorySize
        );

        m_snapshotId =
            header->snapshotId;

        return true;
    }

    bool MemoryReader::isOpen() const
    {
#ifdef _WIN32
        return
            m_sharedMemory !=
            nullptr;
#else
        return false;
#endif
    }

    uint64_t
        MemoryReader::snapshotId() const
    {
        return m_snapshotId;
    }

    const std::vector<uint8_t>&
        MemoryReader::memory() const
    {
        return m_memory;
    }

    void MemoryReader::close()
    {
#ifdef _WIN32
        if (m_sharedMemory != nullptr)
        {
            UnmapViewOfFile(
                m_sharedMemory
            );

            m_sharedMemory =
                nullptr;
        }

        if (m_mapping != nullptr)
        {
            CloseHandle(
                m_mapping
            );

            m_mapping =
                nullptr;
        }
#endif

        m_snapshotId = 0;
        m_memory.clear();
    }
}