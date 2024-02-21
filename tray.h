//
// Created by whyask37 on 2024-02-16.
//

#ifndef HORIZON_TRAY_H
#define HORIZON_TRAY_H

#include <Windows.h>

void addTrayIcon(HINSTANCE hInstance, HWND hWnd);

void deleteTrayIcon(HWND hWnd);

BOOL processTrayMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif //HORIZON_TRAY_H
