//
// Created by whyask37 on 2024-02-16.
//

#include "keyHook.h"
#include <Windows.h>

static HHOOK _hKeyboardHook;
bool _isCapsPressed = false;

LRESULT CALLBACK KeyEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        auto params = reinterpret_cast<LPKBDLLHOOKSTRUCT>(lParam);
        if (wParam == WM_KEYDOWN) {
            if (params->vkCode == VK_CAPITAL) {
                _isCapsPressed = true;
                return 1;
            }
        } else if (wParam == WM_KEYUP) {
            if (params->vkCode == VK_CAPITAL) {
                _isCapsPressed = false;
                return 1;
            }
        }
    }
    return CallNextHookEx(_hKeyboardHook, nCode, wParam, lParam);
}

void registerKeyboardHook(HINSTANCE hInstance) {
    _hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC) KeyEvent, hInstance, 0);
}

void unregisterKeyboardHook() {
    UnhookWindowsHookEx(_hKeyboardHook);
}
