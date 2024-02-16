//
// Created by whyask37 on 2024-02-16.
//

#ifndef MOUSEYLOCK_TRAY_H
#define MOUSEYLOCK_TRAY_H

#include <Windows.h>

void addTrayIcon(HINSTANCE hInstance, HWND hWnd);
void deleteTrayIcon(HWND hWnd);
BOOL processTrayMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif //MOUSEYLOCK_TRAY_H
