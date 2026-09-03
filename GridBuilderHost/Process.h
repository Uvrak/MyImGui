#pragma once

#include <string>

namespace DosBoxX
{
    class Process
    {
    public:
        Process();
        ~Process();

        bool start(
            const std::wstring& executablePath
        );

        void stop();

        bool running() const;

    private:
        void* m_processHandle =
            nullptr;
    };
}