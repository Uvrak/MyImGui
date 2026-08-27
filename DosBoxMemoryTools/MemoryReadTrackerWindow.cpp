#include "MemoryReadTrackerWindow.h"

#include "MemoryScanner.h"
#include "MemoryScannerWindow.h"

#include <unordered_set>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <cstdio>
#include <cstring>


#include "imgui.h"
#include "MemoryTools.h"
#include <windows.h>

namespace DosBoxMemoryTools
{
    MemoryReadTrackerWindow::
        MemoryReadTrackerWindow(
            MemoryScanner& scanner,
            MemoryScannerWindow& scannerWindow,
            const std::string& gameId
        )
        : m_scanner(
            scanner
        ),
        m_scannerWindow(
            scannerWindow
        ),
        m_gameId(
            gameId
        )
    {
        loadSession();
    }

    bool MemoryTools::refreshMemory()
    {
        return m_scannerWindow.refreshMemory();
    }

    void MemoryReadTrackerWindow::draw(
        bool* isOpen
    )
    {
        if (isOpen &&
            !*isOpen)
        {
            return;
        }

        if (!ImGui::Begin(
            "DOSBox Memory Read Tracker",
            isOpen
        ))
        {
            ImGui::End();
            return;
        }



        ImGui::TextUnformatted(
            "Idle"
        );

        ImGui::PushID(
            "IdleRecord"
        );

        if (m_idleRecordButton.draw())
        {
            if (m_idleRecordButton.recording())
            {
                m_previousAttackOnlyReadAddresses =
                    m_attackOnlyReadAddresses;

                m_scanner.clearReadTracking();
                m_scanner.startReadTracking();
            }
            else
            {
                m_scanner.stopReadTracking();

                m_scanner.getReadTrackingAddresses(
                    m_idleReadAddresses
                );
            }
        }

        ImGui::PopID();

        ImGui::TextUnformatted(
            "Attack"
        );

        ImGui::PushID(
            "AttackRecord"
        );

        if (m_attackRecordButton.draw())
        {
            if (m_attackRecordButton.recording())
            {
                m_scanner.clearReadTracking();
                m_scanner.startReadTracking();
            }
            else
            {
                m_scanner.stopReadTracking();

                m_scanner.getReadTrackingAddresses(
                    m_attackReadAddresses
                );

                m_scanner.getReadTrackingInstructions(
                    m_attackReadInstructions
                );
            }
        }

		ImGui::PopID();

        if (ImGui::Button(
            "Compare"
        ))
        {
            m_previousAttackOnlyReadAddresses =
                m_attackOnlyReadAddresses;

            m_attackOnlyReadAddresses.clear();

            const std::unordered_set<size_t> idleAddresses(
                m_idleReadAddresses.begin(),
                m_idleReadAddresses.end()
            );

            for (const size_t address :
            m_attackReadAddresses)
            {
                if (idleAddresses.find(address) ==
                    idleAddresses.end())
                {
                    m_attackOnlyReadAddresses.push_back(
                        address
                    );
                }
            }
            m_scanner.setCandidatesFromAddresses(
                m_attackOnlyReadAddresses
            );
        }

        if (ImGui::Button(
            "Intersect Attack Only"
        ))
        {
            std::unordered_set<size_t> currentAddresses(
                m_attackOnlyReadAddresses.begin(),
                m_attackOnlyReadAddresses.end()
            );

            std::vector<size_t> intersection;

            for (size_t address :
            m_previousAttackOnlyReadAddresses)
            {
                if (currentAddresses.contains(
                    address
                ))
                {
                    intersection.push_back(
                        address
                    );
                }
            }

            m_attackOnlyReadAddresses =
                std::move(
                    intersection
                );

            m_scanner.setCandidatesFromAddresses(
                m_attackOnlyReadAddresses
            );
        }

        ImGui::Text(
            "Idle addresses: %zu",
            m_idleReadAddresses.size()
        );

        ImGui::Text(
            "Attack addresses: %zu",
            m_attackReadAddresses.size()
        );

        ImGui::Text(
            "Attack only: %zu",
            m_attackOnlyReadAddresses.size()
        );



        ImGui::SameLine();

        if (ImGui::Button("Previous - Current"))
        {
            std::vector<size_t> difference;

            for (const size_t address :
            m_previousAttackOnlyReadAddresses)
            {
                if (std::find(
                    m_attackOnlyReadAddresses.begin(),
                    m_attackOnlyReadAddresses.end(),
                    address
                ) == m_attackOnlyReadAddresses.end())
                {
                    difference.push_back(address);
                }
            }

            m_attackOnlyReadAddresses =
                std::move(difference);
        }

        if (ImGui::Button(
            "Pin Attack Only"
        ))
        {
            m_scannerWindow.scanner().
                setCandidatesFromAddresses(
                    m_attackOnlyReadAddresses
                );

            m_scannerWindow.pinAddresses(
                m_attackOnlyReadAddresses
            );
        }

        if (ImGui::Button(
            "Pin Previous Attack Only"
        ))
        {
            m_scannerWindow.pinAddresses(
                m_previousAttackOnlyReadAddresses
            );
        }

        ImGui::Text(
            "Previous attack only: %zu",
            m_previousAttackOnlyReadAddresses.size()
        );

        ImGui::Separator();

        ImGui::Checkbox(
            "Limit Address Range",
            &m_limitAddressRange
        );

        ImGui::SameLine();

        ImGui::SetNextItemWidth(
            110.0f
        );

        ImGui::TextUnformatted("From");
        ImGui::SameLine();

        ImGui::SetNextItemWidth(
            110.0f
        );

        ImGui::InputText(
            "##RangeFrom",
            m_rangeStartText,
            sizeof(m_rangeStartText)
        );

        ImGui::SameLine();

        ImGui::SetNextItemWidth(
            110.0f
        );

        ImGui::TextUnformatted("To");
        ImGui::SameLine();

        ImGui::SetNextItemWidth(
            110.0f
        );

        ImGui::InputText(
            "##RangeTo",
            m_rangeEndText,
            sizeof(m_rangeEndText)
        );

        ImGui::SameLine();

        ImGui::Checkbox(
            "Limit Instruction Range",
            &m_limitInstructionRange
        );

        ImGui::SameLine();

        ImGui::TextUnformatted("From");
        ImGui::SameLine();

        ImGui::SetNextItemWidth(
            110.0f
        );

        ImGui::InputText(
            "##InstructionRangeFrom",
            m_instructionRangeStartText,
            sizeof(m_instructionRangeStartText)
        );

        ImGui::SameLine();

        ImGui::TextUnformatted("To");
        ImGui::SameLine();

        ImGui::SetNextItemWidth(
            110.0f
        );

        ImGui::InputText(
            "##InstructionRangeTo",
            m_instructionRangeEndText,
            sizeof(m_instructionRangeEndText)
        );
        size_t rangeStart = 0;
        size_t rangeEnd =
            static_cast<size_t>(-1);

        bool validRange =
            !m_limitAddressRange;

        if (m_limitAddressRange)
        {
            char* startEnd = nullptr;
            char* endEnd = nullptr;

            const unsigned long long parsedStart =
                std::strtoull(
                    m_rangeStartText,
                    &startEnd,
                    0
                );

            const unsigned long long parsedEnd =
                std::strtoull(
                    m_rangeEndText,
                    &endEnd,
                    0
                );

            if (startEnd != m_rangeStartText &&
                *startEnd == '\0' &&
                endEnd != m_rangeEndText &&
                *endEnd == '\0' &&
                parsedStart <= parsedEnd)
            {
                rangeStart =
                    static_cast<size_t>(
                        parsedStart
                        );

                rangeEnd =
                    static_cast<size_t>(
                        parsedEnd
                        );

                validRange = true;
            }
        }

        size_t instructionRangeStart = 0;
        size_t instructionRangeEnd =
            static_cast<size_t>(-1);

        bool validInstructionRange =
            !m_limitInstructionRange;

        if (m_limitInstructionRange)
        {
            char* startEnd = nullptr;
            char* endEnd = nullptr;

            const unsigned long long parsedStart =
                std::strtoull(
                    m_instructionRangeStartText,
                    &startEnd,
                    0
                );

            const unsigned long long parsedEnd =
                std::strtoull(
                    m_instructionRangeEndText,
                    &endEnd,
                    0
                );

            if (startEnd != m_instructionRangeStartText &&
                *startEnd == '\0' &&
                endEnd != m_instructionRangeEndText &&
                *endEnd == '\0' &&
                parsedStart <= parsedEnd)
            {
                instructionRangeStart =
                    static_cast<size_t>(
                        parsedStart
                        );

                instructionRangeEnd =
                    static_cast<size_t>(
                        parsedEnd
                        );

                validInstructionRange = true;
            }
        }

        size_t visibleInstructionReads = 0;

        for (const auto& entry :
            m_attackReadInstructions)
        {
            const size_t memoryAddress =
                entry.first;

            const size_t instructionAddress =
                entry.second;

            if (m_limitAddressRange &&
                validRange &&
                (memoryAddress < rangeStart ||
                    memoryAddress > rangeEnd))
            {
                continue;
            }

            if (m_limitInstructionRange &&
                validInstructionRange &&
                (instructionAddress < instructionRangeStart ||
                    instructionAddress > instructionRangeEnd))
            {
                continue;
            }

            ++visibleInstructionReads;
        }

        ImGui::Text(
            "Instruction reads: %zu / %zu",
            visibleInstructionReads,
            m_attackReadInstructions.size()
        );

        for (const auto& entry :
            m_attackReadInstructions)
        {
            const size_t memoryAddress =
                entry.first;

            const size_t instructionAddress =
                entry.second;

            if (m_limitAddressRange &&
                validRange &&
                (memoryAddress < rangeStart ||
                    memoryAddress > rangeEnd))
            {
                continue;
            }

            if (m_limitInstructionRange &&
                validInstructionRange &&
                (instructionAddress < instructionRangeStart ||
                    instructionAddress > instructionRangeEnd))
            {
                continue;
            }

            ImGui::Text(
                "0x%zX -> 0x%zX",
                memoryAddress,
                instructionAddress
            );
        }

        ImGui::Separator();

        ImGui::TextWrapped(
            "%s",
            m_scanner.status().c_str()
        );

        ImGui::End();
    }

