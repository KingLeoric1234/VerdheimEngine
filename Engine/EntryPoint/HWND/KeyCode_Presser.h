#ifndef KEYCODEPRESSER_H
#define KEYCODEPRESSER_H

#include <windows.h>

BYTE insert_keyvalue(MSG msg);
BYTE delete_keyvalue();
void listen_keyvalue(MSG msg);
int search_keystates(BYTE keystate[], BYTE target);

#endif // KEYCODEPRESSER_H