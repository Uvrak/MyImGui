#include "pch.h"
#include "MemoryScannerWindow.h"
#include "MemoryScannerCandidateTable.h"
#include "MemoryScannerWriteValuePopup.h"
#include "MemoryScannerToolbar.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <vector>

#include "imgui.h"

#include "View.h"

namespace DosBoxMemoryTools
{
    MemoryScannerWindow::MemoryScannerWindow(
        MemoryReader& memoryReader,
        const std::string& gameId,
        DosBoxX::View* dosBoxView,
        ScannerAddress& scannerAddress,
        ScannerRange& scannerRange
    )
        :
        m_scanner(memoryReader),
        m_patternScan(m_scanner),
        m_scannerAddress(scannerAddress),
        m_scannerRange(scannerRange),
        m_gameId(gameId),
        m_dosBoxView(dosBoxView)
    {
        loadPinnedAddresses();
        loadScannerSettings();
    }

    void MemoryScannerWindow::draw(
        bool* isOpen,
        bool& liveView
    )

    {
        if (m_applyDifferenceDeleteRequested)
        {
            m_scanner.keepDifference(
                m_differenceValue
            );

            m_applyDifferenceDeleteRequested =
                false;
        }

        if (isOpen &&
            !*isOpen)
        {
            return;
        }

        ImGui::SetNextWindowSize(
            ImVec2(
                700.0f,
                520.0f
            ),
            ImGuiCond_FirstUseEver
        );

        if (!ImGui::Begin(
            "Memory Scanner",
            isOpen
        ))
        {
            ImGui::End();
            return;
        }
        MemoryScannerToolbar::draw(*this, liveView);

        ImGui::Separator();

        ImGui::SameLine();

        m_patternScan.draw();

        ImGui::Separator();

        ImGui::TextWrapped(
            "%s",
            m_scanner.
            status().
            c_str()
        );

        ImGui::Separator();

        MemoryScannerCandidateTable::draw(*this, liveView);

        MemoryScannerWriteValuePopup::draw(*this);

        ImGui::End();
    }

    void MemoryScannerWindow::pinAddresses(
        const std::vector<size_t>& addresses
    )
    {
        for (const size_t address :
        addresses)
        {
            m_pinnedAddresses.insert(
                address
            );

            m_scanner.pinAddress(
                address
            );
        }

        savePinnedAddresses();
    }

    std::string MemoryScannerWindow::
        pinnedAddressesFilePath() const
    {
        return
            "../settings/memory_pins_" +
            m_gameId +
            ".cfg";
    }

    std::string MemoryScannerWindow::
        scannerSettingsFilePath() const
    {
        return
            "../settings/memory_scanner_" +
            m_gameId +
            ".cfg";
    }

    bool MemoryScannerWindow::hasDescription(
        size_t address
    ) const
    {
        const auto it =
            m_pinnedDescriptions.find(
                address
            );

        return
            it != m_pinnedDescriptions.end() &&
            !it->second.empty();
    }
    void MemoryScannerWindow::setGameId(
        const std::string& gameId
    )
    {
        if (m_gameId == gameId)
        {
            return;
        }

        m_gameId = gameId;

        m_pinnedAddresses.clear();
        m_pinnedDescriptions.clear();
        m_scanner.clearPinnedAddresses();

        loadPinnedAddresses();
        loadScannerSettings();
    }

    bool MemoryScannerWindow::
        refreshMemory()
    {
        return m_scanner.refreshMemory();
    }

    void MemoryScannerWindow::
        refreshPinnedValues()
    {
        refreshPinnedDisplayValues();
    }

    bool MemoryScannerWindow::takeSelectedAddress(
        size_t& address
    )
    {
        if (!m_hasSelectedAddress)
        {
            return false;
        }

        address =
            m_lastSelectedAddress;

        m_hasSelectedAddress =
            false;

        return true;
    }

    MemoryScanner&
        MemoryScannerWindow::scanner()
    {
        return m_scanner;
    }

