#pragma once

#include <Windows.h>
#include <cstdint>

namespace DosBoxX
{
    struct DosBoxFrameHeader
    {
        uint32_t width;
        uint32_t height;

        uint32_t contentWidth;
        uint32_t contentHeight;

        uint32_t pitch;
        uint32_t format;
        uint64_t frameCounter;
    };

    class FrameReader
    {
    public:
        FrameReader();
        ~FrameReader();

        const DosBoxFrameHeader* header() const;
        const uint8_t* pixels() const;
        bool tryOpen();

    private:
        HANDLE m_mapping = nullptr;
        const uint8_t* m_sharedMemory = nullptr;
    };
}
