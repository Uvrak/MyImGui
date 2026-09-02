#ifdef _WIN32
#include <Windows.h>
#include <TlHelp32.h>
#endif
#include "DosBoxWindow.h"
#include "imgui.h"
#include <string>
#include <cstdio>
#include "Controller.h"

namespace
{
    constexpr const char* DosBoxExecutablePath =
        "C:\\Projects\\MyImGui\\dosbox-x\\bin\\x64\\Debug SDL2\\dosbox-x.exe";

    constexpr const char* DosBoxWorkingDirectory =
        "C:\\Projects\\MyImGui\\dosbox-x\\bin\\x64\\Debug SDL2";

    constexpr const char* DosBoxPipeName =
        R"(\\.\pipe\GridBuilderDOSBox)";

    constexpr int HiddenDosBoxX = -5000;
    constexpr int HiddenDosBoxY = 0;
    constexpr int HiddenDosBoxWidth = 640;
    constexpr int HiddenDosBoxHeight = 400;
}

DosBoxWindow::DosBoxWindow(
    SDL_Window* parentWindow,
    SDL_Renderer* renderer
)
    : m_parentWindow(parentWindow),
      m_frameTexture(renderer),
      m_pipeClient(
          DosBoxPipeName
      )

{

    ZeroMemory(
        &m_processInfo,
        sizeof(m_processInfo)
    );

    if (m_parentWindow == nullptr)
    {
        return;
    }

    const SDL_PropertiesID properties =
        SDL_GetWindowProperties(
            m_parentWindow
        );

    m_parentHwnd =
        static_cast<HWND>(
            SDL_GetPointerProperty(
                properties,
                SDL_PROP_WINDOW_WIN32_HWND_POINTER,
                nullptr
            )
            );
}

DosBoxWindow::~DosBoxWindow()
{
    shutdown();
}

