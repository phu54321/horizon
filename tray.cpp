//
// Created by whyask37 on 2024-02-16.
//

#include "tray.h"
#include "resource.h"
#include "version.h"
#include <tchar.h>

#define TRAY_ICON_ID 12567
#define TRAY_NOTIFY (WM_APP + 100)
#define MENU_QUIT_MESSAGE 0x101
#define MENU_ABOUT_MESSAGE 0x102

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

INT_PTR CALLBACK AboutDialogProc(HWND hwndDlg,
                                 UINT message,
                                 WPARAM wParam,
                                 LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            auto hIcon = (HICON) LoadImageW(
                    GetModuleHandleW(nullptr),
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
                    ShellExecute(nullptr, "open", "https://github.com/phu54321", nullptr, nullptr, SW_SHOWNORMAL);
                    return TRUE;
            }
            break;

        default:;
    }
    return FALSE;
}

BOOL processTrayMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == TRAY_NOTIFY) {
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
            TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, nullptr);
            SetForegroundWindow(hWnd);
            PostMessage(hWnd, WM_NULL, 0, 0);

            DestroyMenu(hMenu);
            DestroyMenu(hMenubar);

            return TRUE;
        }
    } else if (msg == WM_COMMAND) {
        auto wmId = LOWORD(wParam);
        if (wmId == MENU_QUIT_MESSAGE) {
            DestroyWindow(hWnd);
            return TRUE;
        } else if (wmId == MENU_ABOUT_MESSAGE) {
            auto hInstance = (HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
            DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUT), hWnd, AboutDialogProc);
            return TRUE;
        }
    }
    return FALSE;
}