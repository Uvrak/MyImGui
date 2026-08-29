#include "PixelCanvasWindow.h"

#include "imgui.h"

#include <utility>
#include <string>

#include <algorithm>
#include <cmath>

void PixelCanvasWindow::draw(
    PixelTool activeTool,
    ImFont* iconFont,
    const std::function<void()>& drawToolbar,
    bool* isOpen
)
{
    const bool windowVisible =
        ImGui::Begin(
            "Pixel Editor",
            isOpen
        );

    if (!windowVisible)
    {
        ImGui::End();
        return;
    }

    if (drawToolbar)
    {
        drawToolbar();
        ImGui::Separator();
    }

    const ImVec2 contentPosition =
        ImGui::GetCursorScreenPos();

    ImVec2 canvasPosition(
        contentPosition.x + m_rulerWidth,
        contentPosition.y + m_rulerHeight
    );

    ImVec2 canvasSize =
        ImGui::GetContentRegionAvail();

    canvasSize.x -= m_rulerWidth;
    canvasSize.y -= m_rulerHeight;
    if (canvasSize.x < 1.0f)
    {
        canvasSize.x = 1.0f;
    }

    if (canvasSize.y < 1.0f)
    {
        canvasSize.y = 1.0f;
    }

    const int imageWidth =
        m_image.width();

    const int imageHeight =
        m_image.height();

    if (ImGui::IsWindowHovered() &&
        ImGui::IsKeyPressed(
            ImGuiKey_Space,
            false
        ))
    {
        m_fitToViewport = true;
    }

    if (m_fitToViewport)
    {
        const float padding =
            20.0f;

        const float availableWidth =
            std::max(
                1.0f,
                canvasSize.x -
                padding * 2.0f
            );

        const float availableHeight =
            std::max(
                1.0f,
                canvasSize.y -
                padding * 2.0f
            );

        const float pixelSizeX =
            availableWidth /
            static_cast<float>(imageWidth);

        const float pixelSizeY =
            availableHeight /
            static_cast<float>(imageHeight);

        m_pixelSize =
            std::clamp(
                std::min(
                    pixelSizeX,
                    pixelSizeY
                ),
                0.1f,
                64.0f
            );

        const float imageDisplayWidth =
            imageWidth *
            m_pixelSize;

        const float imageDisplayHeight =
            imageHeight *
            m_pixelSize;

        m_panX =
            (
                canvasSize.x -
                imageDisplayWidth
                ) * 0.5f;

        m_panY =
            (
                canvasSize.y -
                imageDisplayHeight
                ) * 0.5f;
    }
    const ImVec2 imagePosition(
        canvasPosition.x + m_panX,
        canvasPosition.y + m_panY
    );

    ImDrawList* drawList =
        ImGui::GetWindowDrawList();

    const ImVec2 canvasEnd(
        canvasPosition.x + canvasSize.x,
        canvasPosition.y + canvasSize.y
    );

    const ImU32 rulerBackgroundColor =
        IM_COL32(45, 45, 45, 255);

    drawList->AddRectFilled(
        ImVec2(
            canvasPosition.x,
            contentPosition.y
        ),
        ImVec2(
            canvasEnd.x,
            canvasPosition.y
        ),
        rulerBackgroundColor
    );

    drawList->AddRectFilled(
        ImVec2(
            contentPosition.x,
            canvasPosition.y
        ),
        ImVec2(
            canvasPosition.x,
            canvasEnd.y
        ),
        rulerBackgroundColor
    );

    drawList->AddRectFilled(
        contentPosition,
        canvasPosition,
        IM_COL32(38, 38, 38, 255)
    );

    drawList->AddRectFilled(
        canvasPosition,
        ImVec2(
            canvasPosition.x + canvasSize.x,
            canvasPosition.y + canvasSize.y
        ),
        IM_COL32(30, 30, 30, 255)
    );

    const int firstVisiblePixelX =
        std::clamp(
            static_cast<int>(
                std::floor(
                    (
                        canvasPosition.x -
                        imagePosition.x
                        ) / m_pixelSize
                )
                ),
            0,
            imageWidth
        );

    const int firstVisiblePixelY =
        std::clamp(
            static_cast<int>(
                std::floor(
                    (
                        canvasPosition.y -
                        imagePosition.y
                        ) / m_pixelSize
                )
                ),
            0,
            imageHeight
        );

    const int lastVisiblePixelX =
        std::clamp(
            static_cast<int>(
                std::ceil(
                    (
                        canvasPosition.x +
                        canvasSize.x -
                        imagePosition.x
                        ) / m_pixelSize
                )
                ),
            0,
            imageWidth
        );

    const int lastVisiblePixelY =
        std::clamp(
            static_cast<int>(
                std::ceil(
                    (
                        canvasPosition.y +
                        canvasSize.y -
                        imagePosition.y
                        ) / m_pixelSize
                )
                ),
            0,
            imageHeight
        );
    const int sampleStep =
        std::max(
            1,
            static_cast<int>(
                std::ceil(
                    2.0f /
                    m_pixelSize
                )
                )
        );
    const float gridWidth =
        imageWidth * m_pixelSize;

    const float gridHeight =
        imageHeight * m_pixelSize;

    const ImVec2 gridEnd(
        imagePosition.x + gridWidth,
        imagePosition.y + gridHeight
    );

    drawList->PushClipRect(
        canvasPosition,
        ImVec2(
            canvasPosition.x + canvasSize.x,
            canvasPosition.y + canvasSize.y
        ),
        true
    );

    drawList->AddRectFilled(
        imagePosition,
        gridEnd,
        IM_COL32(50, 50, 50, 255)
    );

    const int firstSampleX =
        firstVisiblePixelX /
        sampleStep *
        sampleStep;

    const int firstSampleY =
        firstVisiblePixelY /
        sampleStep *
        sampleStep;

    for (int y = firstSampleY;
        y < lastVisiblePixelY;
        y += sampleStep)
    {
        for (int x = firstSampleX;
            x < lastVisiblePixelX;
            x += sampleStep)
        {
            const PixelColor& color =
                m_image.pixel(x, y);

            if (color.alpha == 0)
            {
                continue;
            }

            const ImVec2 pixelStart(
                imagePosition.x +
                x * m_pixelSize,
                imagePosition.y +
                y * m_pixelSize
            );

            const ImVec2 pixelEnd(
                pixelStart.x +
                m_pixelSize * sampleStep,
                pixelStart.y +
                m_pixelSize * sampleStep
            );

            drawList->AddRectFilled(
                pixelStart,
                pixelEnd,
                IM_COL32(
                    color.red,
                    color.green,
                    color.blue,
                    color.alpha
                )
            );
        }
    }

    if (m_pixelSize >= 5.0f)
    {
        for (int x = firstVisiblePixelX;
            x <= lastVisiblePixelX;
            ++x)
        {
            const float lineX =
                imagePosition.x +
                x * m_pixelSize;

            drawList->AddLine(
                ImVec2(
                    lineX,
                    imagePosition.y +
                    firstVisiblePixelY *
                    m_pixelSize
                ),
                ImVec2(
                    lineX,
                    imagePosition.y +
                    lastVisiblePixelY *
                    m_pixelSize
                ),
                IM_COL32(25, 25, 25, 255)
            );
        }

        for (int y = firstVisiblePixelY;
            y <= lastVisiblePixelY;
            ++y)
        {
            const float lineY =
                imagePosition.y +
                y * m_pixelSize;

            drawList->AddLine(
                ImVec2(
                    imagePosition.x +
                    firstVisiblePixelX *
                    m_pixelSize,
                    lineY
                ),
                ImVec2(
                    imagePosition.x +
                    lastVisiblePixelX *
                    m_pixelSize,
                    lineY
                ),
                IM_COL32(25, 25, 25, 255)
            );
        }
    }
    const ImU32 helperGridColor =
        IM_COL32(90, 90, 90, 255);

    const float guideExtension =
        8.0f;

    for (int x = 4;
        x < imageWidth;
        x += 4)
    {
        const float lineX =
            imagePosition.x +
            x * m_pixelSize;

        drawList->AddLine(
            ImVec2(
                lineX,
                imagePosition.y -
                guideExtension
            ),
            ImVec2(
                lineX,
                gridEnd.y +
                guideExtension
            ),
            helperGridColor,
            1.0f
        );
    }

    for (int y = 4;
        y < imageHeight;
        y += 4)
    {
        const float lineY =
            imagePosition.y +
            y * m_pixelSize;

        drawList->AddLine(
            ImVec2(
                imagePosition.x -
                guideExtension,
                lineY
            ),
            ImVec2(
                gridEnd.x +
                guideExtension,
                lineY
            ),
            helperGridColor,
            1.0f
        );
    }

    drawList->PopClipRect();
    
    const int firstRulerPixelX =
        static_cast<int>(
            std::floor(
                (
                    canvasPosition.x -
                    imagePosition.x
                    ) / m_pixelSize
            )
            );

    const int lastRulerPixelX =
        static_cast<int>(
            std::ceil(
                (
                    canvasEnd.x -
                    imagePosition.x
                    ) / m_pixelSize
            )
            );

    const int firstRulerPixelY =
        static_cast<int>(
            std::floor(
                (
                    canvasPosition.y -
                    imagePosition.y
                    ) / m_pixelSize
            )
            );

    const int lastRulerPixelY =
        static_cast<int>(
            std::ceil(
                (
                    canvasEnd.y -
                    imagePosition.y
                    ) / m_pixelSize
            )
            );

    int rulerStep = 1;

    while (m_pixelSize * rulerStep < 14.0f)
    {
        rulerStep *= 2;
    }

    const int labelStep =
        rulerStep * 4;

    const int firstRulerTickX =
        static_cast<int>(
            std::floor(
                static_cast<float>(
                    firstRulerPixelX
                    ) / rulerStep
            )
            ) * rulerStep;

    const int firstRulerTickY =
        static_cast<int>(
            std::floor(
                static_cast<float>(
                    firstRulerPixelY
                    ) / rulerStep
            )
            ) * rulerStep;
    const ImU32 rulerLineColor =
        IM_COL32(180, 180, 180, 255);

    for (int x = firstRulerTickX;
        x <= lastRulerPixelX;
        x += rulerStep)
    {
        const float lineX =
            imagePosition.x +
            x * m_pixelSize;

        const float tickLength =
            x % labelStep == 0
            ? 10.0f
            : 5.0f;

        drawList->AddLine(
            ImVec2(
                lineX,
                canvasPosition.y
            ),
            ImVec2(
                lineX,
                canvasPosition.y -
                tickLength
            ),
            rulerLineColor
        );
        if (x % labelStep == 0)
        {
            const std::string label =
                std::to_string(x);

            const ImVec2 labelSize =
                ImGui::CalcTextSize(
                    label.c_str()
                );

            drawList->AddText(
                ImVec2(
                    lineX -
                    labelSize.x * 0.5f,
                    contentPosition.y + 3.0f
                ),
                IM_COL32(210, 210, 210, 255),
                label.c_str()
            );
        }
    }

    for (int y = firstRulerTickY;
        y <= lastRulerPixelY;
        y += rulerStep)
    {
        const float lineY =
            imagePosition.y +
            y * m_pixelSize;

        const float tickLength =
            y % labelStep == 0
            ? 10.0f
            : 5.0f;

        drawList->AddLine(
            ImVec2(
                canvasPosition.x,
                lineY
            ),
            ImVec2(
                canvasPosition.x -
                tickLength,
                lineY
            ),
            rulerLineColor
        );
        if (y % labelStep == 0)
        {
            const std::string label =
                std::to_string(y);

            const ImVec2 labelSize =
                ImGui::CalcTextSize(
                    label.c_str()
                );

            drawList->AddText(
                ImVec2(
                    canvasPosition.x -
                    labelSize.x -
                    13.0f,
                    lineY -
                    labelSize.y * 0.5f
                ),
                IM_COL32(210, 210, 210, 255),
                label.c_str()
            );
        }
    }

    ImGui::SetCursorScreenPos(
        canvasPosition
    );

    ImGui::InvisibleButton(
        "##PixelCanvas",
        canvasSize
    );

    m_isMouseOverCanvas =
        ImGui::IsItemHovered();

    const bool isLeftButtonPanning =
        activeTool == PixelTool::Pan &&
        ImGui::IsMouseDragging(
            ImGuiMouseButton_Left,
            0.0f
        );

    if (ImGui::IsItemHovered() &&
        isLeftButtonPanning)
    {
        const ImVec2 mouseDelta =
            ImGui::GetIO().MouseDelta;

        m_fitToViewport = false;

        m_panX += mouseDelta.x;
        m_panY += mouseDelta.y;
    }

    if (ImGui::IsItemHovered() &&
        ImGui::GetIO().KeyCtrl)
    {
        const float mouseWheel =
            ImGui::GetIO().MouseWheel;

        if (mouseWheel != 0.0f)
        {
            m_fitToViewport = false;

            const ImVec2 mousePosition =
                ImGui::GetMousePos();

            const float pixelX =
                (
                    mousePosition.x -
                    imagePosition.x
                    ) / m_pixelSize;

            const float pixelY =
                (
                    mousePosition.y -
                    imagePosition.y
                    ) / m_pixelSize;

            const float zoomFactor =
                std::pow(
                    1.25f,
                    mouseWheel
                );

            const float newPixelSize =
                std::clamp(
                    m_pixelSize * zoomFactor,
                    0.1f,
                    64.0f
                );

            m_panX =
                mousePosition.x -
                canvasPosition.x -
                pixelX * newPixelSize;

            m_panY =
                mousePosition.y -
                canvasPosition.y -
                pixelY * newPixelSize;

            m_pixelSize =
                newPixelSize;
        }
    }

    if (ImGui::IsMouseClicked(
        ImGuiMouseButton_Left
    ))
    {
        m_pencilPixelsThisStroke.clear();
    }

    if (ImGui::IsItemHovered() &&
        ImGui::IsMouseDown(
            ImGuiMouseButton_Left
        ))
    {
        const ImVec2 mousePosition =
            ImGui::GetMousePos();

        const int pixelX =
            static_cast<int>(
                (
                    mousePosition.x -
                    imagePosition.x
                    ) / m_pixelSize
                );

        const int pixelY =
            static_cast<int>(
                (
                    mousePosition.y -
                    imagePosition.y
                    ) / m_pixelSize
                );

        if (pixelX >= 0 &&
            pixelX < imageWidth &&
            pixelY >= 0 &&
            pixelY < imageHeight)
        {
            switch (activeTool)
            {
         
            case PixelTool::Pencil:
            {
                const int pixelIndex =
                    pixelY * imageWidth +
                    pixelX;

                const bool firstVisit =
                    m_pencilPixelsThisStroke.insert(
                        pixelIndex
                    ).second;

                if (!firstVisit)
                {
                    break;
                }

                PixelColor& pixel =
                    m_image.pixel(
                        pixelX,
                        pixelY
                    );

                if (pixel.alpha != 0)
                {
                    pixel =
                    {
                        0,
                        0,
                        0,
                        0
                    };
                }
                else
                {
                    pixel =
                    {
                        255,
                        255,
                        255,
                        255
                    };
                }

                break;
            }
         

            case PixelTool::Eraser:
                m_image.pixel(pixelX, pixelY) =
                {
                    0,
                    0,
                    0,
                    0
                };
                break;

            case PixelTool::Pan:
                break;
            }
        }
    }

    ImGui::End();
}

