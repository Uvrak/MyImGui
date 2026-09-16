#include "TraceComparisonToolbar.h"
#include "imgui.h"

namespace DosBoxMemoryTools
{
    void TraceComparisonToolbar::draw(const State& state, const Callbacks& callbacks)
    {
        if (ImGui::Button("Load A")) callbacks.loadA();
        ImGui::SameLine();
        ImGui::Text("A: %s   Records: %zu",
            state.traceAFilename()[0] ? state.traceAFilename() : "<not loaded>",
            state.traceACount());

        if (state.traceLoadPending() || state.traceLoadCount() != 0)
        {
            ImGui::Text(
                "Loaded: %zu / %zu datasets",
                state.traceLoadIndex(),
                state.traceLoadCount()
            );
        }

        if (ImGui::Button("Load B")) callbacks.loadB();
        ImGui::SameLine();
        ImGui::Text("B: %s   Records: %zu",
            state.traceBFilename()[0] ? state.traceBFilename() : "<not loaded>",
            state.traceBCount());

        if (ImGui::Button("Save A")) callbacks.saveA();
        ImGui::SameLine();
        if (ImGui::Button("Save B")) callbacks.saveB();
        ImGui::SameLine();
        if (ImGui::Button("Prev Diff")) callbacks.previousDifference();
        ImGui::SameLine();
        if (ImGui::Button("Next Diff")) callbacks.nextDifference();
        ImGui::NewLine();

        const char* registerNames[] =
        {
            "AX",
            "BX",
            "CX",
            "DX",
            "SI",
            "DI",
            "BP",
            "SP",
            "DS", 
            "ES", 
            "SS"
        };

        int selectedRegister =
            static_cast<int>(state.selectedRegister);

        ImGui::SetNextItemWidth(80.0f);

        if (ImGui::Combo(
            "Register",
            &selectedRegister,
            registerNames,
            IM_ARRAYSIZE(registerNames)
        ))
        {
            state.selectedRegister =
                static_cast<TraceRegister>(
                    selectedRegister
                    );
        }

        ImGui::SameLine();

        if (ImGui::Button("Prev Register Diff"))
        {
            callbacks.previousRegisterDifference();
        }

        ImGui::SameLine();

        if (ImGui::Button("Next Register Diff"))
        {
            callbacks.nextRegisterDifference();
        }

        ImGui::SameLine();

        if (ImGui::Button("Prev Register Change"))
        {
            callbacks.previousRegisterChange();
        }

        ImGuiIO& io = ImGui::GetIO();
        const bool focusFilterRequested = io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F);
        callbacks.keyboardNavigation();

        static char filter[256] = {};
        if (focusFilterRequested) ImGui::SetKeyboardFocusHere();
        if (ImGui::Checkbox("Collapse identical", &state.collapseIdentical))
            callbacks.collapseChanged();

        ImGui::NewLine();
        if (ImGui::SmallButton("Add to Baseline")) callbacks.addToBaseline();
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear Baseline")) callbacks.clearBaseline();
        ImGui::SameLine();
        ImGui::Text("Baseline: %zu", state.baselineCount());
        ImGui::SameLine();
        ImGui::Checkbox("Ignore Baseline", &state.ignoreBaseline);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        ImGui::InputTextWithHint("##filter", "Filter (addr or text)", filter, sizeof(filter));
        ImGui::Separator();
    }
}
