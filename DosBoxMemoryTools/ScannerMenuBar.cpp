#include "pch.h"

#include <cstdlib>
#include <cstdio>

#include "ScannerMenuBar.h"

#include "imgui.h"

namespace DosBoxMemoryTools
{
    bool ScannerMenuBar::draw(
        ScannerAddress& address,
        ScannerRange& range
    )
    {
        bool changed = false;

        static char addressText[32] =
            "0x0";

        static char rangeStartText[32] =
            "0x0";

        static char rangeEndText[32] =
            "0x0";

        static bool initialized = false;

        if (!initialized)
        {
            std::snprintf(
                addressText,
                sizeof(addressText),
                "0x%zX",
                address.value
            );

            std::snprintf(
                rangeStartText,
                sizeof(rangeStartText),
                "0x%zX",
                range.start
            );

            std::snprintf(
                rangeEndText,
                sizeof(rangeEndText),
                "0x%zX",
                range.end

            );

            initialized = true;
        }

        if (!ImGui::IsAnyItemActive())
        {
            std::snprintf(
                addressText,
                sizeof(addressText),
                "0x%zX",
                address.value
            );
        }

        ImGui::SetNextItemWidth(
            120.0f
        );

        if (ImGui::InputText(
            "##ScannerAddress",
            addressText,
            sizeof(addressText)
        ))
        {
            char* end = nullptr;

            const unsigned long long value =
                std::strtoull(
                    addressText,
                    &end,
                    0
                );

            if (end != addressText &&
                *end == '\0')
            {
                address.value =
                    static_cast<size_t>(
                        value
                        );
                changed = true;
            }
        }

        ImGui::SameLine();

        if (ImGui::Checkbox(
            "Range",
            &range.enabled
        ))
        {
            changed = true;
        }

        if (range.enabled)
        {
            ImGui::SameLine();

            ImGui::SetNextItemWidth(
                120.0f
            );

            if (ImGui::InputText(
                "##ScannerRangeStart",
                rangeStartText,
                sizeof(rangeStartText)
            ))
            {
                char* end = nullptr;

                const unsigned long long value =
                    std::strtoull(
                        rangeStartText,
                        &end,
                        0
                    );

                if (end != rangeStartText &&
                    *end == '\0')
                {
                    range.start =
                        static_cast<size_t>(
                            value
                            );

                    changed = true;
                }
            }

            ImGui::SameLine();

            ImGui::SetNextItemWidth(
                120.0f
            );

            if (ImGui::InputText(
                "##ScannerRangeEnd",
                rangeEndText,
                sizeof(rangeEndText)
            ))
            {
                char* end = nullptr;

                const unsigned long long value =
                    std::strtoull(
                        rangeEndText,
                        &end,
                        0
                    );

                if (end != rangeEndText &&
                    *end == '\0')
                {
                    range.end =
                        static_cast<size_t>(
                            value
                            );
					changed = true;
                }
            }
        }
        return changed;
    }
}