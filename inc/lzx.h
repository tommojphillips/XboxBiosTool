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
#include <stdbool.h>
#include <stdio.h>

#define LZX_WINDOW_SIZE (128*1024)
#define LZX_CHUNK_SIZE (32*1024)
#define LZX_MAX_GROWTH 6144
#define LZX_OUTPUT_SIZE (LZX_CHUNK_SIZE+LZX_MAX_GROWTH)
#define LZX_NUM_REPEATED_OFFSETS 3
#define LZX_MAIN_TREE_ELEMENTS(x) (256 + (x << 3))
#define LZX_MIN_MATCH 2
#define LZX_MAX_MATCH (LZX_MIN_MATCH + 255)
#define LZX_NUM_PRIMARY_LEN 7
#define LZX_NUM_SECONDARY_LEN ((LZX_MAX_MATCH - LZX_MIN_MATCH + 1) - LZX_NUM_PRIMARY_LEN)
#define LZX_ALIGNED_TABLE_BITS 7
#define LZX_ALIGNED_NUM_ELEMENTS 8
#define LZX_MAX_MAIN_TREE_ELEMENTS 700
#define MAIN_TREE_TABLE_BITS 10
#define SECONDARY_LEN_TREE_TABLE_BITS 8

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
    uint16_t compressed_size;
    uint16_t uncompressed_size;
} LZX_BLOCK;

typedef struct {
    uint8_t* mem_window;
    uint32_t window_size;
    uint32_t window_mask;
    uint32_t last_matchpos_offset[LZX_NUM_REPEATED_OFFSETS];
    short main_tree_table[1 << MAIN_TREE_TABLE_BITS];
    short secondary_len_tree_table[1 << SECONDARY_LEN_TREE_TABLE_BITS];
    uint8_t main_tree_len[LZX_MAX_MAIN_TREE_ELEMENTS];
    uint8_t secondary_len_tree_len[LZX_NUM_SECONDARY_LEN];
    uint8_t pad1[2];
    uint8_t num_position_slots;
    char aligned_table[1 << LZX_ALIGNED_TABLE_BITS];
    uint8_t aligned_len[LZX_ALIGNED_NUM_ELEMENTS];
    short main_tree_left_right[LZX_MAX_MAIN_TREE_ELEMENTS * 4];
    short secondary_len_tree_left_right[LZX_NUM_SECONDARY_LEN * 4];
    const uint8_t* input_curpos;
    const uint8_t* end_input_pos;
    uint8_t* output_buffer;
    uint8_t* input_buffer;
    long position_at_start;
    uint8_t main_tree_prev_len[LZX_MAX_MAIN_TREE_ELEMENTS];
    uint8_t secondary_len_tree_prev_len[LZX_NUM_SECONDARY_LEN];
    char bitcount;
    bool first_time_this_group;
    bool error_condition;
    uint32_t bitbuf;
    long pos;
    uint32_t current_file_size;
    uint32_t instr_pos;
    uint32_t num_cfdata_frames;
    long original_block_size;
    long block_size;
    int block_type;
    int decoder_state;
} LZX_DECODER_CONTEXT;

typedef struct {
    uint32_t link;
    uint32_t path;
    uint32_t repeated_offset[LZX_NUM_REPEATED_OFFSETS];
    uint32_t numbits;
} DECISION_NODE;

