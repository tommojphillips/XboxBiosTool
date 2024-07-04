// lzx.h

#ifndef XB_BIOS_TOOLS_LXZ_H
#define XB_BIOS_TOOLS_LXZ_H

#include "type_defs.h"

#define LZX_WINDOW_SIZE (128*1024)
#define LZX_CHUNK_SIZE  (32*1024)
#define LZX_WORKSPACE   (256*1024)

// error codes
#define ERROR_NOT_ENOUGH_MEMORY 1
#define ERROR_BAD_PARAMETERS    2
#define ERROR_BUFFER_OVERFLOW   3
#define ERROR_FAILED            4
#define ERROR_CONFIGURATION     5

#define ALIGNED_TABLE_BITS		7
#define ALIGNED_NUM_ELEMENTS	8
#define MAX_MAIN_TREE_ELEMENTS  672
#define MAIN_TREE_TABLE_BITS    10
#define SECONDARY_LEN_TREE_TABLE_BITS 8

#define NUM_REPEATED_OFFSETS    3
#define MIN_MATCH               2
#define MAX_MATCH               (MIN_MATCH+255)
#define NUM_PRIMARY_LENGTHS     7
#define NUM_SECONDARY_LENGTHS   ((MAX_MATCH-MIN_MATCH+1)-NUM_PRIMARY_LENGTHS)

typedef struct _LZX_BLOCK
{
    USHORT compressedSize;
    USHORT uncompressedSize;
} LZX_BLOCK;

typedef enum
{
    BLOCKTYPE_INVALID =      0,
    BLOCKTYPE_VERBATIM =     1, // normal block
    BLOCKTYPE_ALIGNED =      2, // aligned offset block
    BLOCKTYPE_UNCOMPRESSED = 3  // uncompressed block
} BLOCK_TYPE;
typedef enum
{
    DECSTATE_UNKNOWN =          0,
    DECSTATE_START_NEW_BLOCK =  1,
    DECSTATE_DECODING_DATA =    2,
} DEC_STATE;

typedef struct
{
    UCHAR* mem_window;

    ULONG window_size;
    ULONG window_mask;
    ULONG last_matchpos_offset[NUM_REPEATED_OFFSETS];

    short main_tree_table[1 << MAIN_TREE_TABLE_BITS];

    short secondary_length_tree_table[1 << SECONDARY_LEN_TREE_TABLE_BITS];

    UCHAR main_tree_len[MAX_MAIN_TREE_ELEMENTS];
    UCHAR secondary_length_tree_len[NUM_SECONDARY_LENGTHS];
    UCHAR pad1[3];

    char aligned_table[1 << ALIGNED_TABLE_BITS];
    UCHAR aligned_len[ALIGNED_NUM_ELEMENTS];

    short main_tree_left_right[MAX_MAIN_TREE_ELEMENTS * 4];

    short secondary_length_tree_left_right[NUM_SECONDARY_LENGTHS * 4];

    const UCHAR* input_curpos;
    const UCHAR* end_input_pos;

    UCHAR* output_buffer;

    long position_at_start;

    UCHAR main_tree_prev_len[MAX_MAIN_TREE_ELEMENTS];
    UCHAR secondary_length_tree_prev_len[NUM_SECONDARY_LENGTHS];

    ULONG bitbuf;
    signed char bitcount;

    UCHAR num_position_slots;

    bool first_time_this_group;
    bool error_condition;

    long pos;
    ULONG current_file_size;
    ULONG instr_pos;
    ULONG num_cfdata_frames;

    long original_block_size;
    long block_size;
    BLOCK_TYPE block_type;
    DEC_STATE decoder_state;

} DEC_CONTEXT;

struct LDI_CONTEXT
{
    ULONG signature;
    UINT blockMax;
    DEC_CONTEXT* decoder_context;
};

int decompress(const UCHAR* compressedData, const UINT compressedSize, UCHAR*& buff, UINT& buffSize, UINT& decompressedSize);

#endif