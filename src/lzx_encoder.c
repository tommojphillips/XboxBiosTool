// lzx_encoder.c: lzx compression algorithm

// std incl
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>

// user incl
#include "lzx.h"

#ifndef NO_MEM_TRACKING
#include "mem_tracking.h"
#else
#include <malloc.h>
#endif

#define MAX_MAIN_TREE_ELEMENTS 700
#define SECONDARY_PARTITION_SIZE (64*1024)

#define DEFAULT_FILE_XLAT_SIZE 12000000
#define MAX_LITERAL_ITEMS 65536
#define MIN_LITERALS_REQUIRED 6144
#define MIN_LITERALS_IN_BLOCK 4096
#define MAX_DIST_ITEMS 32768
#define NL_SHIFT 3

#define EXTRA_SIZE 4096
#define LOOK (EXTRA_SIZE-LZX_MAX_MATCH-2)

#define OUTPUT_EXTRA_BYTES 64

#define MAX_LENGTH_TWO_OFFSET (128*1024)
#define BREAK_MAX_LENGTH_TWO_OFFSET 2048

#define E8_CFDATA_FRAME_THRESHOLD 32768
#define NUM_REPEATED_OFFSETS 3
#define BREAK_LENGTH 50
#define TREE_CREATE_INTERVAL 4096
#define STEP_SIZE 64
#define RESOLUTION 1024
#define THRESHOLD 1400
#define EARLY_BREAK_THRESHOLD 1700
#define MAX_BLOCK_SPLITS 4
#define FAST_DECISION_THRESHOLD 50
#define MPSLOT3_CUTOFF 16

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define log2(x) ((x) < 256 ? log2_table[(x)] : 8 + log2_table[(x) >> 8])

#define MEM_WINDOW_ALLOC_SIZE (context->window_size + (LZX_MAX_MATCH + EXTRA_SIZE) + context->encoder_second_partition_size)
#define NUM_CHARS 256

#define TREE_ENC_REP_MIN 4
#define TREE_ENC_REP_ZERO_FIRST 16
#define TREE_ENC_REP_ZERO_SECOND 32
#define TREE_ENC_REP_SAME_FIRST 2
#define TREE_ENC_REPZ_FIRST_EXTRA_BITS 4
#define TREE_ENC_REPZ_SECOND_EXTRA_BITS 5
#define TREE_ENC_REP_SAME_EXTRA_BITS 1

#define NUM_SEARCH_TREES 65536

#define MP_SLOT(matchpos) \
    ((matchpos) < 1024 ? (uint8_t) context->slot_table[(matchpos)] : ((matchpos) < 524288L ? (uint8_t)18 + (uint8_t)context->slot_table[(matchpos) >> 9] : ((uint8_t)34 + (uint8_t)(matchpos >> 17))))

#define IS_MATCH(l) (context->item_type[(l) >> 3] & (1 << ((l) & 7)))
#define CHAR_EST(c) (uint32_t)(context->main_tree_len[(c)])
#define MATCH_EST(ml,mp,result) { \
	uint8_t mp_slot = (uint8_t)MP_SLOT(mp); \
	if (ml < (LZX_NUM_PRIMARY_LEN+2)) \
		result = (uint32_t)(context->main_tree_len[(NUM_CHARS-2)+(mp_slot<<NL_SHIFT)+ml] +	lzx_extra_bits[mp_slot]); \
	else \
		result = (uint32_t)(context->main_tree_len[(NUM_CHARS+LZX_NUM_PRIMARY_LEN) + (mp_slot<<NL_SHIFT)] + context->secondary_tree_len[ml-(LZX_NUM_PRIMARY_LEN+2)] + lzx_extra_bits[mp_slot]); \
}

#define OUT_MATCH(len, pos) { \
    context->item_type[(context->literals >> 3)] |= (1 << (context->literals & 7)); \
    context->lit_data [context->literals++] = (uint8_t)(len-2); \
    context->dist_data[context->distances++] = pos; \
}

#define TREE_CREATE_CHECK()	\
if (context->literals >= context->next_tree_create) { \
    update_tree_estimates(context); \
    context->next_tree_create += TREE_CREATE_INTERVAL; \
}

#define OUTPUT_BITS(N, X) { \
   context->bitbuf |= (((uint32_t) (X)) << (context->bitcount-(N))); \
   context->bitcount -= (N); \
   while (context->bitcount <= 16) { \
      if (context->output_buffer_curpos >= context->output_buffer_end) { \
          context->output_overflow = true; \
          context->output_buffer_curpos = context->output_buffer_start; \
      } \
      *context->output_buffer_curpos++ = (uint8_t) ((context->bitbuf >> 16) & 255); \
      *context->output_buffer_curpos++ = (uint8_t) (context->bitbuf >> 24); \
      context->bitbuf <<= 16; \
      context->bitcount += 16; \
   } \
}

#define OUT_CHAR_BITS OUTPUT_BITS(context->main_tree_len[context->lit_data[l]], context->main_tree_code[context->lit_data[l]]);

static const long square_table[17] = {
    0,1,4,9,16,25,36,49,64,81,100,121,144,169,196,225,256
};
static const uint8_t log2_table[256] = {
    0,1,2,2,3,3,3,3,
    4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,
    8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8
};
static const uint32_t slot_mask[] = {
         0,      0,      0,      0,     1,       1,      3,      3,
         7,      7,     15,     15,    31,      31,     63,     63,
       127,    127,    255,    255,   511,     511,   1023,   1023,
      2047,   2047,   4095,   4095,  8191,    8191,  16383,  16383,
     32767,  32767,  65535,  65535, 131071, 131071, 131071, 131071,
    131071, 131071, 131071, 131071, 131071, 131071, 131071, 131071,
    131071, 131071, 131071
};

static const uint8_t lzx_extra_bits[] = {
    0,0,0,0,1,1,2,2,
    3,3,4,4,5,5,6,6,
    7,7,8,8,9,9,10,10,
    11,11,12,12,13,13,14,14,
    15,15,16,16,17,17,17,17,
    17,17,17,17,17,17,17,17,
    17,17,17
};

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
    bool allocated_compression_memory;
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
    short tree_heap[MAX_MAIN_TREE_ELEMENTS + 2];
    uint16_t tree_leftright[2 * (2 * MAX_MAIN_TREE_ELEMENTS - 1)];
    uint16_t tree_len_cnt[17];
    short tree_heapsize;
    int tree_n;
    uint32_t next_tree_create;
    uint32_t last_literals;
    uint32_t last_distances;
    DECISION_NODE* decision_node;
    uint8_t main_tree_len[MAX_MAIN_TREE_ELEMENTS + 1];
    uint8_t secondary_tree_len[LZX_NUM_SECONDARY_LEN + 1];
    uint16_t main_tree_freq[MAX_MAIN_TREE_ELEMENTS * 2];
    uint16_t main_tree_code[MAX_MAIN_TREE_ELEMENTS];
    uint8_t main_tree_prev_len[MAX_MAIN_TREE_ELEMENTS + 1];
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

static void encode_flush(ENCODER_CONTEXT* context);

static bool init_compressed_output_buffer(ENCODER_CONTEXT* context)
{
    context->output_buffer_start = (uint8_t*)malloc(LZX_OUTPUT_SIZE);
    if (context->output_buffer_start == NULL)
        return false;

    context->output_buffer_curpos = context->output_buffer_start;
    context->output_buffer_end = context->output_buffer_start + (LZX_OUTPUT_SIZE - OUTPUT_EXTRA_BYTES);

    return true;
}

static void create_ones_table(ENCODER_CONTEXT* context)
{
    int i, j;
    uint8_t ones;

    for (i = 0; i < 256; i++) {
        ones = 0;
        for (j = i; j; j >>= 1) {
            if (j & 1)
                ones++;
        }
        context->ones[i] = ones;
    }
}
static void create_slot_lookup_table(ENCODER_CONTEXT* context)
{
    int j;
    int p;
    int elements_to_init;
    uint8_t slotnum;

    context->slot_table[0] = 0;
    context->slot_table[1] = 1;
    context->slot_table[2] = 2;
    context->slot_table[3] = 3;

    elements_to_init = 2;

    slotnum = 4;
    p = 4;

    do {
        for (j = elements_to_init; j > 0; j--)
            context->slot_table[p++] = slotnum;

        slotnum++;

        for (j = elements_to_init; j > 0; j--)
            context->slot_table[p++] = slotnum;

        slotnum++;
        elements_to_init <<= 1;

    } while (p < 1024);
}

static void translate_e8(ENCODER_CONTEXT* context, uint8_t* mem, long bytes)
{
    long offset;
    long absolute;
    uint32_t end_instr_pos;
    uint8_t temp[6];
    uint8_t* mem_backup;

    if (bytes <= 6) {
        context->instr_pos += bytes;
        return;
    }

    mem_backup = mem;

    memcpy(temp, &mem[bytes - 6], 6);
    memset(&mem[bytes - 6], 0xE8, 6);

    end_instr_pos = context->instr_pos + bytes - 10;

    while (1) {
        while (*mem++ != 0xE8)
            context->instr_pos++;

        if (context->instr_pos >= end_instr_pos)
            break;

        offset = *(long*)mem;

        absolute = context->instr_pos + offset;

        if (absolute >= 0)
        {
            if ((uint32_t)absolute < context->file_size_for_translation + context->instr_pos) {
                if ((uint32_t)absolute >= context->file_size_for_translation)
                    absolute = offset - context->file_size_for_translation;

                * (uint32_t*)mem = (uint32_t)absolute;
            }
        }

        mem += 4;
        context->instr_pos += 5;
    }

    memcpy(&mem_backup[bytes - 6], temp, 6);

    context->instr_pos = end_instr_pos + 10;
}
static void output_bits(ENCODER_CONTEXT* context, int n, uint32_t x)
{
    context->bitbuf |= (x << (context->bitcount - n));
    context->bitcount -= (char)n;

    while (context->bitcount <= 16) {
        if (context->output_buffer_curpos >= context->output_buffer_end) {
            context->output_overflow = true;
            context->output_buffer_curpos = context->output_buffer_start;
        }

        *context->output_buffer_curpos++ = (uint8_t)((context->bitbuf >> 16) & 255);
        *context->output_buffer_curpos++ = (uint8_t)(context->bitbuf >> 24);

        context->bitbuf <<= 16;
        context->bitcount += 16;
    }
}

