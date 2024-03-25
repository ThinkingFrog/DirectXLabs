#pragma once

#include <string>
#include <d3d11.h>

class Render {
public:
    void CreateRenderTargetView(IDXGISwapChain* swap_chain, ID3D11Device* device);
    void RenderFrame(ID3D11DeviceContext* device_context, ID3D11Device* device, IDXGISwapChain* swap_chain, LONG width, LONG height);
    void CleanupRenderTargetView();
    bool RenderTargetViewCreated();
    void SetGeometry(ID3D11Device* device);
    void SetIndices(ID3D11Device* device);
    void CompileAndCreateShader(ID3D11Device* device, const std::string& path, const std::string& ext);
    void CreateMarkupObject(ID3D11Device* device);
    void SetupInputAssembler(ID3D11DeviceContext* device_context);
    ~Render();

private:
    ID3D11RenderTargetView* m_render_target_view = nullptr;
    ID3D11Buffer* m_vertex_buffer = nullptr;
    ID3D11Buffer* m_index_buffer = nullptr;
    ID3D11VertexShader* m_vertex_shader = nullptr;
    ID3D11PixelShader* m_pixel_shader = nullptr;
    ID3D11InputLayout* m_input_layout = nullptr;
    ID3DBlob* m_code = nullptr;

    struct Vertex {
        float x, y, z;
        COLORREF color;
    };
};

