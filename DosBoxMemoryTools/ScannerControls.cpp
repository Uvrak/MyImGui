#include "pch.h"
#include "ScannerControls.h"

#include <cstdlib>

#include "imgui.h"

namespace DosBoxMemoryTools
{
    void ScannerControls::draw(
        ScannerAddress& address,
        ScannerRange& range
    )
    {
        static char addressText[32] =
            "0x0";

        static char rangeStartText[32] =
            "0x0";

        static char rangeEndText[32] =
            "0x0";

        address.value =
            static_cast<size_t>(
                std::strtoull(
                    addressText,
                    nullptr,
                    0
                )
                );

        range.start =
            static_cast<size_t>(
                std::strtoull(
                    rangeStartText,
                    nullptr,
                    0
                )
                );

        range.end =
            static_cast<size_t>(
                std::strtoull(
                    rangeEndText,
                    nullptr,
                    0
                )
                );

        ImGui::InputText(
            "Address",
            addressText,
            sizeof(addressText)
        );

        ImGui::Checkbox(
            "Limit Range",
            &range.enabled
        );

        if (range.enabled)
        {
            ImGui::InputText(
                "From",
                rangeStartText,
                sizeof(rangeStartText)
            );

            ImGui::InputText(
                "To",
                rangeEndText,
                sizeof(rangeEndText)
            );
        }
    }
}