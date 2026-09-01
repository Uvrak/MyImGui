#pragma once

#ifdef WIN32
#include <windows.h>
#endif

#include <cstdint>

#include "dosbox_memory_snapshot.h"

class DosBoxMemorySnapshotWriter
{
public:
    DosBoxMemorySnapshotWriter();
    ~DosBoxMemorySnapshotWriter();

    bool publish();

private:
#ifdef WIN32
    HANDLE m_mapping =
        nullptr;

    uint8_t* m_sharedMemory =
        nullptr;
#endif

    uint64_t m_snapshotId = 0;
};
