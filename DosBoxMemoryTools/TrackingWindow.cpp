#include "TrackingWindow.h"

#include "imgui.h"

namespace DosBoxMemoryTools
{
    TrackingWindow::TrackingWindow(
        MemoryScanner& scanner,
        const std::string& gameId,
        ScannerAddress& scannerAddress,
        ScannerRange& scannerRange
    )
        :
        m_scannerAddress(scannerAddress),
        m_scannerRange(scannerRange),
        m_gameId(gameId),
        m_traceTracking(
            scanner
        ),
        m_transitionTracking(
            scanner
        ),
        m_executionTracking(
            scanner,
            scannerAddress
        ), m_memoryReadTracker(
            scanner
        ),
        m_memoryWriteTracker(
            scanner
        )
    {

    }

    void TrackingWindow::draw(
        bool* isOpen
    )
    {

        if (isOpen &&
            !*isOpen)
        {
            return;
        }

        if (!ImGui::Begin(
            "Tracking",
            isOpen
        ))
        {
            ImGui::End();
            return;
        }

        if (m_activeTab ==
            TrackingTab::Trans)
        {
            m_transitionTracking.drawNavigation();
        }

        ImGui::Separator();

        if (ImGui::BeginTabBar(
            "TrackingTabs"
        ))
        {
            if (ImGui::BeginTabItem(
                "Trace"
            ))
            {
                m_activeTab =
                    TrackingTab::Trace;

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(
                "Trans"
            ))
            {
                m_activeTab =
                    TrackingTab::Trans;

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(
                "Exec"
            ))
            {
                m_activeTab =
                    TrackingTab::Exec;

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(
                "MemRd"
            ))
            {
                m_activeTab =
                    TrackingTab::MemRd;

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(
                "MemWr"
            ))
            {
                m_activeTab =
                    TrackingTab::MemWr;

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        if (m_activeTab ==
            TrackingTab::Trans)
        {
            m_transitionTracking.drawRecorder(
                m_targetText
            );
        }

        ImGui::Separator();

        ImGui::BeginChild(
            "TrackingContent",
            ImVec2(0.0f, 0.0f),
            false,
            ImGuiWindowFlags_HorizontalScrollbar
        );
        {
            switch (m_activeTab)
            {
            case TrackingTab::Trace:
            {
                m_traceTracking.draw(
                    m_scannerAddress
                );

                if (m_traceTracking.takeCompletedTrace())
                {
                    const auto& trace =
                        m_traceTracking.trace();

                    if (m_traceTracking.targetDatasetA())
                    {
                        m_traceComparisonWindow.setTraceA(
                            trace
                        );
                    }
                    else
                    {
                        m_traceComparisonWindow.setTraceB(
                            trace
                        );
                    }
                }

                m_traceComparisonWindow.setSelectedDatasetA(
                    m_traceTracking.targetDatasetA()
                );

                m_traceComparisonWindow.draw();

                break;
            }

            case TrackingTab::Trans:
                m_transitionTracking.draw(
                    m_targetText,
                    sizeof(m_targetText)
                );
                break;

            case TrackingTab::Exec:
                m_executionTracking.draw();
                break;

            case TrackingTab::MemRd:
                m_memoryReadTracker.draw(
                    m_scannerAddress,
                    m_scannerRange
                );
                break;

            case TrackingTab::MemWr:
                m_memoryWriteTracker.draw(
                    m_scannerAddress,
                    m_scannerRange
                );
            break;
            }
        }

        ImGui::EndChild();
        ImGui::End();

}


    void TrackingWindow::saveSession() const
    {
        
    }

    void TrackingWindow::setGameId(
        const std::string& gameId
    )
    {
        if (m_gameId == gameId)
        {
            return;
        }

        m_gameId =
            gameId;
    }
}


