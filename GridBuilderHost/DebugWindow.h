#pragma once
#pragma once

#include <string>
#include <utility>
#include <vector>

namespace GridBuilderHost
{
    class DebugWindow
    {
    public:
        explicit DebugWindow(std::string title = "Debug") : m_title(std::move(title)) {}
        void clear();

        void addLine(
            const std::string& text
        );

        void setLine(
            const std::string& text
        );

        void draw();

    private:
        std::string m_title;
        std::vector<std::string> m_lines;
        bool m_scrollToEnd = false;
    };
}
