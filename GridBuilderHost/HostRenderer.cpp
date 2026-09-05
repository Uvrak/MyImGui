#include "HostRenderer.h"

#include "FrameTexture.h"

#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

#include <cstring>
#include <cstdio>
#include <cstdint>
#include <Windows.h>

namespace GridBuilderHost
{
    HostRenderer::HostRenderer()
    {}

    HostRenderer::~HostRenderer()
    {
        if (m_uvBuffer != nullptr)
        {
            m_uvBuffer->Release();
            m_uvBuffer = nullptr;
        }

        if (m_sampler != nullptr)
        {
            m_sampler->Release();
            m_sampler = nullptr;
        }

        if (m_pixelShader != nullptr)
        {
            m_pixelShader->Release();
            m_pixelShader = nullptr;
        }

        if (m_vertexShader != nullptr)
        {
            m_vertexShader->Release();
            m_vertexShader = nullptr;
        }

        if (m_renderTargetView != nullptr)
        {
            m_renderTargetView->Release();
            m_renderTargetView = nullptr;
        }

        if (m_swapChain != nullptr)
        {
            m_swapChain->Release();
            m_swapChain = nullptr;
        }

        if (m_context != nullptr)
        {
            m_context->Release();
            m_context = nullptr;
        }

        if (m_device != nullptr)
        {
            m_device->Release();
            m_device = nullptr;
        }
    }

    bool HostRenderer::initialize(
        HWND windowHandle
    )
    {
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

        swapChainDesc.BufferCount = 2;

        swapChainDesc.BufferDesc.Format =
            DXGI_FORMAT_R8G8B8A8_UNORM;

        swapChainDesc.BufferUsage =
            DXGI_USAGE_RENDER_TARGET_OUTPUT;

        swapChainDesc.OutputWindow =
            windowHandle;

        swapChainDesc.SampleDesc.Count = 1;

        swapChainDesc.Windowed =
            TRUE;

        swapChainDesc.SwapEffect =
            DXGI_SWAP_EFFECT_DISCARD;

        D3D_FEATURE_LEVEL featureLevel;

        const HRESULT result =
            D3D11CreateDeviceAndSwapChain(
                nullptr,
                D3D_DRIVER_TYPE_HARDWARE,
                nullptr,
                0,
                nullptr,
                0,
                D3D11_SDK_VERSION,
                &swapChainDesc,
                &m_swapChain,
                &m_device,
                &featureLevel,
                &m_context
            );

        if (FAILED(result))
        {
            return false;
        }

        ID3D11Texture2D* backBuffer =
            nullptr;

        const HRESULT backBufferResult =
            m_swapChain->GetBuffer(
                0,
                __uuidof(ID3D11Texture2D),
                reinterpret_cast<void**>(
                    &backBuffer
                    )
            );

        if (FAILED(backBufferResult))
        {
            return false;
        }

        const HRESULT renderTargetResult =
            m_device->CreateRenderTargetView(
                backBuffer,
                nullptr,
                &m_renderTargetView
            );

        backBuffer->Release();

        if (FAILED(renderTargetResult))
        {
            return false;
        }

        const char* vertexShaderSource = R"(
struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOutput main(uint vertexId : SV_VertexID)
{
    VSOutput output;

    float2 positions[3] =
    {
        float2(-1.0, -1.0),
        float2(-1.0,  3.0),
        float2( 3.0, -1.0)
    };

    float2 uvs[3] =
    {
        float2(0.0, 1.0),
        float2(0.0, -1.0),
        float2(2.0, 1.0)
    };

    output.position =
        float4(
            positions[vertexId],
            0.0,
            1.0
        );

    output.uv =
        uvs[vertexId];

    return output;
}
)";

