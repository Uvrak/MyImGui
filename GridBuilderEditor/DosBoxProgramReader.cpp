#include "DosBoxProgramReader.h"

#include "NamedPipeClient.h"

bool DosBoxProgramReader::update(
    DosBoxX::NamedPipeClient& pipeClient
)
{
    std::string response;

    if (!pipeClient.request(
        "RUNNING_PROGRAM",
        response
    ))
    {
        m_runningProgram.clear();
        return false;
    }

    constexpr const char* Prefix =
        "RUNNING_PROGRAM:";

    if (response.rfind(
        Prefix,
        0
    ) != 0)
    {
        m_runningProgram.clear();
        return false;
    }

    m_runningProgram =
        response.substr(
            std::char_traits<char>::
            length(Prefix)
        );

    return true;
}

const std::string&
DosBoxProgramReader::runningProgram() const
{
    return m_runningProgram;
}

bool DosBoxProgramReader::isRunning(
    const std::string& programName
) const
{
    return
        m_runningProgram ==
        programName;
}