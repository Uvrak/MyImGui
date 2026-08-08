#include "pch.h"
#include "WindowCapture.h"

#include <windows.graphics.directx.direct3d11.interop.h>
#include <windows.graphics.directx.direct3d11.h>
#include <windows.graphics.capture.interop.h>
#include <Unknwn.h>
#include <dxgi.h>

struct __declspec(uuid("A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1"))
    IDirect3DDxgiInterfaceAccess : IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetInterface(
        REFIID iid,
        void** object
    ) = 0;
};
namespace MyImGui
{
    WindowCapture::WindowCapture(
        ID3D11Device* device,
        ID3D11DeviceContext* deviceContext
    )
        : m_device(device),
        m_deviceContext(deviceContext)
    {

        if (m_device == nullptr)
        {
            return;
        }

        winrt::com_ptr<IDXGIDevice> dxgiDevice;

        HRESULT result =
            m_device->QueryInterface(
                IID_PPV_ARGS(
                    dxgiDevice.put()
                )
            );

        if (FAILED(result))
        {
            return;
        }

        winrt::com_ptr<IInspectable>
            inspectable;

        result =
            CreateDirect3D11DeviceFromDXGIDevice(
                dxgiDevice.get(),
                inspectable.put()
            );

        if (FAILED(result))
        {
            return;
        }

        m_direct3DDevice =
            inspectable.as<
            winrt::Windows::Graphics::
            DirectX::Direct3D11::
            IDirect3DDevice
            >();
    }

    WindowCapture::~WindowCapture()
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

    bool WindowCapture::setWindow(
        HWND window
    )
    {
        m_window = nullptr;
        m_captureItem = nullptr;

        if (window == nullptr)
        {
            return false;
        }

        auto interop =
            winrt::get_activation_factory<
            winrt::Windows::Graphics::Capture::
            GraphicsCaptureItem,
            IGraphicsCaptureItemInterop
            >();

        winrt::Windows::Graphics::Capture::
            GraphicsCaptureItem captureItem{
                nullptr
        };

        HRESULT result =
            interop->CreateForWindow(
                window,
                winrt::guid_of<
                ABI::Windows::Graphics::Capture::
                IGraphicsCaptureItem
                >(),
                winrt::put_abi(
                    captureItem
                )
            );

        if (FAILED(result))
        {
            return false;
        }

        m_window = window;
        m_captureItem = captureItem;

        return true;
    }

    bool WindowCapture::windowSize(
        int& width,
        int& height
    ) const
    {
        width = 0;
        height = 0;

        if (m_window == nullptr)
        {
            return false;
        }

        RECT clientRect = {};

        if (!GetClientRect(
            m_window,
            &clientRect
        ))
        {
            return false;
        }

        width =
            clientRect.right -
            clientRect.left;

        height =
            clientRect.bottom -
            clientRect.top;

        return width > 0 &&
            height > 0;
    }
    bool WindowCapture::start()
    {
        if (m_captureItem == nullptr ||
            m_direct3DDevice == nullptr)
        {
            return false;
        }

        auto size =
            m_captureItem.Size();

        if (size.Width <= 0 ||
            size.Height <= 0)
        {
            return false;
        }

        m_framePool =
            winrt::Windows::Graphics::Capture::
            Direct3D11CaptureFramePool::CreateFreeThreaded(
                m_direct3DDevice,
                winrt::Windows::Graphics::DirectX::
                DirectXPixelFormat::
                B8G8R8A8UIntNormalized,
                2,
                size
            );

        m_captureSession =
            m_framePool.CreateCaptureSession(
                m_captureItem
            );

        m_captureSession.StartCapture();

        return true;
    }

    bool WindowCapture::update()
    {
        if (m_framePool == nullptr)
        {
            return false;
        }

        auto frame =
            m_framePool.TryGetNextFrame();

        if (frame == nullptr)
        {
            return false;
        }

        auto surface =
            frame.Surface();

        auto access =
            surface.as<
            IDirect3DDxgiInterfaceAccess
            >();

        winrt::com_ptr<ID3D11Texture2D>
            capturedTexture;

        HRESULT result =
            access->GetInterface(
                __uuidof(ID3D11Texture2D),
                capturedTexture.put_void()
            );

        if (FAILED(result))
        {
            return false;
        }

        D3D11_TEXTURE2D_DESC capturedDesc = {};

        capturedTexture->GetDesc(
            &capturedDesc
        );

        if (m_texture == nullptr)
        {
            D3D11_TEXTURE2D_DESC textureDesc =
                capturedDesc;

            textureDesc.BindFlags =
                D3D11_BIND_SHADER_RESOURCE;

            textureDesc.MiscFlags = 0;

            textureDesc.CPUAccessFlags = 0;

            HRESULT textureResult =
                m_device->CreateTexture2D(
                    &textureDesc,
                    nullptr,
                    &m_texture
                );

            if (FAILED(textureResult))
            {
                return false;
            }

            HRESULT viewResult =
                m_device->CreateShaderResourceView(
                    m_texture,
                    nullptr,
                    &m_textureView
                );

            if (FAILED(viewResult))
            {
                m_texture->Release();
                m_texture = nullptr;

                return false;
            }
        }

        m_deviceContext->CopyResource(
            m_texture,
            capturedTexture.get()
        );

        return true;
    }

    ID3D11ShaderResourceView* WindowCapture::textureView() const
    {
        return m_textureView;
    }
}