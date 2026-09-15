#include "pch.h"
#include "MemoryScannerWriteValuePopup.h"
#include "MemoryScannerWindow.h"

#include "imgui.h"

namespace DosBoxMemoryTools
{
    void MemoryScannerWriteValuePopup::draw(MemoryScannerWindow& window)
    {
        if (window.m_showWriteValuePopup)
        {
            ImGui::OpenPopup(
                "Write Memory Value"
            );

            window.m_showWriteValuePopup = false;
        }

        ImGui::SetNextWindowPos(
            ImGui::GetMousePos(),
            ImGuiCond_Appearing
        );

        if (ImGui::BeginPopupModal(
            "Write Memory Value",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize
        ))
        {
            ImGui::InputScalar(
                "Value",
                ImGuiDataType_U32,
                &window.m_writeValue,
                nullptr,
                nullptr,
                "%X",
                ImGuiInputTextFlags_CharsHexadecimal
            );

            switch (window.m_valueType)
            {
            case MemoryValueType::Byte:
                if (window.m_writeValue > 0xFFu)
                {
                    window.m_writeValue = 0xFFu;
                }
                break;

            case MemoryValueType::Short:
                if (window.m_writeValue > 0xFFFFu)
                {
                    window.m_writeValue = 0xFFFFu;
                }
                break;

            case MemoryValueType::Int:
                break;
            }


            if (ImGui::Button(
                "Write"
            ))
            {
                size_t captureCount = 0;

                if (window.m_scanner.getMemoryWriteWatchCaptureCount(
                    captureCount
                ))
                {
                    window.m_scanner.setMemoryWriteMarker(
                        captureCount
                    );
                }

                if (window.m_scanner.writeValue(
                    window.m_writeAddress,
                    static_cast<uint32_t>(window.m_writeValue),
                    window.m_valueType
                ))
                {
                    window.m_scanner.refreshValues(
                        window.m_valueType
                    );
                    window.refreshPinnedDisplayValues();

                    if (window.m_dosBoxView != nullptr)
                    {
                        window.m_dosBoxView->requestRefresh();
                    }

                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }

    }
}
