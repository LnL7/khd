#ifndef LOCALE_H
#define LOCALE_H

#include "hotkey.h"

bool KeycodeFromChar(char Key, hotkey *Hotkey);
bool LayoutIndependentKeycode(char *Key, hotkey *Hotkey);
bool StringsAreEqual(const char *A, const char *B);

#endif
