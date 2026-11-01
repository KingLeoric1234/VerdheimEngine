#ifndef DEBUGPANEL_H
#define DEBUGPANEL_H

#include <windows.h>

// Create the Debug panel as a child of the given window. Returns the panel HWND.
HWND create_debug_panel(HWND parent);

// Nonzero while a command is running in the Debug panel.
int debug_panel_busy(void);

#endif // DEBUGPANEL_H