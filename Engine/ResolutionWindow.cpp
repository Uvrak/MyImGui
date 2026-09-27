#include "ResolutionWindow.h"
#include "Renderer.h"

#include <imgui.h>

namespace ow3d
{
    void ResolutionWindow::draw(Renderer& renderer)
    {
        if (!m_open)
            return;

        struct Resolution
        {
            const char* label;
            int width;
            int height;
        };

        static const Resolution resolutions[] =
        {
            { "640 x 480",   640,  480 },
            { "1280 x 720",  1280, 720 },
            { "1600 x 900",  1600, 900 },
            { "1920 x 1080", 1920, 1080 },
            { "2560 x 1440", 2560, 1440 },
            { "3440 x 1440", 3440, 1440 }
        };

        if (!ImGui::Begin("Resolution", &m_open))
        {
            ImGui::End();
            return;
        }

        int selectedResolution = -1;

        const int currentWidth =
            renderer.viewportWidth();

        const int currentHeight =
            renderer.viewportHeight();

        for (int i = 0; i < IM_ARRAYSIZE(resolutions); ++i)
        {
            if (resolutions[i].width == currentWidth &&
                resolutions[i].height == currentHeight)
            {
                selectedResolution = i;
                break;
            }
        }

        ImGui::Text("Window Resolution");

        ImGui::BeginChild(
            "ResolutionList",
            ImVec2(0.0f, 180.0f),
            true
        );

        for (int i = 0; i < IM_ARRAYSIZE(resolutions); ++i)
        {
            const bool selected =
                (selectedResolution == i);

            if (ImGui::Selectable(
                resolutions[i].label,
                selected))
            {
                selectedResolution = i;

                renderer.resizeViewport(
                    resolutions[i].width,
                    resolutions[i].height
                );
            }
        }

        ImGui::EndChild();

        ImGui::End();
    }

    void ResolutionWindow::open()
    {
        m_open = true;
    }

    bool ResolutionWindow::isOpen() const
    {
        return m_open;
    }
}