#pragma once

namespace ow3d
{
    class Renderer;

    class ResolutionWindow
    {
    public:
        void draw(Renderer& renderer);

        void open();
        bool isOpen() const;

    private:
        bool m_open = false;
    };
}