    void MemoryScannerWindow::
        loadPinnedAddresses()
    {
        std::ifstream file(
            pinnedAddressesFilePath()
        );

        if (!file.is_open())
        {
            return;
        }

        std::string line;

        while (std::getline(
            file,
            line
        ))
        {
            if (line.empty())
            {
                continue;
            }

            const size_t firstSeparator =
                line.find('|');

            if (firstSeparator ==
                std::string::npos)
            {
                continue;
            }

            const size_t secondSeparator =
                line.find(
                    '|',
                    firstSeparator + 1
                );

            if (secondSeparator ==
                std::string::npos)
            {
                continue;
            }

            const std::string addressText =
                line.substr(
                    0,
                    firstSeparator
                );

            const std::string pinnedText =
                line.substr(
                    firstSeparator + 1,
                    secondSeparator -
                    firstSeparator - 1
                );

            const std::string description =
                line.substr(
                    secondSeparator + 1
                );

            const size_t address =
                static_cast<size_t>(
                    std::stoull(
                        addressText,
                        nullptr,
                        0
                    )
                    );

            const bool pinned =
                pinnedText == "1";

            if (pinned)
            {
                m_pinnedAddresses.insert(
                    address
                );

                m_scanner.pinAddress(
                    address
                );
            }

            if (pinned ||
                !description.empty())
            {
                m_pinnedDescriptions[
                    address
                ] = description;
            }
        }
    }
    void MemoryScannerWindow::
        savePinnedAddresses() const
    {
        const std::string filename =
            pinnedAddressesFilePath();

        std::ofstream file(
            filename
        );

        if (!file.is_open())
        {
            return;
        }

        for (const size_t address :
        m_pinnedAddresses)
        {
            const auto description =
                m_pinnedDescriptions.find(
                    address
                );

            file
                << "0x"
                << std::hex
                << address
                << "|1|";

            if (description !=
                m_pinnedDescriptions.end())
            {
                file
                    << description->second;
            }

            file
                << '\n';
        }

        for (const auto& [address, description] :
            m_pinnedDescriptions)
        {
            if (description.empty())
            {
                continue;
            }

            if (m_pinnedAddresses.contains(
                address
            ))
            {
                continue;
            }

            file
                << "0x"
                << std::hex
                << address
                << "|0|"
                << description
                << '\n';
        }
    }
    
    void MemoryScannerWindow::
        loadScannerSettings()
    {
        std::ifstream file(
            scannerSettingsFilePath()
        );

        if (!file.is_open())
        {
            return;
        }

        std::string line;

        while (std::getline(
            file,
            line
        ))
        {
            const size_t separator =
                line.find('=');

            if (separator ==
                std::string::npos)
            {
                continue;
            }

            const std::string key =
                line.substr(
                    0,
                    separator
                );

            const std::string value =
                line.substr(
                    separator + 1
                );

            if (key == "ScanMode")
            {
                m_scanMode =
                    static_cast<MemoryScanMode>(
                        std::stoi(value)
                        );
            }
            else if (key == "ValueType")
            {
                m_valueType =
                    static_cast<MemoryValueType>(
                        std::stoi(value)
                        );
            }
            else if (key == "ExactValue")
            {
                m_exactValue =
                    std::stoi(value);
            }
            
            else if (key == "FilterPrevious")
            {
                m_filterPrevious =
                    std::stoi(value) != 0;
            }
            else if (key == "PreviousValue")
            {
                m_previousValue =
                    std::stoi(value);
            }
            else if (key == "FilterCurrent")
            {
                m_filterCurrent =
                    std::stoi(value) != 0;
            }
            else if (key == "CurrentValue")
            {
                m_currentValue =
                    std::stoi(value);
            }
            else if (key == "FilterDifference")
            {
                m_filterDifference =
                    std::stoi(value) != 0;
            }
            else if (key == "DifferenceValue")
            {
                m_differenceValue =
                    std::stoi(value);
            }
            else if (key == "DescriptionsFirst")
            {
                m_descriptionsFirst =
                    std::stoi(value) != 0;
            }
        }
    }

    void MemoryScannerWindow::
        saveScannerSettings() const
    {
        std::ofstream file(
            scannerSettingsFilePath()
        );

        if (!file.is_open())
        {
            return;
        }

        file
            << "ScanMode="
            << static_cast<int>(m_scanMode)
            << '\n'

            << "ValueType="
            << static_cast<int>(m_valueType)
            << '\n'

            << "ExactValue="
            << m_exactValue
            << '\n'



            << "FilterPrevious="
            << (m_filterPrevious ? 1 : 0)
            << '\n'

            << "PreviousValue="
            << m_previousValue
            << '\n'

            << "FilterCurrent="
            << (m_filterCurrent ? 1 : 0)
            << '\n'

            << "CurrentValue="
            << m_currentValue
            << '\n'

            << "FilterDifference="
            << (m_filterDifference ? 1 : 0)
            << '\n'

            << "DifferenceValue="
            << m_differenceValue
            << '\n'
            << "DescriptionsFirst="
            << (m_descriptionsFirst ? 1 : 0)
            << '\n';
    }

    void MemoryScannerWindow::
        refreshPinnedDisplayValues()
    {
        for (size_t address :
        m_pinnedAddresses)
        {
            uint32_t value = 0;

            if (m_scanner.readCurrentValue(
                address,
                m_valueType,
                value
            ))
            {
                m_pinnedDisplayValues[
                    address
                ] = value;
            }
        }
    }
}