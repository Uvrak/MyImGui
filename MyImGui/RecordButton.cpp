#include "pch.h"
#include "RecordButton.h"

#include "imgui.h"

namespace MyImGui
{
    bool RecordButton::draw(
        const char* recordingLabel
    )
    {
        const char* label =
            m_recording
            ? recordingLabel
            : "Record";

        if (m_recording)
        {
            ImGui::PushStyleColor(
                ImGuiCol_Button,
                ImVec4(
                    0.15f,
                    0.55f,
                    0.20f,
                    1.0f
                )
            );

            ImGui::PushStyleColor(
                ImGuiCol_ButtonHovered,
                ImVec4(
                    0.20f,
                    0.65f,
                    0.25f,
                    1.0f
                )
            );

            ImGui::PushStyleColor(
                ImGuiCol_ButtonActive,
                ImVec4(
                    0.10f,
                    0.45f,
                    0.15f,
                    1.0f
                )
            );
        }

        const ImVec2 textSize =
            ImGui::CalcTextSize(
                label
            );

        const float circleRadius =
            4.0f;

        const float circleSpacing =
            6.0f;

        const ImGuiStyle& style =
            ImGui::GetStyle();

        const ImVec2 buttonSize(
            style.FramePadding.x * 2.0f +
            circleRadius * 2.0f +
            circleSpacing +
            textSize.x,

            textSize.y +
            style.FramePadding.y * 2.0f
        );

        const bool clicked =
            ImGui::Button(
                "##RecordButton",
                buttonSize
            );

        const ImVec2 buttonMin =
            ImGui::GetItemRectMin();

        const ImVec2 buttonMax =
            ImGui::GetItemRectMax();

        const ImVec2 circleCenter(
            buttonMin.x +
            style.FramePadding.x +
            circleRadius,

            (buttonMin.y +
                buttonMax.y) * 0.5f
        );

        ImGui::GetWindowDrawList()->
            AddCircleFilled(
                circleCenter,
                circleRadius,
                IM_COL32(
                    255,
                    60,
                    60,
                    255
                )
            );

        const ImVec2 textPosition(
            circleCenter.x +
            circleRadius +
            circleSpacing,

            buttonMin.y +
            style.FramePadding.y
        );

        ImGui::GetWindowDrawList()->
            AddText(
                textPosition,
                ImGui::GetColorU32(
                    ImGuiCol_Text
                ),
                label
            );

        if (m_recording)
        {
            ImGui::PopStyleColor(
                3
            );
        }

        if (!clicked)
        {
            return false;
        }

        m_recording =
            !m_recording;

        return true;
    }

    bool RecordButton::recording() const
    {
        return m_recording;
    }

    void RecordButton::start()
    {
        m_recording = true;
    }

    void RecordButton::stop()
    {
        m_recording = false;
    }
}