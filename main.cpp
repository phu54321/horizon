#include <windows.h>
#include <tchar.h>
#include "resource.h"
#include "version.h"
#include "keyHook.h"
#include "tray.h"

#define TRAY_ICON_ID 12567
#define TRAY_NOTIFY (WM_APP + 100)
#define MENU_QUIT_MESSAGE 0x101
#define MENU_ABOUT_MESSAGE 0x102

LONG dxSum = 0;
LONG dySum = 0;
const int moveThreshold = 4;
enum ClipMode {
    NONE,
    VERTICAL,
    HORIZONTAL
} clipMode;

bool _shouldClip = false;
bool _isLButtonPressed = true;

void sendMouseMoveEvent(LONG dx, LONG dy, DWORD time, DWORD mouseData, DWORD dwExtraInfo) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.time = time;
    input.mi.mouseData = mouseData;
    input.mi.dwExtraInfo = dwExtraInfo;
    SendInput(1, &input, sizeof(INPUT));
}

void resetDxDySum(DWORD time) {
    if (dxSum != 0 || dySum != 0) {
        if (!_isCapsPressed) {
            sendMouseMoveEvent(dxSum, dySum, time, 0, 0);
        }
        else {
            auto absDxSum = abs(dxSum);
            auto absDySum = abs(dySum);
            if (absDxSum > absDySum) {
                sendMouseMoveEvent(dxSum, 0, time, 0, 0);
            } else if (absDySum > absDxSum) {
                sendMouseMoveEvent(0, dySum, time, 0, 0);
            } else {
                sendMouseMoveEvent(dxSum, dySum, time, 0, 0);
            }
        }
        dxSum = dySum = 0;
    }
}

HHOOK _hMouseHook;

LRESULT CALLBACK MouseEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    static POINT oldMousePoint;

    if (nCode >= 0) {
        POINT currentMousePoint = oldMousePoint;
        auto params = reinterpret_cast<LPMSLLHOOKSTRUCT>(lParam);
        oldMousePoint = params->pt;

        if (wParam == WM_LBUTTONDOWN && !_isLButtonPressed) {
            _isLButtonPressed = true;
            _shouldClip = _isCapsPressed;
            if (_shouldClip) {
                dxSum = dySum = 0;
                clipMode = ClipMode::NONE;
            }
        } else if (wParam == WM_LBUTTONUP && _isLButtonPressed) {
            if (_shouldClip) {
                resetDxDySum(params->time);
            }
            _isLButtonPressed = false;
        } else if (wParam == WM_MOUSEMOVE) {
            if (params->flags & LLMHF_INJECTED) {}
            else if (_isLButtonPressed && _isCapsPressed) {
                auto dx = params->pt.x - currentMousePoint.x;
                auto dy = params->pt.y - currentMousePoint.y;
                dxSum += dx;
                dySum += dy;

                auto absDxSum = abs(dxSum);
                auto absDySum = abs(dySum);
                if (clipMode == ClipMode::HORIZONTAL ||
                    (clipMode == ClipMode::NONE && absDxSum > absDySum && absDxSum > moveThreshold)) {
                    sendMouseMoveEvent(dxSum, 0, params->time, params->mouseData, params->dwExtraInfo);
                    dxSum = dySum = 0;
                    clipMode = ClipMode::HORIZONTAL;
                } else if (clipMode == ClipMode::VERTICAL ||
                           (clipMode == ClipMode::NONE && absDySum > absDxSum && absDySum > moveThreshold)) {
                    sendMouseMoveEvent(0, dySum, params->time, params->mouseData, params->dwExtraInfo);
                    dxSum = dySum = 0;
                    clipMode = ClipMode::VERTICAL;
                }
                return 1;
            }
        }
    }
    return CallNextHookEx(_hMouseHook, nCode, wParam, lParam);
}




LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CREATE) {
        auto hInstance = (HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
        addTrayIcon(hInstance, hWnd);

        _hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC) MouseEvent, hInstance, 0);
        registerKeyboardHook(hInstance);

        return 0;
    } else if (msg == WM_DESTROY) {
        unregisterKeyboardHook();
        UnhookWindowsHookEx(_hMouseHook);

        deleteTrayIcon(hWnd);
        PostQuitMessage(0);
        return 0;
    } else if (processTrayMessage(hWnd, msg, wParam, lParam)) {
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    SetProcessDPIAware();

    auto mutex = CreateMutex(nullptr, TRUE, TEXT("Global\\trgksoft_Horizon"));
    if (!mutex || GetLastError()) {
        MessageBox(nullptr, TEXT("Another instance already running."), TEXT("Horizon"), MB_OK);
        return 0;
    }


    WNDCLASS wndClass;
    memset(&wndClass, 0, sizeof(WNDCLASS));
    wndClass.hInstance = hInstance;
    wndClass.lpszClassName = TEXT("delaySleepOnLidClose");
    wndClass.lpfnWndProc = WndProc;
    wndClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
    RegisterClass(&wndClass);

    HWND hWnd = CreateWindow(
            TEXT("delaySleepOnLidClose"),
            TEXT("delaySleepOnLidClose"),
            0,
            0, 0,
            CW_USEDEFAULT, CW_USEDEFAULT,
            HWND_MESSAGE,
            nullptr,
            hInstance,
            nullptr);
    if (!hWnd) {
        MessageBox(nullptr, TEXT("Cannot create window"), TEXT("Error"), MB_OK);
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CloseHandle(mutex);

    return 0;
}
