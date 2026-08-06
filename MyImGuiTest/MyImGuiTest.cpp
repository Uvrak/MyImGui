#include "FloatingWindow.h"

#include "imgui.h"

int main()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io =
        ImGui::GetIO();

    io.DisplaySize =
        ImVec2(800.0f, 600.0f);

    io.DeltaTime =
        1.0f / 60.0f;

    unsigned char* pixels = nullptr;
    int textureWidth = 0;
    int textureHeight = 0;

    io.Fonts->GetTexDataAsRGBA32(
        &pixels,
        &textureWidth,
        &textureHeight
    );

    ImGui::NewFrame();

    MyImGui::FloatingWindow window(
        "Test Window"
    );

    if (window.begin())
    {
        ImGui::TextUnformatted(
            "MyImGui test succeeded."
        );
    }

    window.end();

    ImGui::Render();
    ImGui::DestroyContext();

    return 0;
}