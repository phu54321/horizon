#include <windows.h>
#include <stdio.h>

bool clipped = true;   // Do we need to clip the mouse?
RECT rc;                // The clip rect
HHOOK hMouseHook;       // Low level mouse hook
short fixedY;

DWORD endTime;

LRESULT CALLBACK MouseEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        if (GetTickCount() < endTime) {
            if (wParam == WM_MOUSEMOVE) {
                auto params = (LPMSLLHOOKSTRUCT) lParam;

                if (params->pt.y != fixedY) {
                    POINT currentPos;
                    GetCursorPos(&currentPos);
                    auto dx = params->pt.x - currentPos.x;

                    INPUT input = {};
                    input.type = INPUT_MOUSE;
                    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_VIRTUALDESK;
                    input.mi.dx = dx;
                    input.mi.dy = 0;
                    input.mi.time = params->time;
                    input.mi.mouseData = params->mouseData;
                    input.mi.dwExtraInfo = params->dwExtraInfo;
                    SendInput(1, &input, sizeof(INPUT));
                    return 1;
                }
            }
        }
    }
    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    SetProcessDPIAware();

    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC) MouseEvent, hInstance, 0);
    MSG msg;

    POINT p;
    GetCursorPos(&p);
    fixedY = p.y;

    endTime = GetTickCount() + 5000;

    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Unhook the mouse
    UnhookWindowsHookEx(hMouseHook);

    return msg.wParam;
}
