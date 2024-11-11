// lzx.h: lzx common definitions

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

#ifndef LZX_H
#define LZX_H

// std incl
#include <stdint.h>
#include <stdio.h>

#define LZX_MAX_GROWTH 6144

#define LZX_WINDOW_SIZE (128*1024)
#define LZX_CHUNK_SIZE (32*1024)
#define LZX_OUTPUT_SIZE (LZX_CHUNK_SIZE+LZX_MAX_GROWTH)

#define LZX_NUM_REPEATED_OFFSETS 3
#define LZX_MAIN_TREE_ELEMENTS(x) (256 + (x << 3))

#define LZX_MIN_MATCH 2
#define LZX_MAX_MATCH (LZX_MIN_MATCH + 255)

#define LZX_NUM_PRIMARY_LEN 7
#define LZX_NUM_SECONDARY_LEN ((LZX_MAX_MATCH - LZX_MIN_MATCH + 1) - LZX_NUM_PRIMARY_LEN)

#define LZX_ALIGNED_TABLE_BITS 7
#define LZX_ALIGNED_NUM_ELEMENTS 8

// error codes
#define LZX_ERROR_SUCCESS 0
#define LZX_ERROR_FAILED 1
#define LZX_ERROR_BUFFER_OVERFLOW 4
#define LZX_ERROR_OUT_OF_MEMORY 5
#define LZX_ERROR_INVALID_DATA 6

// block type
#define LZX_BLOCK_TYPE_INVALID 0
#define LZX_BLOCK_TYPE_VERBATIM 1
#define LZX_BLOCK_TYPE_ALIGNED 2
#define LZX_BLOCK_TYPE_UNCOMPRESSED 3

typedef struct _LZX_BLOCK {
    uint16_t compressedSize;
    uint16_t uncompressedSize;
} LZX_BLOCK;

#ifdef __cplusplus
extern "C" {
#endif

inline void lzxPrintError(int error) {
    switch (error)
    {
        case LZX_ERROR_OUT_OF_MEMORY:
            printf("out of memory\n");
            break;
        case LZX_ERROR_INVALID_DATA:
            printf("bad parameters\n");
            break;
        case LZX_ERROR_BUFFER_OVERFLOW:
            printf("buffer overflow\n");
            break;
        case LZX_ERROR_FAILED:
            printf("fatal error\n");
            break;
        case LZX_ERROR_SUCCESS:
            printf("success\n");
            break;
        default:
            printf("unknown error");
            break;
    }
};

// Decompress data.
// decompresses block by block into the output buffer. reallocates the output buffer as needed.
// src: Compressed data.
// src_size: Compressed data size.
// dest: Address of the output buffer. pre-allocate or null buffer.
// dest_size: Output buffer size; returns the output buffer size. if output buffer is pre-allocated, this should be the size of the pre-allocated buffer.
// decompressed_size: Returns the decompressed size.
// returns 0 otherwise, LZX_ERROR
int lzxDecompress(const uint8_t* src, const uint32_t src_size, uint8_t** dest, uint32_t* dest_size, uint32_t* decompressed_size);

// Decompress coff/pe image. 
// decompresses the first block and determines how much space is needed to 
// decompress the entire image, allocates and contiunes with decompression.
// src: Compressed image.
// src_size: Compressed image size.
// image_size: Returns the decompressed size.
// returns the decompressed image. or NULL if failed.
uint8_t* lzxDecompressImage(const uint8_t* src, const uint32_t src_size, uint32_t* image_size);


// Compress data
// src: data.
// src_size: size.
// dest: Address of the output buffer. pre-allocate or null the buffer; reallocated as needed.
// compressed_size: Returns the compressed size.
int lzxCompress(const uint8_t* src, const uint32_t src_size, uint8_t** dest, uint32_t* compressed_size);

#ifdef __cplusplus
};
#endif

#endif // LZX_H