        const char* pixelShaderSource = R"(
cbuffer UVBuffer : register(b0)
{
    float2 uvMax;
    float2 padding;
};

Texture2D frameTexture : register(t0);
SamplerState frameSampler : register(s0);

float4 main(
    float4 position : SV_POSITION,
    float2 uv : TEXCOORD0
) : SV_TARGET
{
    float2 croppedUV =
        uv * uvMax;

    return frameTexture.Sample(
        frameSampler,
        croppedUV
    );
}
)";
        ID3DBlob* vertexShaderBlob =
            nullptr;

        ID3DBlob* pixelShaderBlob =
            nullptr;

        HRESULT shaderResult =
            D3DCompile(
                vertexShaderSource,
                std::strlen(
                    vertexShaderSource
                ),
                nullptr,
                nullptr,
                nullptr,
                "main",
                "vs_5_0",
                0,
                0,
                &vertexShaderBlob,
                nullptr
            );

        if (FAILED(shaderResult))
        {
            return false;
        }

        shaderResult =
            D3DCompile(
                pixelShaderSource,
                std::strlen(
                    pixelShaderSource
                ),
                nullptr,
                nullptr,
                nullptr,
                "main",
                "ps_5_0",
                0,
                0,
                &pixelShaderBlob,
                nullptr
            );

        if (FAILED(shaderResult))
        {
            vertexShaderBlob->Release();
            return false;
        }

        shaderResult =
            m_device->CreateVertexShader(
                vertexShaderBlob->GetBufferPointer(),
                vertexShaderBlob->GetBufferSize(),
                nullptr,
                &m_vertexShader
            );

        if (SUCCEEDED(shaderResult))
        {
            shaderResult =
                m_device->CreatePixelShader(
                    pixelShaderBlob->GetBufferPointer(),
                    pixelShaderBlob->GetBufferSize(),
                    nullptr,
                    &m_pixelShader
                );
        }

        vertexShaderBlob->Release();
        pixelShaderBlob->Release();

        if (FAILED(shaderResult))
        {
            return false;
        }

        D3D11_SAMPLER_DESC samplerDesc = {};

        samplerDesc.Filter =
            D3D11_FILTER_MIN_MAG_MIP_POINT;

        samplerDesc.AddressU =
            D3D11_TEXTURE_ADDRESS_CLAMP;

        samplerDesc.AddressV =
            D3D11_TEXTURE_ADDRESS_CLAMP;

        samplerDesc.AddressW =
            D3D11_TEXTURE_ADDRESS_CLAMP;

        shaderResult =
            m_device->CreateSamplerState(
                &samplerDesc,
                &m_sampler
            );

        D3D11_BUFFER_DESC bufferDesc = {};

        bufferDesc.ByteWidth =
            16;

        bufferDesc.Usage =
            D3D11_USAGE_DEFAULT;

        bufferDesc.BindFlags =
            D3D11_BIND_CONSTANT_BUFFER;

        shaderResult =
            m_device->CreateBuffer(
                &bufferDesc,
                nullptr,
                &m_uvBuffer
            );

        if (FAILED(shaderResult))
        {
            return false;
        }

        return SUCCEEDED(
            shaderResult
        );
    }

    void HostRenderer::beginFrame()
    {
        if (m_context == nullptr ||
            m_renderTargetView == nullptr)
        {
            return;
        }

        const float clearColor[4] =
        {
            0.1f,
            0.1f,
            0.1f,
            1.0f
        };

        m_context->OMSetRenderTargets(
            1,
            &m_renderTargetView,
            nullptr
        );

        m_context->ClearRenderTargetView(
            m_renderTargetView,
            clearColor
        );
    }

    void HostRenderer::present()
    {
        if (m_swapChain == nullptr)
        {
            return;
        }

        m_swapChain->Present(
            1,
            0
        );
    }

    void HostRenderer::resize(
        uint32_t width,
        uint32_t height
    )
    {
        if (width == 0 ||
            height == 0)
        {
            return;
        }

        if (m_renderTargetView != nullptr)
        {
            m_renderTargetView->Release();
            m_renderTargetView = nullptr;
        }

        HRESULT result =
            m_swapChain->ResizeBuffers(
                0,
                width,
                height,
                DXGI_FORMAT_UNKNOWN,
                0
            );

        if (FAILED(result))
        {
            return;
        }

        ID3D11Texture2D* backBuffer =
            nullptr;

        result =
            m_swapChain->GetBuffer(
                0,
                __uuidof(ID3D11Texture2D),
                reinterpret_cast<void**>(
                    &backBuffer
                    )
            );

        result =
            m_device->CreateRenderTargetView(
                backBuffer,
                nullptr,
                &m_renderTargetView
            );

        backBuffer->Release();

        if (FAILED(result))
        {
            return;
        }

        if (FAILED(result))
        {
            return;
        }
    }

    ID3D11Device* HostRenderer::device() const
    {
        return m_device;
    }

    ID3D11DeviceContext* HostRenderer::context() const
    {
        return m_context;
    }
    
    void HostRenderer::updateViewport(
        uint32_t clientWidth,
        uint32_t clientHeight,
        uint32_t contentWidth,
        uint32_t contentHeight
    )
    {
        if (m_clientWidth == clientWidth &&
            m_clientHeight == clientHeight &&
            m_contentWidth == contentWidth &&
            m_contentHeight == contentHeight)
        {
            return;
        }

        m_clientWidth =
            clientWidth;

        m_clientHeight =
            clientHeight;

        m_contentWidth =
            contentWidth;

        m_contentHeight =
            contentHeight;

        if (contentWidth == 0 ||
            contentHeight == 0)
        {
            return;
        }

        const float clientWidthFloat =
            static_cast<float>(
                clientWidth
                );

        const float clientHeightFloat =
            static_cast<float>(
                clientHeight
                );

        const float scaleX =
            clientWidthFloat /
            static_cast<float>(
                contentWidth
                );

        const float scaleY =
            clientHeightFloat /
            static_cast<float>(
                contentHeight
                );

        const float scale =
            (scaleX < scaleY)
            ? scaleX
            : scaleY;

        const float imageWidth =
            static_cast<float>(
                contentWidth
                ) * scale;

        const float imageHeight =
            static_cast<float>(
                contentHeight
                ) * scale;

        D3D11_VIEWPORT viewport = {};

        viewport.TopLeftX =
            (clientWidthFloat - imageWidth) *
            0.5f;

        viewport.TopLeftY =
            (clientHeightFloat - imageHeight) *
            0.5f;

        viewport.Width =
            imageWidth;

        viewport.Height =
            imageHeight;

        viewport.MinDepth =
            0.0f;

        viewport.MaxDepth =
            1.0f;

        m_context->RSSetViewports(
            1,
            &viewport
        );
    }
}