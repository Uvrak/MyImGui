#pragma once

#include <cstdint>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11SamplerState;
struct IDXGISwapChain;
struct ID3D11Buffer;

#ifdef _WIN32
struct HWND__;
using HWND = HWND__*;
#endif

namespace DosBoxX
{
    class FrameTexture;
}

namespace GridBuilderHost
{
    class HostRenderer
    {
    public:
        HostRenderer();
        ~HostRenderer();

        bool initialize(
            HWND windowHandle
        );

        void render(
            DosBoxX::FrameTexture& frameTexture,
            uint32_t contentWidth,
            uint32_t contentHeight
        );

        void resize(
            uint32_t width,
            uint32_t height
        );

        ID3D11Device* device() const;
        ID3D11DeviceContext* context() const;

    private:
        void updateViewport(
            uint32_t clientWidth,
            uint32_t clientHeight,
            uint32_t contentWidth,
            uint32_t contentHeight
        );

        ID3D11Device* m_device =
            nullptr;

        ID3D11DeviceContext* m_context =
            nullptr;

        IDXGISwapChain* m_swapChain =
            nullptr;

        ID3D11RenderTargetView* m_renderTargetView =
            nullptr;

        ID3D11VertexShader* m_vertexShader =
            nullptr;

        ID3D11PixelShader* m_pixelShader =
            nullptr;

        ID3D11SamplerState* m_sampler =
            nullptr;

        ID3D11Buffer* m_uvBuffer =
            nullptr;

        float m_uMax =
            1.0f;

        float m_vMax =
            1.0f;

        uint32_t m_clientWidth =
            0;

        uint32_t m_clientHeight =
            0;

        uint32_t m_contentWidth =
            0;

        uint32_t m_contentHeight =
            0;
    };
}