//
// Created by whyask37 on 2024-02-16.
//

#ifndef HORIZON_KEYHOOK_H
#define HORIZON_KEYHOOK_H

#include <Windows.h>

extern bool g_isCapsPressed;

void registerKeyboardHook(HINSTANCE hInstance);

void unregisterKeyboardHook();

#endif //HORIZON_KEYHOOK_H