static long read_input_data(ENCODER_CONTEXT* context, uint8_t* mem, long amount)
{
    if (amount <= context->input_left) {
        memcpy(mem, context->input_ptr, amount);
        context->input_left -= amount;
        context->input_ptr += amount;
        return amount;
    }
    else {
        long bytes_read;

        if (context->input_left <= 0)
            return 0;

        bytes_read = context->input_left;

        memcpy(mem, context->input_ptr, context->input_left);
        context->input_ptr += context->input_left;
        context->input_left = 0;

        return bytes_read;
    }
}
static void prevent_far_matches(ENCODER_CONTEXT* context)
{
    for (uint32_t i = MP_SLOT(MAX_LENGTH_TWO_OFFSET); i < context->num_position_slots; i++) {
        context->main_tree_len[NUM_CHARS + (i << NL_SHIFT)] = 100;
    }
}

static void init_compression_memory(ENCODER_CONTEXT* context)
{
    memset(context->tree_root, 0, NUM_SEARCH_TREES * sizeof(context->tree_root[0]));

    context->mem_window = context->real_mem_window - context->window_size;
    context->left = context->real_left - context->window_size;
    context->right = context->real_right - context->window_size;
    context->bufpos = context->window_size;

    context->last_matchpos_offset[0] = 1;
    context->last_matchpos_offset[1] = 1;
    context->last_matchpos_offset[2] = 1;

    context->repeated_offset_at_literal_zero[0] = 1;
    context->repeated_offset_at_literal_zero[1] = 1;
    context->repeated_offset_at_literal_zero[2] = 1;

    context->first_block = true;
    context->need_to_recalc_stats = true;
    context->bufpos_last_output_block = context->bufpos;
    context->bitcount = 32;
    context->bitbuf = 0;
    context->output_overflow = false;

    memset(context->main_tree_prev_len, 0, LZX_MAIN_TREE_ELEMENTS(context->num_position_slots));
    memset(context->secondary_tree_prev_len, 0, LZX_NUM_SECONDARY_LEN);
    memset(context->main_tree_len, 8, 256);
    memset(&context->main_tree_len[256], 9, LZX_MAIN_TREE_ELEMENTS(context->num_position_slots) - 256);
    memset(context->secondary_tree_len, 6, LZX_NUM_SECONDARY_LEN);
    memset(context->aligned_tree_len, 3, LZX_ALIGNED_NUM_ELEMENTS);

    prevent_far_matches(context);

    context->bufpos_at_last_block = context->bufpos;
    context->earliest_window_data_remaining = context->bufpos;
    context->input_running_total = 0;
    context->first_time_this_group = true;

    memset(context->item_type, 0, MAX_LITERAL_ITEMS / 8);

    context->literals = 0;
    context->distances = 0;
    context->instr_pos = 0;
    context->cfdata_frames = 0;
    context->num_block_splits = 0;

    context->repeated_offset_at_literal_zero[0] = 1;
    context->repeated_offset_at_literal_zero[1] = 1;
    context->repeated_offset_at_literal_zero[2] = 1;

    memset(context->main_tree_freq, 0, sizeof(context->main_tree_freq));
    memset(context->secondary_tree_freq, 0, sizeof(context->secondary_tree_freq));
    memset(context->aligned_tree_freq, 0, sizeof(context->aligned_tree_freq));
    memset(context->real_mem_window, 0, MEM_WINDOW_ALLOC_SIZE);
}
static void free_compress_memory(ENCODER_CONTEXT* context)
{
    if (context->tree_root != NULL) {
        free(context->tree_root);
        context->tree_root = NULL;
    }

    if (context->real_left != NULL) {
        free(context->real_left);
        context->real_left = NULL;
    }

    if (context->real_right != NULL) {
        free(context->real_right);
        context->real_right = NULL;
    }

    if (context->real_mem_window != NULL) {
        free(context->real_mem_window);
        context->real_mem_window = NULL;
        context->mem_window = NULL;
    }

    if (context->lit_data != NULL) {
        free(context->lit_data);
        context->lit_data = NULL;
    }

    if (context->dist_data != NULL) {
        free(context->dist_data);
        context->dist_data = NULL;
    }

    if (context->item_type != NULL) {
        free(context->item_type);
        context->item_type = NULL;
    }

    if (context->decision_node != NULL) {
        free(context->decision_node);
        context->decision_node = NULL;
    }

    if (context->output_buffer_start != NULL) {
        free(context->output_buffer_start);
        context->output_buffer_start = NULL;
    }

    if (context->input_buffer != NULL) {
		free(context->input_buffer);
		context->input_buffer = NULL;
	}

    context->allocated_compression_memory = false;
}
static bool alloc_compress_memory(ENCODER_CONTEXT* context)
{
    uint32_t pos_start;

    context->tree_root = NULL;
    context->real_left = NULL;
    context->real_right = NULL;
    context->mem_window = NULL;
    context->decision_node = NULL;
    context->lit_data = NULL;
    context->dist_data = NULL;
    context->item_type = NULL;
    context->output_buffer_start = NULL;
    context->num_position_slots = 4;
    pos_start = 4;

    while (1) {
        pos_start += 1 << lzx_extra_bits[context->num_position_slots];

        context->num_position_slots++;

        if (pos_start >= context->window_size)
            break;
    }

    context->tree_root = (uint32_t*)malloc(sizeof(context->tree_root[0]) * NUM_SEARCH_TREES);
    if (context->tree_root == NULL) {
        free_compress_memory(context);
        return false;
    }

    context->real_left = (uint32_t*)malloc(sizeof(uint32_t) * MEM_WINDOW_ALLOC_SIZE);
    if (context->real_left == NULL) {
        free_compress_memory(context);
        return false;
    }

    context->real_right = (uint32_t*)malloc(sizeof(uint32_t) * MEM_WINDOW_ALLOC_SIZE);
    if (context->real_right == NULL) {
        free_compress_memory(context);
        return false;
    }

    context->real_mem_window = (uint8_t*)malloc(MEM_WINDOW_ALLOC_SIZE);
    if (context->real_mem_window == NULL) {
        free_compress_memory(context);
        return false;
    }

    context->mem_window = context->real_mem_window;

    context->lit_data = (uint8_t*)malloc(MAX_LITERAL_ITEMS * sizeof(uint8_t));
    if (context->lit_data == NULL) {
        free_compress_memory(context);
        return false;
    }

    context->dist_data = (uint32_t*)malloc(MAX_DIST_ITEMS * sizeof(*context->dist_data));
    if (context->dist_data == NULL) {
        free_compress_memory(context);
        return false;
    }

    context->item_type = (uint8_t*)malloc(MAX_LITERAL_ITEMS / 8);

    if (context->item_type == NULL) {
        free_compress_memory(context);
        return false;
    }

    create_slot_lookup_table(context);
    create_ones_table(context);

    if (init_compressed_output_buffer(context) == false) {
        free_compress_memory(context);
        return false;
    }

    context->decision_node = (DECISION_NODE*)malloc(sizeof(DECISION_NODE) * (LOOK + LZX_MAX_MATCH + 16));
    if (context->decision_node == NULL) {
        free_compress_memory(context);
        return false;
    }

    context->input_buffer = (uint8_t*)malloc(LZX_CHUNK_SIZE);
    if (context->input_buffer == NULL) {
		free_compress_memory(context);
		return false;
	}

    context->allocated_compression_memory = true;

    return true;
}

