#include "DisassemblyToolbar.h"

#include <cstdlib>
#include "imgui.h"

namespace DosBoxMemoryTools
{
    void DisassemblyToolbar::draw(
        DisassemblyState& state,
        const std::vector<uint8_t>& liveMemory,
        const DisassemblyNavigate& goToAddress,
        ScannerAddress& scannerAddress
    )
    {
        ImGui::SetNextItemWidth(
            120.0f
        );

        ImGui::Text(
            "Target: 0x%zX",
            scannerAddress.value
        );

        ImGui::SameLine();

        if (ImGui::Button("Go"))
        {
            const size_t address =
                scannerAddress.value;

            if (address < liveMemory.size())
            {
                state.m_disassemblyMemory =
                    liveMemory;

                goToAddress(
                    address
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