void DosBoxWindow::draw()
{
    if (!m_startAttempted)
    {

        m_startAttempted = true;

        m_started =
            start();

        if (m_started)
        {
            m_focusRequested = false;
            m_inputActive = false;
        }
    }

    if (m_started &&
        m_dosBoxHwnd == nullptr)
    {
        findDosBoxWindow();

        if (m_dosBoxHwnd != nullptr)
        {
            ShowWindow(
                m_dosBoxHwnd,
                SW_HIDE
            );
        }
    }

    if (m_started &&
        !m_pingTested)
    {
        std::string response;

        if (m_pipeClient.request(
            "PING",
            response
        ))
        {
            m_pingTested = true;

            if (response == "PONG")
            {
                OutputDebugStringA(
                    "GridBuilder IPC: PONG received\n"
                );

                m_focusRequested = false;
                m_inputActive = false;
            }
            else
            {
                OutputDebugStringA(
                    "GridBuilder IPC: unexpected response: ["
                );

                OutputDebugStringA(
                    response.c_str()
                );

                OutputDebugStringA(
                    "]\n"
                );
            }
        }

        else
        {
            OutputDebugStringA(
                "GridBuilder IPC: not ready, retrying\n"
            );
        }

  
    }

    ImGui::SetNextWindowSize(
        ImVec2(
            500.0f,
            350.0f
        ),
        ImGuiCond_FirstUseEver
    );

    const ImVec4 tabColor =
        m_inputActive
        ? ImVec4(
            0.0f,
            0.55f,
            0.0f,
            1.0f
        )
        : ImVec4(
            0.65f,
            0.0f,
            0.0f,
            1.0f
        );

    ImGui::PushStyleColor(
        ImGuiCol_Tab,
        tabColor
    );

    ImGui::PushStyleColor(
        ImGuiCol_TabSelected,
        tabColor
    );

    ImGui::PushStyleColor(
        ImGuiCol_TabHovered,
        tabColor
    );

    ImGui::PushStyleColor(
        ImGuiCol_TabDimmed,
        tabColor
    );

    ImGui::PushStyleColor(
        ImGuiCol_TabDimmedSelected,
        tabColor
    );

    std::string windowTitle =
        m_gameFilename.empty()
        ? "DOSBox"
        : m_gameFilename;

    windowTitle +=
        "###DOSBoxWindow";

    if (m_focusRequested)
    {
        ImGui::SetNextWindowFocus();
    }

    const bool windowContentsVisible =
        ImGui::Begin(
            windowTitle.c_str()
        );

    ImGui::PopStyleColor(5);
    
    if (!windowContentsVisible)
    {
        if (m_inputActive &&
            !m_focusRequested)
        {
            deactivateInput();
        }

        ImGui::End();
        return;
    }

    if (m_focusRequested)
    {
        m_focusRequested = false;
        m_inputActive = true;
    }
    if (m_started &&
        m_pingTested)
    {
        const Uint64 currentTicks =
            SDL_GetTicks();

       

        
    }

    const bool switchViewPressed =
        ImGui::IsKeyPressed(
            ImGuiKey_Tab,
            false
        ) ||
        (
            m_customSwitchViewKey !=
            ImGuiKey_None &&
            ImGui::IsKeyPressed(
                m_customSwitchViewKey,
                false
            )
            );

    const bool dosBoxWindowFocused =
        ImGui::IsWindowFocused(
            ImGuiFocusedFlags_RootAndChildWindows
        );

    if (switchViewPressed)
    {
        if (m_inputActive)
        {
            deactivateInput();
        }
        else
        {
            m_focusRequested = true;
        }
    }

    if (ImGui::Button(
        "Reset Memory Candidates"
    ))
    {
        if (m_pipeClient.send(
            "MEMORY_RESET"
        ))
        {
            OutputDebugStringA(
                "GridBuilder IPC: memory candidates reset\n"
            );
        }
        else
        {
            OutputDebugStringA(
                "GridBuilder IPC: memory reset request failed\n"
            );
        }
    }

    ImGui::SameLine();
    if (ImGui::Button(
        "Capture Memory Snapshot"
    ))
    {
        if (m_pipeClient.send(
            "MEMORY_SNAPSHOT"
        ))
        {
            m_memorySnapshotRequested = true;

            OutputDebugStringA(
                "GridBuilder IPC: memory snapshot requested\n"
            );
        }
        else
        {
            OutputDebugStringA(
                "GridBuilder IPC: memory snapshot request failed\n"
            );
        }
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Refine Unchanged"
    ))
    {
        if (m_pipeClient.send(
            "MEMORY_REFINE_UNCHANGED"
        ))
        {
            OutputDebugStringA(
                "GridBuilder IPC: unchanged refinement requested\n"
            );
        }
        else
        {
            OutputDebugStringA(
                "GridBuilder IPC: unchanged refinement failed\n"
            );
        }
    }

    m_frameReader.tryOpen();

    const DosBoxX::DosBoxFrameHeader* frameHeader =
        m_frameReader.header();
    if (frameHeader != nullptr &&
        frameHeader->width > 0 &&
        frameHeader->height > 0 &&
        frameHeader->contentWidth > 0 &&
        frameHeader->contentHeight > 0)
    {
        const uint8_t* framePixels =
            m_frameReader.pixels();

        auto pixelMatches =
            [framePixels, frameHeader](
                int x,
                int y,
                int r,
                int g,
                int b
                )
            {
                const int bytesPerPixel =
                    frameHeader->pitch /
                    frameHeader->width;

                const uint8_t* pixel =
                    framePixels +
                    y * frameHeader->pitch +
                    x * bytesPerPixel;

                return
                    pixel[2] == r &&
                    pixel[1] == g &&
                    pixel[0] == b;
            };

       if (framePixels != nullptr)
        {
            if (m_frameTexture.update(
                framePixels,
                frameHeader->width,
                frameHeader->height,
                frameHeader->pitch
            ))
            {
                SDL_Texture* texture =
                    m_frameTexture.texture();

                if (texture != nullptr)
                {
                    ImVec2 availableSize =
                        ImGui::GetContentRegionAvail();

                    availableSize.x =
                        (availableSize.x > 1.0f)
                        ? availableSize.x
                        : 1.0f;

                    availableSize.y =
                        (availableSize.y > 1.0f)
                        ? availableSize.y
                        : 1.0f;

                    const float scaleX =
                        availableSize.x /
                        static_cast<float>(
                            frameHeader->contentWidth
                            );

                    const float scaleY =
                        availableSize.y /
                        static_cast<float>(
                            frameHeader->contentHeight
                            );

                    const float scale =
                        (scaleX < scaleY)
                        ? scaleX
                        : scaleY;

                    ImVec2 imageSize(
                        frameHeader->contentWidth * scale,
                        frameHeader->contentHeight * scale
                    );

                    const ImVec2 imagePos =
                        ImGui::GetCursorScreenPos();

                    ImGui::Image(
                        reinterpret_cast<ImTextureID>(
                            texture
                            ),
                        imageSize,
                        ImVec2(
                            0.0f,
                            0.0f
                        ),
                        ImVec2(
                            static_cast<float>(
                                frameHeader->contentWidth
                                ) /
                            static_cast<float>(
                                frameHeader->width
                                ),
                            static_cast<float>(
                                frameHeader->contentHeight
                                ) /
                            static_cast<float>(
                                frameHeader->height
                                )
                        )
                    );

                    const bool imageHovered =
                        ImGui::IsItemHovered();

                    if (imageHovered)
                    {
                        const ImVec2 mousePos =
                            ImGui::GetMousePos();

                        const int contentX =
                            static_cast<int>(
                                (mousePos.x - imagePos.x) /
                                scale
                                );

                        const int contentY =
                            static_cast<int>(
                                (mousePos.y - imagePos.y) /
                                scale
                                );

                        if (contentX >= 0 &&
                            contentY >= 0 &&
                            contentX <
                            frameHeader->contentWidth &&
                            contentY <
                            frameHeader->contentHeight)
                        {
                            const int bytesPerPixel =
                                frameHeader->pitch /
                                frameHeader->width;

                            const uint8_t* pixel =
                                framePixels +
                                contentY *
                                frameHeader->pitch +
                                contentX *
                                bytesPerPixel;

                            ImGui::SetTooltip(
                                "DOS %d,%d  RGB %u,%u,%u",
                                contentX,
                                contentY,
                                static_cast<unsigned>(pixel[2]),
                                static_cast<unsigned>(pixel[1]),
                                static_cast<unsigned>(pixel[0])
                            );
                        }
                    }

                    if (imageHovered &&
                        ImGui::GetIO().KeyShift &&
                        ImGui::IsMouseClicked(
                            ImGuiMouseButton_Left
                        ))
                    {
                        const ImVec2 mousePos =
                            ImGui::GetMousePos();

                        m_buttonRectStart =
                            ImVec2(
                                (mousePos.x - imagePos.x) /
                                scale,
                                (mousePos.y - imagePos.y) /
                                scale
                            );

                        m_buttonRectEnd =
                            m_buttonRectStart;

                        m_buttonRectDragging =
                            true;
                    }

                    if (m_buttonRectDefined &&
                        !m_buttonRectDragging &&
                        ImGui::IsMouseClicked(
                            ImGuiMouseButton_Left
                        ))
                    {
                        const ImVec2 mousePos =
                            ImGui::GetMousePos();

                        const float contentX =
                            (mousePos.x - imagePos.x) /
                            scale;

                        const float contentY =
                            (mousePos.y - imagePos.y) /
                            scale;

                        const float left =
                            (std::min)(
                                m_buttonRectStart.x,
                                m_buttonRectEnd.x
                                );

                        const float right =
                            (std::max)(
                                m_buttonRectStart.x,
                                m_buttonRectEnd.x
                                );

                        const float top =
                            (std::min)(
                                m_buttonRectStart.y,
                                m_buttonRectEnd.y
                                );

                        const float bottom =
                            (std::max)(
                                m_buttonRectStart.y,
                                m_buttonRectEnd.y
                                );

                        const float handleHitSize =
                            12.0f /
                            scale;

                        const bool resizeHandleHit =
                            contentX >=
                            right - handleHitSize &&
                            contentX <=
                            right + handleHitSize &&
                            contentY >=
                            bottom - handleHitSize &&
                            contentY <=
                            bottom + handleHitSize;


                        if (resizeHandleHit)
                        {
                            m_buttonRectResizing =
                                true;

                            m_buttonRectMoving =
                                false;
                        }
                        else if (
                            contentX >= left &&
                            contentX <= right &&
                            contentY >= top &&
                            contentY <= bottom)
                        {
                            m_buttonRectMoving =
                                true;

                            m_buttonRectMoveOffset =
                                ImVec2(
                                    contentX - left,
                                    contentY - top
                                );
                        }
                    }

                    if (m_buttonRectResizing)
                    {
                        const ImVec2 mousePos =
                            ImGui::GetMousePos();

                        const float contentX =
                            (mousePos.x - imagePos.x) /
                            scale;

                        const float contentY =
                            (mousePos.y - imagePos.y) /
                            scale;

                        m_buttonRectEnd =
                            ImVec2(
                                contentX,
                                contentY
                            );
                    }

                    if (m_buttonRectResizing &&
                        ImGui::IsMouseReleased(
                            ImGuiMouseButton_Left
                        ))
                    {
                        m_buttonRectResizing =
                            false;
                    }

                    if (m_buttonRectDragging)
                    {
                        const ImVec2 mousePos =
                            ImGui::GetMousePos();

                        m_buttonRectEnd =
                            ImVec2(
                                (mousePos.x - imagePos.x) /
                                scale,
                                (mousePos.y - imagePos.y) /
                                scale
                            );
                    }

                    if (m_buttonRectMoving)
                    {
                        const ImVec2 mousePos =
                            ImGui::GetMousePos();

                        const float contentX =
                            (mousePos.x - imagePos.x) /
                            scale;

                        const float contentY =
                            (mousePos.y - imagePos.y) /
                            scale;

                        const float width =
                            std::abs(
                                m_buttonRectEnd.x -
                                m_buttonRectStart.x
                            );

                        const float height =
                            std::abs(
                                m_buttonRectEnd.y -
                                m_buttonRectStart.y
                            );

                        const float newLeft =
                            contentX -
                            m_buttonRectMoveOffset.x;

                        const float newTop =
                            contentY -
                            m_buttonRectMoveOffset.y;

                        m_buttonRectStart =
                            ImVec2(
                                newLeft,
                                newTop
                            );

                        m_buttonRectEnd =
                            ImVec2(
                                newLeft + width,
                                newTop + height
                            );
                    }

                    if (m_buttonRectMoving &&
                        ImGui::IsMouseReleased(
                            ImGuiMouseButton_Left
                        ))
                    {
                        m_buttonRectMoving =
                            false;
                    }

                    if (m_buttonRectDragging ||
                        m_buttonRectDefined)
                    {
                        ImDrawList* drawList =
                            ImGui::GetWindowDrawList();

                        const ImVec2 rectMin(
                            imagePos.x +
                            (std::min)(
                                m_buttonRectStart.x,
                                m_buttonRectEnd.x
                            ) * scale,
                            imagePos.y +
                            (std::min)(
                                m_buttonRectStart.y,
                                m_buttonRectEnd.y
                            ) * scale
                        );

                        const ImVec2 rectMax(
                            imagePos.x +
                            (std::max)(
                                m_buttonRectStart.x,
                                m_buttonRectEnd.x
                            ) * scale,
                            imagePos.y +
                            (std::max)(
                                m_buttonRectStart.y,
                                m_buttonRectEnd.y
                            ) * scale
                        );

                        drawList->AddRect(
                            rectMin,
                            rectMax,
                            IM_COL32(
                                0,
                                255,
                                255,
                                255
                            ),
                            0.0f,
                            0,
                            2.0f
                        );

                        const float handleSize =
                            6.0f;

                        drawList->AddRectFilled(
                            ImVec2(
                                rectMax.x - handleSize,
                                rectMax.y - handleSize
                            ),
                            rectMax,
                            IM_COL32(
                                0,
                                255,
                                255,
                                255
                            )
                        );
                    }

                    if (m_buttonRectDefined &&
                        ImGui::IsMouseClicked(
                            ImGuiMouseButton_Right
                        ))
                    {
                        const ImVec2 mousePos =
                            ImGui::GetMousePos();

                        const float contentX =
                            (mousePos.x - imagePos.x) /
                            scale;

                        const float contentY =
                            (mousePos.y - imagePos.y) /
                            scale;

                        const float left =
                            (std::min)(
                                m_buttonRectStart.x,
                                m_buttonRectEnd.x
                                );

                        const float right =
                            (std::max)(
                                m_buttonRectStart.x,
                                m_buttonRectEnd.x
                                );

                        const float top =
                            (std::min)(
                                m_buttonRectStart.y,
                                m_buttonRectEnd.y
                                );

                        const float bottom =
                            (std::max)(
                                m_buttonRectStart.y,
                                m_buttonRectEnd.y
                                );

                        if (contentX >= left &&
                            contentX <= right &&
                            contentY >= top &&
                            contentY <= bottom)
                        {
                            ImGui::OpenPopup(
                                "ButtonRectContextMenu"
                            );
                        }
                    }

                    if (ImGui::BeginPopup(
                        "ButtonRectContextMenu"
                    ))
                    {
                        if (ImGui::MenuItem(
                            "Als Button speichern"
                        ))
                        {
                            const float x1 =
                                (std::min)(
                                    m_buttonRectStart.x,
                                    m_buttonRectEnd.x
                                    );

                            const float y1 =
                                (std::min)(
                                    m_buttonRectStart.y,
                                    m_buttonRectEnd.y
                                    );

                            const float x2 =
                                (std::max)(
                                    m_buttonRectStart.x,
                                    m_buttonRectEnd.x
                                    );

                            const float y2 =
                                (std::max)(
                                    m_buttonRectStart.y,
                                    m_buttonRectEnd.y
                                    );

                            m_debugButtonRect.x =
                                x1;

                            m_debugButtonRect.y =
                                y1;

                            m_debugButtonRect.width =
                                x2 - x1;

                            m_debugButtonRect.height =
                                y2 - y1;

                            if (m_buttonRectSaveCallback)
                            {
                                m_buttonRectSaveCallback(
                                    m_debugButtonRect
                                );
                            }

                            m_buttonRectDefined =
                                false;

                            m_buttonRectDragging =
                                false;

                            m_buttonRectMoving =
                                false;

                            m_buttonRectResizing =
                                false;
                        }

                        ImGui::EndPopup();
                    }

                    if (m_buttonRectDragging &&
                        ImGui::IsMouseReleased(
                            ImGuiMouseButton_Left
                        ))
                    {
                        m_buttonRectDragging =
                            false;

                        m_buttonRectDefined =
                            true;

                        const float x1 =
                            (std::min)(
                                m_buttonRectStart.x,
                                m_buttonRectEnd.x
                            );

                        const float y1 =
                            (std::min)(
                                m_buttonRectStart.y,
                                m_buttonRectEnd.y
                            );

                        const float x2 =
                            (std::max)(
                                m_buttonRectStart.x,
                                m_buttonRectEnd.x
                            );

                        const float y2 =
                            (std::max)(
                                m_buttonRectStart.y,
                                m_buttonRectEnd.y
                            );

                        m_debugButtonRect.x =
                            x1;

                        m_debugButtonRect.y =
                            y1;

                        m_debugButtonRect.width =
                            x2 - x1;

                        m_debugButtonRect.height =
                            y2 - y1;

                        m_buttonRectStart =
                            ImVec2(
                                x1,
                                y1
                            );

                        m_buttonRectEnd =
                            ImVec2(
                                x2,
                                y2
                            );
                    }
                    
                    ImGui::Text(
                        "Button Rect: x=%.1f y=%.1f w=%.1f h=%.1f",
                        m_debugButtonRect.x,
                        m_debugButtonRect.y,
                        m_debugButtonRect.width,
                        m_debugButtonRect.height
                    );

                    ImGui::Text(
                        "Selection: %d x=%.1f y=%.1f w=%.1f h=%.1f",
                        m_gameButtonSelectionActive ? 1 : 0,
                        m_gameButtonRect.x,
                        m_gameButtonRect.y,
                        m_gameButtonRect.width,
                        m_gameButtonRect.height
                    );

                    if (m_gameButtonSelectionActive &&
                        !m_modifyingGameButton)
                    {
                        ImDrawList* drawList =
                            ImGui::GetWindowDrawList();

                        const ImVec2 selectionMin(
                            imagePos.x +
                            m_gameButtonRect.x * scale,
                            imagePos.y +
                            m_gameButtonRect.y * scale
                        );

                        const ImVec2 selectionMax(
                            selectionMin.x +
                            m_gameButtonRect.width * scale,
                            selectionMin.y +
                            m_gameButtonRect.height * scale
                        );

                        drawList->AddRect(
                            selectionMin,
                            selectionMax,
                            IM_COL32(
                                255,
                                0,
                                255,
                                255
                            ),
                            0.0f,
                            0,
                            3.0f
                        );
                    }

                    if (m_gameButtonSelectionActive &&
                        ImGui::IsMouseClicked(
                            ImGuiMouseButton_Right
                        ))
                    {
                        const ImVec2 mousePos =
                            ImGui::GetMousePos();

                        const ImVec2 selectionMin(
                            imagePos.x +
                            m_gameButtonRect.x * scale,
                            imagePos.y +
                            m_gameButtonRect.y * scale
                        );

                        const ImVec2 selectionMax(
                            selectionMin.x +
                            m_gameButtonRect.width * scale,
                            selectionMin.y +
                            m_gameButtonRect.height * scale
                        );

                        if (mousePos.x >= selectionMin.x &&
                            mousePos.x <= selectionMax.x &&
                            mousePos.y >= selectionMin.y &&
                            mousePos.y <= selectionMax.y)
                        {
                            ImGui::OpenPopup(
                                "GameButtonContextMenu"
                            );
                        }
                    }

                    if (ImGui::BeginPopup(
                        "GameButtonContextMenu"
                    ))
                    {
                        if (ImGui::MenuItem(
                            "Modify Button"
                        ))
                        {
                            m_debugButtonRect =
                                m_gameButtonRect;

                            m_buttonRectStart =
                                ImVec2(
                                    m_gameButtonRect.x,
                                    m_gameButtonRect.y
                                );

                            m_buttonRectEnd =
                                ImVec2(
                                    m_gameButtonRect.x +
                                    m_gameButtonRect.width,
                                    m_gameButtonRect.y +
                                    m_gameButtonRect.height
                                );

                            m_buttonRectDefined =
                                true;

                            m_buttonRectDragging =
                                false;

                            m_buttonRectMoving =
                                false;

                            m_buttonRectResizing =
                                false;

                            m_modifyingGameButton =
                                true;
                        }

                        if (ImGui::MenuItem(
                            "Delete Button"
                        ))
                        {
                            if (m_buttonRectDeleteCallback)
                            {
                                m_buttonRectDeleteCallback();
                            }
                        }

                        ImGui::EndPopup();
                    }

                    if (m_modifyingGameButton &&
                        ImGui::IsMouseClicked(
                            ImGuiMouseButton_Left
                        ))
                    {
                        const ImVec2 mousePos =
                            ImGui::GetMousePos();

                        const ImVec2 rectMin(
                            imagePos.x +
                            (std::min)(
                                m_buttonRectStart.x,
                                m_buttonRectEnd.x
                                ) * scale,
                            imagePos.y +
                            (std::min)(
                                m_buttonRectStart.y,
                                m_buttonRectEnd.y
                                ) * scale
                        );

                        const ImVec2 rectMax(
                            imagePos.x +
                            (std::max)(
                                m_buttonRectStart.x,
                                m_buttonRectEnd.x
                                ) * scale,
                            imagePos.y +
                            (std::max)(
                                m_buttonRectStart.y,
                                m_buttonRectEnd.y
                                ) * scale
                        );

                        const bool clickedInside =
                            mousePos.x >= rectMin.x &&
                            mousePos.x <= rectMax.x &&
                            mousePos.y >= rectMin.y &&
                            mousePos.y <= rectMax.y;

                        if (!clickedInside)
                        {
                            const float x1 =
                                (std::min)(
                                    m_buttonRectStart.x,
                                    m_buttonRectEnd.x
                                    );

                            const float y1 =
                                (std::min)(
                                    m_buttonRectStart.y,
                                    m_buttonRectEnd.y
                                    );

                            const float x2 =
                                (std::max)(
                                    m_buttonRectStart.x,
                                    m_buttonRectEnd.x
                                    );

                            const float y2 =
                                (std::max)(
                                    m_buttonRectStart.y,
                                    m_buttonRectEnd.y
                                    );

                            m_debugButtonRect.x =
                                x1;

                            m_debugButtonRect.y =
                                y1;

                            m_debugButtonRect.width =
                                x2 - x1;

                            m_debugButtonRect.height =
                                y2 - y1;

                            if (m_buttonRectModifyCallback)
                            {
                                m_buttonRectModifyCallback(
                                    m_debugButtonRect
                                );
                            }

                            m_modifyingGameButton =
                                false;

                            m_buttonRectDefined =
                                false;
                        }
                    }

                    const ImVec2 imageMin =
                        ImGui::GetItemRectMin();

                    const ImVec2 imageMax =
                        ImGui::GetItemRectMax();

                    const bool dosBoxImageHovered =
                        ImGui::IsItemHovered();

                    if (m_inputActive)
                    {
                        m_mouse.update(
                            m_pipeClient,
                            *frameHeader,
                            imageSize.x,
                            imageSize.y,
                            imagePos.x,
                            imagePos.y
                        );
                    }
                }
            }
        }
    }

    if (m_inputActive)
    {

        ImGui::GetIO().ConfigFlags |=
            ImGuiConfigFlags_NoMouseCursorChange;

        SDL_HideCursor();
    }

    if (m_inputActive &&
        !m_directKeyboardBlocked)
    {
        m_keyboard.update(
            m_pipeClient,
            [this](
                ImGuiKey key,
                const char* defaultCommand
                ) -> std::string
            {
                if (m_keyBindings == nullptr)
                {
                    return defaultCommand;
                }

                const bool mightAndMagic1Active =
                    m_programReader.isRunning(
                        "MM"
                    );

                if (m_keyBindings->
                    germanKeyboardForUsGame())
                {
                    if (key == ImGuiKey_Y)
                    {
                        return "Z";
                    }

                    if (key == ImGuiKey_Z)
                    {
                        return "Y";
                    }
                }

                for (const DosBoxKeyBinding& binding :
                    m_keyBindings->bindings())
                {
                    if (binding.customKey != key)
                    {
                        continue;
                    }

                    if (binding.action ==
                        DosBoxAction::SwitchView)
                    {
                        return {};
                    }

                    if (mightAndMagic1Active)
                    {
                        return binding.dosCommand;
                    }
                }

                return defaultCommand;
            }
        );
    }
    ImGui::End();
} 

