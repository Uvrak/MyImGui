#pragma once

#ifdef _DEBUG
#include <Windows.h>
#include <cstdio>
#include <cstring>

inline void TraceGridBuilderMouse(const char* stage, unsigned long long detail = 0)
{
    char path[MAX_PATH] = {};
    const DWORD length = GetTempPathA(MAX_PATH, path);
    if (length == 0 || length >= MAX_PATH)
        return;
    strcat_s(path, "GridBuilderMouseLatency.log");

    HANDLE file = CreateFileA(path, FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return;

    char line[160] = {};
    const int count = std::snprintf(line, sizeof(line), "%llu %lu %s %llu\r\n",
        static_cast<unsigned long long>(GetTickCount64()),
        static_cast<unsigned long>(GetCurrentProcessId()), stage, detail);
    if (count > 0)
    {
        DWORD written = 0;
        WriteFile(file, line, static_cast<DWORD>(count), &written, nullptr);
    }
    CloseHandle(file);
}
#else
inline void TraceGridBuilderMouse(const char*, unsigned long long = 0) {}
#endif
