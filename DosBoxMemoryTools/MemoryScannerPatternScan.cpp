#include "pch.h"
#include "MemoryScannerPatternScan.h"

#include "MemoryScanner.h"

#include <sstream>
#include <string>
#include <vector>

#include "imgui.h"

namespace DosBoxMemoryTools
{
    MemoryScannerPatternScan::MemoryScannerPatternScan(
        MemoryScanner& scanner
    )
        :
        m_scanner(
            scanner
        )
    {}

    void MemoryScannerPatternScan::draw()
    {
        ImGui::SetNextItemWidth(
            300.0f
        );

        ImGui::InputText(
            "Pattern",
            m_patternText,
            sizeof(m_patternText)
        );

        ImGui::SameLine();

        if (ImGui::Button(
            "Scan Pattern"
        ))
        {
            std::vector<uint8_t> pattern;

            std::istringstream stream(
                m_patternText
            );

            std::string token;

            while (stream >> token)
            {
                try
                {
                    const unsigned long value =
                        std::stoul(
                            token,
                            nullptr,
                            16
                        );

                    if (value > 0xFF)
                    {
                        pattern.clear();
                        break;
                    }

                    pattern.push_back(
                        static_cast<uint8_t>(
                            value
                            )
                    );
                }
                catch (...)
                {
                    pattern.clear();
                    break;
                }
            }

            if (!pattern.empty())
            {
                m_scanner.scanBytePattern(
                    pattern
                );
            }
        }
    }
}