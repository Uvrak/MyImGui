#include "pch.h"
#include "NamedPipeClient.h"

#include <utility>

namespace MyImGui
{
    NamedPipeClient::NamedPipeClient(
        std::string pipeName
    )
        : m_pipeName(
            std::move(pipeName)
        )
    {}

    NamedPipeClient::~NamedPipeClient()
    {
        disconnect();
    }

    bool NamedPipeClient::request(
        const std::string& message,
        std::string& response
    )
    {
        response.clear();

        if (!connect(
            GENERIC_READ |
            GENERIC_WRITE
        ))
        {
            return false;
        }

        char buffer[256] = {};
        DWORD bytesRead = 0;

        const BOOL result =
            TransactNamedPipe(
                m_pipe,
                const_cast<char*>(
                    message.data()
                    ),
                static_cast<DWORD>(
                    message.size()
                    ),
                buffer,
                sizeof(buffer) - 1,
                &bytesRead,
                nullptr
            );

        if (!result)
        {
            disconnect();
            return false;
        }

        response.assign(
            buffer,
            bytesRead
        );

        return true;
    }

    bool NamedPipeClient::send(
        const std::string& message
    )
    {
        if (!connect(
            GENERIC_WRITE
        ))
        {
            return false;
        }

        DWORD bytesWritten = 0;

        const BOOL result =
            WriteFile(
                m_pipe,
                message.data(),
                static_cast<DWORD>(
                    message.size()
                    ),
                &bytesWritten,
                nullptr
            );

        if (!result ||
            bytesWritten != message.size())
        {
            disconnect();
            return false;
        }

        return true;
    }

    bool NamedPipeClient::connect(
        DWORD access
    )
    {
        if (m_pipe != INVALID_HANDLE_VALUE &&
            m_access == access)
        {
            return true;
        }

        disconnect();

        if (!WaitNamedPipeA(
            m_pipeName.c_str(),
            500
        ))
        {
            return false;
        }

        m_pipe =
            CreateFileA(
                m_pipeName.c_str(),
                access,
                0,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr
            );

        if (m_pipe == INVALID_HANDLE_VALUE)
        {
            m_access = 0;
            return false;
        }

        m_access = access;

        return true;

    }
    void NamedPipeClient::disconnect()
    {
        if (m_pipe == INVALID_HANDLE_VALUE)
        {
            return;
        }

        CloseHandle(
            m_pipe
        );

        m_pipe =
            INVALID_HANDLE_VALUE;

        m_access = 0;
    }
}