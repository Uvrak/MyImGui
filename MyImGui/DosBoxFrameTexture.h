#pragma once

#include <cstdint>
#include <d3d11.h>

namespace MyImGui
{

    class DosBoxFrameTexture
    {
    public:
        DosBoxFrameTexture(
            ID3D11Device* device,
            ID3D11DeviceContext* context
        );

        ~DosBoxFrameTexture();

        bool update(
            const uint8_t* pixels,
            uint32_t width,
            uint32_t height,
            uint32_t pitch
        );

        void reset();

        ID3D11ShaderResourceView* textureView() const;

    private:
        ID3D11Device* m_device = nullptr;
        ID3D11DeviceContext* m_context = nullptr;

        ID3D11Texture2D* m_texture = nullptr;
        ID3D11ShaderResourceView* m_textureView = nullptr;

        uint32_t m_width = 0;
        uint32_t m_height = 0;
    };

}