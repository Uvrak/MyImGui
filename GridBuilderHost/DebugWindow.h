#pragma once
#pragma once

#include <string>
#include <vector>

namespace GridBuilderHost
{
    class DebugWindow
    {
    public:
        void clear();

        void addLine(
            const std::string& text
        );

        void setLine(
            const std::string& text
        );

        void draw();

    private:
        std::vector<std::string> m_lines;
    };
}