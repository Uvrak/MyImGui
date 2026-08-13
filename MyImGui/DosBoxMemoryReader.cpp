#include "pch.h"
#include "DosBoxMemoryReader.h"

#include <cstring>

namespace
{
    constexpr const char*
        DosBoxMemorySnapshotMappingName =
        "DosBoxMemorySnapshot";

    constexpr size_t SharedMemorySize =
        sizeof(
            MyImGui::
            DosBoxMemorySnapshotHeader
            ) +
        MyImGui::
        DosBoxMemorySnapshotCapacity;
}

namespace MyImGui
{
    DosBoxMemoryReader::
        DosBoxMemoryReader()
    {}

    DosBoxMemoryReader::
        ~DosBoxMemoryReader()
    {
        close();
    }

    bool DosBoxMemoryReader::tryOpen()
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

    bool DosBoxMemoryReader::readSnapshot()
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
            DosBoxMemorySnapshotVersion ||
            header->memorySize == 0 ||
            header->memorySize >
            DosBoxMemorySnapshotCapacity)
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

    bool DosBoxMemoryReader::isOpen() const
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
        DosBoxMemoryReader::snapshotId() const
    {
        return m_snapshotId;
    }

    const std::vector<uint8_t>&
        DosBoxMemoryReader::memory() const
    {
        return m_memory;
    }

    void DosBoxMemoryReader::close()
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