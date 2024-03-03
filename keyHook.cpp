//
// Created by whyask37 on 2024-02-16.
//

#include "keyHook.h"
#include <Windows.h>
#include <stdio.h>

static HHOOK g_hKeyboardHook;
bool g_isCapsPressed = false;
bool g_wasMouseEventAfterKey = false;

LRESULT CALLBACK KeyEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        auto params = reinterpret_cast<LPKBDLLHOOKSTRUCT>(lParam);
        if (wParam == WM_KEYDOWN) {
            if (!(params->flags & (LLKHF_INJECTED | LLKHF_LOWER_IL_INJECTED))) {
                if (params->vkCode == VK_CAPITAL) {
                    if (!g_isCapsPressed) {
                        g_isCapsPressed = true;
                        g_wasMouseEventAfterKey = false;  // reset
                    }
                    return 1;
                }
            }
        } else if (wParam == WM_KEYUP) {
            if (params->vkCode == VK_CAPITAL && g_isCapsPressed) {
                g_isCapsPressed = false;

                if (!g_wasMouseEventAfterKey) {
                    printf("caps lock\n");
                    // Simulate caps lock
                    INPUT input = {};
                    input.type = INPUT_KEYBOARD;

                    input.ki.wScan = MapVirtualKey(VK_CAPITAL, MAPVK_VK_TO_VSC);
                    input.ki.time = 0;
                    input.ki.dwExtraInfo = 0;
                    input.ki.wVk = VK_CAPITAL;
                    input.ki.dwFlags = 0;
                    SendInput(1, &input, sizeof(INPUT));

                    input.ki.dwFlags = KEYEVENTF_KEYUP;
                    SendInput(1, &input, sizeof(INPUT));
                }
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
