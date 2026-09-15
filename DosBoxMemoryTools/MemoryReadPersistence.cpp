#include "MemoryReadPersistence.h"
#include "MemoryReadTrackerWindow.h"

#include <fstream>
#include <filesystem>
#include <cstring>
#include <windows.h>

namespace DosBoxMemoryTools
{
    void MemoryReadPersistence::save(
        const MemoryReadTrackerWindow& window
    )
    {

        if (window.m_idleReadAddresses.empty() &&
            window.m_attackReadAddresses.empty() &&
            window.m_attackOnlyReadAddresses.empty() &&
            window.m_previousAttackOnlyReadAddresses.empty() &&
            window.m_attackReadInstructions.empty() &&
            window.m_scanner.candidates().empty())
        {
            return;
        }

        if (window.m_gameId.empty())
        {
            OutputDebugStringA(
                "saveSession: gameId is empty\n"
            );

            return;
        }
        
        OutputDebugStringA(
            "saveSession called\n"
        );

        std::filesystem::create_directories(
            "settings"
        );

        const std::string filename =
            "../settings/memory_read_session_" +
            window.m_gameId +
            ".cfg";

        OutputDebugStringA(
            filename.c_str()
        );

        OutputDebugStringA(
            "\n"
        );

        std::ofstream file(
            filename
        );

        if (!file)
        {
            return;
        }

        file <<
            "GridBuilderMemoryReadSession 1\n";

        file <<
            "Idle\n";

        file <<
            window.m_idleReadAddresses.size() <<
            '\n';

        for (const size_t address :
        window.m_idleReadAddresses)
        {
            file <<
                address <<
                '\n';
        }

        file <<
            "Attack\n";

        file <<
            window.m_attackReadAddresses.size() <<
            '\n';

        for (const size_t address :
        window.m_attackReadAddresses)
        {
            file <<
                address <<
                '\n';
        }

        file <<
            "AttackOnly\n";

        file <<
            window.m_attackOnlyReadAddresses.size() <<
            '\n';

        for (const size_t address :
        window.m_attackOnlyReadAddresses)
        {
            file <<
                address <<
                '\n';
        }

        const auto& candidates =
            window.m_scanner.candidates();

        file <<
            "Candidates\n";

        file <<
            candidates.size() <<
            '\n';

        for (const MemoryCandidate& candidate :
            candidates)
        {
            file <<
                candidate.address <<
                '\n';
        }

        file <<
            "PreviousAttackOnly\n";

        file <<
            window.m_previousAttackOnlyReadAddresses.size() <<
            '\n';

        for (const size_t address :
        window.m_previousAttackOnlyReadAddresses)
        {
            file <<
                address <<
                '\n';
        }

        file <<
            "AttackInstructions\n";

        file <<
            window.m_attackReadInstructions.size() <<
            '\n';

        for (const auto& entry :
            window.m_attackReadInstructions)
        {
            file <<
                entry.first << ' ' <<
                entry.second << '\n';
        }

        file <<
            "RangeSettings\n";

        file <<
            (window.m_limitAddressRange ? 1 : 0) <<
            '\n';

        file <<
            window.m_rangeStartText <<
            '\n';

        file <<
            window.m_rangeEndText <<
            '\n';

        file <<
            (window.m_limitInstructionRange ? 1 : 0) <<
            '\n';

        file <<
            window.m_instructionRangeStartText <<
            '\n';

        file <<
            window.m_instructionRangeEndText <<
            '\n';
    }

