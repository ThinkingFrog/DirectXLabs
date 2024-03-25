#include "GlobalState.h"
#include "utils.h"

HRESULT GlobalState::InitWindow(HINSTANCE hInstance, int nCmdShow) {
    // Create the window
    m_width = 800;
    m_height = 600;

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"WindowClass";
    if (!RegisterClassEx(&wc))
        return E_FAIL;

    RECT wr = { 0, 0, m_width, m_height };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    m_hwnd = CreateWindowEx(NULL,
        L"WindowClass",
        L"DirectX Window",
        WS_OVERLAPPEDWINDOW,
        100,
        100,
        wr.right - wr.left,
        wr.bottom - wr.top,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (!m_hwnd)
        return E_FAIL;
    ShowWindow(m_hwnd, nCmdShow);

    return S_OK;
}

HRESULT GlobalState::InitAdapter() {
    HRESULT result;
    IDXGIFactory* dxgiFactory = nullptr;
    result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&dxgiFactory);

    if (SUCCEEDED(result)) {
        IDXGIAdapter* curAdapter = nullptr;
        UINT adapterIdx = 0;
        while (SUCCEEDED(dxgiFactory->EnumAdapters(adapterIdx, &curAdapter))) {
            DXGI_ADAPTER_DESC desc;
            curAdapter->GetDesc(&desc);

            if (wcscmp(desc.Description, L"Microsoft Basic Render Driver") != 0) {
                m_adapter = curAdapter;
                break;
            }
            curAdapter->Release();

            ++adapterIdx;
        }
    }

    if (m_adapter == nullptr) {
        return E_FAIL;
    }

    return S_OK;
}

HRESULT GlobalState::InitDevice() {
    HRESULT result;

    UINT createDeviceFlags = 0;

#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG


    // Driver types
    D3D_DRIVER_TYPE driverTypes[] = {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };

    // feature levels
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

    int numDriverTypes = ARRAYSIZE(driverTypes);
    int numFeatureLevels = ARRAYSIZE(featureLevels);

    for (int driverTypeIndex = 0; driverTypeIndex < numDriverTypes; ++driverTypeIndex) {
        result = D3D11CreateDevice(nullptr, driverTypes[driverTypeIndex], nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
            D3D11_SDK_VERSION, &m_device, &featureLevel, &m_device_context);

        if (result == E_INVALIDARG)
            result = D3D11CreateDevice(nullptr, driverTypes[driverTypeIndex], nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels,
                D3D11_SDK_VERSION, &m_device, &featureLevel, &m_device_context);
        if (SUCCEEDED(result))
            break;
    }

    return result;
}

HRESULT GlobalState::InitSwapChain() {
    HRESULT result;

    IDXGIFactory* dxgiFactory = nullptr;
    result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&dxgiFactory);

    if (SUCCEEDED(result)) {
        DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
        swapChainDesc.BufferCount = 2;
        swapChainDesc.BufferDesc.Width = 16;
        swapChainDesc.BufferDesc.Height = 16;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = m_hwnd;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = true;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.Flags = 0;

        result = dxgiFactory->CreateSwapChain(m_device, &swapChainDesc, &m_swap_chain);
    }

    return result;
}

HRESULT GlobalState::InitDirectX() {
    HRESULT result;

    result = InitAdapter();
    if (FAILED(result)) {
        return result;
    }

    result = InitDevice();
    if (FAILED(result)) {
        return result;
    }

    result = InitSwapChain();
    if (FAILED(result)) {
        return result;
    }

    m_render = new Render();

    m_render->CreateRenderTargetView(m_swap_chain, m_device);
    m_render->SetGeometry(m_device);
    m_render->SetIndices(m_device);
    m_render->CompileAndCreateShader(m_device, m_vertex_shader_path, "vs");
    m_render->CreateMarkupObject(m_device);
    m_render->CompileAndCreateShader(m_device, m_pixel_shader_path, "ps");
    m_render->SetupInputAssembler(m_device_context);

    m_render->CleanupRenderTargetView();
    m_swap_chain->ResizeBuffers(0, m_width, m_height, DXGI_FORMAT_UNKNOWN, 0);
    m_render->CreateRenderTargetView(m_swap_chain, m_device);

    return S_OK;
}

void GlobalState::CleanupDirectX() {
    m_render->CleanupRenderTargetView();

    if (m_swap_chain) {
        m_swap_chain->Release();
        m_swap_chain = nullptr;
    }
    
    if (m_device_context) {
        m_device_context->Release();
        m_device_context = nullptr;
    }
    
    if (m_device) {
        m_device->Release();
        m_device = nullptr;
    }
    
    if (m_adapter) {
        m_adapter->Release();
        m_adapter = nullptr;
    }

    delete m_render;
}

void GlobalState::RenderFrame() {
    if (m_render->RenderTargetViewCreated()) {
        m_render->RenderFrame(m_device_context, m_device, m_swap_chain, m_width, m_height);
        m_swap_chain->Present(0, 0);
    }
}

void GlobalState::ResizeWindow(LPARAM lParam) {
    if (m_swap_chain) {
        // Retrieve the new client area size
        m_width = LOWORD(lParam);
        m_height = HIWORD(lParam);

        // Cleanup and recreate the render target view
        m_render->CleanupRenderTargetView();
        m_swap_chain->ResizeBuffers(0, m_width, m_height, DXGI_FORMAT_UNKNOWN, 0);
        m_render->CreateRenderTargetView(m_swap_chain, m_device);
    }
}
