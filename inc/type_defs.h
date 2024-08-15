// type_defs.h

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

#ifndef  XB_TYPE_DEFS_H
#define XB_TYPE_DEFS_H

#ifndef NULL
#define NULL 0
#endif

typedef unsigned long ULONG;
typedef unsigned int UINT;
typedef unsigned char UCHAR;
typedef unsigned short USHORT;

#define MAKE_4BYTE_SIGNATURE(a,b,c,d) (a + (b<<8) + (c<<16) + (d<<24))

#endif // ! XB_BIOS_TYPE_DEFS_H