    void MemoryReadTrackerWindow::saveSession() const
    {

        if (m_idleReadAddresses.empty() &&
            m_attackReadAddresses.empty() &&
            m_attackOnlyReadAddresses.empty() &&
            m_previousAttackOnlyReadAddresses.empty() &&
            m_attackReadInstructions.empty() &&
            m_scanner.candidates().empty())
        {
            return;
        }

        if (m_gameId.empty())
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
            "settings/memory_read_session_" +
            m_gameId +
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
            m_idleReadAddresses.size() <<
            '\n';

        for (const size_t address :
        m_idleReadAddresses)
        {
            file <<
                address <<
                '\n';
        }

        file <<
            "Attack\n";

        file <<
            m_attackReadAddresses.size() <<
            '\n';

        for (const size_t address :
        m_attackReadAddresses)
        {
            file <<
                address <<
                '\n';
        }

        file <<
            "AttackOnly\n";

        file <<
            m_attackOnlyReadAddresses.size() <<
            '\n';

        for (const size_t address :
        m_attackOnlyReadAddresses)
        {
            file <<
                address <<
                '\n';
        }

        const auto& candidates =
            m_scanner.candidates();

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
            m_previousAttackOnlyReadAddresses.size() <<
            '\n';

        for (const size_t address :
        m_previousAttackOnlyReadAddresses)
        {
            file <<
                address <<
                '\n';
        }

        file <<
            "AttackInstructions\n";

        file <<
            m_attackReadInstructions.size() <<
            '\n';

        for (const auto& entry :
            m_attackReadInstructions)
        {
            file <<
                entry.first << ' ' <<
                entry.second << '\n';
        }

        file <<
            "RangeSettings\n";

        file <<
            (m_limitAddressRange ? 1 : 0) <<
            '\n';

        file <<
            m_rangeStartText <<
            '\n';

        file <<
            m_rangeEndText <<
            '\n';

        file <<
            (m_limitInstructionRange ? 1 : 0) <<
            '\n';

        file <<
            m_instructionRangeStartText <<
            '\n';

        file <<
            m_instructionRangeEndText <<
            '\n';
    }

