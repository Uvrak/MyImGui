#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif

#include <d3d11.h>

#include <winrt/base.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>

namespace MyImGui
{
    class WindowCapture
    {
    public:
        WindowCapture(
            ID3D11Device* device,
            ID3D11DeviceContext* deviceContext
        );

        ~WindowCapture();

        bool setWindow(
            HWND window
        );

        bool windowSize(
            int& width,
            int& height
        ) const;

        bool start();
        bool update();

        ID3D11ShaderResourceView* textureView() const;

    private:
        ID3D11Device* m_device = nullptr;

        ID3D11DeviceContext*
            m_deviceContext = nullptr;

        ID3D11Texture2D* m_texture = nullptr;

        ID3D11ShaderResourceView*
            m_textureView = nullptr;

        HWND m_window = nullptr;

        winrt::Windows::Graphics::Capture::
            GraphicsCaptureItem m_captureItem{
                nullptr
        };

        winrt::Windows::Graphics::DirectX::Direct3D11::
            IDirect3DDevice m_direct3DDevice{
                nullptr
        };

        winrt::Windows::Graphics::Capture::
            Direct3D11CaptureFramePool m_framePool{
                nullptr
        };

        winrt::Windows::Graphics::Capture::
            GraphicsCaptureSession m_captureSession{
                nullptr
        };
    };
}
