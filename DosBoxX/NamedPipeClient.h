#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif

#include <string>

namespace DosBoxX
{
    class NamedPipeClient
    {
    public:
        explicit NamedPipeClient(
            std::string pipeName
        );

        ~NamedPipeClient();

        NamedPipeClient(
            const NamedPipeClient&
        ) = delete;

        NamedPipeClient& operator=(
            const NamedPipeClient&
            ) = delete;

        bool send(
            const std::string& message
        );

        bool request(
            const std::string& message,
            std::string& response
        );

        void disconnect();
   
    private:
        bool connect(
            DWORD access
        );

        std::string m_pipeName;

        HANDLE m_pipe =
            INVALID_HANDLE_VALUE;

        DWORD m_access = 0;
    };
}