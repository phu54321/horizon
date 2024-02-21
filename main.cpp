#include <windows.h>
#include <winuser.h>
#include <tchar.h>
#include "resource.h"
#include "version.h"
#include "keyHook.h"
#include "tray.h"

#define TRAY_ICON_ID 12567
#define TRAY_NOTIFY (WM_APP + 100)
#define MENU_QUIT_MESSAGE 0x101
#define MENU_ABOUT_MESSAGE 0x102

#include <stdio.h>

const int moveThreshold = 5;
enum ClipMode {
    NONE,
    VERTICAL,
    HORIZONTAL
} clipMode;

bool _shouldClip = false;
bool _isLButtonPressed = true;

void sendMouseMoveEvent(LONG dx, LONG dy, DWORD time, DWORD mouseData, DWORD dwExtraInfo) {
//    printf(" - sendMouseMoveEvent: dx %ld dy %ld\n", dx, dy);
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

LONG _dxSum = 0;
LONG _dySum = 0;
//DWORD startTime;

void resetDxDySum(DWORD time) {
    if (_dxSum != 0 || _dySum != 0) {
        if (!_isCapsPressed) {
            sendMouseMoveEvent(_dxSum, _dySum, time, 0, 0);
        } else {
            auto absDxSum = abs(_dxSum);
            auto absDySum = abs(_dySum);
            if (absDxSum > absDySum) {
                sendMouseMoveEvent(_dxSum, 0, time, 0, 0);
            } else if (absDySum > absDxSum) {
                sendMouseMoveEvent(0, _dySum, time, 0, 0);
            } else {
                sendMouseMoveEvent(_dxSum, _dySum, time, 0, 0);
            }
        }
        _dxSum = _dySum = 0;
    }
}

HHOOK _hMouseHook;

LRESULT CALLBACK MouseEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    static POINT oldMousePoint;

//    static int invocationCounter = 0;
//    int thisInvocation = invocationCounter++;
//    auto currentTime = GetTickCount() - startTime;

    if (nCode >= 0) {
        auto params = reinterpret_cast<LPMSLLHOOKSTRUCT>(lParam);
//        printf("[%4d %8lu] (%ld, %ld) -> (%ld, %ld)\n", thisInvocation, currentTime, oldMousePoint.x, oldMousePoint.y, params->pt.x, params->pt.y);

        if (wParam == WM_LBUTTONDOWN && !_isLButtonPressed) {
//            printf("[%4d %8lu] - [lbd]\n");
            _isLButtonPressed = true;
            _shouldClip = _isCapsPressed;
            if (_shouldClip) {
                _dxSum = _dySum = 0;
                clipMode = ClipMode::NONE;
            }
            oldMousePoint = params->pt;
//            printf("[%4d %8lu] - omp = (%ld, %ld)\n", thisInvocation, currentTime, oldMousePoint.x, oldMousePoint.y);
        } else if (wParam == WM_LBUTTONUP && _isLButtonPressed) {
//            printf("[%4d %8lu] - [lbu]\n");
            if (_shouldClip) {
                resetDxDySum(params->time);
            }
            _isLButtonPressed = false;
            oldMousePoint = params->pt;
//            printf("[%4d %8lu] - omp = (%ld, %ld)\n", thisInvocation, currentTime, oldMousePoint.x, oldMousePoint.y);

        } else if (wParam == WM_MOUSEMOVE) {
            if (params->flags & (LLMHF_LOWER_IL_INJECTED | LLMHF_INJECTED)) {
                oldMousePoint = params->pt;
            } else {
//                printf("[%4d %8lu] - [lbm]\n");
                if (_isLButtonPressed && _isCapsPressed) {
//                    printf("[%4d %8lu] - [captured]\n");
                    auto dx = params->pt.x - oldMousePoint.x;
                    auto dy = params->pt.y - oldMousePoint.y;
                    _dxSum += dx;
                    _dySum += dy;
//                    printf("[%4d %8lu] %ld %ld %ld %ld\n", thisInvocation, currentTime, dx, dy, _dxSum, _dySum);

                    auto absDxSum = abs(_dxSum);
                    auto absDySum = abs(_dySum);

                    if (clipMode == NONE) {
                        if (absDxSum > absDySum && absDxSum > moveThreshold) {
                            clipMode = ClipMode::HORIZONTAL;
                        } else if (absDySum > absDxSum && absDySum > moveThreshold) {
                            clipMode = ClipMode::VERTICAL;
                        }
                    }

                    if (clipMode == ClipMode::HORIZONTAL) {
                        sendMouseMoveEvent(_dxSum, 0, params->time, params->mouseData, params->dwExtraInfo);
//                        printf("[%4d %8lu] - omp = (%ld, %ld) [+dxSum]\n", thisInvocation, currentTime, oldMousePoint.x,
//                               oldMousePoint.y);
                        _dxSum = _dySum = 0;
                    } else if (clipMode == ClipMode::VERTICAL) {
                        sendMouseMoveEvent(0, _dySum, params->time, params->mouseData, params->dwExtraInfo);
//                        printf("[%4d %8lu] - omp = (%ld, %ld) [+dySum]\n", thisInvocation, currentTime, oldMousePoint.x,
//                               oldMousePoint.y);
                        _dxSum = _dySum = 0;
                    }
                    return 1;
                }
                oldMousePoint = params->pt;
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
//    startTime = GetTickCount();

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
