#include "Process.h"

#include <Windows.h>

namespace DosBoxX
{
    Process::Process()
    {}

    Process::~Process()
    {
        stop();
    }

    bool Process::start(
        const std::wstring& executablePath
    )
    {
        if (running())
        {
            return true;
        }

        STARTUPINFOW startupInfo{};
        startupInfo.cb =
            sizeof(startupInfo);

        PROCESS_INFORMATION processInfo{};

        const BOOL result =
            CreateProcessW(
                executablePath.c_str(),
                nullptr,
                nullptr,
                nullptr,
                FALSE,
                0,
                nullptr,
                nullptr,
                &startupInfo,
                &processInfo
            );

        if (!result)
        {
            return false;
        }

        CloseHandle(
            processInfo.hThread
        );

        m_processHandle =
            processInfo.hProcess;

        return true;
    }

    void Process::stop()
    {
        if (m_processHandle == nullptr)
        {
            return;
        }

        HANDLE processHandle =
            static_cast<HANDLE>(
                m_processHandle
                );

        TerminateProcess(
            processHandle,
            0
        );

        CloseHandle(
            processHandle
        );

        m_processHandle =
            nullptr;
    }

    bool Process::running() const
    {
        if (m_processHandle == nullptr)
        {
            return false;
        }

        const HANDLE processHandle =
            static_cast<HANDLE>(
                m_processHandle
                );

        return WaitForSingleObject(
            processHandle,
            0
        ) == WAIT_TIMEOUT;
    }
}