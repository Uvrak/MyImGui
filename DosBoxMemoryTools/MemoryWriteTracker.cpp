#include "pch.h"
#include "MemoryWriteTracker.h"

#include <cstdio>
#include <cstdlib>

#include "imgui.h"

namespace DosBoxMemoryTools
{
    MemoryWriteTracker::MemoryWriteTracker(
        MemoryScanner& scanner
    )
        :
        m_scanner(
            scanner
        )
    {}

    void MemoryWriteTracker::draw(
        char* targetText,
        size_t targetTextSize
    )
    {
        if (m_recordButton.draw())
        {
            if (m_recordButton.recording())
            {
                char* end = nullptr;

                const unsigned long long targetAddress =
                    std::strtoull(
                        targetText,
                        &end,
                        0
                    );

                if (end != targetText &&
                    *end == '\0')
                {
                    if (m_scanner.setMemoryWriteWatchTarget(
                        static_cast<size_t>(
                            targetAddress
                            )
                    ))
                    {
                        m_captureHit =
                            false;
                    }
                    else
                    {
                        m_recordButton.stop();
                    }
                }
                else
                {
                    m_recordButton.stop();
                }
            }
            else
            {
                m_scanner.clearMemoryWriteWatch();
            }
        }

        // Keep the record button above the scrolling captures and details.
        ImGui::BeginChild(

        // Keep the record button above the scrolling captures and details.
        ImGui::BeginChild(
            "MemoryWriteRecords",
            ImVec2(0.0f, 0.0f),
            false,
            ImGuiWindowFlags_HorizontalScrollbar
        ));

        bool memoryWriteHit =
            false;

        if (m_scanner.getMemoryWriteWatchHit(
            memoryWriteHit
        ))
        {
            if (memoryWriteHit &&
                m_recordButton.recording())
            {
                size_t captureCount = 0;

                m_scanner.getMemoryWriteWatchCaptureCount(
                    captureCount
                );

                m_captures.clear();

                m_captures.reserve(
                    captureCount
                );

                for (size_t i = 0;
                    i < captureCount;
                    ++i)
                {
                    RuntimeInstruction capture{};

                    if (m_scanner.getMemoryWriteWatchCapture(
                        i,
                        capture
                    ))
                    {
                        m_captures.push_back(
                            capture
                        );
                    }
                }

                ImGui::Text(
                    "Capture Count: %zu",
                    captureCount
                );

                ImGui::Separator();

                ImGui::Text(
                    "Captured Writes: %zu",
                    m_captures.size()
                );

                for (size_t i = m_captures.size();
                    i > 0;
                    --i)
                {
                    const size_t index =
                        i - 1;

                    const bool valueChanged =
                        index + 1 < m_captures.size() &&
                        m_captures[index].writeValue !=
                        m_captures[index + 1].writeValue;

                    if (valueChanged)
                    {
                        ImGui::PushStyleColor(
                            ImGuiCol_Text,
                            ImVec4(
                                1.0f,
                                1.0f,
                                0.0f,
                                1.0f
                            )
                        );
                    }

                    ImGui::Text(
                        "%zu  Address: 0x%zX  Value: 0x%02X",
                        index,
                        m_captures[index].address,
                        static_cast<unsigned int>(
                            m_captures[index].writeValue
                            )
                    );

                    if (valueChanged)
                    {
                        ImGui::PopStyleColor();
                    }
                }
                RuntimeInstruction instruction{};

                if (captureCount > 0 &&
                    m_scanner.getMemoryWriteWatchCapture(
                        captureCount - 1,
                        instruction
                    ))
                {
                    m_valueChanged =
                        m_hasPreviousValue &&
                        instruction.writeValue !=
                        m_previousValue;

                    m_previousValue =
                        instruction.writeValue;

                    m_hasPreviousValue =
                        true;

                    m_capture =
                        instruction;

                    m_captureHit =
                        true;
                }
            }
        }

        if (m_scanner.getMemoryWriteWatchHit(
            memoryWriteHit
        ))
        {
            ImGui::Text(
                "State: %s",
                memoryWriteHit
                ? "HIT"
                : m_recordButton.recording()
                ? "ARMED / NO HIT"
                : "IDLE"
            );
        }
        else
        {
            ImGui::TextDisabled(
                "State unavailable."
            );
        }

        ImGui::Text(
            "Scanner status: %s",
            m_scanner.status().c_str()
        );

        if (!m_captureHit)
        {
            ImGui::EndChild();
            return;
        }

        ImGui::Separator();

        char addressText[64];

        std::snprintf(
            addressText,
            sizeof(addressText),
            "Address: 0x%zX",
            m_capture.address
        );

        ImGui::Selectable(
            addressText,
            false,
            ImGuiSelectableFlags_AllowDoubleClick,
            ImVec2(
                ImGui::CalcTextSize(
                    addressText
                ).x,
                0.0f
            )
        );

        if (ImGui::IsItemHovered() &&
            ImGui::IsMouseDoubleClicked(
                ImGuiMouseButton_Left
            ))
        {
            std::snprintf(
                targetText,
                targetTextSize,
                "0x%zX",
                m_capture.address
            );
        }

        ImGui::Text(
            "CS:IP %04X:%04X",
            static_cast<unsigned int>(
                m_capture.cs
                ),
            static_cast<unsigned int>(
                m_capture.ip
                )
        );

        ImGui::Text(
            "AX=%04X BX=%04X CX=%04X DX=%04X",
            static_cast<unsigned int>(
                m_capture.registers.ax
                ),
            static_cast<unsigned int>(
                m_capture.registers.bx
                ),
            static_cast<unsigned int>(
                m_capture.registers.cx
                ),
            static_cast<unsigned int>(
                m_capture.registers.dx
                )
        );

        ImGui::Text(
            "SI=%04X DI=%04X BP=%04X SP=%04X",
            static_cast<unsigned int>(
                m_capture.registers.si
                ),
            static_cast<unsigned int>(
                m_capture.registers.di
                ),
            static_cast<unsigned int>(
                m_capture.registers.bp
                ),
            static_cast<unsigned int>(
                m_capture.registers.sp
                )
        );

        ImGui::Text(
            "DS=%04X ES=%04X SS=%04X",
            static_cast<unsigned int>(
                m_capture.registers.ds
                ),
            static_cast<unsigned int>(
                m_capture.registers.es
                ),
            static_cast<unsigned int>(
                m_capture.registers.ss
                )
        );

        if (m_valueChanged)
        {
            ImGui::PushStyleColor(
                ImGuiCol_Text,
                ImVec4(
                    1.0f,
                    1.0f,
                    0.0f,
                    1.0f
                )
            );
        }

        ImGui::Text(
            "Written Value: 0x%02X",
            static_cast<unsigned int>(
                m_capture.writeValue
                )
        );

        if (m_valueChanged)
        {
            ImGui::PopStyleColor();
        }

        ImGui::Separator();

        ImGui::TextUnformatted(
            "Stack at SS:SP:"
        );

        for (size_t i = 0;
            i < m_capture.stackBytes.size();
            i += 8)
        {
            ImGui::Text(
                "+%02zX: %02X %02X %02X %02X %02X %02X %02X %02X",
                i,
                static_cast<unsigned int>(
                    m_capture.stackBytes[i + 0]
                    ),
                static_cast<unsigned int>(
                    m_capture.stackBytes[i + 1]
                    ),
                static_cast<unsigned int>(
                    m_capture.stackBytes[i + 2]
                    ),
                static_cast<unsigned int>(
                    m_capture.stackBytes[i + 3]
                    ),
                static_cast<unsigned int>(
                    m_capture.stackBytes[i + 4]
                    ),
                static_cast<unsigned int>(
                    m_capture.stackBytes[i + 5]
                    ),
                static_cast<unsigned int>(
                    m_capture.stackBytes[i + 6]
                    ),
                static_cast<unsigned int>(
                    m_capture.stackBytes[i + 7]
                    )
            );
        }
        ImGui::EndChild();
    }
}
