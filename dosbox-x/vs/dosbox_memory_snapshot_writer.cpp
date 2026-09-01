#include "dosbox_memory_snapshot_writer.h"

#include <algorithm>
#include <cstring>

#include "dosbox.h"
#include "mem.h"

namespace
{
    constexpr const char*
        DosBoxMemorySnapshotMappingName =
        "DosBoxMemorySnapshot";

    constexpr size_t SharedMemorySize =
        sizeof(
            DosBoxMemorySnapshotHeader
            ) +
        DosBoxMemorySnapshotCapacity;
}

DosBoxMemorySnapshotWriter::
DosBoxMemorySnapshotWriter()
{
#ifdef WIN32
    m_mapping =
        CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            static_cast<DWORD>(
                SharedMemorySize
                ),
            DosBoxMemorySnapshotMappingName
        );

    if(m_mapping == nullptr)
    {
        return;
    }

    m_sharedMemory =
        static_cast<uint8_t*>(
            MapViewOfFile(
                m_mapping,
                FILE_MAP_WRITE,
                0,
                0,
                SharedMemorySize
            )
            );

    if(m_sharedMemory == nullptr)
    {
        CloseHandle(
            m_mapping
        );

        m_mapping =
            nullptr;
    }
#endif
}

DosBoxMemorySnapshotWriter::
~DosBoxMemorySnapshotWriter()
{
#ifdef WIN32
    if(m_sharedMemory != nullptr)
    {
        UnmapViewOfFile(
            m_sharedMemory
        );

        m_sharedMemory =
            nullptr;
    }

    if(m_mapping != nullptr)
    {
        CloseHandle(
            m_mapping
        );

        m_mapping =
            nullptr;
    }
#endif
}

bool DosBoxMemorySnapshotWriter::publish()
{
#ifdef WIN32
    if(m_sharedMemory == nullptr)
    {
        return false;
    }

    const size_t memorySize =
        std::min<size_t>(
            MemSize,
            DosBoxMemorySnapshotCapacity
        );

    auto* header =
        reinterpret_cast<
        DosBoxMemorySnapshotHeader*
        >(
            m_sharedMemory
            );

    uint8_t* destination =
        m_sharedMemory +
        sizeof(
            DosBoxMemorySnapshotHeader
            );

    for(size_t address = 0;
        address < memorySize;
        ++address)
    {
        destination[address] =
            phys_readb(
                static_cast<PhysPt>(
                    address
                    )
            );
    }

    ++m_snapshotId;

    header->version =
        DosBoxMemorySnapshotVersion;

    header->memorySize =
        static_cast<uint32_t>(
            memorySize
            );

    header->snapshotId =
        m_snapshotId;

    return true;
#else
    return false;
#endif
}
