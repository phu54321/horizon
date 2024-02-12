#include <windows.h>
#include <tchar.h>
#include "resource.h"
#include "version.h"
#include <stdio.h>

#define TRAY_ICON_ID 12567
#define TRAY_NOTIFY (WM_APP + 100)
#define MENU_QUIT_MESSAGE 0x101
#define MENU_ABOUT_MESSAGE 0x102

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
    _tcsncpy(nid.szTip, TEXT("Horizon - Mouse moves horizontally"), 128);
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
POINT oldMousePoint;
LONG dxSum = 0;
LONG dySum = 0;
const int moveThreshold = 3;

void sendMouseMoveEvent(LONG dx, LONG dy, DWORD time, DWORD mouseData, DWORD dwExtraInfo) {
//    printf("dxSum %ld dySum %ld\n", dx, dy);
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

LRESULT CALLBACK MouseEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        POINT currentMousePoint = oldMousePoint;
        auto params = reinterpret_cast<LPMSLLHOOKSTRUCT>(lParam);
        oldMousePoint = params->pt;
//        printf("[1] oldX %ld oldY %ld newX %ld newY %ld\n", currentMousePoint.x, currentMousePoint.y, params->pt.x,
//               params->pt.y);

        if (wParam == WM_LBUTTONDOWN) {
            lButtonClicked = true;
            dxSum = dySum = 0;
        } else if (wParam == WM_LBUTTONUP) {
            if (dxSum != 0 || dySum != 0) {
                auto absDxSum = abs(dxSum);
                auto absDySum = abs(dySum);
                if (absDxSum > absDySum) {
                    sendMouseMoveEvent(dxSum, 0, params->time, params->mouseData, params->dwExtraInfo);
                } else if (absDySum > absDxSum) {
                    sendMouseMoveEvent(0, dySum, params->time, params->mouseData, params->dwExtraInfo);
                } else {
                    sendMouseMoveEvent(dxSum, dySum, params->time, params->mouseData, params->dwExtraInfo);
                }
                dxSum = dySum = 0;
            }
            lButtonClicked = false;
        } else if (wParam == WM_MOUSEMOVE) {
            if (params->flags & LLMHF_INJECTED) {}
            else if (lButtonClicked && clipped) {
//                printf("[2] oldX %ld oldY %ld newX %ld newY %ld\n", currentMousePoint.x, currentMousePoint.y,
//                       params->pt.x, params->pt.y);
                auto dx = params->pt.x - currentMousePoint.x;
                auto dy = params->pt.y - currentMousePoint.y;
                dxSum += dx;
                dySum += dy;

                auto absDxSum = abs(dxSum);
                auto absDySum = abs(dySum);
                if (absDxSum > absDySum && absDxSum > moveThreshold) {
                    sendMouseMoveEvent(dxSum, 0, params->time, params->mouseData, params->dwExtraInfo);
                    dxSum = dySum = 0;
                } else if (absDySum > absDxSum && absDySum > moveThreshold) {
                    sendMouseMoveEvent(0, dySum, params->time, params->mouseData, params->dwExtraInfo);
                    dxSum = dySum = 0;
                }
                return 1;
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


INT_PTR CALLBACK AboutDialogProc(HWND hwndDlg,
                                 UINT message,
                                 WPARAM wParam,
                                 LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            auto hIcon = (HICON) LoadImageW(
                    GetModuleHandleW(NULL),
                    MAKEINTRESOURCEW(IDI_ICON),
                    IMAGE_ICON,
                    0, 0, 0);
            if (hIcon) {
                SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM) hIcon);
            }
            auto versionText = TEXT("Horizon ") SPRODUCT_VERSION;
            SetDlgItemText(hwndDlg, IDC_VERSIONTEXT, versionText);
            return TRUE;
        }

        case WM_CLOSE:
        case WM_DESTROY:
            EndDialog(hwndDlg, 0);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hwndDlg, 0);
                    return TRUE;

                case IDB_GITHUB_URL:
                    ShellExecute(NULL, "open", "https://github.com/phu54321", NULL, NULL, SW_SHOWNORMAL);
                    return TRUE;
            }
            break;
    }
    return FALSE;
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
            AppendMenu(hMenu, MF_STRING, MENU_ABOUT_MESSAGE, TEXT("About"));
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
        } else if (wmId == MENU_ABOUT_MESSAGE) {
            auto hInstance = (HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
            DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUT), hWnd, AboutDialogProc);
            return 0;
        }
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