    void MemoryReadPersistence::load(
        MemoryReadTrackerWindow& window
    )
    {
        if (window.m_gameId.empty())
        {
            return;
        }

        const std::string filename =
            "../settings/memory_read_session_" +
            window.m_gameId +
            ".cfg";

        OutputDebugStringA(
            "loadSession: "
        );

        OutputDebugStringA(
            filename.c_str()
        );

        OutputDebugStringA(
            "\n"
        );

        char cwd[MAX_PATH]{};

        GetCurrentDirectoryA(
            MAX_PATH,
            cwd
        );

        OutputDebugStringA(
            "Current directory: "
        );

        OutputDebugStringA(
            cwd
        );

        OutputDebugStringA(
            "\n"
        );

        std::ifstream file(
            filename
        );

        if (!file)
        {
            OutputDebugStringA(
                "loadSession FAILED: could not open file\n"
            );

            return;
        }

        OutputDebugStringA(
            "loadSession OK: file opened\n"
        );

        std::string header;

        std::getline(
            file,
            header
        );

        OutputDebugStringA(
            "loadSession header: "
        );

        OutputDebugStringA(
            header.c_str()
        );

        OutputDebugStringA(
            "\n"
        );

        if (header !=
            "GridBuilderMemoryReadSession 1")
        {
            OutputDebugStringA(
                "loadSession FAILED: bad header\n"
            );

            return;
        }
        auto readAddresses =
            [&file](
                const char* expectedSection,
                std::vector<size_t>& addresses
                ) -> bool
            {
                std::string section;

                if (!(file >> section) ||
                    section != expectedSection)
                {
                    return false;
                }

                size_t count = 0;

                if (!(file >> count))
                {
                    return false;
                }

                addresses.clear();
                addresses.reserve(
                    count
                );

                for (size_t i = 0;
                    i < count;
                    ++i)
                {
                    size_t address = 0;

                    if (!(file >> address))
                    {
                        addresses.clear();
                        return false;
                    }

                    addresses.push_back(
                        address
                    );
                }

                return true;
            };

        std::vector<size_t> candidates;

        if (!readAddresses(
            "Idle",
            window.m_idleReadAddresses
        ))
        {
            OutputDebugStringA(
                "loadSession FAILED: Idle\n"
            );

            return;
        }

        OutputDebugStringA(
            "loadSession OK: Idle\n"
        );

        if (!readAddresses(
            "Attack",
            window.m_attackReadAddresses
        ))
        {
            OutputDebugStringA(
                "loadSession FAILED: Attack\n"
            );

            return;
        }

        OutputDebugStringA(
            "loadSession OK: Attack\n"
        );

        if (!readAddresses(
            "AttackOnly",
            window.m_attackOnlyReadAddresses
        ))
        {
            OutputDebugStringA(
                "loadSession FAILED: AttackOnly\n"
            );

            return;
        }

        OutputDebugStringA(
            "loadSession OK: AttackOnly\n"
        );

        if (!readAddresses(
            "Candidates",
            candidates
        ))
        {
            OutputDebugStringA(
                "loadSession FAILED: Candidates\n"
            );

            return;
        }

        std::string optionalSection;

        while (file >> optionalSection)
        {
            if (optionalSection ==
                "PreviousAttackOnly")
            {
                size_t count = 0;

                if (!(file >> count))
                {
                    break;
                }

                window.m_previousAttackOnlyReadAddresses.clear();

                window.m_previousAttackOnlyReadAddresses.reserve(
                    count
                );

                for (size_t i = 0;
                    i < count;
                    ++i)
                {
                    size_t address = 0;

                    if (!(file >> address))
                    {
                        break;
                    }

                    window.m_previousAttackOnlyReadAddresses.push_back(
                        address
                    );
                }
            }
            else if (optionalSection ==
                "AttackInstructions")
            {
                size_t count = 0;

                if (!(file >> count))
                {
                    break;
                }

                window.m_attackReadInstructions.clear();

                window.m_attackReadInstructions.reserve(
                    count
                );

                for (size_t i = 0;
                    i < count;
                    ++i)
                {
                    size_t memoryAddress = 0;
                    size_t instructionAddress = 0;

                    if (!(file >>
                        memoryAddress >>
                        instructionAddress))
                    {
                        break;
                    }

                    window.m_attackReadInstructions.emplace_back(
                        memoryAddress,
                        instructionAddress
                    );
                }

            }
                else if (optionalSection ==
                    "RangeSettings")
                {
                    int limitAddressRange = 0;
                    int limitInstructionRange = 0;

                    std::string rangeStart;
                    std::string rangeEnd;

                    std::string instructionRangeStart;
                    std::string instructionRangeEnd;

                    if (!(file >>
                        limitAddressRange >>
                        rangeStart >>
                        rangeEnd >>
                        limitInstructionRange >>
                        instructionRangeStart >>
                        instructionRangeEnd))
                    {
                        break;
                    }

                    window.m_limitAddressRange =
                        limitAddressRange != 0;

                    window.m_limitInstructionRange =
                        limitInstructionRange != 0;

                    strncpy_s(
                        window.m_rangeStartText,
                        sizeof(window.m_rangeStartText),
                        rangeStart.c_str(),
                        _TRUNCATE
                    );

                    strncpy_s(
                        window.m_rangeEndText,
                        sizeof(window.m_rangeEndText),
                        rangeEnd.c_str(),
                        _TRUNCATE
                    );

                    strncpy_s(
                        window.m_instructionRangeStartText,
                        sizeof(window.m_instructionRangeStartText),
                        instructionRangeStart.c_str(),
                        _TRUNCATE
                    );

                    strncpy_s(
                        window.m_instructionRangeEndText,
                        sizeof(window.m_instructionRangeEndText),
                        instructionRangeEnd.c_str(),
                        _TRUNCATE
                    );
                }
            }
        

        OutputDebugStringA(
            "loadSession OK: Candidates\n"
        );

        if (!candidates.empty())
        {
            window.m_scanner.setCandidatesFromAddresses(
                candidates
            );
        }
    }
}
