#include "DisassemblyToolbar.h"

#include <cstdlib>
#include "imgui.h"

namespace DosBoxMemoryTools
{
    void DisassemblyToolbar::draw(DisassemblyState& state, const std::vector<uint8_t>& liveMemory, const DisassemblyNavigate& goToAddress)
    {
        ImGui::SetNextItemWidth(
            120.0f
        );

        ImGui::InputText(
            "Address",
            state.m_addressText,
            sizeof(state.m_addressText)
        );

        ImGui::SameLine();

        if (ImGui::Button("Go"))
        {
            char* end = nullptr;

            const unsigned long long address =
                std::strtoull(
                    state.m_addressText,
                    &end,
                    0
                );

            if (end != state.m_addressText &&
                *end == '\0' &&
                address < liveMemory.size())
            {
                state.m_disassemblyMemory =
                    liveMemory;

                goToAddress(
                    static_cast<size_t>(
                        address
                        )
                );

                state.m_status =
                    "Disassembly snapshot captured.";
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Refresh Snapshot"))
        {
            if (!liveMemory.empty())
            {
                state.m_disassemblyMemory =
                    liveMemory;

                state.m_scrollToTop = true;

                state.m_status =
                    "Disassembly snapshot refreshed.";
            }
        }

    }

}