typedef struct {
    uint8_t* mem_window;
    uint32_t window_size;
    uint32_t* tree_root;
    uint32_t* left;
    uint32_t* right;
    uint32_t bitbuf;
    char bitcount;
    char depth;
    bool output_overflow;
    uint32_t literals;
    uint32_t distances;
    uint32_t* dist_data;
    uint8_t* lit_data;
    uint8_t* item_type;
    uint32_t repeated_offset_at_literal_zero[LZX_NUM_REPEATED_OFFSETS];
    uint32_t last_matchpos_offset[LZX_NUM_REPEATED_OFFSETS];
    uint32_t matchpos_table[LZX_MAX_MATCH + 1];
    uint32_t bufpos;
    uint8_t slot_table[1024];
    uint8_t* output_buffer_start;
    uint8_t* output_buffer_curpos;
    uint8_t* output_buffer_end;
    uint32_t input_running_total;
    uint32_t bufpos_last_output_block;
    uint32_t num_position_slots;
    uint32_t file_size_for_translation;
    uint8_t num_block_splits;
    uint8_t first_block;
    bool need_to_recalc_stats;
    bool first_time_this_group;
    uint8_t ones[256];
    uint32_t encoder_second_partition_size;
    uint32_t earliest_window_data_remaining;
    uint32_t bufpos_at_last_block;
    uint8_t* input_ptr;
    long input_left;
    uint32_t instr_pos;
    uint16_t* tree_freq;
    uint16_t* tree_sortptr;
    uint8_t* len;
    short tree_heap[LZX_MAX_MAIN_TREE_ELEMENTS + 2];
    uint16_t tree_leftright[2 * (2 * LZX_MAX_MAIN_TREE_ELEMENTS - 1)];
    uint16_t tree_len_cnt[17];
    short tree_heapsize;
    int tree_n;
    uint32_t next_tree_create;
    uint32_t last_literals;
    uint32_t last_distances;
    DECISION_NODE* decision_node;
    uint8_t main_tree_len[LZX_MAX_MAIN_TREE_ELEMENTS + 1];
    uint8_t secondary_tree_len[LZX_NUM_SECONDARY_LEN + 1];
    uint16_t main_tree_freq[LZX_MAX_MAIN_TREE_ELEMENTS * 2];
    uint16_t main_tree_code[LZX_MAX_MAIN_TREE_ELEMENTS];
    uint8_t main_tree_prev_len[LZX_MAX_MAIN_TREE_ELEMENTS + 1];
    uint16_t secondary_tree_freq[LZX_NUM_SECONDARY_LEN * 2];
    uint16_t secondary_tree_code[LZX_NUM_SECONDARY_LEN];
    uint8_t secondary_tree_prev_len[LZX_NUM_SECONDARY_LEN + 1];
    uint16_t aligned_tree_freq[LZX_ALIGNED_NUM_ELEMENTS * 2];
    uint16_t aligned_tree_code[LZX_ALIGNED_NUM_ELEMENTS];
    uint8_t aligned_tree_len[LZX_ALIGNED_NUM_ELEMENTS];
    uint8_t aligned_tree_prev_len[LZX_ALIGNED_NUM_ELEMENTS];
    uint8_t* real_mem_window;
    uint32_t* real_left;
    uint32_t* real_right;
    uint32_t cfdata_frames;
    uint8_t* input_buffer;
    uint8_t* output_buffer;
    uint32_t output_buffer_size;
    uint32_t output_buffer_block_count;
} ENCODER_CONTEXT;

#ifdef __cplusplus
extern "C" {
#endif

/* Create lzx decoder */
LZX_DECODER_CONTEXT* lzx_create_decompression();

/* Destroy lzx decoder */
void lzx_destroy_decompression(LZX_DECODER_CONTEXT* context);

/* Decompress block */
int lzx_decompress_block(LZX_DECODER_CONTEXT* context, const uint8_t* src, uint32_t src_size, uint8_t* dest, uint32_t* bytes_decompressed);

/* Decompress next block */
int lzx_decompress_next_block(LZX_DECODER_CONTEXT* context, const uint8_t** src, uint32_t* src_size, uint8_t** dest, uint32_t* bytes_decompressed);

/* Decompress the input buffer block by block into the output buffer
 src: Input buffer
 src_size: Input buffer size
 dest: Address of the output buffer. pre-allocate or null buffer.
 dest_size: Output buffer size; returns the output buffer size. if output buffer is pre-allocated, this should be the size of the pre-allocated buffer.
 decompressed_size: Returns the decompressed size.
 returns 0 on SUCCESS, otherwise LZX_ERROR */
int lzx_decompress(const uint8_t* src, const uint32_t src_size, uint8_t** dest, uint32_t* dest_size, uint32_t* decompressed_size);

/* Create lzx encoder */
ENCODER_CONTEXT* lzx_create_compression(uint8_t* dest);

/* Destroy lzx encoder */
void lzx_destroy_compression(ENCODER_CONTEXT* context);

/* Compress block */
int lzx_compress_block(ENCODER_CONTEXT* context, const uint8_t* src, uint32_t bytes_read);

/* Compress next block */
int lzx_compress_next_block(ENCODER_CONTEXT* context, const uint8_t** src, uint32_t bytes_read, uint32_t* bytes_remaining);

/* Flush lzx encoder */
void lzx_flush_compression(ENCODER_CONTEXT* context);

/* Compress the input buffer block by block into the output buffer
 src: Input buffer
 src_size: Input buffer size
 dest: Address of the output buffer. pre-allocate or null buffer
 compressed_size: Returns the compressed size
 returns 0 on SUCCESS, otherwise LZX_ERROR */
int lzx_compress(const uint8_t* src, const uint32_t src_size, uint8_t** dest, uint32_t* compressed_size);

#ifdef __cplusplus
};
#endif

#endif // LZX_H
