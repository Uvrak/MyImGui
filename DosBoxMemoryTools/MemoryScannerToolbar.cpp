#include "pch.h"
#include "MemoryScannerToolbar.h"
#include "MemoryScannerWindow.h"

#include "imgui.h"

namespace DosBoxMemoryTools
{
    void MemoryScannerToolbar::draw(MemoryScannerWindow& window, bool& liveView)
    {
        if (window.m_toolbarWindow.begin())
        {
            window.m_toolbarLayout.begin();

            const ImVec2 framePadding =
                ImGui::GetStyle().FramePadding;

            // New Scan
            const ImVec2 newScanSize(
                ImGui::CalcTextSize("New Scan").x +
                framePadding.x * 2.0f,
                ImGui::GetFrameHeight()
            );

            window.m_toolbarLayout.beginItem(
                newScanSize
            );

            ImGui::Text(
                "Candidates: %zu",
                window.m_scanner.candidates().size()
            );

            if (ImGui::Button(
                "New Scan"
            ))
            {
                    if (window.m_scannerRange.enabled &&
                    window.m_scannerRange.start <=
                    window.m_scannerRange.end)
                {
                    window.m_scanner.setScanRange(
                        window.m_scannerRange.start,
                        window.m_scannerRange.end
                    );
                }
                else
                {
                    window.m_scanner.clearScanRange();
                }

                window.m_scanner.reset();

                MemoryScanMode initialMode =
                    window.m_scanMode;

                if (window.m_scanMode ==
                    MemoryScanMode::Changed ||
                    window.m_scanMode ==
                    MemoryScanMode::Unchanged ||
                    window.m_scanMode ==
                    MemoryScanMode::Increased ||
                    window.m_scanMode ==
                    MemoryScanMode::Decreased)
                {
                    initialMode =
                        MemoryScanMode::UnknownInitialValue;
                }

                window.m_scanner.scan(
                    initialMode,
                    window.m_valueType,
                    static_cast<uint32_t>(
                        window.m_exactValue
                        )
                );
            }

            window.m_toolbarLayout.endItem();


            // Scan mode
            const char* scanModes[] =
            {
                "Unknown Initial Value",
                "Exact Value",
                "Changed",
                "Unchanged",
                "Increased",
                "Decreased"
            };

            int selectedMode =
                static_cast<int>(
                    window.m_scanMode
                    ) - 1;

            window.m_toolbarLayout.beginItem(
                ImVec2(
                    160.0f,
                    ImGui::GetFrameHeight()
                )
            );

            ImGui::SetNextItemWidth(
                160.0f
            );

            if (ImGui::Combo(
                "##ScanMode",
                &selectedMode,
                scanModes,
                IM_ARRAYSIZE(scanModes)
            ))
            {
                window.m_scanMode =
                    static_cast<
                    MemoryScanMode
                    >(
                        selectedMode + 1
                        );

                window.saveScannerSettings();
            }

            window.m_toolbarLayout.endItem();

            // Value type
            const char* valueTypes[] =
            {
                "Byte",
                "Short",
                "Int"
            };

            int selectedValueType =
                static_cast<int>(
                    window.m_valueType
                    );

            window.m_toolbarLayout.beginItem(
                ImVec2(
                    80.0f,
                    ImGui::GetFrameHeight()
                )
            );

            ImGui::SetNextItemWidth(
                80.0f
            );

            if (ImGui::Combo(
                "##ValueType",
                &selectedValueType,
                valueTypes,
                IM_ARRAYSIZE(valueTypes)
            ))
            {
                window.m_valueType =
                    static_cast<
                    MemoryValueType
                    >(
                        selectedValueType
                        );

                window.saveScannerSettings();
            }

            window.m_toolbarLayout.endItem();

            // Previous

            // Exact value
            if (window.m_scanMode ==
                MemoryScanMode::ExactValue)
            {
                window.m_toolbarLayout.beginItem(
                    ImVec2(
                        80.0f,
                        ImGui::GetFrameHeight()
                    )
                );

                ImGui::SetNextItemWidth(
                    120.0f
                );

                if (ImGui::InputInt(
                    "##ExactValue",
                    &window.m_exactValue
                ))
                {
                    window.saveScannerSettings();
                }
            }

            // Next Scan
            const ImVec2 nextScanSize(
                ImGui::CalcTextSize("Next Scan").x +
                framePadding.x * 2.0f,
                ImGui::GetFrameHeight()
            );

            window.m_toolbarLayout.beginItem(
                nextScanSize
            );

            if (ImGui::Button(
                "Next Scan"
            ))
            {
                window.m_scanner.scan(
                    window.m_scanMode,
                    window.m_valueType,
                    static_cast<uint32_t>(
                        window.m_exactValue
                        )
                );
            }

            window.m_toolbarLayout.endItem();

            // Refresh
            const ImVec2 refreshSize(
                ImGui::CalcTextSize("Refresh").x +
                framePadding.x * 2.0f,
                ImGui::GetFrameHeight()
            );

            window.m_toolbarLayout.beginItem(
                refreshSize
            );

            if (ImGui::Button("Refresh"))
            {
                window.m_scanner.refreshValues(
                    window.m_valueType
                );
                window.refreshPinnedDisplayValues();
            }

            window.m_toolbarLayout.endItem();

            // Live View
            window.m_toolbarLayout.beginItem(
                ImVec2(
                    ImGui::CalcTextSize("Live").x +
                    30.0f,
                    ImGui::GetFrameHeight()
                )
            );

            ImGui::Checkbox(
                "Live",
                &liveView
            );

            window.m_toolbarLayout.endItem();
            // Reset
            const ImVec2 resetSize(
                ImGui::CalcTextSize("Reset").x +
                framePadding.x * 2.0f,
                ImGui::GetFrameHeight()
            );

            window.m_toolbarLayout.beginItem(
                resetSize
            );

            if (ImGui::Button(
                "Reset"
            ))
            {
                window.m_scanner.reset();

                window.m_selectedAddresses.clear();
            }

            window.m_toolbarLayout.endItem();

            // Clear Filters
            const ImVec2 clearFiltersSize(
                ImGui::CalcTextSize("Clear Filters").x +
                framePadding.x * 2.0f,
                ImGui::GetFrameHeight()
            );

            window.m_toolbarLayout.beginItem(
                clearFiltersSize
            );

            if (ImGui::Button(
                "Clear Filters"
            ))
            {
                window.m_filterPrevious = false;
                window.m_previousValue = 0;

                window.m_filterCurrent = false;
                window.m_currentValue = 0;

                window.m_filterDifference = false;
                window.m_differenceValue = 0;

                window.saveScannerSettings();
            }

            window.m_toolbarLayout.endItem();

            window.m_toolbarLayout.end();
        }

        window.m_toolbarWindow.end();

    }
}