static long read_input(ENCODER_CONTEXT* context, uint32_t buf_pos, long size)
{
    long bytes_read;

    if (size <= 0)
        return 0;

    bytes_read = read_input_data(context, &context->real_mem_window[buf_pos], size);

    if (bytes_read < 0)
        return 0;

    if (context->file_size_for_translation == 0 || context->cfdata_frames >= E8_CFDATA_FRAME_THRESHOLD)
    {
        context->cfdata_frames++;
        return bytes_read;
    }

    translate_e8(context, &context->real_mem_window[buf_pos], bytes_read);

    context->cfdata_frames++;

    return bytes_read;
}
static uint32_t return_difference(ENCODER_CONTEXT* context, uint32_t item_start1, uint32_t item_start2, uint32_t dist_at_1, uint32_t dist_at_2, uint32_t size)
{
    uint16_t freq1[800];
    uint16_t freq2[800];
    uint32_t i;
    uint32_t cum_diff;
    int element;

    if (LZX_MAIN_TREE_ELEMENTS(context->num_position_slots) >= (sizeof(freq1) / sizeof(freq1[0])))
        return 0;

    memset(freq1, 0, sizeof(freq1[0]) * LZX_MAIN_TREE_ELEMENTS(context->num_position_slots));
    memset(freq2, 0, sizeof(freq2[0]) * LZX_MAIN_TREE_ELEMENTS(context->num_position_slots));

    for (i = 0; i < size; i++)
    {
        if (!IS_MATCH(item_start1))
        {
            element = context->lit_data[item_start1];
        }
        else
        {
            if (context->lit_data[item_start1] < LZX_NUM_PRIMARY_LEN)
                element = NUM_CHARS + (MP_SLOT(context->dist_data[dist_at_1]) << NL_SHIFT) + context->lit_data[item_start1];
            else
                element = (NUM_CHARS + LZX_NUM_PRIMARY_LEN) + (MP_SLOT(context->dist_data[dist_at_1]) << NL_SHIFT);

            dist_at_1++;
        }

        item_start1++;
        freq1[element]++;

        if (!IS_MATCH(item_start2))
        {
            element = context->lit_data[item_start2];
        }
        else
        {
            if (context->lit_data[item_start2] < LZX_NUM_PRIMARY_LEN)
                element = NUM_CHARS + (MP_SLOT(context->dist_data[dist_at_2]) << NL_SHIFT) + context->lit_data[item_start2];
            else
                element = (NUM_CHARS + LZX_NUM_PRIMARY_LEN) + (MP_SLOT(context->dist_data[dist_at_2]) << NL_SHIFT);

            dist_at_2++;
        }

        item_start2++;
        freq2[element]++;
    }

    cum_diff = 0;

    for (i = 0; i < (uint32_t)LZX_MAIN_TREE_ELEMENTS(context->num_position_slots); i++)
    {
        uint32_t log2a, log2b, diff;

        log2a = (uint32_t)log2(freq1[i]);
        log2b = (uint32_t)log2(freq2[i]);

        diff = square_table[log2a] - square_table[log2b];

        cum_diff += abs((long)diff);
    }

    return cum_diff;
}
static bool split_block(ENCODER_CONTEXT* context, uint32_t start, uint32_t end, uint32_t distance_to_end_at, uint32_t* split_at_literal, uint32_t* split_at_distance)
{
    uint32_t i, j;
    uint32_t d = 0;
    int nd = 0;
    uint16_t num_dist_at_item[(MAX_LITERAL_ITEMS / STEP_SIZE) + 8] = {0};

    *split_at_literal = end;

    if (split_at_distance)
        *split_at_distance = distance_to_end_at;

    if (end - start < MIN_LITERALS_REQUIRED)
        return false;
    if (context->num_block_splits >= MAX_BLOCK_SPLITS)
        return false;

    for (i = 0; i < (end >> 3); i++)
    {
        if ((i & ((STEP_SIZE >> 3) - 1)) == 0)
            num_dist_at_item[nd++] = (uint16_t)d;

        d += context->ones[context->item_type[i]];
    }

    start = (start + (STEP_SIZE - 1)) & (~(STEP_SIZE - 1));

    for (i = start + 2 * RESOLUTION;
        i < end - 4 * RESOLUTION;
        i += RESOLUTION)
    {
        if (return_difference(context, i, i + 1 * RESOLUTION, (uint32_t)num_dist_at_item[i / STEP_SIZE], (uint32_t)num_dist_at_item[(i + 1 * RESOLUTION) / STEP_SIZE], RESOLUTION) > THRESHOLD
            && return_difference(context, i - RESOLUTION, i + 2 * RESOLUTION, (uint32_t)num_dist_at_item[(i - RESOLUTION) / STEP_SIZE], (uint32_t)num_dist_at_item[(i + 2 * RESOLUTION) / STEP_SIZE], RESOLUTION) > THRESHOLD
            && return_difference(context, i - 2 * RESOLUTION, i + 3 * RESOLUTION, (uint32_t)num_dist_at_item[(i - 2 * RESOLUTION) / STEP_SIZE], (uint32_t)num_dist_at_item[(i + 3 * RESOLUTION) / STEP_SIZE], RESOLUTION) > THRESHOLD)
        {
            uint32_t max_diff = 0;
            uint32_t literal_split = 0;

            for (j = i + RESOLUTION / 2; j < i + (5 * RESOLUTION) / 2; j += STEP_SIZE)
            {
                uint32_t diff;
                diff = return_difference(context, j - RESOLUTION, j, (uint32_t)num_dist_at_item[(j - RESOLUTION) / STEP_SIZE], (uint32_t)num_dist_at_item[j / STEP_SIZE], RESOLUTION);

                if (diff > max_diff)
                {
                    max_diff = diff;
                    literal_split = j;
                }
            }

            if (max_diff >= EARLY_BREAK_THRESHOLD &&
                (literal_split - start) >= MIN_LITERALS_IN_BLOCK)
            {
                context->num_block_splits++;

                *split_at_literal = literal_split;

                if (split_at_distance)
                    *split_at_distance = num_dist_at_item[literal_split / STEP_SIZE];

                return true;
            }
        }
    }

    return false;
}

static void count_len(ENCODER_CONTEXT* context, short i)
{
    if (i < context->tree_n)
    {
        context->tree_len_cnt[(context->depth < 16) ? context->depth : 16]++;
    }
    else
    {
        context->depth++;
        count_len(context, context->tree_leftright[i * 2]);
        count_len(context, context->tree_leftright[i * 2 + 1]);
        context->depth--;
    }
}
static void make_len(ENCODER_CONTEXT* context, short root)
{
    short k;
    uint16_t cum;
    uint8_t i;

    for (i = 0; i <= 16; i++)
        context->tree_len_cnt[i] = 0;

    count_len(context, root);

    cum = 0;

    for (i = 16; i > 0; i--)
        cum += (uint16_t)(context->tree_len_cnt[i] << (16 - i));

    while (cum)
    {
        context->tree_len_cnt[16]--;

        for (i = 15; i > 0; i--)
        {
            if (context->tree_len_cnt[i])
            {
                context->tree_len_cnt[i]--;
                context->tree_len_cnt[i + 1] += 2;
                break;
            }
        }

        cum--;
    }

    for (i = 16; i > 0; i--)
    {
        k = context->tree_len_cnt[i];

        while (--k >= 0)
            context->len[*context->tree_sortptr++] = (uint8_t)i;
    }
}

static void down_heap(ENCODER_CONTEXT* context, short i)
{
    short  j, k;

    k = context->tree_heap[i];

    while ((j = (i << 1)) <= context->tree_heapsize)
    {
        if (j < context->tree_heapsize &&
            context->tree_freq[context->tree_heap[j]] > context->tree_freq[context->tree_heap[j + 1]])
            j++;

        if (context->tree_freq[k] <= context->tree_freq[context->tree_heap[j]])
            break;

        context->tree_heap[i] = context->tree_heap[j];
        i = j;
    }

    context->tree_heap[i] = k;
}
static void make_code(ENCODER_CONTEXT* context, int n, char len[], uint16_t code[])
{
    int i;
    uint16_t start[18];

    start[0] = 0;
    start[1] = 0;

    for (i = 1; i <= 16; i++)
        start[i + 1] = (start[i] + context->tree_len_cnt[i]) << 1;

    for (i = 0; i < n; i++)
    {
        code[i] = start[len[i]]++;
    }
}
static void make_tree2(ENCODER_CONTEXT* context, short avail, uint16_t freqparm[], uint16_t codeparm[])
{
    short i, j, k;

    for (i = context->tree_heapsize >> 1; i >= 1; i--)
        down_heap(context, i);

    context->tree_sortptr = codeparm;

    do
    {
        i = context->tree_heap[1];

        if (i < context->tree_n)
            *context->tree_sortptr++ = i;

        context->tree_heap[1] = context->tree_heap[context->tree_heapsize--];
        down_heap(context, 1);

        j = context->tree_heap[1];

        if (j < context->tree_n)
            *context->tree_sortptr++ = j;

        k = avail++;

        freqparm[k] = freqparm[i] + freqparm[j];
        context->tree_heap[1] = k;
        down_heap(context, 1);

        context->tree_leftright[k * 2] = i;
        context->tree_leftright[k * 2 + 1] = j;

    } while (context->tree_heapsize > 1);

    context->tree_sortptr = codeparm;
    make_len(context, k);
}
static void make_tree(ENCODER_CONTEXT* context, int nparm, uint16_t* freqparm, uint8_t* lenparm, uint16_t* codeparm, bool make_codes)
{
    short i, avail;

RedoTree:
    context->tree_n = nparm;
    context->tree_freq = freqparm;
    context->len = lenparm;
    avail = (short)context->tree_n;
    context->depth = 0;
    context->tree_heapsize = 0;
    context->tree_heap[1] = 0;

    for (i = 0; i < nparm; i++)
    {
        context->len[i] = 0;

        if (freqparm[i])
            context->tree_heap[++context->tree_heapsize] = i;
    }

    if (context->tree_heapsize < 2)
    {
        if (!context->tree_heapsize)
        {
            codeparm[context->tree_heap[1]] = 0;
            return;
        }

        if (!context->tree_heap[1])
            freqparm[1] = 1;
        else
            freqparm[0] = 1;

        goto RedoTree;
    }

    make_tree2(context, avail, freqparm, codeparm);

    if (make_codes)
        make_code(context, nparm, (char*)lenparm, codeparm);
}
static void create_trees(ENCODER_CONTEXT* context, bool generate_codes)
{
    make_tree(context, NUM_CHARS + (context->num_position_slots * (LZX_NUM_PRIMARY_LEN + 1)), context->main_tree_freq, context->main_tree_len, context->main_tree_code, generate_codes);
    make_tree(context, LZX_NUM_SECONDARY_LEN, context->secondary_tree_freq, context->secondary_tree_len, context->secondary_tree_code, generate_codes);
    make_tree(context, LZX_ALIGNED_NUM_ELEMENTS, context->aligned_tree_freq, context->aligned_tree_len, context->aligned_tree_code, true);
}

