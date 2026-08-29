#pragma once

#include <string>

namespace DosBoxX
{
    class NamedPipeClient;
}

class DosBoxProgramReader
{
public:
    bool update(
        DosBoxX::NamedPipeClient& pipeClient
    );

    const std::string&
        runningProgram() const;

    bool isRunning(
        const std::string& programName
    ) const;

private:
    std::string m_runningProgram;
};