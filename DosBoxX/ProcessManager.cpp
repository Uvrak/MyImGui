#include "ProcessManager.h"

#include <Windows.h>
#include <TlHelp32.h>

namespace DosBoxX
{
    void ProcessManager::terminateRunningInstances()
    {
        HANDLE snapshot =
            CreateToolhelp32Snapshot(
                TH32CS_SNAPPROCESS,
                0
            );

        if (snapshot ==
            INVALID_HANDLE_VALUE)
        {
            return;
        }

        PROCESSENTRY32W entry = {};

        entry.dwSize =
            sizeof(entry);

        if (Process32FirstW(
            snapshot,
            &entry
        ))
        {
            do
            {
                if (_wcsicmp(
                    entry.szExeFile,
                    L"dosbox-x.exe"
                ) == 0)
                {
                    HANDLE process =
                        OpenProcess(
                            PROCESS_TERMINATE,
                            FALSE,
                            entry.th32ProcessID
                        );

                    if (process != nullptr)
                    {
                        TerminateProcess(
                            process,
                            0
                        );

                        CloseHandle(
                            process
                        );
                    }
                }
            } while (Process32NextW(
                snapshot,
                &entry
            ));
        }

        CloseHandle(
            snapshot
        );
    }
}