static uint32_t tally_frequency(ENCODER_CONTEXT* context, uint32_t start, uint32_t start_at, uint32_t end)
{
    uint32_t i;
    uint32_t d;
    uint32_t bytes_compressed = 0;

    d = start_at;

    for (i = start; i < end; i++)
    {
        if (!IS_MATCH(i))
        {
            context->main_tree_freq[context->lit_data[i]]++;
            bytes_compressed++;
        }
        else
        {
            if (context->lit_data[i] < LZX_NUM_PRIMARY_LEN)
            {
                context->main_tree_freq[NUM_CHARS + (MP_SLOT(context->dist_data[d]) << NL_SHIFT) + context->lit_data[i]]++;
            }
            else
            {
                context->main_tree_freq[(NUM_CHARS + LZX_NUM_PRIMARY_LEN) + (MP_SLOT(context->dist_data[d]) << NL_SHIFT)]++;
                context->secondary_tree_freq[context->lit_data[i] - LZX_NUM_PRIMARY_LEN]++;
            }

            bytes_compressed += context->lit_data[i] + LZX_MIN_MATCH;
            d++;
        }
    }

    return bytes_compressed;
}

static void fix_tree_cost_estimates(ENCODER_CONTEXT* context)
{
    uint32_t i;

    for (i = 0; i < NUM_CHARS; i++)
    {
        if (context->main_tree_len[i] == 0)
            context->main_tree_len[i] = 11;
    }

    for (; i < NUM_CHARS + (context->num_position_slots * (LZX_NUM_PRIMARY_LEN + 1)); i++)
    {
        if (context->main_tree_len[i] == 0)
            context->main_tree_len[i] = 12;
    }

    for (i = 0; i < LZX_NUM_SECONDARY_LEN; i++)
    {
        if (context->secondary_tree_len[i] == 0)
            context->secondary_tree_len[i] = 8;
    }

    prevent_far_matches(context);
}
static uint32_t get_block_stats(ENCODER_CONTEXT* context, uint32_t start, uint32_t start_at, uint32_t end)
{
    memset(context->main_tree_freq, 0, LZX_MAIN_TREE_ELEMENTS(context->num_position_slots) * sizeof(context->main_tree_freq[0]));
    memset(context->secondary_tree_freq, 0, LZX_NUM_SECONDARY_LEN * sizeof(context->secondary_tree_freq[0]));
    return tally_frequency(context, start, start_at, end);
}
static void update_tree_estimates(ENCODER_CONTEXT* context)
{
    if (context->literals)
    {
        if (context->need_to_recalc_stats)
        {
            get_block_stats(context, 0, 0, context->literals);
            context->need_to_recalc_stats = false;
        }
        else
        {
            tally_frequency(context, context->last_literals, context->last_distances, context->literals);
        }

        create_trees(context, false);

        fix_tree_cost_estimates(context);

        context->last_literals = context->literals;
        context->last_distances = context->distances;
    }
}

static void tally_aligned_bits(ENCODER_CONTEXT* context, uint32_t end_at)
{
    uint32_t* dist_ptr;
    uint32_t i;
    uint32_t match_pos;

    dist_ptr = context->dist_data;

    for (i = end_at; i > 0; i--)
    {
        match_pos = *dist_ptr++;

        if (match_pos >= MPSLOT3_CUTOFF)
            context->aligned_tree_freq[match_pos & 7]++;
    }
}
static int get_aligned_stats(ENCODER_CONTEXT* context, uint32_t end_at)
{
    uint8_t i;
    uint32_t total_L3 = 0;
    uint32_t largest_L3 = 0;

    memset(context->aligned_tree_freq, 0, LZX_ALIGNED_NUM_ELEMENTS * sizeof(context->aligned_tree_freq[0]));

    tally_aligned_bits(context, end_at);

    for (i = 0; i < LZX_ALIGNED_NUM_ELEMENTS; i++)
    {
        if (context->aligned_tree_freq[i] > largest_L3)
            largest_L3 = context->aligned_tree_freq[i];

        total_L3 += context->aligned_tree_freq[i];
    }

    if ((largest_L3 > total_L3 / 5) && end_at >= 100)
        return LZX_BLOCK_TYPE_ALIGNED;
    else
        return LZX_BLOCK_TYPE_VERBATIM;
}
static uint32_t estimate_compressed_block_size(ENCODER_CONTEXT* context)
{
    uint32_t block_size = 0;
    uint32_t i;
    uint8_t mpslot;

    block_size = 150 * 8;

    for (i = 0; i < NUM_CHARS; i++)
    {
        block_size += (context->main_tree_len[i] * context->main_tree_freq[i]);
    }

    for (mpslot = 0; mpslot < context->num_position_slots; mpslot++)
    {
        long element;
        int primary;

        element = NUM_CHARS + (mpslot << NL_SHIFT);

        for (primary = 0; primary <= LZX_NUM_PRIMARY_LEN; primary++)
        {
            block_size += ((context->main_tree_len[element] + lzx_extra_bits[mpslot]) * context->main_tree_freq[element]);
            element++;
        }
    }

    for (i = 0; i < LZX_NUM_SECONDARY_LEN; i++)
    {
        block_size += (context->secondary_tree_freq[i] * context->secondary_tree_len[i]);
    }

    return (block_size + 7) >> 3;
}

static void write_rep_tree(ENCODER_CONTEXT* context, uint8_t* pLen, uint8_t* pLastLen, int Num)
{
    int i;
    int	j;
    int	same;
    uint16_t small_freq[2 * 24] = {0};
    uint16_t mini_code[24] = {0};
    char mini_len[24] = {0};

    char k;
    uint8_t temp_store;

    temp_store = pLen[Num];
    pLen[Num] = 123;

    for (i = 0; i < Num; i++)
    {
        same = 0;

        for (j = i + 1; pLen[j] == pLen[i]; j++)
            same++;

        if (same >= TREE_ENC_REP_MIN)
        {
            if (!pLen[i])
            {
                if (same > TREE_ENC_REP_MIN + TREE_ENC_REP_ZERO_FIRST + TREE_ENC_REP_ZERO_SECOND - 1)
                    same = TREE_ENC_REP_MIN + TREE_ENC_REP_ZERO_FIRST + TREE_ENC_REP_ZERO_SECOND - 1;

                if (same <= TREE_ENC_REP_MIN + TREE_ENC_REP_ZERO_FIRST - 1)
                    small_freq[17]++;
                else
                    small_freq[18]++;
            }
            else
            {
                if (same > TREE_ENC_REP_MIN + TREE_ENC_REP_SAME_FIRST - 1)
                    same = TREE_ENC_REP_MIN + TREE_ENC_REP_SAME_FIRST - 1;

                small_freq[(pLastLen[i] - pLen[i] + 17) % 17]++;

                small_freq[19]++;
            }

            i += same - 1;
        }
        else
        {
            small_freq[(pLastLen[i] - pLen[i] + 17) % 17]++;
        }
    }

    make_tree(context, 20, small_freq, (uint8_t*)mini_len, mini_code, true);

    for (i = 0; i < 20; i++)
    {
        output_bits(context, 4, mini_len[i]);
    }

    for (i = 0; i < Num; i++)
    {
        same = 0;

        for (j = i + 1; pLen[j] == pLen[i]; j++)
            same++;

        if (same >= TREE_ENC_REP_MIN)
        {
            if (!pLen[i])
            {
                if (same > TREE_ENC_REP_MIN + TREE_ENC_REP_ZERO_FIRST + TREE_ENC_REP_ZERO_SECOND - 1)
                    same = TREE_ENC_REP_MIN + TREE_ENC_REP_ZERO_FIRST + TREE_ENC_REP_ZERO_SECOND - 1;

                if (same <= TREE_ENC_REP_MIN + TREE_ENC_REP_ZERO_FIRST - 1)
                    k = 17;
                else
                    k = 18;
            }
            else
            {
                if (same > TREE_ENC_REP_MIN + TREE_ENC_REP_SAME_FIRST - 1)
                    same = TREE_ENC_REP_MIN + TREE_ENC_REP_SAME_FIRST - 1;

                k = 19;
            }
        }
        else
        {
            k = (pLastLen[i] - pLen[i] + 17) % 17;
        }

        output_bits(context, mini_len[k], mini_code[k]);

        if (k == 17)
        {
            output_bits(context, TREE_ENC_REPZ_FIRST_EXTRA_BITS, same - TREE_ENC_REP_MIN);
            i += same - 1;
        }
        else if (k == 18)
        {
            output_bits(context, TREE_ENC_REPZ_SECOND_EXTRA_BITS, same - (TREE_ENC_REP_MIN + TREE_ENC_REP_ZERO_FIRST));
            i += same - 1;
        }
        else if (k == 19)
        {
            output_bits(context, TREE_ENC_REP_SAME_EXTRA_BITS, same - TREE_ENC_REP_MIN);

            k = (pLastLen[i] - pLen[i] + 17) % 17;
            
            output_bits(context, mini_len[k], mini_code[k]);

            i += same - 1;
        }
    }

    pLen[Num] = temp_store;

    memcpy(pLastLen, pLen, Num);
}
static void encode_trees(ENCODER_CONTEXT* context)
{
    write_rep_tree(context, context->main_tree_len, context->main_tree_prev_len, NUM_CHARS);
    write_rep_tree(context, &context->main_tree_len[NUM_CHARS], &context->main_tree_prev_len[NUM_CHARS], context->num_position_slots * (LZX_NUM_PRIMARY_LEN + 1));
    write_rep_tree(context, context->secondary_tree_len, context->secondary_tree_prev_len, LZX_NUM_SECONDARY_LEN);
}
static void encode_aligned_tree(ENCODER_CONTEXT* context)
{
    int i;
    make_tree(context, LZX_ALIGNED_NUM_ELEMENTS, context->aligned_tree_freq, context->aligned_tree_len, context->aligned_tree_code, true);

    for (i = 0; i < 8; i++) {
        output_bits(context, 3, context->aligned_tree_len[i]);
    }
}

