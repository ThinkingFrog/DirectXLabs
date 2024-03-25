#pragma once

#include <d3d11.h>
#include "Render.h"

class GlobalState {
public:
    HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
    HRESULT InitAdapter();
    HRESULT InitDevice();
    HRESULT InitSwapChain();
    HRESULT InitDirectX();
    void CleanupDirectX();
    void RenderFrame();
    void ResizeWindow(LPARAM lParam);

private:
    HWND m_hwnd = nullptr;
    ID3D11Device* m_device = nullptr;
    IDXGISwapChain* m_swap_chain = nullptr;
    ID3D11DeviceContext* m_device_context = nullptr;
    IDXGIAdapter* m_adapter = nullptr;
    Render* m_render = nullptr;

    LONG m_width;
    LONG m_height;

    std::string m_vertex_shader_path = "VertexShader.hlsl";
    std::string m_pixel_shader_path = "PixelShader.hlsl";
};
