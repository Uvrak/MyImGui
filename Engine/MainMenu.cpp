#include "MainMenu.h"
#include "Window.h"
#include "ResolutionWindow.h"
#include "Renderer.h"
#include <imgui.h>

namespace ow3d
{
    void MainMenu::draw(
        Window& window,
        ResolutionWindow& resolutionWindow,
        Renderer& renderer)
    {
        if (!ImGui::BeginMainMenuBar())
            return;

        if (ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem("Exit");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Settings"))
        {
            if (ImGui::MenuItem("Resolution..."))
            {
                resolutionWindow.open();
            }

            ImGui::EndMenu();
        }

        const char* label = renderer.character().loaded() && renderer.character().follow
            ? (renderer.character().topDown ? "Schraegansicht" : "Draufsicht") : "Draufsicht 4,05 Grad";
        if (additionalMenu) additionalMenu();
        const float width = ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2.0f;
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - width - ImGui::GetStyle().WindowPadding.x);
        if (ImGui::Button(label)) renderer.showSceneTopView();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(renderer.character().loaded() && renderer.character().follow
            ? "Ansicht umschalten; Sir Canegm bleibt in der Mitte." : "1 Meter ueber Dachhoehe, auf die Charakterposition zentriert.");
        ImGui::EndMainMenuBar();
    }
}
