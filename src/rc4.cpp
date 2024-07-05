// rc4.cpp : defines functions for symmetric encryption and decryption using the rc4 algorithm

/* Copyright(C) 2024 tommojphillips
 *
 * This program is free software : you can redistribute it and /or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see < https://www.gnu.org/licenses/>.
*/

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
