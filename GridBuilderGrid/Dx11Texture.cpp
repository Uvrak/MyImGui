#include "Dx11Texture.h"

Dx11Texture::Dx11Texture(
    ID3D11Device* device
)
    :
    m_device(
        device
    )
{}

Dx11Texture::~Dx11Texture()
{
    clear();
}

bool Dx11Texture::create(
    const uint8_t* pixels,
    uint32_t width,
    uint32_t height,
    uint32_t pitch
)
{
    clear();

    if (m_device == nullptr ||
        pixels == nullptr ||
        width == 0 ||
        height == 0)
    {
        return false;
    }

    D3D11_TEXTURE2D_DESC description{};

    description.Width =
        width;

    description.Height =
        height;

    description.MipLevels =
        1;

    description.ArraySize =
        1;

    description.Format =
        DXGI_FORMAT_R8G8B8A8_UNORM;

    description.SampleDesc.Count =
        1;

    description.Usage =
        D3D11_USAGE_IMMUTABLE;

    description.BindFlags =
        D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initialData{};

    initialData.pSysMem =
        pixels;

    initialData.SysMemPitch =
        pitch;

    HRESULT result =
        m_device->CreateTexture2D(
            &description,
            &initialData,
            &m_texture
        );

    if (FAILED(result))
    {
        clear();

        return false;
    }

    result =
        m_device->CreateShaderResourceView(
            m_texture,
            nullptr,
            &m_textureView
        );

    if (FAILED(result))
    {
        clear();

        return false;
    }

    return true;
}

ID3D11ShaderResourceView*
Dx11Texture::textureView() const
{
    return m_textureView;
}

void Dx11Texture::clear()
{
    if (m_textureView != nullptr)
    {
        m_textureView->Release();

        m_textureView =
            nullptr;
    }

    if (m_texture != nullptr)
    {
        m_texture->Release();

        m_texture =
            nullptr;
    }
}