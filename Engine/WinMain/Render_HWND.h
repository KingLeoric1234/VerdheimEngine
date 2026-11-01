#ifndef HWND_H
#define HWND_H

#include <windows.h>

HWND RegisterWindow(HINSTANCE hInstance, int nCmdShow);
void FrameLoop(void);
extern int *frame_counter_ptr;

#endif // HWND_H