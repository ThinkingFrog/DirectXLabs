#include <string>

#include "utils.h"
#include "GlobalState.h"

extern GlobalState gs;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }
    case WM_SIZE: {
        gs.ResizeWindow(lParam);

        return 0;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

HRESULT SetResourceName(ID3D11DeviceChild* pResource, const std::string& name) {
    return pResource->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.length(), name.c_str());
}
