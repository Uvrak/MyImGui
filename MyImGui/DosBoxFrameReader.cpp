#include "pch.h"
#include "DosBoxFrameReader.h"
#include <cstdio>

namespace MyImGui
{
    DosBoxFrameReader::DosBoxFrameReader()
    {
        m_mapping =
            OpenFileMappingA(
                FILE_MAP_READ,
                FALSE,
                "GridBuilderDOSBoxFrame"
            );

        if (m_mapping == nullptr)
        {
            DWORD error =
                GetLastError();

            printf(
                "DosBoxFrameReader: OpenFileMapping failed: %lu\n",
                error
            );
        }
    }

    DosBoxFrameReader::~DosBoxFrameReader()
    {
        if (m_sharedMemory != nullptr)
        {
            UnmapViewOfFile(
                m_sharedMemory
            );

            m_sharedMemory = nullptr;
        }

        if (m_mapping != nullptr)
        {
            CloseHandle(
                m_mapping
            );

            m_mapping = nullptr;
        }
    }

    const DosBoxFrameHeader*
        DosBoxFrameReader::header() const
    {
        if (m_sharedMemory == nullptr)
        {
            return nullptr;
        }

        return reinterpret_cast<
            const DosBoxFrameHeader*
        >(
            m_sharedMemory
            );
    }

    const uint8_t*
        DosBoxFrameReader::pixels() const
    {
        if (m_sharedMemory == nullptr)
        {
            return nullptr;
        }

        return m_sharedMemory +
            sizeof(DosBoxFrameHeader);
    }

    bool DosBoxFrameReader::tryOpen()
    {
        if (m_mapping == nullptr)
        {
            m_mapping =
                OpenFileMappingA(
                    FILE_MAP_READ,
                    FALSE,
                    "GridBuilderDOSBoxFrame"
                );

            if (m_mapping == nullptr)
            {
                return false;
            }
        }

        if (m_sharedMemory == nullptr)
        {
            m_sharedMemory =
                static_cast<const uint8_t*>(
                    MapViewOfFile(
                        m_mapping,
                        FILE_MAP_READ,
                        0,
                        0,
                        0
                    )
                    );
        }

        return m_sharedMemory != nullptr;
    }
}