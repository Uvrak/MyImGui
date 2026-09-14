#include "DisassemblyWindow.h"

#include <cstdio>
#include "DisassemblyToolbar.h"
#include "DisassemblySavedAddresses.h"
#include "DisassemblyCallerSearch.h"

#include "imgui.h"
#include "DisassemblyView.h"

namespace DosBoxMemoryTools
{
    DisassemblyWindow::
        DisassemblyWindow(
            MemoryReader& memoryReader,
            ScannerAddress& scannerAddress
        )
        :
        m_memoryReader(
            memoryReader
        ),
        m_scannerAddress(
            scannerAddress
        )
    {}

    void DisassemblyWindow::draw(
        bool* isOpen
    )
    {
        if (!state.m_savedAddressesLoaded)
        {
            DisassemblySavedAddresses::loadSession(state);
            state.m_savedAddressesLoaded = true;
        }

        if (isOpen &&
            !*isOpen)
        {
            return;
        }

        state.m_wasOpen = true;

        ImGui::SetNextWindowSize(
            ImVec2(
                700.0f,
                500.0f
            ),
            ImGuiCond_FirstUseEver
        );

        const bool windowVisible =
            ImGui::Begin(
                "DOSBox Disassembly",
                isOpen
            );

        if (isOpen &&
            !*isOpen)
        {
            DisassemblySavedAddresses::saveSession(state);
            state.m_wasOpen = false;
        }

        if (!windowVisible)
        {
            ImGui::End();
            return;
        }

        state.m_wasOpen = true;

        const DisassemblyNavigate navigate = [this](size_t address)
        {
            goToAddress(address);
        };

        DisassemblyToolbar::draw(
            state,
            m_memoryReader.memory(),
            navigate,
            m_scannerAddress
        );

        DisassemblySavedAddresses::draw(state, navigate);
        DisassemblyCallerSearch::drawControls(state);

        if (!state.m_status.empty())
        {
            ImGui::TextUnformatted(
                state.m_status.c_str()
            );
        }

        DisassemblyCallerSearch::drawResults(state, navigate);
        ImGui::Separator();
        DisassemblyView::draw(state, navigate);

        ImGui::End();
    }

    void DisassemblyWindow::goToAddress(
        size_t address
    )
    {
        state.m_address = address;
        state.m_hasAddress = true;
        state.m_scrollToTop = true;

        if (state.m_callerTargetText[0] == '\0')
        {
            std::snprintf(
                state.m_callerTargetText,
                sizeof(state.m_callerTargetText),
                "0x%zX",
                address
            );
        }
        std::snprintf(
            state.m_addressText,
            sizeof(state.m_addressText),
            "0x%zX",
            address
        );
    }

}