static void encode_verbatim_block(ENCODER_CONTEXT* context, uint32_t literals)
{
    uint32_t match_pos;
    uint8_t match_len;
    uint8_t mp_slot;

    uint32_t d = 0;
    uint32_t l = 0;

    while (l < literals) {
        if (!IS_MATCH(l)) {
            OUT_CHAR_BITS;
            l++;
            context->input_running_total++;
        }
        else {
            match_len = context->lit_data[l++];
            match_pos = context->dist_data[d++];

            mp_slot = (uint8_t)MP_SLOT(match_pos);

            if (match_len < LZX_NUM_PRIMARY_LEN) {
                OUTPUT_BITS(context->main_tree_len[NUM_CHARS + (mp_slot << NL_SHIFT) + match_len], context->main_tree_code[NUM_CHARS + (mp_slot << NL_SHIFT) + match_len]);
            }
            else {
                OUTPUT_BITS(context->main_tree_len[(NUM_CHARS + LZX_NUM_PRIMARY_LEN) + (mp_slot << NL_SHIFT)], context->main_tree_code[(NUM_CHARS + LZX_NUM_PRIMARY_LEN) + (mp_slot << NL_SHIFT)]);
                OUTPUT_BITS(context->secondary_tree_len[match_len - LZX_NUM_PRIMARY_LEN], context->secondary_tree_code[match_len - LZX_NUM_PRIMARY_LEN]);
            }

            if (lzx_extra_bits[mp_slot]) {
                OUTPUT_BITS(lzx_extra_bits[mp_slot], match_pos & slot_mask[mp_slot]);
            }

            context->input_running_total += (match_len + LZX_MIN_MATCH);
        }

        if (context->input_running_total == LZX_CHUNK_SIZE) {
            encode_flush(context);
            context->num_block_splits = 0;
        }
    }
}
static void encode_aligned_block(ENCODER_CONTEXT* context, uint32_t literals)
{
    uint32_t match_pos;
    uint8_t match_len;
    uint8_t mp_slot;

    uint32_t l = 0;
    uint32_t d = 0;

    uint8_t lower;

    while (l < literals) {
        if (!IS_MATCH(l)) {
            OUT_CHAR_BITS;
            l++;
            context->input_running_total++;
        }
        else {
            match_len = context->lit_data[l++];
            match_pos = context->dist_data[d++];

            mp_slot = (uint8_t)MP_SLOT(match_pos);

            if (match_len < LZX_NUM_PRIMARY_LEN) {
                OUTPUT_BITS(context->main_tree_len[NUM_CHARS + (mp_slot << NL_SHIFT) + match_len], context->main_tree_code[NUM_CHARS + (mp_slot << NL_SHIFT) + match_len]);
            }
            else {
                OUTPUT_BITS(context->main_tree_len[(NUM_CHARS + LZX_NUM_PRIMARY_LEN) + (mp_slot << NL_SHIFT)], context->main_tree_code[(NUM_CHARS + LZX_NUM_PRIMARY_LEN) + (mp_slot << NL_SHIFT)]);
                OUTPUT_BITS(context->secondary_tree_len[match_len - LZX_NUM_PRIMARY_LEN], context->secondary_tree_code[match_len - LZX_NUM_PRIMARY_LEN]);
            }

            if (lzx_extra_bits[mp_slot] >= 3) {
                if (lzx_extra_bits[mp_slot] > 3) {
                    OUTPUT_BITS(lzx_extra_bits[mp_slot] - 3, (match_pos >> 3) & ((1 << (lzx_extra_bits[mp_slot] - 3)) - 1));
                }

                lower = (uint8_t)(match_pos & 7);

                OUTPUT_BITS(context->aligned_tree_len[lower], context->aligned_tree_code[lower]);
            }
            else if (lzx_extra_bits[mp_slot]) {
                OUTPUT_BITS(lzx_extra_bits[mp_slot], match_pos & slot_mask[mp_slot]);
            }

            context->input_running_total += (match_len + LZX_MIN_MATCH);
        }

        if (context->input_running_total == LZX_CHUNK_SIZE) {
            encode_flush(context);
            context->num_block_splits = 0;
        }
    }
}
static void encode_uncompressed_block(ENCODER_CONTEXT* context, uint32_t bufpos, uint32_t block_size)
{
    int i;
    int j;
    bool block_size_odd;
    uint32_t val;

    output_bits(context, context->bitcount - 16, 0);

    for (i = 0; i < NUM_REPEATED_OFFSETS; i++) {
        val = context->repeated_offset_at_literal_zero[i];

        for (j = 0; j < sizeof(long); j++) {
            *context->output_buffer_curpos++ = (uint8_t)val;
            val >>= 8;
        }
    }

    block_size_odd = block_size & 1;

    while (block_size > 0) {
        *context->output_buffer_curpos++ = context->mem_window[bufpos];

        bufpos++;
        block_size--;
        context->input_running_total++;

        if (context->input_running_total >= LZX_CHUNK_SIZE) {
            encode_flush(context);
            context->num_block_splits = 0;
        }
    }

    if (block_size_odd) {
        *context->output_buffer_curpos++ = 0;
    }

    context->bitcount = 32;
    context->bitbuf = 0;
}