#ifdef _WIN32
namespace
{
    void closeExistingDosBoxInstances()
    {
        HANDLE snapshot =
            CreateToolhelp32Snapshot(
                TH32CS_SNAPPROCESS,
                0
            );

        if (snapshot == INVALID_HANDLE_VALUE)
        {
            return;
        }

        PROCESSENTRY32 processEntry{};
        processEntry.dwSize =
            sizeof(processEntry);

        if (Process32First(
            snapshot,
            &processEntry
        ))
        {
            do
            {
                if (_wcsicmp(
                    processEntry.szExeFile,
                    L"dosbox-x.exe"
                ) != 0)
                {
                    continue;
                }

                HANDLE process =
                    OpenProcess(
                        PROCESS_TERMINATE,
                        FALSE,
                        processEntry.th32ProcessID
                    );

                if (process == nullptr)
                {
                    continue;
                }

                TerminateProcess(
                    process,
                    0
                );

                CloseHandle(
                    process
                );

            } while (Process32Next(
                snapshot,
                &processEntry
            ));
        }

        CloseHandle(
            snapshot
        );
    }
}
#endif

bool DosBoxWindow::start()
{
#ifdef _WIN32
    closeExistingDosBoxInstances();

    HANDLE existingPipe =
        CreateFileA(
            DosBoxPipeName,
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

    if (existingPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(
            existingPipe
        );

        OutputDebugStringA(
            "DOSBox-X already running - not starting another instance.\n"
        );

        return false;
    }

    if (m_processInfo.hProcess != nullptr)
    {
        return false;
    }

    const char* dosBoxPath =
        DosBoxExecutablePath;

    STARTUPINFOA startupInfo{};
    startupInfo.cb =
        sizeof(startupInfo);

    startupInfo.dwFlags |=
        STARTF_USESHOWWINDOW;

    startupInfo.wShowWindow =
        SW_HIDE;

    char commandLine[MAX_PATH] = {};

    std::snprintf(
        commandLine,
        sizeof(commandLine),
        "\"%s\"",
        dosBoxPath
    );

    BOOL started =
        CreateProcessA(
            nullptr,
            commandLine,
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            DosBoxWorkingDirectory,
            &startupInfo,
            &m_processInfo
        );

    if (!started)
    {
        char text[128] = {};

        std::snprintf(
            text,
            sizeof(text),
            "DOSBox-X start failed: %lu\n",
            GetLastError()
        );

        OutputDebugStringA(
            text
        );

        return false;
    }

    CloseHandle(
        m_processInfo.hThread
    );

    m_processInfo.hThread =
        nullptr;

    return true;
#else
    return false;
#endif
}

bool DosBoxWindow::inputActive() const
{
    return m_inputActive;
}

DosBoxX::NamedPipeClient&
DosBoxWindow::pipeClient()
{
    return m_pipeClient;
}

void DosBoxWindow::openGame(
    const std::string& mountDirectory,
    const std::string& dosDirectory,
    const std::string& gameFilename
)
{
    m_gameFilename = gameFilename;

    m_controller.openGame(
        m_pipeClient,
        mountDirectory,
        dosDirectory,
        gameFilename
    );
}

bool DosBoxWindow::sendDosKey(
    const char* key
)
{
    const bool sent =
        m_controller.sendDosKey(
            m_pipeClient,
            key
        );

    OutputDebugStringA(
        sent
        ? "DosBoxWindow: sendDosKey OK\n"
        : "DosBoxWindow: sendDosKey FAILED\n"
    );

    return sent;
}

void DosBoxWindow::setGermanKeyboardLayout()
{
    m_externalWindow.sendIpcCommand(
        "KEYBOARD_LAYOUT:GR"
    );
}

void DosBoxWindow::setUSKeyboardLayout()
{
    m_externalWindow.sendIpcCommand(
        "KEYBOARD_LAYOUT:US"
    );
}

void DosBoxWindow::setCustomSwitchViewKey(
    ImGuiKey key
)
{
    m_customSwitchViewKey =
        key;
}

void DosBoxWindow::setKeyBindings(
    const DosBoxKeyBindings& keyBindings
)
{
    m_keyBindings =
        &keyBindings;
}

void DosBoxWindow::setDirectKeyboardBlocked(
    bool blocked
)
{
    if (m_directKeyboardBlocked != blocked)
    {
        OutputDebugStringA(
            blocked
            ? "DOSBox direct keyboard: BLOCKED\n"
            : "DOSBox direct keyboard: ENABLED\n"
        );
    }

    m_directKeyboardBlocked =
        blocked;
};

void DosBoxWindow::setGameButtonSelection(
    bool active,
    const GameButtonRect& rect
)
{
    m_gameButtonSelectionActive =
        active;

    if (active)
    {
        m_gameButtonRect =
            rect;
    }
    else
    {
        m_gameButtonRect =
        {};
    }
}

void DosBoxWindow::setButtonRectSaveCallback(
    std::function<void(
        const GameButtonRect&
        )> callback
)
{
    m_buttonRectSaveCallback =
        std::move(
            callback
        );
}

void DosBoxWindow::setButtonRectModifyCallback(
    std::function<void(
        const GameButtonRect&
        )> callback
)
{
    m_buttonRectModifyCallback =
        std::move(
            callback
        );
}

void DosBoxWindow::setButtonRectDeleteCallback(
    std::function<void()> callback
)
{
    m_buttonRectDeleteCallback =
        std::move(
            callback
        );
}

bool DosBoxWindow::sendDosMouseClick(
    float x,
    float y
)
{
    const DosBoxX::DosBoxFrameHeader*
        frameHeader =
        m_frameReader.header();

    if (frameHeader == nullptr ||
        frameHeader->contentWidth <= 0 ||
        frameHeader->contentHeight <= 0)
    {
        return false;
    }

    char command[128] = {};

    std::snprintf(
        command,
        sizeof(command),
        "MOUSECLICK:%d:%d:%d:%d",
        static_cast<int>(x),
        static_cast<int>(y),
        frameHeader->contentWidth,
        frameHeader->contentHeight
    );

    return m_pipeClient.send(
        command
    );
}

const DosBoxX::DosBoxFrameHeader*
DosBoxWindow::frameHeader() const
{
    return m_frameReader.header();
}

const uint8_t*
DosBoxWindow::framePixels() const
{
    return m_frameReader.pixels();
}

bool DosBoxWindow::sendDosMouseDoubleClick(
    float x,
    float y
)
{
    const DosBoxX::DosBoxFrameHeader*
        frameHeader =
        m_frameReader.header();

    if (frameHeader == nullptr ||
        frameHeader->contentWidth <= 0 ||
        frameHeader->contentHeight <= 0)
    {
        return false;
    }

    char command[128] = {};

    std::snprintf(
        command,
        sizeof(command),
        "MOUSEDOUBLECLICK:%d:%d:%d:%d",
        static_cast<int>(x),
        static_cast<int>(y),
        frameHeader->contentWidth,
        frameHeader->contentHeight
    );

    return m_pipeClient.send(
        command
    );
}

bool DosBoxWindow::sendDosMousePosition(
    float x,
    float y
)
{
    const DosBoxX::DosBoxFrameHeader*
        frameHeader =
        m_frameReader.header();

    if (frameHeader == nullptr ||
        frameHeader->contentWidth <= 0 ||
        frameHeader->contentHeight <= 0)
    {
        return false;
    }

    char command[128] = {};

    std::snprintf(
        command,
        sizeof(command),
        "MOUSESETPOS:%d:%d:%d:%d",
        static_cast<int>(x),
        static_cast<int>(y),
        frameHeader->contentWidth,
        frameHeader->contentHeight
    );

    return m_pipeClient.send(
        command
    );
}

bool DosBoxWindow::handleButtonRectEditorKeyDown(
    SDL_Keycode key
)
{
    if (!m_modifyingGameButton)
    {
        return false;
    }

    if (key != SDLK_RETURN &&
        key != SDLK_KP_ENTER)
    {
        return false;
    }

    const float x1 =
        (std::min)(
            m_buttonRectStart.x,
            m_buttonRectEnd.x
            );

    const float y1 =
        (std::min)(
            m_buttonRectStart.y,
            m_buttonRectEnd.y
            );

    const float x2 =
        (std::max)(
            m_buttonRectStart.x,
            m_buttonRectEnd.x
            );

    const float y2 =
        (std::max)(
            m_buttonRectStart.y,
            m_buttonRectEnd.y
            );

    m_debugButtonRect.x =
        x1;

    m_debugButtonRect.y =
        y1;

    m_debugButtonRect.width =
        x2 - x1;

    m_debugButtonRect.height =
        y2 - y1;

    if (m_buttonRectModifyCallback)
    {
        m_buttonRectModifyCallback(
            m_debugButtonRect
        );
    }

    m_modifyingGameButton =
        false;

    m_buttonRectDefined =
        false;

    m_buttonRectDragging =
        false;

    m_buttonRectMoving =
        false;

    m_buttonRectResizing =
        false;

    return true;
}

void DosBoxWindow::findDosBoxWindow()
{
    if (m_dosBoxHwnd != nullptr &&
        IsWindow(m_dosBoxHwnd))
    {
        return;
    }

    m_dosBoxHwnd = nullptr;

    const DWORD targetProcessId =
        m_processInfo.dwProcessId;

    if (targetProcessId == 0)
    {
        return;
    }

    EnumWindows(
        [](HWND hwnd, LPARAM parameter) -> BOOL
        {
            auto* window =
                reinterpret_cast<DosBoxWindow*>(
                    parameter
                    );

            DWORD processId = 0;

            GetWindowThreadProcessId(
                hwnd,
                &processId
            );

            if (processId !=
                window->m_processInfo.dwProcessId)
            {
                return TRUE;
            }

            window->m_dosBoxHwnd =
                hwnd;

            char className[256] = {};
            char title[256] = {};
            char text[512] = {};

            GetClassNameA(
                hwnd,
                className,
                sizeof(className)
            );

            GetWindowTextA(
                hwnd,
                title,
                sizeof(title)
            );

            std::snprintf(
                text,
                sizeof(text),
                "FOUND DOSBOX PROCESS WINDOW HWND=%p CLASS=%s TITLE=%s\n",
                hwnd,
                className,
                title
            );

            OutputDebugStringA(text);

            return FALSE;
        },
        reinterpret_cast<LPARAM>(this)
    );
}

void DosBoxWindow::shutdown()
{
#ifdef _WIN32
    deactivateInput();

    if (m_processInfo.hProcess != nullptr)
    {
        m_externalWindow.sendIpcCommand(
            "SHUTDOWN"
        );
    
        const DWORD waitResult =
            WaitForSingleObject(
                m_processInfo.hProcess,
                1000
            );
    
        if (waitResult == WAIT_TIMEOUT)
        {
            TerminateProcess(
                m_processInfo.hProcess,
                0
            );
        }
    
        CloseHandle(
            m_processInfo.hProcess
        );
    
        ZeroMemory(
            &m_processInfo,
            sizeof(m_processInfo)
        );
    }
#endif
}

void DosBoxWindow::deactivateInput()
{
    ClipCursor(
        nullptr
    );

    if (ImGui::GetCurrentContext() != nullptr)
    {
        ImGui::GetIO().ConfigFlags &=
            ~ImGuiConfigFlags_NoMouseCursorChange;
    }

    SDL_ShowCursor();


    if (!m_inputActive)
    {
        return;
    }

    m_externalWindow.sendIpcCommand(
        "RELEASE_ALL"
    );

    m_inputActive = false;
}