    void    MemoryReadTrackerWindow::loadSession()
    {
        if (m_gameId.empty())
        {
            return;
        }

        const std::string filename =
            "settings/memory_read_session_" +
            m_gameId +
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
            m_idleReadAddresses
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
            m_attackReadAddresses
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
            m_attackOnlyReadAddresses
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

                m_previousAttackOnlyReadAddresses.clear();

                m_previousAttackOnlyReadAddresses.reserve(
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

                    m_previousAttackOnlyReadAddresses.push_back(
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

                m_attackReadInstructions.clear();

                m_attackReadInstructions.reserve(
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

                    m_attackReadInstructions.emplace_back(
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

                    m_limitAddressRange =
                        limitAddressRange != 0;

                    m_limitInstructionRange =
                        limitInstructionRange != 0;

                    strncpy_s(
                        m_rangeStartText,
                        sizeof(m_rangeStartText),
                        rangeStart.c_str(),
                        _TRUNCATE
                    );

                    strncpy_s(
                        m_rangeEndText,
                        sizeof(m_rangeEndText),
                        rangeEnd.c_str(),
                        _TRUNCATE
                    );

                    strncpy_s(
                        m_instructionRangeStartText,
                        sizeof(m_instructionRangeStartText),
                        instructionRangeStart.c_str(),
                        _TRUNCATE
                    );

                    strncpy_s(
                        m_instructionRangeEndText,
                        sizeof(m_instructionRangeEndText),
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
            m_scanner.setCandidatesFromAddresses(
                candidates
            );
        }
    }

    void MemoryReadTrackerWindow::setGameId(
        const std::string& gameId
    )
    {
        m_gameId =
            gameId;

        loadSession();
    }
    
}