#include "pch.h"
#include "MemoryWriteTracker.h"
#include "MemoryWritePersistence.h"

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
    {
        MemoryWritePersistence::Snapshot snapshotA;

        if (MemoryWritePersistence::load(
            "../settings/memory_write_a.bin",
            snapshotA
        ))
        {
            m_comparison.setA(
                snapshotA.captures
            );
        }

        m_rangeAStart =
            snapshotA.rangeStart;

        m_rangeAEnd =
            snapshotA.rangeEnd;

        MemoryWritePersistence::Snapshot snapshotB;

        if (MemoryWritePersistence::load(
            "..settings/memory_write_b.bin",
            snapshotB
        ))
        {
            m_comparison.setB(
                snapshotB.captures
            );

            m_rangeBStart =
                snapshotB.rangeStart;

            m_rangeBEnd =
                snapshotB.rangeEnd;
        }
    }

    void MemoryWriteTracker::draw(
        ScannerAddress& scannerAddress,
        const ScannerRange& scannerRange
    )
    {
        ImGui::Text(
            "Debug Target: 0x%zX",
            scannerAddress.value
        );

        ImGui::Text(
            "Debug Instruction: 0x%zX",
            m_scanner.lastMemoryWriteInstruction()
        );

        if (m_captureHit)
        {
            ImGui::Text(
                "Selected: I 0x%zX  W 0x%zX  V 0x%02X",
                m_capture.address,
                m_capture.writeAddress,
                static_cast<unsigned int>(
                    m_capture.writeValue
                    )
            );
        }
        else
        {
            ImGui::TextUnformatted(
                "Selected: none"
            );
        }

        ImGui::SameLine();
        if (m_recordButton.draw())
        {
            if (m_recordButton.recording())
            {
                const size_t startAddress =
                    scannerRange.enabled
                    ? scannerRange.start
                    : scannerAddress.value;

                const size_t endAddress =
                    scannerRange.enabled
                    ? scannerRange.end
                    : scannerAddress.value;

                if (startAddress <= endAddress)
                {
                    if (m_scanner.setMemoryWriteWatchRange(
                        startAddress,
                        endAddress
                    ))
                    {
                        m_captureHit = false;
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
        }

        ImGui::Columns(
            2,
            "MemoryWriteSetColumns",
            true
        );

        // ----- A -----

        if (ImGui::Button("Set A"))
        {
            m_comparison.setA(
                m_captures
            );

            MemoryWritePersistence::Snapshot snapshotA;

            snapshotA.rangeStart =
                scannerRange.start;

            snapshotA.rangeEnd =
                scannerRange.end;

            snapshotA.captures =
                m_captures;

            MemoryWritePersistence::save(
                "../settings/memory_write_a.bin",
                snapshotA
            );

            m_rangeAStart =
                snapshotA.rangeStart;

            m_rangeAEnd =
                snapshotA.rangeEnd;
        }

        ImGui::SameLine();

        ImGui::Text(
            "Range: 0x%zX - 0x%zX",
            m_rangeAStart,
            m_rangeAEnd
        );

        ImGui::Text(
            "A: %zu",
            m_comparison.a().size()
        );

        // ----- B -----

        ImGui::NextColumn();

        if (ImGui::Button("Set B"))
        {
            m_comparison.setB(
                m_captures
            );

            MemoryWritePersistence::Snapshot snapshotB;

            snapshotB.rangeStart =
                scannerRange.start;

            snapshotB.rangeEnd =
                scannerRange.end;

            snapshotB.captures =
                m_captures;

            MemoryWritePersistence::save(
                "../settings/memory_write_b.bin",
                snapshotB
            );

            m_rangeBStart =
                snapshotB.rangeStart;

            m_rangeBEnd =
                snapshotB.rangeEnd;
        }

        ImGui::SameLine();

        ImGui::Text(
            "Range: 0x%zX - 0x%zX",
            m_rangeBStart,
            m_rangeBEnd
        );

        ImGui::Text(
            "B: %zu",
            m_comparison.b().size()
        );

        ImGui::Columns(
            1
        );

        m_comparison.draw(
            scannerAddress
        );

        if (m_captureHit)
        {
            ImGui::Separator();

            ImGui::Text(
                "Address: 0x%zX   CS:IP %04X:%04X",
                m_capture.address,
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
                "DS=%04X ES=%04X SS=%04X   Written=%02X",
                static_cast<unsigned int>(
                    m_capture.registers.ds
                    ),
                static_cast<unsigned int>(
                    m_capture.registers.es
                    ),
                static_cast<unsigned int>(
                    m_capture.registers.ss
                    ),
                static_cast<unsigned int>(
                    m_capture.writeValue
                    )
            );

            ImGui::TextUnformatted(
                "Bytes:"
            );

            for (size_t i = 0;
                i < m_capture.bytes.size();
                ++i)
            {
                ImGui::SameLine();

                ImGui::Text(
                    "%02X",
                    static_cast<unsigned int>(
                        m_capture.bytes[i]
                        )
                );
            }

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
                    static_cast<unsigned int>(m_capture.stackBytes[i + 0]),
                    static_cast<unsigned int>(m_capture.stackBytes[i + 1]),
                    static_cast<unsigned int>(m_capture.stackBytes[i + 2]),
                    static_cast<unsigned int>(m_capture.stackBytes[i + 3]),
                    static_cast<unsigned int>(m_capture.stackBytes[i + 4]),
                    static_cast<unsigned int>(m_capture.stackBytes[i + 5]),
                    static_cast<unsigned int>(m_capture.stackBytes[i + 6]),
                    static_cast<unsigned int>(m_capture.stackBytes[i + 7])
                );
            }

            ImGui::Separator();
        }

        // Keep the record button above the scrolling captures and details.
        ImGui::BeginChild(
            "MemoryWriteRecords",
            ImVec2(0.0f, 0.0f),
            false,
            ImGuiWindowFlags_HorizontalScrollbar
        );

        bool memoryWriteHit =
            false;

        if (m_scanner.getMemoryWriteWatchHit(
            memoryWriteHit
        ))
        {
            if (memoryWriteHit &&
                m_recordButton.recording())
            {
                m_recordButton.stop();

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

            ImGui::Text(
                "Capture Count: %zu",
                m_captures.size()
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

                size_t markerIndex = 0;

                if (m_scanner.getMemoryWriteMarker(
                    markerIndex
                ) &&
                    index + 1 == markerIndex)
                {
                    ImGui::Separator();
                    ImGui::TextUnformatted(
                        "----- WRITE MARKER -----"
                    );
                    ImGui::Separator();
                }

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

                char captureText[128];

                std::snprintf(
                    captureText,
                    sizeof(captureText),
                    "%zu  I: 0x%zX  W: 0x%zX  V: 0x%02X",
                    index,
                    m_captures[index].address,
                    m_captures[index].writeAddress,
                    static_cast<unsigned int>(
                        m_captures[index].writeValue
                        )
                );

                ImGui::PushID(
                    static_cast<int>(
                        index
                        )
                );

                ImGui::Selectable(
                    captureText,
                    false,
                    ImGuiSelectableFlags_AllowDoubleClick
                );

                if (ImGui::IsItemClicked(
                    ImGuiMouseButton_Left
                ))
                {
                    m_capture =
                        m_captures[index];

                    m_captureHit =
                        true;
                }

                if (ImGui::IsItemHovered() &&
                    ImGui::IsMouseDoubleClicked(
                        ImGuiMouseButton_Left
                    ))
                {
                    
                }

                ImGui::PopID();

                if (valueChanged)
                {
                    ImGui::PopStyleColor();
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
