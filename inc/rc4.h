// rc4.h

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef _RC4_H
#define _RC4_H

#include "type_defs.h"

void rc4_key(UCHAR* s, const UCHAR* key, const ULONG len);
void rc4(UCHAR* s, UCHAR* data, const ULONG len);

void symmetricEncDec(UCHAR* data, const ULONG len, const UCHAR* key, const ULONG keyLen);

#endif // _RC4_H
