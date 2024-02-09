#include <windows.h>
#include <tchar.h>
#include "resource.h"

#define TRAY_ICON_ID 12567
#define TRAY_NOTIFY (WM_APP + 100)
#define MENU_QUIT_MESSAGE 0x101

bool clipped = false;
HHOOK hMouseHook;
HHOOK hKeyboardhook;

void addTrayIcon(HINSTANCE hInstance, HWND hWnd) {
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hWnd;
    nid.uID = TRAY_ICON_ID;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = TRAY_NOTIFY;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
    _tcsncpy(nid.szTip, TEXT("Horizon"), 128);
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void deleteTrayIcon(HWND hWnd) {
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hWnd;
    nid.uID = TRAY_ICON_ID;
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

bool lButtonClicked = false;

LRESULT CALLBACK MouseEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        if (wParam == WM_LBUTTONDOWN) {
            lButtonClicked = true;
        } else if (wParam == WM_LBUTTONUP) {
            lButtonClicked = false;
        } else if (wParam == WM_MOUSEMOVE) {
            if (lButtonClicked && clipped) {
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


int hotkeyVKCode[] = {VK_CAPITAL};
constexpr int hotkeySize = sizeof(hotkeyVKCode) / sizeof(int);
bool hotkeyPressed[hotkeySize] = {};


LRESULT CALLBACK KeyEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        auto params = reinterpret_cast<LPKBDLLHOOKSTRUCT>(lParam);
        if (wParam == WM_KEYDOWN) {
            bool processed = false;
            for (int i = 0; i < hotkeySize; i++) {
                if (params->vkCode == hotkeyVKCode[i]) {
                    hotkeyPressed[i] = true;
                    processed = true;
                    break;
                }
            }

            if (processed) {
                int pressedCount = 0;
                for (int i = 0; i < hotkeySize; i++) {
                    if (hotkeyPressed[i]) pressedCount++;
                }
                if (pressedCount == hotkeySize) {
                    clipped = true;
                }
                return 1;
            }
        } else if (wParam == WM_KEYUP) {
            bool processed = false;
            for (int i = 0; i < hotkeySize; i++) {
                if (params->vkCode == hotkeyVKCode[i]) {
                    hotkeyPressed[i] = false;
                    processed = true;
                    break;
                }
            }

            if (processed) {
                int pressedCount = 0;
                for (int i = 0; i < hotkeySize; i++) {
                    if (hotkeyPressed[i]) pressedCount++;
                }
                if (pressedCount < hotkeySize) {
                    clipped = false;
                }
                return 1;
            }
        }
    }
    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CREATE) {
        auto hInstance = (HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
        addTrayIcon(hInstance, hWnd);

        hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC) MouseEvent, hInstance, 0);
        hKeyboardhook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC) KeyEvent, hInstance, 0);

        return 0;
    } else if (msg == WM_DESTROY) {
        UnhookWindowsHookEx(hMouseHook);
        UnhookWindowsHookEx(hKeyboardhook);

        deleteTrayIcon(hWnd);
        PostQuitMessage(0);
        return 0;
    } else if (msg == TRAY_NOTIFY) {
        if (wParam == TRAY_ICON_ID && lParam == WM_RBUTTONUP) {
            auto hMenu = CreateMenu();
            AppendMenu(hMenu, MF_STRING, MENU_QUIT_MESSAGE, TEXT("Quit"));

            auto hMenubar = CreateMenu();
            AppendMenu(hMenubar, MF_POPUP, (UINT_PTR) hMenu, TEXT("Menu"));

            auto hPopupMenu = GetSubMenu(hMenubar, 0);
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hWnd);
            TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
            SetForegroundWindow(hWnd);
            PostMessage(hWnd, WM_NULL, 0, 0);

            DestroyMenu(hMenu);
            DestroyMenu(hMenubar);

            return 0;
        }
    } else if (msg == WM_COMMAND) {
        auto wmId = LOWORD(wParam);
        if (wmId == MENU_QUIT_MESSAGE) {
            DestroyWindow(hWnd);
            return 0;
        }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    SetProcessDPIAware();

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

    return 0;
}
