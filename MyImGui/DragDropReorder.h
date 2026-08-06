#pragma once

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace MyImGui
{
    enum class ReorderAxis
    {
        Horizontal,
        Vertical
    };

    struct DragDropReorderOptions
    {
        bool draggable = true;
        ReorderAxis axis = ReorderAxis::Horizontal;
    };

    class DragDropReorder
    {
    public:
        explicit DragDropReorder(
            DragDropReorderOptions options = {}
        );

        void begin();

        void handleItem(
            std::size_t itemIndex
        );

        template<typename Item>
        bool apply(std::vector<Item>& items)
        {
            if (!m_sourceIndex.has_value() ||
                !m_insertionIndex.has_value())
            {
                return false;
            }

            std::size_t source =
                *m_sourceIndex;

            std::size_t insertion =
                *m_insertionIndex;

            m_sourceIndex.reset();
            m_insertionIndex.reset();

            if (source >= items.size() ||
                insertion > items.size())
            {
                return false;
            }

            Item movedItem =
                std::move(items[source]);

            items.erase(
                items.begin() + source
            );

            if (source < insertion)
            {
                --insertion;
            }

            if (insertion > items.size())
            {
                insertion = items.size();
            }

            items.insert(
                items.begin() + insertion,
                std::move(movedItem)
            );

            return source != insertion;
        }

    private:
        struct Payload
        {
            const DragDropReorder* owner = nullptr;
            std::size_t sourceIndex = 0;
        };

        DragDropReorderOptions m_options;

        std::optional<std::size_t> m_sourceIndex;
        std::optional<std::size_t> m_insertionIndex;
    };
}