static long binary_search_findmatch(ENCODER_CONTEXT* context, long buf_pos)
{
    uint32_t ptr;
    uint32_t a, b;
    uint32_t* small_ptr, * big_ptr;
    uint32_t end_pos;
    int val;
    int bytes_to_boundary;
    int clen;
    int same;
    int match_length;
    int small_len, big_len;
    int i, best_repeated_offset;
    uint16_t tree_to_use;

    tree_to_use = *((uint16_t*)&context->mem_window[buf_pos]);
    ptr = context->tree_root[tree_to_use];
    context->tree_root[tree_to_use] = buf_pos;

    end_pos = buf_pos - (context->window_size - 4);

    if (ptr <= end_pos) {
        context->left[buf_pos] = context->right[buf_pos] = 0;
        return 0;
    }
  
    clen = 2;
    match_length = 2;
    small_len = 2;
    big_len = 2;
    context->matchpos_table[2] = buf_pos - ptr + 2;

    small_ptr = &context->left[buf_pos];
    big_ptr = &context->right[buf_pos];

    do {
        same = clen;
        a = ptr + clen;
        b = buf_pos + clen;

        while ((val = ((int)context->mem_window[a++]) - ((int)context->mem_window[b++])) == 0) {
            if (++same >= LZX_MAX_MATCH)
                goto long_match;
        }

        if (val < 0) {
            if (same > big_len) {
                if (same > match_length) {
                long_match:
                    do {
                        context->matchpos_table[++match_length] = buf_pos - ptr + (NUM_REPEATED_OFFSETS - 1);
                    } 
                    while (match_length < same);

                    if (same >= BREAK_LENGTH) {
                        *small_ptr = context->left[ptr];
                        *big_ptr = context->right[ptr];
                        goto end_bsearch;
                    }
                }

                big_len = same;
                clen = min(small_len, big_len);
            }

            *big_ptr = ptr;
            big_ptr = &context->left[ptr];
            ptr = *big_ptr;
        }
        else {
            if (same > small_len) {
                if (same > match_length) {
                    do {
                        context->matchpos_table[++match_length] = buf_pos - ptr + (NUM_REPEATED_OFFSETS - 1);
                    } 
                    while (match_length < same);

                    if (same >= BREAK_LENGTH) {
                        *small_ptr = context->left[ptr];
                        *big_ptr = context->right[ptr];
                        goto end_bsearch;
                    }
                }

                small_len = same;
                clen = min(small_len, big_len);
            }

            *small_ptr = ptr;
            small_ptr = &context->right[ptr];
            ptr = *small_ptr;
        }
    } while (ptr > end_pos);

    *small_ptr = 0;
    *big_ptr = 0;


end_bsearch:

    for (i = 0; i < match_length; i++) {
        if (context->mem_window[buf_pos + i] != context->mem_window[buf_pos - context->last_matchpos_offset[0] + i])
            break;
    }

    best_repeated_offset = i;

    if (i >= LZX_MIN_MATCH) {
        do {
            context->matchpos_table[i] = 0;
        } 
        while (--i >= LZX_MIN_MATCH);

        if (best_repeated_offset > BREAK_LENGTH)
            goto quick_return;
    }

    for (i = 0; i < match_length; i++) {
        if (context->mem_window[buf_pos + i] != context->mem_window[buf_pos - context->last_matchpos_offset[1] + i])
            break;
    }

    if (i > best_repeated_offset) {
        do {
            context->matchpos_table[++best_repeated_offset] = 1;
        } while (best_repeated_offset < i);
    }

    for (i = 0; i < match_length; i++) {
        if (context->mem_window[buf_pos + i] != context->mem_window[buf_pos - context->last_matchpos_offset[2] + i])
            break;
    }

    if (i > best_repeated_offset) {
        do {
            context->matchpos_table[++best_repeated_offset] = 2;
        } 
        while (best_repeated_offset < i);
    }

quick_return:

    bytes_to_boundary = (LZX_CHUNK_SIZE - 1) - ((int)buf_pos & (LZX_CHUNK_SIZE - 1));

    if (match_length > bytes_to_boundary) {
        match_length = bytes_to_boundary;

        if (match_length < LZX_MIN_MATCH)
            match_length = 0;
    }

    return (long)match_length;
}
static void binary_search_remove_node(ENCODER_CONTEXT* context, long buf_pos, uint32_t end_pos)
{
    uint32_t ptr;
    uint32_t left_node_pos;
    uint32_t right_node_pos;
    uint32_t* link;
    uint16_t tree_to_use;

    tree_to_use = *((uint16_t*)&context->mem_window[buf_pos]);

    if (context->tree_root[tree_to_use] != (uint32_t)buf_pos) {
        return;
    }

    link = &context->tree_root[tree_to_use];

    if (*link <= end_pos) {
        *link = 0;
        context->left[buf_pos] = context->right[buf_pos] = 0;
        return;
    }

    ptr = buf_pos;

    left_node_pos = context->left[ptr];

    if (left_node_pos <= end_pos)
        left_node_pos = context->left[ptr] = 0;

    right_node_pos = context->right[ptr];

    if (right_node_pos <= end_pos)
        right_node_pos = context->right[ptr] = 0;

    while (1) {
        if (left_node_pos > right_node_pos)
        {
            if (left_node_pos <= end_pos)
                left_node_pos = 0;

            ptr = *link = left_node_pos;

            if (!ptr)
                break;

            left_node_pos = context->right[ptr];
            link = &context->right[ptr];
        }
        else {
            if (right_node_pos <= end_pos)
                right_node_pos = 0;

            ptr = *link = right_node_pos;

            if (!ptr)
                break;

            right_node_pos = context->left[ptr];
            link = &context->left[ptr];
        }
    }
}
static void quick_insert_bsearch_findmatch(ENCODER_CONTEXT* context, long buf_pos, long end_pos)
{
    long ptr;
    uint32_t a, b;
    uint32_t* small_ptr;
    uint32_t* big_ptr;
    int val;
    int small_len;
    int big_len;
    int same;
    int clen;

    uint16_t tree_to_use;
    tree_to_use = *((uint16_t*)&context->mem_window[buf_pos]);
    ptr = context->tree_root[tree_to_use];
    context->tree_root[tree_to_use] = buf_pos;

    if (ptr <= end_pos) {
        context->left[buf_pos] = context->right[buf_pos] = 0;
        return;
    }

    clen = 2;
    small_len = 2;
    big_len = 2;

    small_ptr = &context->left[buf_pos];
    big_ptr = &context->right[buf_pos];

    do {
        same = clen;
        a = ptr + clen;
        b = buf_pos + clen;

        while ((val = ((int)context->mem_window[a++]) - ((int)context->mem_window[b++])) == 0) {
            if (++same >= BREAK_LENGTH)
                break;
        }

        if (val < 0) {
            if (same > big_len) {
                if (same >= BREAK_LENGTH)
                {
                    *small_ptr = context->left[ptr];
                    *big_ptr = context->right[ptr];
                    return;
                }

                big_len = same;
                clen = min(small_len, big_len);
            }

            *big_ptr = ptr;
            big_ptr = &context->left[ptr];
            ptr = *big_ptr;
        }
        else {
            if (same > small_len) {
                if (same >= BREAK_LENGTH) {
                    *small_ptr = context->left[ptr];
                    *big_ptr = context->right[ptr];
                    return;
                }

                small_len = same;
                clen = min(small_len, big_len);
            }

            *small_ptr = ptr;
            small_ptr = &context->right[ptr];
            ptr = *small_ptr;
        }
    } 
    while (ptr > end_pos);

    *small_ptr = 0;
    *big_ptr = 0;
}

static void get_final_repeated_offset_states(ENCODER_CONTEXT* context, uint32_t distances)
{
    uint32_t match_pos;
    long d;
    uint8_t consecutive;

    consecutive = 0;

    for (d = distances - 1; d >= 0; d--) {
        if (context->dist_data[d] > 2)
        {
            consecutive++;

            if (consecutive >= 3)
                break;
        }
        else {
            consecutive = 0;
        }
    }

    if (consecutive < 3) {
        d = 0;
    }

    for (; d < (long)distances; d++) {
        match_pos = context->dist_data[d];

        if (match_pos == 0) {
        }
        else if (match_pos <= 2) {
            uint32_t temp;
            temp = context->repeated_offset_at_literal_zero[match_pos];
            context->repeated_offset_at_literal_zero[match_pos] = context->repeated_offset_at_literal_zero[0];
            context->repeated_offset_at_literal_zero[0] = temp;
        }
        else
        {
            context->repeated_offset_at_literal_zero[2] = context->repeated_offset_at_literal_zero[1];
            context->repeated_offset_at_literal_zero[1] = context->repeated_offset_at_literal_zero[0];
            context->repeated_offset_at_literal_zero[0] = match_pos - 2;
        }
    }
}
static void do_block_output(ENCODER_CONTEXT* context, long end, long distance_to_end_at)
{
    uint32_t bytes_compressed;
    int block_type;
    uint32_t estimated_block_size;

    bytes_compressed = get_block_stats(context, 0, 0, end);

    block_type = get_aligned_stats(context, distance_to_end_at);

    create_trees(context, true);

    estimated_block_size = estimate_compressed_block_size(context);

    if (estimated_block_size >= bytes_compressed) {
        if (context->bufpos_at_last_block >= context->earliest_window_data_remaining) {
            block_type = LZX_BLOCK_TYPE_UNCOMPRESSED;
        }
    }

    output_bits(context, 3, (uint8_t)block_type);
    output_bits(context, 8, (bytes_compressed >> 16) & 255);
    output_bits(context, 8, ((bytes_compressed >> 8) & 255));
    output_bits(context, 8, (bytes_compressed & 255));

    if (block_type == LZX_BLOCK_TYPE_VERBATIM) {
        encode_trees(context);
        encode_verbatim_block(context, end);
        get_final_repeated_offset_states(context, distance_to_end_at);
    }
    else if (block_type == LZX_BLOCK_TYPE_ALIGNED) {
        encode_aligned_tree(context);
        encode_trees(context);
        encode_aligned_block(context, end);
        get_final_repeated_offset_states(context, distance_to_end_at);
    }
    else if (block_type == LZX_BLOCK_TYPE_UNCOMPRESSED) {
        get_final_repeated_offset_states(context, distance_to_end_at);
        encode_uncompressed_block(context, context->bufpos_at_last_block, bytes_compressed);
    }

    context->bufpos_at_last_block += bytes_compressed;
}
static void output_block(ENCODER_CONTEXT* context)
{
    uint32_t where_to_split;
    uint32_t distances;

    context->first_block = 0;

    split_block(context, 0, context->literals, context->distances, &where_to_split, &distances);
    do_block_output(context, where_to_split, distances);

    if (where_to_split == context->literals) {
        memset(context->item_type, 0, MAX_LITERAL_ITEMS / 8);

        context->literals = 0;
        context->distances = 0;
    }
    else {
        memmove(&context->item_type[0], &context->item_type[where_to_split / 8],
            (int)(&context->item_type[1 + (context->literals / 8)] - &context->item_type[where_to_split / 8]));

        memset(&context->item_type[1 + (context->literals - where_to_split) / 8], 0,
            (int)(&context->item_type[MAX_LITERAL_ITEMS / 8] - &context->item_type[1 + (context->literals - where_to_split) / 8]));

        memmove(&context->lit_data[0], &context->lit_data[where_to_split], context->literals - where_to_split);

        memmove(&context->dist_data[0], &context->dist_data[distances], sizeof(uint32_t) * (context->distances - distances));

        context->literals -= where_to_split;
        context->distances -= distances;
    }

    fix_tree_cost_estimates(context);
}
static void block_end(ENCODER_CONTEXT* context, long buf_pos)
{
    context->first_block = false;
    context->need_to_recalc_stats = true;

    output_block(context);

    if (context->literals < TREE_CREATE_INTERVAL) {
        context->next_tree_create = TREE_CREATE_INTERVAL;
    }
    else {
        context->next_tree_create = context->literals + TREE_CREATE_INTERVAL;
    }

    context->bufpos_last_output_block = buf_pos;
}

