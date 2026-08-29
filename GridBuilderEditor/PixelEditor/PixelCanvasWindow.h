#pragma once

#include "PixelImage.h"
#include "PixelTool.h"
#include <functional>
#include <unordered_set>

struct ImFont;

class PixelCanvasWindow
{
public:
    void draw(
        PixelTool activeTool,
        ImFont* iconFont,
        const std::function<void()>& drawToolbar,
        bool* isOpen
    );

    const PixelImage& image() const;

    bool setImage(
        PixelImage image
    );
    void resizeLoadedImage(int size);

    void newImage();

    float pixelSize() const;
    float panX() const;
    float panY() const;

    void setView(
        float pixelSize,
        float panX,
        float panY
    );

    bool isMouseOverCanvas() const;

private:
    PixelImage m_image{ 32, 32 };

    float m_pixelSize = 20.0f;
    float m_rulerWidth = 56.0f;
    float m_rulerHeight = 32.0f;

    bool m_iconLoaded = true;

    PixelImage m_loadedSourceImage{ 32, 32 };
    bool m_hasLoadedImage = false;

    float m_panX = 0.0f;
    float m_panY = 0.0f;

    bool m_fitToViewport = false;

    bool m_isMouseOverCanvas = false;

    std::unordered_set<int>
        m_pencilPixelsThisStroke;

};