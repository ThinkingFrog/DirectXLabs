#include <Windows.h>
#include <d3d11.h>


// Global variables
HWND hwnd;
ID3D11Device* device = nullptr;
IDXGISwapChain* swapChain = nullptr;
ID3D11DeviceContext* deviceContext = nullptr;
ID3D11RenderTargetView* renderTargetView = nullptr;
IDXGIAdapter* adapter= nullptr;

// Functions declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDirectX();
HRESULT InitAdapter();
HRESULT InitDevice();
HRESULT InitSwapChain();
void CleanupDirectX();
void CreateRenderTargetView(ID3D11Device* device, IDXGISwapChain* swapChain);
void RenderFrame(ID3D11Device* device, ID3D11DeviceContext* deviceContext);


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (FAILED(InitWindow(hInstance, nCmdShow)))
        return 0;
    if (FAILED(InitDirectX()))
        return 0;

    // Main message loop
    MSG msg = {};
    bool exit = false;

    while (!exit) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
                exit = true;
        }

        // Render a frame
        if (renderTargetView) {
            RenderFrame(device, deviceContext);
            swapChain->Present(1, 0);
        }
    }

    // Cleanup DirectX resources
    CleanupDirectX();

    return static_cast<int>(msg.wParam);
}

HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow) {
    // Create the window
    const int screenWidth = 800;
    const int screenHeight = 600;

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"WindowClass";
    if (!RegisterClassEx(&wc))
        return E_FAIL;

    RECT wr = { 0, 0, screenWidth, screenHeight };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    hwnd = CreateWindowEx(NULL,
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
    
    if (!hwnd)
        return E_FAIL;
    ShowWindow(hwnd, nCmdShow);

    return S_OK;
}

HRESULT InitAdapter() {
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
                adapter = curAdapter;
                break;
            }
            curAdapter->Release();

            ++adapterIdx;
        }
    }

    if (adapter == nullptr) {
        return E_FAIL;
    }

    return S_OK;
}

HRESULT InitDevice() {
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
            D3D11_SDK_VERSION, &device, &featureLevel, &deviceContext);

        if (result == E_INVALIDARG)
            result = D3D11CreateDevice(nullptr, driverTypes[driverTypeIndex], nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels,
                D3D11_SDK_VERSION, &device, &featureLevel, &deviceContext);
        if (SUCCEEDED(result))
            break;
    }

    return result;
}

HRESULT InitSwapChain() {
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
        swapChainDesc.OutputWindow = hwnd;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = true;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.Flags = 0;

        result = dxgiFactory->CreateSwapChain(device, &swapChainDesc, &swapChain);
    }

    return result;
}

HRESULT InitDirectX() {
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

    CreateRenderTargetView(device, swapChain);

    return S_OK;
}

void CleanupDirectX() {
    if (renderTargetView) {
        renderTargetView->Release();
        renderTargetView = nullptr;
    }
    if (swapChain) {
        swapChain->Release();
        swapChain = nullptr;
    }
    if (deviceContext) {
        deviceContext->Release();
        deviceContext = nullptr;
    }
    if (device) {
        device->Release();
        device = nullptr;
    }
    if (adapter) {
        adapter->Release();
        adapter = nullptr;
    }
}

void CreateRenderTargetView(ID3D11Device* device, IDXGISwapChain* swapChain) {
    ID3D11Texture2D* backBuffer;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));

    device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);

    backBuffer->Release();
}

void RenderFrame(ID3D11Device* device, ID3D11DeviceContext* deviceContext) {
    deviceContext->OMSetRenderTargets(1, &renderTargetView, nullptr);

    float clearColor[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    deviceContext->ClearRenderTargetView(renderTargetView, clearColor);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }
    case WM_SIZE: {
        if (swapChain) {
            // Retrieve the new client area size
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);

            // Cleanup and recreate the render target view
            if (renderTargetView) {
                renderTargetView->Release();
                renderTargetView = nullptr;
            }
            swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTargetView(device, swapChain);
        }

        return 0;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}