static bool redo_first_block(ENCODER_CONTEXT* context, uint32_t* bufpos_ptr)
{
    long start_at;
    long earliest_can_start_at;
    long pos_in_file;
    long history_needed;
    long history_avail;
    uint32_t split_at_literal;
    uint32_t buf_pos;

    context->first_block = false;

    buf_pos = *bufpos_ptr;

    pos_in_file = buf_pos - context->window_size;

    history_needed = buf_pos - context->bufpos_last_output_block;

    if (context->bufpos_last_output_block - context->window_size < context->window_size)
        history_needed += context->bufpos_last_output_block - context->window_size;
    else
        history_needed += context->window_size;

    history_avail = (long)(&context->mem_window[buf_pos] - &context->real_mem_window[0]);

    if (history_needed <= history_avail) {
        earliest_can_start_at = context->bufpos_last_output_block;
    }
    else {
        return false;
    }

    start_at = earliest_can_start_at;

    split_block(context, 0, context->literals, context->distances, &split_at_literal, NULL);

    get_block_stats(context, 0, 0, split_at_literal);

    create_trees(context, false);
    fix_tree_cost_estimates(context);

    memset(context->tree_root, 0, NUM_SEARCH_TREES * sizeof(uint32_t));

    memset(context->item_type, 0, (MAX_LITERAL_ITEMS / 8));

    context->last_matchpos_offset[0] = 1;
    context->last_matchpos_offset[1] = 1;
    context->last_matchpos_offset[2] = 1;

    context->repeated_offset_at_literal_zero[0] = 1;
    context->repeated_offset_at_literal_zero[1] = 1;
    context->repeated_offset_at_literal_zero[2] = 1;

    context->input_running_total = 0;

    context->literals = 0;
    context->distances = 0;

    context->need_to_recalc_stats = true;

    context->next_tree_create = split_at_literal;

    *bufpos_ptr = start_at;

    return true;
}
static void opt_encode_top(ENCODER_CONTEXT* context, long bytes_read)
{
    uint32_t buf_pos;
    uint32_t real_buf_pos;
    uint32_t buf_pos_end;
    uint32_t buf_pos_end_this_chunk;
    uint32_t match_pos;
    uint32_t i;
    uint32_t end_pos;
    int enc_match_len;

    buf_pos = context->bufpos;
    buf_pos_end = context->bufpos + bytes_read;

    if (context->first_time_this_group) {
        context->first_time_this_group = false;

        context->next_tree_create = 10000;

        if (context->file_size_for_translation) {
            output_bits(context, 1, 1);
            output_bits(context, 16, context->file_size_for_translation >> 16);
            output_bits(context, 16, context->file_size_for_translation & 65535);
        }
        else {
            output_bits(context, 1, 0);
        }
    }
    else {
        for (i = BREAK_LENGTH; i > 0; --i) {
            quick_insert_bsearch_findmatch(context, (buf_pos - (long)i), (buf_pos - context->window_size + 4));
        }
    }

    while (1) {
    top_of_main_loop:
        while (buf_pos < buf_pos_end) {
            buf_pos_end_this_chunk = (buf_pos + LZX_CHUNK_SIZE) & ~(LZX_CHUNK_SIZE - 1);

            if (buf_pos_end_this_chunk > buf_pos_end)
                buf_pos_end_this_chunk = buf_pos_end;

            enc_match_len = binary_search_findmatch(context, buf_pos);

            if (enc_match_len < LZX_MIN_MATCH) {
            output_literal:
                context->lit_data[context->literals++] = context->mem_window[buf_pos];
                buf_pos++;
                if (context->literals >= (MAX_LITERAL_ITEMS - 8))
                    block_end(context, buf_pos);
                continue;
            }

            if ((uint32_t)enc_match_len + buf_pos > buf_pos_end_this_chunk) {
                enc_match_len = buf_pos_end_this_chunk - buf_pos;

                if (enc_match_len < LZX_MIN_MATCH)
                    goto output_literal;
            }

            if (enc_match_len < FAST_DECISION_THRESHOLD) {
                uint32_t span, epos, bpos, NextPrevPos;
                DECISION_NODE* decision_node_ptr;
                long iterations;

                span = buf_pos + enc_match_len;
                epos = buf_pos + LOOK;
                bpos = buf_pos;

                context->decision_node[1].numbits = CHAR_EST(context->mem_window[buf_pos]);
                context->decision_node[1].path = buf_pos;

                for (i = LZX_MIN_MATCH; i <= (uint32_t)enc_match_len; ++i) {
                    MATCH_EST(i, context->matchpos_table[i], context->decision_node[i].numbits);

                    context->decision_node[i].path = buf_pos;
                    context->decision_node[i].link = context->matchpos_table[i];
                }

                context->decision_node[0].numbits = 0;
                context->decision_node[0].repeated_offset[0] = context->last_matchpos_offset[0];
                context->decision_node[0].repeated_offset[1] = context->last_matchpos_offset[1];
                context->decision_node[0].repeated_offset[2] = context->last_matchpos_offset[2];

                decision_node_ptr = &context->decision_node[-(long)bpos];

                #define rpt_offset_ptr(where,which_offset) decision_node_ptr[(where)].repeated_offset[(which_offset)]

                while (1) {
                    uint32_t est, cum_numbits;

                    buf_pos++;

                    if (decision_node_ptr[buf_pos].path != (uint32_t)(buf_pos - 1)) {
                        uint32_t LastPos = decision_node_ptr[buf_pos].path;

                        if (decision_node_ptr[buf_pos].link >= NUM_REPEATED_OFFSETS) {
                            context->last_matchpos_offset[0] = decision_node_ptr[buf_pos].link - (NUM_REPEATED_OFFSETS - 1);
                            context->last_matchpos_offset[1] = rpt_offset_ptr(LastPos, 0);
                            context->last_matchpos_offset[2] = rpt_offset_ptr(LastPos, 1);
                        }
                        else if (decision_node_ptr[buf_pos].link == 0) {
                            context->last_matchpos_offset[0] = rpt_offset_ptr(LastPos, 0);
                            context->last_matchpos_offset[1] = rpt_offset_ptr(LastPos, 1);
                            context->last_matchpos_offset[2] = rpt_offset_ptr(LastPos, 2);
                        }
                        else if (decision_node_ptr[buf_pos].link == 1) {
                            context->last_matchpos_offset[0] = rpt_offset_ptr(LastPos, 1);
                            context->last_matchpos_offset[1] = rpt_offset_ptr(LastPos, 0);
                            context->last_matchpos_offset[2] = rpt_offset_ptr(LastPos, 2);
                        }
                        else {
                            context->last_matchpos_offset[0] = rpt_offset_ptr(LastPos, 2);
                            context->last_matchpos_offset[1] = rpt_offset_ptr(LastPos, 1);
                            context->last_matchpos_offset[2] = rpt_offset_ptr(LastPos, 0);
                        }
                    }

                    rpt_offset_ptr(buf_pos, 0) = context->last_matchpos_offset[0];
                    rpt_offset_ptr(buf_pos, 1) = context->last_matchpos_offset[1];
                    rpt_offset_ptr(buf_pos, 2) = context->last_matchpos_offset[2];

                    if (span == buf_pos)
                        break;

                    enc_match_len = binary_search_findmatch(context, buf_pos);

                    if ((uint32_t)enc_match_len + buf_pos > buf_pos_end_this_chunk) {
                        enc_match_len = buf_pos_end_this_chunk - buf_pos;

                        if (enc_match_len < LZX_MIN_MATCH)
                            enc_match_len = 0;
                    }

                    if (enc_match_len > FAST_DECISION_THRESHOLD || buf_pos + (uint32_t)enc_match_len >= epos) {
                        match_pos = context->matchpos_table[enc_match_len];
                        decision_node_ptr[buf_pos + enc_match_len].link = match_pos;
                        decision_node_ptr[buf_pos + enc_match_len].path = buf_pos;

                        if (match_pos == 3 && enc_match_len > 16) {
                            quick_insert_bsearch_findmatch(context, buf_pos + 1, buf_pos - context->window_size + (1 + 4));
                        }
                        else {
                            for (i = 1; i < (uint32_t)enc_match_len; i++)
                                quick_insert_bsearch_findmatch(context, buf_pos + i, buf_pos + i - context->window_size + 4);
                        }

                        buf_pos += enc_match_len;

                        if (match_pos >= NUM_REPEATED_OFFSETS) {
                            context->last_matchpos_offset[2] = context->last_matchpos_offset[1];
                            context->last_matchpos_offset[1] = context->last_matchpos_offset[0];
                            context->last_matchpos_offset[0] = match_pos - (NUM_REPEATED_OFFSETS - 1);
                        }
                        else if (match_pos) {
                            uint32_t t = context->last_matchpos_offset[0];
                            context->last_matchpos_offset[0] = context->last_matchpos_offset[match_pos];
                            context->last_matchpos_offset[match_pos] = t;
                        }

                        break;
                    }

                    if (enc_match_len > 2 || (enc_match_len == 2 && context->matchpos_table[2] < BREAK_MAX_LENGTH_TWO_OFFSET)) {
                        if (span < (uint32_t)(buf_pos + enc_match_len)) {
                            long end;
                            long j;
                            end = min(buf_pos + enc_match_len - bpos, LOOK - 1);

                            for (j = span - bpos + 1; j <= end; j++)
                                context->decision_node[j].numbits = (uint32_t)-1;

                            span = buf_pos + enc_match_len;
                        }
                    }

                    cum_numbits = decision_node_ptr[buf_pos].numbits;
                    est = cum_numbits + CHAR_EST(context->mem_window[buf_pos]);

                    if (est < decision_node_ptr[buf_pos + 1].numbits) {
                        decision_node_ptr[buf_pos + 1].numbits = est;
                        decision_node_ptr[buf_pos + 1].path = buf_pos;
                    }

                    for (i = LZX_MIN_MATCH; i <= (uint32_t)enc_match_len; i++) {
                        MATCH_EST(i, context->matchpos_table[i], est);
                        est += cum_numbits;

                        if (est < decision_node_ptr[buf_pos + i].numbits) {
                            decision_node_ptr[buf_pos + i].numbits = est;
                            decision_node_ptr[buf_pos + i].path = buf_pos;
                            decision_node_ptr[buf_pos + i].link = context->matchpos_table[i];
                        }
                    }
                }

                iterations = 0;
                NextPrevPos = decision_node_ptr[buf_pos].path;

                do {
                    uint32_t PrevPos;
                    PrevPos = NextPrevPos;
                    NextPrevPos = decision_node_ptr[PrevPos].path;
                    decision_node_ptr[PrevPos].path = buf_pos;
                    buf_pos = PrevPos;
                    iterations++;
                } while (buf_pos != bpos);

                while (context->literals + iterations >= (MAX_LITERAL_ITEMS - 8) || context->distances + iterations >= (MAX_DIST_ITEMS - 8)) {
                    block_end(context, buf_pos);
                }

                do {
                    if (decision_node_ptr[buf_pos].path > buf_pos + 1) {
                        OUT_MATCH(decision_node_ptr[buf_pos].path - buf_pos, decision_node_ptr[decision_node_ptr[buf_pos].path].link);
                        buf_pos = decision_node_ptr[buf_pos].path;
                    }
                    else {
                        context->lit_data[context->literals++] = context->mem_window[buf_pos];
                        buf_pos++;
                    }
                } while (--iterations != 0);

                TREE_CREATE_CHECK();

                if (context->first_block && (context->literals >= (MAX_LITERAL_ITEMS - 512) || context->distances >= (MAX_DIST_ITEMS - 512))) {
                    if (redo_first_block(context, &buf_pos))
                        goto top_of_main_loop;

                    block_end(context, buf_pos);
                }
            }
            else {
                match_pos = context->matchpos_table[enc_match_len];
                if (match_pos == 3 && enc_match_len > 16) {
                    quick_insert_bsearch_findmatch(context, buf_pos + 1, buf_pos - context->window_size + 5);
                }
                else {
                    for (i = 1; i < (uint32_t)enc_match_len; i++) 
                        quick_insert_bsearch_findmatch(context, buf_pos + i, buf_pos + i - context->window_size + 4);
                }

                buf_pos += enc_match_len;
                OUT_MATCH(enc_match_len, match_pos);

                if (match_pos >= NUM_REPEATED_OFFSETS) {
                    context->last_matchpos_offset[2] = context->last_matchpos_offset[1];
                    context->last_matchpos_offset[1] = context->last_matchpos_offset[0];
                    context->last_matchpos_offset[0] = match_pos - (NUM_REPEATED_OFFSETS - 1);
                }
                else if (match_pos) {
                    uint32_t t = context->last_matchpos_offset[0];
                    context->last_matchpos_offset[0] = context->last_matchpos_offset[match_pos];
                    context->last_matchpos_offset[match_pos] = t;
                }

                if (context->literals >= (MAX_LITERAL_ITEMS - 8) || context->distances >= (MAX_DIST_ITEMS - 8))
                    block_end(context, buf_pos);

            }

        }

        context->earliest_window_data_remaining = buf_pos - context->window_size;

        if (bytes_read < LZX_CHUNK_SIZE) {
            if (context->first_block) {
                if (redo_first_block(context, &buf_pos))
                    goto top_of_main_loop;
            }
            break;
        }

        end_pos = buf_pos - (context->window_size - 4 - BREAK_LENGTH);

        for (i = 1; (i <= BREAK_LENGTH); i++)
            binary_search_remove_node(context, buf_pos - i, end_pos);

        real_buf_pos = buf_pos - (uint32_t)(context->real_mem_window - context->mem_window);

        if (real_buf_pos < context->window_size + context->encoder_second_partition_size)
            break;

        if (context->first_block) {
            if (redo_first_block(context, &buf_pos))
                goto top_of_main_loop;
        }

        memmove(&context->real_mem_window[0], &context->real_mem_window[context->encoder_second_partition_size], context->window_size);
        memmove(&context->real_left[0], &context->real_left[context->encoder_second_partition_size], sizeof(uint32_t) * context->window_size);
        memmove(&context->real_right[0], &context->real_right[context->encoder_second_partition_size], sizeof(uint32_t) * context->window_size);

        context->earliest_window_data_remaining = buf_pos - context->window_size;
        context->mem_window -= context->encoder_second_partition_size;
        context->left -= context->encoder_second_partition_size;
        context->right -= context->encoder_second_partition_size;

        break;
    }

    context->bufpos = buf_pos;
}

