#ifndef KHD_PARSE_H
#define KHD_PARSE_H

void ParseConfig(char *Contents);
void ParseKhd(char *Contents);

struct hotkey;
void ParseKeySym(char *KeySym, hotkey *Hotkey);

#endif
