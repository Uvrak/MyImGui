#include "pch.h"
#include "DosBoxFrameTexture.h"

namespace MyImGui
{

    DosBoxFrameTexture::DosBoxFrameTexture(
        ID3D11Device* device,
        ID3D11DeviceContext* context
    )
        : m_device(device),
        m_context(context)
    {}

    DosBoxFrameTexture::~DosBoxFrameTexture()
    {
        if (m_textureView != nullptr)
        {
            m_textureView->Release();
            m_textureView = nullptr;
        }

        if (m_texture != nullptr)
        {
            m_texture->Release();
            m_texture = nullptr;
        }
    }

    bool DosBoxFrameTexture::update(
        const uint8_t* pixels,
        uint32_t width,
        uint32_t height,
        uint32_t pitch
    )
    {
        if (m_device == nullptr ||
            m_context == nullptr ||
            pixels == nullptr ||
            width == 0 ||
            height == 0 ||
            pitch == 0)
        {
            return false;
        }

        if (m_texture != nullptr &&
            m_width == width &&
            m_height == height)
        {
            m_context->UpdateSubresource(
                m_texture,
                0,
                nullptr,
                pixels,
                pitch,
                0
            );

            return true;
        }

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8X8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA data = {};
        data.pSysMem = pixels;
        data.SysMemPitch = pitch;

        HRESULT result =
            m_device->CreateTexture2D(
                &desc,
                &data,
                &m_texture
            );

        if (FAILED(result))
        {
            return false;
        }

        m_width = width;
        m_height = height;

        result =
            m_device->CreateShaderResourceView(
                m_texture,
                nullptr,
                &m_textureView
            );

        return SUCCEEDED(result);
    }

    ID3D11ShaderResourceView* DosBoxFrameTexture::textureView() const
    {
        return m_textureView;
    }

}