static void encode_flush(ENCODER_CONTEXT* context)
{
    long output_size = 0;
    LZX_BLOCK block;

    if (context->input_running_total > 0) {
        // output the bit buffer
        if (context->bitcount < 32) {
            output_bits(context, (context->bitcount - 16), 0);
        }

        output_size = (long)(context->output_buffer_curpos - context->output_buffer_start);

        if (output_size > 0) {   
            block.compressedSize = (uint16_t)(context->output_buffer_curpos - context->output_buffer_start);
            block.uncompressedSize = (uint16_t)context->input_running_total;
            
            context->output_buffer_size += sizeof(LZX_BLOCK) + block.compressedSize;
            context->output_buffer_block_count++;

            // write block header
            memcpy(context->output_buffer, &block, sizeof(LZX_BLOCK));
            context->output_buffer += sizeof(LZX_BLOCK);
            
            // write compressed data
            memcpy(context->output_buffer, context->output_buffer_start, block.compressedSize);
            context->output_buffer += block.compressedSize;

            //printf("block %d: %5d -> %d\n", context->output_buffer_block_count, block.uncompressedSize, block.compressedSize);
        }
    }

    // reset the output buffer
    context->input_running_total = 0;
    context->output_buffer_curpos = context->output_buffer_start;
    context->bitcount = 32;
    context->bitbuf = 0;
}
static void encode_start(ENCODER_CONTEXT* context)
{
    long bytesRead;
    long realBufPos;

    realBufPos = context->bufpos - (long)(context->real_mem_window - context->mem_window);
    bytesRead = read_input(context, realBufPos, LZX_CHUNK_SIZE);

    if (bytesRead > 0)
        opt_encode_top(context, bytesRead);
}
static long encode_data(ENCODER_CONTEXT* context, long input_size)
{
    context->input_ptr = context->input_buffer;
    context->input_left = input_size;
    context->file_size_for_translation = DEFAULT_FILE_XLAT_SIZE;

    encode_start(context);

    if (context->output_overflow) {
        return LZX_ERROR_BUFFER_OVERFLOW;
    }
        
    return 0;
}
static bool encode_init(ENCODER_CONTEXT* context, uint8_t* dest)
{
    context->window_size = LZX_WINDOW_SIZE;
    context->encoder_second_partition_size = SECONDARY_PARTITION_SIZE;

    context->output_buffer = dest;
    context->output_buffer_size = 0;
    context->output_buffer_block_count = 0;

    if (alloc_compress_memory(context) == false)
        return false;

    init_compression_memory(context);

    return true;
}

void encoder_flush(ENCODER_CONTEXT* context)
{
    while (context->literals > 0) {
        output_block(context);
    }

    encode_flush(context);
}

ENCODER_CONTEXT* createCompression(uint8_t* dest)
{
    ENCODER_CONTEXT* context;
    context = (ENCODER_CONTEXT*)malloc(sizeof(ENCODER_CONTEXT));
    if (context == NULL)
        return NULL;

    if (encode_init(context, dest) == false) {
        free(context);
        return NULL;
    }

    return context;
}
void destroyCompression(ENCODER_CONTEXT* context)
{
    if (context != NULL) {
        free_compress_memory(context);
        free(context);
    }
}

int compressBlock(ENCODER_CONTEXT* context, const uint8_t* src, uint32_t bytes_read)
{
    if (bytes_read > LZX_CHUNK_SIZE) {
        return LZX_ERROR_INVALID_DATA;
    }

    // copy the data to the working buffer
    memcpy(context->input_buffer, src, bytes_read);
        
    return encode_data(context, bytes_read);
}
int compressNextBlock(ENCODER_CONTEXT* context, const uint8_t** src, uint32_t bytes_read, uint32_t* bytes_remaining)
{
    int result;

    result = compressBlock(context, *src, bytes_read);

    *src += bytes_read;
    
    if (bytes_remaining != NULL) {
        *bytes_remaining -= bytes_read;
    }

    return result;
}

int lzxCompress(const uint8_t* src, const uint32_t src_size, uint8_t** dest, uint32_t* compressed_size)
{
    const uint8_t* src_ptr = NULL;
    ENCODER_CONTEXT* context = NULL;
    uint32_t bytes_read = 0;
    uint32_t bytes_remaining = 0;
    uint32_t total_compressed_size = 0;
    int result = 0;

    // Allocate a buffer if one was not provided
    if (*dest == NULL) {
        *dest = (uint8_t*)malloc(src_size);
        if (*dest == NULL) {
            result = LZX_ERROR_OUT_OF_MEMORY;
            goto Cleanup;
        }
    }

    // Create the compression context
    context = createCompression(*dest);
    if (context == NULL) {
        result = LZX_ERROR_OUT_OF_MEMORY;
        goto Cleanup;
    }

    bytes_remaining = src_size;
    src_ptr = src;
    
    encoder_flush(context);

    for (;;) {
        if ((uint32_t)(src_ptr - src) >= src_size) {
            result = 0;
			break;
		}

        bytes_read = min(LZX_CHUNK_SIZE, bytes_remaining);

        result = compressNextBlock(context, &src_ptr, bytes_read, &bytes_remaining);
        if (result != 0) {
            goto Cleanup;
        }
    }

    encoder_flush(context);

    total_compressed_size = context->output_buffer_size;

    if (compressed_size != NULL) {
        *compressed_size = total_compressed_size;
    }

Cleanup:
    
    if (context != NULL) {
        destroyCompression(context);
        context = NULL;
    }

    return result;
}
