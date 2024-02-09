#include <windows.h>
#include <stdio.h>

bool clipped = false;
HHOOK hMouseHook;
HHOOK hKeyboardhook;

DWORD endTime;

LRESULT CALLBACK MouseEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        if (clipped && GetTickCount() < endTime) {
            if (wParam == WM_MOUSEMOVE) {
                auto params = reinterpret_cast<LPMSLLHOOKSTRUCT>(lParam);
                POINT currentPos;
                GetCursorPos(&currentPos);
                auto dx = params->pt.x - currentPos.x;
                auto dy = params->pt.y - currentPos.y;

                if (dy != 0) {
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


int hotkeyVKCode[] = { VK_LSHIFT, VK_LCONTROL };
constexpr int hotkeySize = sizeof(hotkeyVKCode) / sizeof(int);
bool hotkeyPressed[hotkeySize] = {};


LRESULT CALLBACK KeyEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        auto params = reinterpret_cast<LPKBDLLHOOKSTRUCT>(lParam);
        if (wParam == WM_KEYDOWN) {
            for (int i = 0 ; i < hotkeySize ; i++) {
                if (params->vkCode == hotkeyVKCode[i]) {
                    hotkeyPressed[i] = true;
                    break;
                }
            }

            int pressedCount = 0;
            for (int i = 0 ; i < hotkeySize ; i++) {
                if (hotkeyPressed[i]) pressedCount++;
            }
            if (pressedCount == hotkeySize) {
                clipped = true;
                printf("Clipped\n");
            }
        } else if (wParam == WM_KEYUP) {
            for (int i = 0 ; i < hotkeySize ; i++) {
                if (params->vkCode == hotkeyVKCode[i]) {
                    hotkeyPressed[i] = false;
                    break;
                }
            }

            int pressedCount = 0;
            for (int i = 0 ; i < hotkeySize ; i++) {
                if (hotkeyPressed[i]) pressedCount++;
            }
            if (pressedCount < hotkeySize) {
                clipped = false;
                printf("Unclipped\n");
            }
        }
    }
    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    SetProcessDPIAware();

    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC) MouseEvent, hInstance, 0);
    hKeyboardhook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC) KeyEvent, hInstance, 0);

    endTime = GetTickCount() + 10000;

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hMouseHook);
    UnhookWindowsHookEx(hKeyboardhook);

    return msg.wParam;
}
