#include "pch.h"
#include "DragDropReorder.h"

#include "imgui.h"

namespace MyImGui
{
    namespace
    {
        constexpr const char* ReorderPayloadType =
            "MYIMGUI_REORDER";
    }

    DragDropReorder::DragDropReorder(
        DragDropReorderOptions options
    )
        : m_options(options)
    {}


    void DragDropReorder::begin()
    {
        m_sourceIndex.reset();
        m_insertionIndex.reset();
    }

    void DragDropReorder::handleItem(
        std::size_t itemIndex
    )
    {
        if (!m_options.draggable)
        {
            return;
        }

        if (ImGui::IsItemActive() &&
            ImGui::IsMouseDown(
                ImGuiMouseButton_Left
            ))
        {
            ImGui::SetMouseCursor(
                ImGuiMouseCursor_Hand
            );
        }
        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetMouseCursor(
                ImGuiMouseCursor_Hand
            );

            const Payload payload{
                this,
                itemIndex
            };

            ImGui::SetDragDropPayload(
                ReorderPayloadType,
                &payload,
                sizeof(payload),
                ImGuiCond_Once
            );

            ImGui::Text(
                "Move item %zu",
                itemIndex + 1
            );

            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget())
        {
            const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload(
                    ReorderPayloadType
                );

            if (payload != nullptr &&
                payload->IsDelivery() &&
                payload->DataSize == sizeof(Payload))
            {
                const Payload& reorderPayload =
                    *static_cast<const Payload*>(
                        payload->Data
                        );

                if (reorderPayload.owner == this)
                {
                    const ImVec2 itemMin =
                        ImGui::GetItemRectMin();

                    const ImVec2 itemMax =
                        ImGui::GetItemRectMax();

                    const ImVec2 mousePosition =
                        ImGui::GetMousePos();

                    bool insertBefore = false;

                    if (m_options.axis ==
                        ReorderAxis::Horizontal)
                    {
                        const float centerX =
                            (itemMin.x + itemMax.x) * 0.5f;

                        insertBefore =
                            mousePosition.x < centerX;
                    }
                    else
                    {
                        const float centerY =
                            (itemMin.y + itemMax.y) * 0.5f;

                        insertBefore =
                            mousePosition.y < centerY;
                    }

                    m_sourceIndex =
                        reorderPayload.sourceIndex;

                    m_insertionIndex =
                        insertBefore
                        ? itemIndex
                        : itemIndex + 1;
                }
            }

            ImGui::EndDragDropTarget();
        }
    }
}