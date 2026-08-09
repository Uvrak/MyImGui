#pragma once

#include <Windows.h>
#include <cstdint>

namespace MyImGui
{
    struct DosBoxFrameHeader
    {
        uint32_t width;
        uint32_t height;
        uint32_t pitch;
        uint32_t format;
        uint64_t frameCounter;
    };

    class DosBoxFrameReader
    {
    public:
        DosBoxFrameReader();
        ~DosBoxFrameReader();

        const DosBoxFrameHeader* header() const;
        bool tryOpen();

    private:
        HANDLE m_mapping = nullptr;
        const uint8_t* m_sharedMemory = nullptr;
    };
}
