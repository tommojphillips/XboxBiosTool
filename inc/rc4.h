// rc4.h

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

#ifndef _RC4_H
#define _RC4_H

#include "type_defs.h"

void rc4_key(UCHAR* s, const UCHAR* key, const ULONG len);
void rc4(UCHAR* s, UCHAR* data, const ULONG len);

void symmetricEncDec(UCHAR* data, const ULONG len, const UCHAR* key, const ULONG keyLen);

#endif // _RC4_H
