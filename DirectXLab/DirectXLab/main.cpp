#include <Windows.h>
#include <d3d11.h>

#include "GlobalState.h"
#include "utils.h"

GlobalState gs;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (FAILED(gs.InitWindow(hInstance, nCmdShow)))
        return 0;
    if (FAILED(gs.InitDirectX()))
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

        gs.RenderFrame();
    }

    gs.CleanupDirectX();

    return static_cast<int>(msg.wParam);
}
