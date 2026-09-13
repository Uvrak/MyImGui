#pragma once

#include <cstdint>
#include <d3d11.h>

class Dx11Texture
{
public:
    explicit Dx11Texture(
        ID3D11Device* device
    );

    ~Dx11Texture();

    Dx11Texture(
        const Dx11Texture&
    ) = delete;

    Dx11Texture& operator=(
        const Dx11Texture&
        ) = delete;

    bool create(
        const uint8_t* pixels,
        uint32_t width,
        uint32_t height,
        uint32_t pitch
    );

    ID3D11ShaderResourceView*
        textureView() const;

private:
    void clear();

private:
    ID3D11Device* m_device =
        nullptr;

    ID3D11Texture2D* m_texture =
        nullptr;

    ID3D11ShaderResourceView*
        m_textureView =
        nullptr;
};