const PixelImage& PixelCanvasWindow::image() const
{
    return m_image;
}

bool PixelCanvasWindow::setImage(
    PixelImage image
)
{
    const int largestDimension =
        std::max(
            image.width(),
            image.height()
        );

    m_pixelSize =
        640.0f /
        static_cast<float>(
            largestDimension
            );

    m_loadedSourceImage = image;
    m_hasLoadedImage = true;

    m_image =
        std::move(image);

    m_panX = 0.0f;
    m_panY = 0.0f;

    return true;
}

void PixelCanvasWindow::resizeLoadedImage(
    int size
)
{
    // Aktuellen bearbeiteten Zustand kopieren.
    const PixelImage sourceImage =
        m_image;

    PixelImage resizedImage{
        size,
        size
    };

    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            const int sourceX =
                x *
                sourceImage.width() /
                size;

            const int sourceY =
                y *
                sourceImage.height() /
                size;

            resizedImage.pixel(x, y) =
                sourceImage.pixel(
                    sourceX,
                    sourceY
                );
        }
    }

    m_image =
        std::move(resizedImage);

    m_loadedSourceImage =
        m_image;

    m_pixelSize =
        640.0f /
        static_cast<float>(size);
}

void PixelCanvasWindow::newImage()
{
    constexpr int imageSize = 32;

    m_image =
        PixelImage(
            imageSize,
            imageSize
        );

    m_pixelSize =
        640.0f /
        static_cast<float>(imageSize);

    m_hasLoadedImage = false;
    m_iconLoaded = true;

    m_panX = 0.0f;
    m_panY = 0.0f;
}

float PixelCanvasWindow::pixelSize() const
{
    return m_pixelSize;
}

float PixelCanvasWindow::panX() const
{
    return m_panX;
}

float PixelCanvasWindow::panY() const
{
    return m_panY;
}

void PixelCanvasWindow::setView(
    float pixelSize,
    float panX,
    float panY
)
{
    m_pixelSize =
        std::clamp(
            pixelSize,
            0.1f,
            64.0f
        );

    m_panX = panX;
    m_panY = panY;

    m_fitToViewport = false;
}

bool PixelCanvasWindow::isMouseOverCanvas() const
{
    return m_isMouseOverCanvas;
}
