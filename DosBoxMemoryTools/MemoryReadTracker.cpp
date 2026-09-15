#include "MemoryReadTracker.h"

#include <cstdio>

#include "imgui.h"

namespace DosBoxMemoryTools
{
    MemoryReadTracker::MemoryReadTracker(
        MemoryScanner& scanner
    )
        :
        m_scanner(
            scanner
        )
    {}

    void MemoryReadTracker::draw(
        ScannerAddress& scannerAddress,
        const ScannerRange& scannerRange
    )
    {
        ImGui::Text(
            "Target: 0x%zX",
            scannerAddress.value
        );

        if (m_recordButton.draw())
        {
            if (m_recordButton.recording())
            {
                m_captureHit =
                    false;

                m_scanner.setMemoryReadWatchTarget(
                    scannerAddress.value
                );
            }
            else
            {
                m_scanner.clearMemoryReadWatch();
            }
        }

        if (m_recordButton.recording())
        {
            bool hit = false;

            if (m_scanner.getMemoryReadWatchHit(
                hit
            ))
            {
                m_captureHit =
                    hit;

                if (hit)
                {
                    m_scanner.getMemoryReadWatchCapture(
                        m_capture
                    );
                }
            }
        }

        if (m_captureHit)
        {
            char csIpText[64];

            std::snprintf(
                csIpText,
                sizeof(csIpText),
                "HIT: %04X:%04X",
                m_capture.cs,
                m_capture.ip
            );

            ImGui::Selectable(
                csIpText,
                false,
                ImGuiSelectableFlags_AllowDoubleClick
            );

            if (ImGui::IsItemHovered() &&
                ImGui::IsMouseDoubleClicked(
                    ImGuiMouseButton_Left
                ))
            {
                scannerAddress.value =
                    static_cast<size_t>(
                        m_capture.cs
                        ) * 0x10 +
                    m_capture.ip;
            }

            ImGui::SameLine();

            char addressText[64];

            std::snprintf(
                addressText,
                sizeof(addressText),
                "Address: 0x%zX",
                m_capture.readAddress
            );

            ImGui::Selectable(
                addressText,
                false,
                ImGuiSelectableFlags_AllowDoubleClick
            );

            if (ImGui::IsItemHovered() &&
                ImGui::IsMouseDoubleClicked(
                    ImGuiMouseButton_Left
                ))
            {
                scannerAddress.value =
                    m_capture.address;
            }
        }
    }
}