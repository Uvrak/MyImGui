#include "ExecutionCaptureWindow.h"

#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <cstring>

#include "imgui.h"

namespace DosBoxMemoryTools
{
    ExecutionCaptureWindow::
        ExecutionCaptureWindow(
            MemoryScanner& scanner
        )
        : m_scanner(
            scanner
        )
    {
        loadSession();
    }

    void ExecutionCaptureWindow::draw(
        bool* isOpen
    )
    {
        if (isOpen &&
            !*isOpen)
        {
            return;
        }

        if (!ImGui::Begin(
            "Execution Capture",
            isOpen
        ))
        {
            ImGui::End();
            return;
        }

        ImGui::SetNextItemWidth(
            140.0f
        );

        ImGui::InputText(
            "Execution Target",
            m_targetText,
            sizeof(m_targetText)
        );

        ImGui::TextUnformatted(
            "Capture"
        );

        if (m_executionRecordButton.draw())
        {
            if (m_executionRecordButton.recording())
            {
                char* end = nullptr;

                const unsigned long long targetAddress =
                    std::strtoull(
                        m_targetText,
                        &end,
                        0
                    );

                if (end != m_targetText &&
                    *end == '\0')
                {
                    if (m_scanner.setExecutionCaptureTarget(
                        static_cast<size_t>(
                            targetAddress
                            )
                    ))
                    {
                        m_captureHit = false;
                    }
                }
            }
            else
            {
                // Manueller Stop/Abbruch:
                // hier NICHT clearExecutionCapture() aufrufen.
            }
        }

        bool executionHit = false;

        if (m_scanner.getExecutionCaptureHit(
            executionHit
        ))
        {
            if (executionHit &&
                m_executionRecordButton.recording())
            {
                RuntimeInstruction instruction;

                if (m_scanner.getExecutionCapture(
                    instruction
                ))
                {
                    m_capture =
                        instruction;

                    m_captureHit =
                        true;

                    saveSession();

                    m_executionRecordButton.stop();
                }
            }

            ImGui::Text(
                "State: %s",
                executionHit
                ? "HIT"
                : m_executionRecordButton.recording()
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

        if (m_captureHit)
        {
            ImGui::Separator();

            ImGui::Text(
                "Address: 0x%zX",
                m_capture.address
            );

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

            ImGui::Separator();

            ImGui::Text(
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
        }

        ImGui::End();
    }

    void ExecutionCaptureWindow::saveSession() const
    {
        std::filesystem::create_directories(
            "settings"
        );

        std::ofstream file(
            "settings/execution_capture.cfg"
        );

        if (!file)
        {
            return;
        }

        file <<
            "ExecutionCaptureSession 1\n";

        file <<
            m_targetText <<
            '\n';

        file <<
            (m_captureHit ? 1 : 0) <<
            '\n';

        if (!m_captureHit)
        {
            return;
        }

        file <<
            m_capture.address << ' ' <<
            m_capture.cs << ' ' <<
            m_capture.ip << ' ' <<

            m_capture.registers.ax << ' ' <<
            m_capture.registers.bx << ' ' <<
            m_capture.registers.cx << ' ' <<
            m_capture.registers.dx << ' ' <<

            m_capture.registers.si << ' ' <<
            m_capture.registers.di << ' ' <<
            m_capture.registers.bp << ' ' <<
            m_capture.registers.sp << ' ' <<

            m_capture.registers.ds << ' ' <<
            m_capture.registers.es << ' ' <<
            m_capture.registers.ss <<
            '\n';

        for (const uint8_t byte :
        m_capture.bytes)
        {
            file <<
                static_cast<unsigned int>(
                    byte
                    ) <<
                ' ';
        }

        file << '\n';

        for (const uint8_t byte :
        m_capture.stackBytes)
        {
            file <<
                static_cast<unsigned int>(
                    byte
                    ) <<
                ' ';
        }

        file << '\n';
    }

    void ExecutionCaptureWindow::loadSession()
    {
        std::ifstream file(
            "settings/execution_capture.cfg"
        );

        if (!file)
        {
            return;
        }

        std::string header;

        std::getline(
            file,
            header
        );

        if (header !=
            "ExecutionCaptureSession 1")
        {
            return;
        }

        std::string target;

        if (!std::getline(
            file,
            target
        ))
        {
            return;
        }

        strncpy_s(
            m_targetText,
            sizeof(m_targetText),
            target.c_str(),
            _TRUNCATE
        );

        int captureHit = 0;

        if (!(file >>
            captureHit))
        {
            return;
        }

        m_captureHit =
            captureHit != 0;

        if (!m_captureHit)
        {
            return;
        }

        unsigned int cs = 0;
        unsigned int ip = 0;

        unsigned int ax = 0;
        unsigned int bx = 0;
        unsigned int cx = 0;
        unsigned int dx = 0;

        unsigned int si = 0;
        unsigned int di = 0;
        unsigned int bp = 0;
        unsigned int sp = 0;

        unsigned int ds = 0;
        unsigned int es = 0;
        unsigned int ss = 0;

        if (!(file >>
            m_capture.address >>
            cs >>
            ip >>

            ax >>
            bx >>
            cx >>
            dx >>

            si >>
            di >>
            bp >>
            sp >>

            ds >>
            es >>
            ss))
        {
            m_captureHit = false;
            return;
        }

        m_capture.cs =
            static_cast<uint16_t>(
                cs
                );

        m_capture.ip =
            static_cast<uint16_t>(
                ip
                );

        m_capture.registers.ax =
            static_cast<uint16_t>(
                ax
                );

        m_capture.registers.bx =
            static_cast<uint16_t>(
                bx
                );

        m_capture.registers.cx =
            static_cast<uint16_t>(
                cx
                );

        m_capture.registers.dx =
            static_cast<uint16_t>(
                dx
                );

        m_capture.registers.si =
            static_cast<uint16_t>(
                si
                );

        m_capture.registers.di =
            static_cast<uint16_t>(
                di
                );

        m_capture.registers.bp =
            static_cast<uint16_t>(
                bp
                );

        m_capture.registers.sp =
            static_cast<uint16_t>(
                sp
                );

        m_capture.registers.ds =
            static_cast<uint16_t>(
                ds
                );

        m_capture.registers.es =
            static_cast<uint16_t>(
                es
                );

        m_capture.registers.ss =
            static_cast<uint16_t>(
                ss
                );

        for (uint8_t& byte :
            m_capture.bytes)
        {
            unsigned int value = 0;

            if (!(file >> value) ||
                value > 255)
            {
                m_captureHit = false;
                return;
            }

            byte =
                static_cast<uint8_t>(
                    value
                    );
        }

        for (uint8_t& byte :
            m_capture.stackBytes)
        {
            unsigned int value = 0;

            if (!(file >> value) ||
                value > 255)
            {
                m_captureHit = false;
                return;
            }

            byte =
                static_cast<uint8_t>(
                    value
                    );
        }
    }
}