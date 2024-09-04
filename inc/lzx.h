// lzx.h

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

#ifndef LXZ_H
#define LXZ_H

#include "type_defs.h"

// decompress a block of data.
// data: the compressed data.
// size: the size of the compressed data.
// buff: the buffer to store the decompressed data. You can pre-allocate a buffer and pass it in or pass in a null buffer; will reallocate as needed.
// buffSize: the size of the buffer; will be updated if the buffer is reallocated. if pre-allocated, this should be the size of the pre-allocated buffer.
// decompressedSize: if not NULL, gets updated with the size of the decompressed data.
int decompress(const UCHAR* data, const UINT size, UCHAR*& buff, UINT& buffSize, UINT* decompressedSize);

// decompress a COFF/PE image. Decompresses the first block and determines how much space is
// needed to decompress the rest of the image, allocates and contiunes with decompression.
// data: the compressed COFF/PE img.
// size: the size of the compressed data.
// decompressedSize: if not NULL, gets updated with the size of the decompressed data.
UCHAR* decompressImg(const UCHAR* data, const UINT size, UINT* decompressedSize);

#endif
