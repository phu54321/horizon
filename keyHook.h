//
// Created by whyask37 on 2024-02-16.
//

#ifndef MOUSEYLOCK_KEYHOOK_H
#define MOUSEYLOCK_KEYHOOK_H

#include <Windows.h>

extern bool _isCapsPressed;
void registerKeyboardHook(HINSTANCE hInstance);
void unregisterKeyboardHook();

#endif //MOUSEYLOCK_KEYHOOK_H
