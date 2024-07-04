// rc4.cpp : defines functions for symmetric encryption and decryption using the rc4 algorithm

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#include "rc4.h"

void swapByte(UCHAR* a, UCHAR* b);

void swapByte(UCHAR* a, UCHAR* b)
{
    UCHAR tmp = *a;
    *a = *b;
    *b = tmp;
}

void rc4_key(UCHAR* s, const UCHAR* key, const ULONG len)
{
    int i = 0;
    UCHAR j = 0;
    UCHAR k[256] = { 0 };

    for (i = 0; i < 256; i++)
    {
        s[i] = i;
        k[i] = key[i % len];
    }
    for (i = 0; i < 256; i++)
    {
        j = (j + s[i] + k[i]);
        swapByte(&s[i], &s[j]);
    }
}
void rc4(UCHAR* s, UCHAR* data, const ULONG len)
{
    UCHAR i = 0;
    UCHAR j = 0;
    UCHAR t = 0;
    ULONG k = 0;

    for (k = 0; k < len; k++)
    {
        i = (i + 1);
        j = (j + s[i]);
        swapByte(&s[i], &s[j]);
        t = (s[i] + s[j]);
        data[k] ^= s[t];
    }
}

void symmetricEncDec(UCHAR* data, const ULONG len, const UCHAR* key, const ULONG keyLen)
{
    UCHAR s[256] = { 0 };

    rc4_key(s, key, keyLen);
    rc4(s, data, len);
}
