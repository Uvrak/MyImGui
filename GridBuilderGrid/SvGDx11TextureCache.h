#pragma once

#include <d3d11.h>

#include <memory>
#include <string>
#include <unordered_map>

#include "Dx11Texture.h"

class SvgDx11TextureCache
{
public:
    explicit SvgDx11TextureCache(
        ID3D11Device* device
    );

    ~SvgDx11TextureCache() = default;

    ID3D11ShaderResourceView* texture(
        const std::string& filename,
        int width,
        int height
    );

    void clear();

private:
    std::string createKey(
        const std::string& filename,
        int width,
        int height
    ) const;

private:
    ID3D11Device* m_device =
        nullptr;

    std::unordered_map<
        std::string,
        std::unique_ptr<Dx11Texture>
    > m_textures;
};