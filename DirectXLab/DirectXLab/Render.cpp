#include <vector>
#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <fstream>

#include "Render.h"
#include "utils.h"

Render::~Render() {
    CleanupRenderTargetView();
    SafeRelease(&m_vertex_buffer);
    SafeRelease(&m_index_buffer);
    SafeRelease(&m_vertex_shader);
    SafeRelease(&m_pixel_shader);
    SafeRelease(&m_input_layout);
    SafeRelease(&m_code);
}

void Render::CreateRenderTargetView(IDXGISwapChain* swap_chain, ID3D11Device* device) {
    ID3D11Texture2D* backBuffer;
    swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));

    device->CreateRenderTargetView(backBuffer, nullptr, &m_render_target_view);

    SafeRelease(&backBuffer);
}

void Render::SetGeometry(ID3D11Device* device) {
    static const Vertex Vertices[] = {
        {-0.5f, -0.5f, 0.0f, RGB(255, 0, 0)},
        {0.5f, -0.5f, 0.0f, RGB(0, 255, 0)},
        {0.0f, 0.5f, 0.0f, RGB(0, 0, 255)},
    };

    D3D11_BUFFER_DESC verticesDesc = {};
    verticesDesc.ByteWidth = sizeof(Vertices);
    verticesDesc.Usage = D3D11_USAGE_IMMUTABLE;
    verticesDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    verticesDesc.CPUAccessFlags = 0;
    verticesDesc.MiscFlags = 0;
    verticesDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA VerticesData;
    VerticesData.pSysMem = &Vertices;
    VerticesData.SysMemPitch = sizeof(Vertices);
    VerticesData.SysMemSlicePitch = 0;

    HRESULT result = device->CreateBuffer(&verticesDesc, &VerticesData, &m_vertex_buffer);
    if (SUCCEEDED(result)) {
        result = SetResourceName(m_vertex_buffer, "VertexBuffer");
    }
}

void Render::SetIndices(ID3D11Device* device) {
    static const USHORT Indices[] = { 0, 2, 1 };
    D3D11_BUFFER_DESC IndicesDesc = {};
    IndicesDesc.ByteWidth = sizeof(Indices);
    IndicesDesc.Usage = D3D11_USAGE_IMMUTABLE;
    IndicesDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    IndicesDesc.CPUAccessFlags = 0;
    IndicesDesc.MiscFlags = 0;
    IndicesDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA IndicesData;
    IndicesData.pSysMem = &Indices;
    IndicesData.SysMemPitch = sizeof(Indices);
    IndicesData.SysMemSlicePitch = 0;

    HRESULT result = device->CreateBuffer(&IndicesDesc, &IndicesData, &m_index_buffer);
    if (SUCCEEDED(result)) {
        result = SetResourceName(m_index_buffer, "IndexBuffer");
    }
}

void Render::CompileAndCreateShader(ID3D11Device* device, const std::string& path, const std::string& ext) {
    std::ifstream file(path);
    std::vector<char> data;
    char c;

    while (file.get(c)) {
        data.push_back(c);
    }

    std::string entry_point = "";
    std::string platform = "";
    if (ext == "vs") {
        entry_point = "vs";
        platform = "vs_5_0";
    }
    else if (ext == "ps") {
        entry_point = "ps";
        platform = "ps_5_0";
    }

    UINT flags1 = 0;
#ifdef _DEBUG
    flags1 |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG
    ID3DBlob* err_msg = nullptr;

    HRESULT result = D3DCompile(data.data(), data.size(), path.c_str(), nullptr, nullptr, entry_point.c_str(), platform.c_str(), flags1, 0, &m_code, &err_msg);
    if (!SUCCEEDED(result) && err_msg != nullptr) {
        OutputDebugStringA((const char*)err_msg->GetBufferPointer());
    }
    SafeRelease(&err_msg);

    if (ext == "vs") {
        result = device->CreateVertexShader(m_code->GetBufferPointer(), m_code->GetBufferSize(), nullptr, &m_vertex_shader);
        if (SUCCEEDED(result)) {
            result = SetResourceName(m_vertex_shader, path.c_str());
        }
    }
    else if (ext == "ps") {
        result = device->CreatePixelShader(m_code->GetBufferPointer(), m_code->GetBufferSize(), nullptr, &m_pixel_shader);
        if (SUCCEEDED(result)) {
            result = SetResourceName(m_pixel_shader, path.c_str());
        }
    }
}

void Render::CreateMarkupObject(ID3D11Device* device) {
    static const D3D11_INPUT_ELEMENT_DESC input_desc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    
    HRESULT result = device->CreateInputLayout(input_desc, 2, m_code->GetBufferPointer(), m_code->GetBufferSize(), &m_input_layout);
    if (SUCCEEDED(result)) {
        result = SetResourceName(m_input_layout, "InputLayout");
    }
}

void Render::SetupInputAssembler(ID3D11DeviceContext* device_context) {
    device_context->IASetIndexBuffer(m_index_buffer, DXGI_FORMAT_R16_UINT, 0);
    ID3D11Buffer* vertex_buffers[] = { m_vertex_buffer };
    UINT strides[] = { 16 };
    UINT offsets[] = { 0 };
    device_context->IASetVertexBuffers(0, 1, vertex_buffers, strides, offsets);
    device_context->IASetInputLayout(m_input_layout);
    device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Render::RenderFrame(ID3D11DeviceContext* device_context, ID3D11Device* device, IDXGISwapChain* swap_chain, LONG width, LONG height) {
    device_context->ClearState();
    ID3D11RenderTargetView* views[] = { m_render_target_view };
    device_context->OMSetRenderTargets(1, views, nullptr);
    static const FLOAT back_color[4] = { 0.25f, 0.25f, 0.25f, 1.0f };
    device_context->ClearRenderTargetView(m_render_target_view, back_color);

    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (FLOAT)width;
    viewport.Height = (FLOAT)height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    device_context->RSSetViewports(1, &viewport);

    D3D11_RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = width;
    rect.bottom = height;
    device_context->RSSetScissorRects(1, &rect);

    device_context->IASetIndexBuffer(m_index_buffer, DXGI_FORMAT_R16_UINT, 0);
    ID3D11Buffer* vertex_buffers[] = { m_vertex_buffer };
    UINT strides[] = { 16 };
    UINT offsets[] = { 0 };

    device_context->IASetVertexBuffers(0, 1, vertex_buffers, strides, offsets);
    device_context->IASetInputLayout(m_input_layout);
    device_context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device_context->VSSetShader(m_vertex_shader, nullptr, 0);
    device_context->PSSetShader(m_pixel_shader, nullptr, 0);
    device_context->DrawIndexed(3, 0, 0);
    //HRESULT result = swap_chain->Present(0, 0);
}

void Render::CleanupRenderTargetView() {
    if (m_render_target_view) {
        m_render_target_view->Release();
        m_render_target_view = nullptr;
    }
}

bool Render::RenderTargetViewCreated() {
    return m_render_target_view != nullptr;
}
