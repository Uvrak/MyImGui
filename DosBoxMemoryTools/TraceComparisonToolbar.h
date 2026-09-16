#pragma once

#include <cstddef>
#include <functional>

namespace DosBoxMemoryTools
{
    class TraceComparisonToolbar
    {
    public:
        using Action = std::function<void()>;

        struct State
        {
            std::function<const char*()> traceAFilename;
            std::function<const char*()> traceBFilename;
            std::function<size_t()> traceACount;
            std::function<size_t()> traceBCount;
            std::function<size_t()> baselineCount;
            bool& collapseIdentical;
            bool& ignoreBaseline;
        };

        struct Callbacks
        {
            Action loadA;
            Action loadB;
            Action saveA;
            Action saveB;
            Action previousDifference;
            Action nextDifference;
            Action keyboardNavigation;
            Action collapseChanged;
            Action addToBaseline;
            Action clearBaseline;
        };

        void draw(const State& state, const Callbacks& callbacks);
    };
}
