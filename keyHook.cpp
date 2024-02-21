//
// Created by whyask37 on 2024-02-16.
//

#include "keyHook.h"
#include <Windows.h>

static HHOOK g_hKeyboardHook;
bool g_isCapsPressed = false;

LRESULT CALLBACK KeyEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        auto params = reinterpret_cast<LPKBDLLHOOKSTRUCT>(lParam);
        if (wParam == WM_KEYDOWN) {
            if (params->vkCode == VK_CAPITAL) {
                g_isCapsPressed = true;
                return 1;
            }
        } else if (wParam == WM_KEYUP) {
            if (params->vkCode == VK_CAPITAL) {
                g_isCapsPressed = false;
                return 1;
            }
        }
    }
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

void registerKeyboardHook(HINSTANCE hInstance) {
    g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC) KeyEvent, hInstance, 0);
}

void unregisterKeyboardHook() {
    UnhookWindowsHookEx(g_hKeyboardHook);
}
