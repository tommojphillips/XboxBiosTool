// lzx_decoder.c: lzx decompression algorithm

// std incl
#include <stdint.h>
#include <memory.h>
#include <malloc.h>

// user incl
#include "lzx.h"
#include "nt_headers.h"

#ifdef MEM_TRACKING
#include "mem_tracking.h"
#endif

#define MAX_MAIN_TREE_ELEMENTS 672
#define MAIN_TREE_TABLE_BITS 10
#define SECONDARY_LEN_TREE_TABLE_BITS 8

#define DEC_STATE_UNK = 0,
#define DEC_STATE_NEW_BLOCK 1
#define DEC_STATE_DECODING 2

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

#define TREE_ENC_REP_MIN 4
#define TREE_ENC_REP_FIRST_EXTRA_BITS 4
#define TREE_ENC_REP_SECOND_EXTRA_BITS 5
#define TREE_ENC_REP_ZERO_FIRST 16
#define TREE_ENC_REP_SAME_EXTRA_BITS 1

#define NUM_DECODE_SMALL 20
#define DS_TABLE_BITS 8
#define E8_CFDATA_FRAME_THRESHOLD 32768

#define FILL_BUF_NOEOFCHECK(N) { \
	dec_bitbuf <<= (N); \
	dec_bitcount -= (N); \
	if (dec_bitcount <= 0) { \
        dec_bitbuf |= ((((uint32_t) *dec_input_curpos | (((uint32_t) *(dec_input_curpos+1)) << 8))) << (-dec_bitcount)); \
        dec_input_curpos += 2; \
		dec_bitcount += 16; \
	} \
}
#define FILL_BUF17_NOEOFCHECK(N) { \
	dec_bitbuf <<= (N); \
	dec_bitcount -= (N); \
	if (dec_bitcount <= 0) { \
        dec_bitbuf |= ((((uint32_t) *dec_input_curpos | (((uint32_t) *(dec_input_curpos+1)) << 8))) << (-dec_bitcount)); \
        dec_input_curpos += 2; \
		dec_bitcount += 16; \
		if (dec_bitcount <= 0) { \
            dec_bitbuf |= ((((uint32_t) *dec_input_curpos | (((uint32_t) *(dec_input_curpos+1)) << 8))) << (-dec_bitcount)); \
            dec_input_curpos += 2; \
			dec_bitcount += 16; \
		} \
	} \
}

#define DECODE_ALIGNED_NOEOFCHECK(j) \
	(j) = context->aligned_table[dec_bitbuf >> (32 - LZX_ALIGNED_TABLE_BITS)]; \
	FILL_BUF_NOEOFCHECK(context->aligned_len[(j)]);

#define GET_BITS_NOEOFCHECK(N,DEST_VAR) { \
   DEST_VAR = dec_bitbuf >> (32-(N)); \
   FILL_BUF_NOEOFCHECK((N)); \
}

#define GET_BITS17_NOEOFCHECK(N,DEST_VAR) { \
   DEST_VAR = dec_bitbuf >> (32-(N)); \
   FILL_BUF17_NOEOFCHECK((N)); \
}

#define DECODE_MAIN_TREE(j) \
	j = context->main_tree_table[dec_bitbuf >> (32-MAIN_TREE_TABLE_BITS)]; \
	if (j < 0) { \
        uint32_t mask = (1L << (32-1-MAIN_TREE_TABLE_BITS)); \
		do { \
	 		j = -j; \
	 		if (dec_bitbuf & mask) \
                j = context->main_tree_left_right[j*2+1]; \
			else \
                j = context->main_tree_left_right[j*2]; \
			mask >>= 1; \
		} while (j < 0); \
	} \
    if (dec_input_curpos >= dec_end_input_pos) \
        return -1; \
	dec_bitbuf <<= (context->main_tree_len[j]); \
	dec_bitcount -= (context->main_tree_len[j]); \
	if (dec_bitcount <= 0) { \
        dec_bitbuf |= ((((uint32_t) *dec_input_curpos | (((uint32_t) *(dec_input_curpos+1)) << 8))) << (-dec_bitcount)); \
        dec_input_curpos += 2; \
		dec_bitcount += 16; \
    } 

#define DECODE_LEN_TREE_NOEOFCHECK(matchlen) \
    matchlen = context->secondary_len_tree_table[dec_bitbuf >> (32-SECONDARY_LEN_TREE_TABLE_BITS)]; \
	if (matchlen < 0) { \
        uint32_t mask = (1L << (32-1-SECONDARY_LEN_TREE_TABLE_BITS)); \
		do  { \
	 		matchlen = -matchlen; \
	 		if (dec_bitbuf & mask) \
                matchlen = context->secondary_len_tree_left_right[matchlen*2+1]; \
			else \
                matchlen = context->secondary_len_tree_left_right[matchlen*2]; \
			mask >>= 1; \
		} while (matchlen < 0);	\
	} \
    FILL_BUF_NOEOFCHECK(context->secondary_len_tree_len[matchlen]); \
	matchlen += LZX_NUM_PRIMARY_LEN;


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
    uint8_t* mem_window;
    uint32_t window_size;
    uint32_t window_mask;
    uint32_t last_matchpos_offset[LZX_NUM_REPEATED_OFFSETS];
    short main_tree_table[1 << MAIN_TREE_TABLE_BITS];
    short secondary_len_tree_table[1 << SECONDARY_LEN_TREE_TABLE_BITS];
    uint8_t main_tree_len[MAX_MAIN_TREE_ELEMENTS];
    uint8_t secondary_len_tree_len[LZX_NUM_SECONDARY_LEN];
    uint8_t pad1[2];
    uint8_t num_position_slots;
    char aligned_table[1 << LZX_ALIGNED_TABLE_BITS];
    uint8_t aligned_len[LZX_ALIGNED_NUM_ELEMENTS];
    short main_tree_left_right[MAX_MAIN_TREE_ELEMENTS * 4];
    short secondary_len_tree_left_right[LZX_NUM_SECONDARY_LEN * 4];
    const uint8_t* input_curpos;
    const uint8_t* end_input_pos;
    uint8_t* output_buffer;
    uint8_t* input_buffer;
    long position_at_start;
    uint8_t main_tree_prev_len[MAX_MAIN_TREE_ELEMENTS];
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

} DECODER_CONTEXT;

static const long match_pos_minus2[sizeof(lzx_extra_bits)] = {
    0 - 2,        1 - 2,        2 - 2,        3 - 2,        4 - 2,        6 - 2,        8 - 2,        12 - 2,
    16 - 2,       24 - 2,       32 - 2,       48 - 2,       64 - 2,       96 - 2,       128 - 2,      192 - 2,
    256 - 2,      384 - 2,      512 - 2,      768 - 2,      1024 - 2,     1536 - 2,     2048 - 2,     3072 - 2,
    4096 - 2,     6144 - 2,     8192 - 2,     12288 - 2,    16384 - 2,    24576 - 2,    32768 - 2,    49152 - 2,
    65536 - 2,    98304 - 2,    131072 - 2,   196608 - 2,   262144 - 2,   393216 - 2,   524288 - 2,   655360 - 2,
    786432 - 2,   917504 - 2,   1048576 - 2,  1179648 - 2,  1310720 - 2,  1441792 - 2,  1572864 - 2,  1703936 - 2,
    1835008 - 2,  1966080 - 2,  2097152 - 2
};

static void init_bitbuf(DECODER_CONTEXT* context)
{
    const uint8_t* p;

    if (context->block_type == LZX_BLOCK_TYPE_UNCOMPRESSED)
        return;

    if ((context->input_curpos + sizeof(uint32_t)) > context->end_input_pos)
        return;

    p = context->input_curpos;

    context->bitbuf = ((uint32_t)p[2] | (((uint32_t)p[3]) << 8)) | ((((uint32_t)p[0] | (((uint32_t)p[1]) << 8))) << 16);

    context->bitcount = 16;
    context->input_curpos += 4;
}
static void fill_bitbuf(DECODER_CONTEXT* context, int n)
{
    context->bitbuf <<= n;
    context->bitcount -= (char)n;

    if (context->bitcount <= 0)
    {
        if (context->input_curpos >= context->end_input_pos)
        {
            context->error_condition = true;
            return;
        }

        context->bitbuf |= ((((uint32_t)*context->input_curpos | (((uint32_t) * (context->input_curpos + 1)) << 8))) << (-context->bitcount));
        context->input_curpos += 2;
        context->bitcount += 16;

        if (context->bitcount <= 0)
        {
            if (context->input_curpos >= context->end_input_pos)
            {
                context->error_condition = true;
                return;
            }

            context->bitbuf |= ((((uint32_t)*context->input_curpos | (((uint32_t) * (context->input_curpos + 1)) << 8))) << (-context->bitcount));
            context->input_curpos += 2;
            context->bitcount += 16;
        }
    }
}

static void decode_small(DECODER_CONTEXT* context, short* temp, short* small_table, uint32_t* mask, uint8_t* small_bitlen, short* leftright_s)
{
    *temp = small_table[context->bitbuf >> (32 - DS_TABLE_BITS)];
    if (*temp < 0) {
        *mask = (1L << (32 - 1 - DS_TABLE_BITS));
        do {
            *temp = -(*temp);

            if (context->bitbuf & *mask)
                *temp = leftright_s[2 * (*temp) + 1];
            else
                *temp = leftright_s[2 * (*temp)];

            *mask >>= 1;
        } while (*temp < 0);
    }
    fill_bitbuf(context, small_bitlen[*temp]);
}

static uint32_t get_bits(DECODER_CONTEXT* context, int n)
{
    uint32_t value;

    value = context->bitbuf >> (32 - (n));
    fill_bitbuf(context, n);

    return value;
}
static void translate_e8(DECODER_CONTEXT* context, uint8_t* mem, long bytes)
{
    uint32_t end_instr_pos;
    uint8_t temp[6];
    uint8_t* mem_backup;

    if (bytes <= 6)
    {
        context->instr_pos += bytes;
        return;
    }

    mem_backup = mem;

    memcpy(temp, &mem[bytes - 6], 6); // backup last 6 bytes
    memset(&mem[bytes - 6], 0xE8, 6); // set last 6 bytes to 0xE8

    end_instr_pos = context->instr_pos + bytes - 10;

    while (1)
    {
        uint32_t absolute;
        uint32_t offset;

        while (*mem++ != 0xE8)
            context->instr_pos++;

        if (context->instr_pos >= end_instr_pos)
            break;

        absolute = ((uint32_t)mem[0]) | (((uint32_t)mem[1]) << 8) | (((uint32_t)mem[2]) << 16) | (((uint32_t)mem[3]) << 24);

        if (absolute < context->current_file_size)
        {
            offset = absolute - context->instr_pos;
            mem[0] = (uint8_t)(offset & 255);
            mem[1] = (uint8_t)((offset >> 8) & 255);
            mem[2] = (uint8_t)((offset >> 16) & 255);
            mem[3] = (uint8_t)((offset >> 24) & 255);
        }
        else if ((uint32_t)(-(long)absolute) <= context->instr_pos)
        {
            offset = absolute + context->current_file_size;
            mem[0] = (uint8_t)(offset & 255);
            mem[1] = (uint8_t)(offset >> 8) & 255;
            mem[2] = (uint8_t)(offset >> 16) & 255;
            mem[3] = (uint8_t)(offset >> 24) & 255;
        }

        mem += 4;
        context->instr_pos += 5;
    }

    context->instr_pos = end_instr_pos + 10;

    memcpy(&mem_backup[bytes - 6], temp, 6);
}

static bool make_table(int nchar, const uint8_t* bitlen, uint8_t tablebits, short* table, short* leftright)
{
    uint32_t i;
    int ch;
    short* p;
    uint32_t count[17] = {0};
    uint32_t weight[17] = {0};
    uint32_t start[18] = {0};
    int avail;
    uint32_t nextcode;
    uint32_t k;
    uint8_t len;
    uint8_t jutbits;

    for (i = 0; i < (uint32_t)nchar; i++)
        count[bitlen[i]]++;

    for (i = 1; i <= 16; i++)
        start[i + 1] = start[i] + (count[i] << (16 - i));

    if (start[17] != 65536)
    {
        if (start[17] == 0)
        {
            memset(table, 0, sizeof(uint16_t) * (1 << tablebits));
            return true;
        }

        return false;
    }

    jutbits = 16 - tablebits;

    for (i = 1; i <= tablebits; i++)
    {
        start[i] >>= jutbits;
        weight[i] = 1 << (tablebits - i);
    }

    while (i <= 16)
    {
        weight[i] = 1 << (16 - i);
        i++;
    }

    i = start[tablebits + 1] >> jutbits;

    if (i != 65536)
    {
        memset(&table[i], 0, sizeof(uint16_t) * ((1 << tablebits) - i));
    }

    avail = nchar;

    for (ch = 0; ch < nchar; ch++)
    {
        if ((len = bitlen[ch]) == 0)
            continue;

        nextcode = start[len] + weight[len];

        if (len <= tablebits)
        {
            if (nextcode > (uint32_t) (1 << tablebits))
                return false;

            for (i = start[len]; i < nextcode; i++)
                table[i] = (short)ch;

            start[len] = nextcode;
        }
        else
        {
            k = start[len];
            start[len] = nextcode;
            p = &table[k >> jutbits];

            i = len - tablebits;
            k <<= tablebits;

            do
            {
                if (*p == 0)
                {
                    leftright[avail * 2] = leftright[avail * 2 + 1] = 0;
                    *p = (short)-avail;
                    avail++;
                }

                if ((short)k < 0)
                    p = &leftright[-(*p) * 2 + 1];
                else
                    p = &leftright[-(*p) * 2];

                k <<= 1;
                i--;
            } while (i);

            *p = (short)ch;
        }
    }

    return true;
}
static bool make_table_8bit(uint8_t bitlen[], uint8_t table[])
{
    uint16_t count[17] = {0};
    uint16_t weight[17] = {0};
    uint16_t start[18] = {0};
    uint16_t i;
    uint16_t nextcode;
    uint8_t len;
    uint8_t ch;

    for (i = 0; i < 8; i++)
        count[bitlen[i]]++;

    for (i = 1; i <= 16; i++)
        start[i + 1] = start[i] + (count[i] << (16 - i));

    if (start[17] != 0)
        return false;

    for (i = 1; i <= 7; i++)
    {
        start[i] >>= 9;
        weight[i] = 1 << (7 - i);
    }

    while (i <= 16)
    {
        weight[i] = 1 << (16 - i);
        i++;
    }

    memset(table, 0, 1 << 7);

    for (ch = 0; ch < 8; ch++)
    {
        if ((len = bitlen[ch]) == 0)
            continue;

        nextcode = start[len] + weight[len];

        if (nextcode > (1 << 7))
            return false;

        for (i = start[len]; i < nextcode; i++)
            table[i] = ch;

        start[len] = nextcode;
    }

    return true;
}

static bool read_rep_tree(DECODER_CONTEXT* context, int num_elements, uint8_t* lastlen, uint8_t* len)
{
    uint32_t mask;
    int i;
    int consecutive;
    uint8_t small_bitlen[24] = {0};
    short small_table[1 << DS_TABLE_BITS];
    short leftright_s[2 * (2 * 24 + 241)];
    short temp;

    for (i = 0; i < NUM_DECODE_SMALL; i++)
    {
        small_bitlen[i] = (uint8_t)get_bits(context, 4);
    }

    if (context->error_condition)
        return false;

    if (!make_table(NUM_DECODE_SMALL, small_bitlen, DS_TABLE_BITS, small_table, leftright_s))
    {
        return false;
    }

    for (i = 0; i < num_elements; i++)
    {
        decode_small(context, &temp, small_table, &mask, small_bitlen, leftright_s);

        if (context->error_condition)
            return false;

        if (temp == 17)
        {
            consecutive = (uint8_t)get_bits(context, TREE_ENC_REP_FIRST_EXTRA_BITS);
            consecutive += TREE_ENC_REP_MIN;

            if (i + consecutive >= num_elements)
                consecutive = num_elements - i;

            while (consecutive-- > 0)
                len[i++] = 0;

            i--;
        }
        else if (temp == 18)
        {
            consecutive = (uint8_t)get_bits(context, TREE_ENC_REP_SECOND_EXTRA_BITS);
            consecutive += (TREE_ENC_REP_MIN + TREE_ENC_REP_ZERO_FIRST);

            if (i + consecutive >= num_elements)
                consecutive = num_elements - i;

            while (consecutive-- > 0)
                len[i++] = 0;

            i--;
        }
        else if (temp == 19)
        {
            uint8_t value;

            consecutive = (uint8_t)get_bits(context, TREE_ENC_REP_SAME_EXTRA_BITS);
            consecutive += TREE_ENC_REP_MIN;

            if (i + consecutive >= num_elements)
                consecutive = num_elements - i;

            decode_small(context, &temp, small_table, &mask, small_bitlen, leftright_s);

            value = (lastlen[i] - temp + 17) % 17;

            while (consecutive-- > 0)
                len[i++] = value;

            i--;
        }
        else
        {
            len[i] = (lastlen[i] - temp + 17) % 17;
        }
    }

    if (context->error_condition)
        return false;

    return true;
}
static bool read_main_and_secondary_trees(DECODER_CONTEXT* context)
{
    if (!read_rep_tree(context, 256, context->main_tree_prev_len, context->main_tree_len))
    {
        return false;
    }

    if (!read_rep_tree(context, context->num_position_slots * (LZX_NUM_PRIMARY_LEN + 1), &context->main_tree_prev_len[256], &context->main_tree_len[256]))
    {
        return false;
    }

    if (!make_table(LZX_MAIN_TREE_ELEMENTS(context->num_position_slots), context->main_tree_len, MAIN_TREE_TABLE_BITS,
        context->main_tree_table, context->main_tree_left_right))
    {
        return false;
    }

    if (!read_rep_tree(context, LZX_NUM_SECONDARY_LEN, context->secondary_len_tree_prev_len, context->secondary_len_tree_len))
    {
        return false;
    }

    if (!make_table(LZX_NUM_SECONDARY_LEN, context->secondary_len_tree_len, SECONDARY_LEN_TREE_TABLE_BITS, context->secondary_len_tree_table, context->secondary_len_tree_left_right))
    {
        return false;
    }

    return true;
}
static bool read_aligned_offset_tree(DECODER_CONTEXT* context)
{
    int i;

    for (i = 0; i < 8; i++)
    {
        context->aligned_len[i] = (uint8_t)get_bits(context, 3);
    }

    if (context->error_condition)
        return false;

    if (!make_table_8bit(context->aligned_len, (uint8_t*)context->aligned_table))
    {
        return false;
    }

    return true;
}

static void copy_data_to_output(DECODER_CONTEXT* context, long amount, const uint8_t* data)
{
    if (context->output_buffer == NULL)
        return;

    memcpy(context->output_buffer, data, amount);

    if ((context->current_file_size != 0) && (context->num_cfdata_frames < E8_CFDATA_FRAME_THRESHOLD))
    {
        translate_e8(context, context->output_buffer, amount);
    }
}

static long decode_aligned_block_special(DECODER_CONTEXT* context, long pos, int amount_to_decode)
{
    uint32_t match_pos;
    uint32_t temp_pos;
    long pos_end;
    int	match_length;
    int	c;
    uint32_t dec_bitbuf;
    char dec_bitcount;
    const uint8_t* dec_input_curpos;
    const uint8_t* dec_end_input_pos;
    uint8_t* dec_mem_window;
    char m;

    dec_bitcount = context->bitcount;
    dec_bitbuf = context->bitbuf;
    dec_input_curpos = context->input_curpos;
    dec_end_input_pos = context->end_input_pos;
    dec_mem_window = context->mem_window;

    pos_end = pos + amount_to_decode;

    while (pos < pos_end)
    {
        DECODE_MAIN_TREE(c);

        if ((c -= 256) < 0)
        {
            dec_mem_window[pos] = (uint8_t)c;
            dec_mem_window[context->window_size + pos] = (uint8_t)c;
            pos++;
        }
        else
        {
            if ((match_length = c & LZX_NUM_PRIMARY_LEN) == LZX_NUM_PRIMARY_LEN)
            {
                DECODE_LEN_TREE_NOEOFCHECK(match_length);
            }

            m = (char)(c >> 3);

            if (m > 2)
            {
                if (lzx_extra_bits[m] >= 3)
                {
                    if (lzx_extra_bits[m] - 3)
                    {
                        GET_BITS_NOEOFCHECK(lzx_extra_bits[m] - 3, temp_pos);
                    }
                    else
                    {
                        temp_pos = 0;
                    }

                    match_pos = match_pos_minus2[m] + (temp_pos << 3);

                    DECODE_ALIGNED_NOEOFCHECK(temp_pos);
                    match_pos += temp_pos;
                }
                else
                {
                    if (lzx_extra_bits[m])
                    {
                        GET_BITS_NOEOFCHECK(lzx_extra_bits[m], match_pos);

                        match_pos += match_pos_minus2[m];
                    }
                    else
                    {
                        match_pos = 1;
                    }
                }

                context->last_matchpos_offset[2] = context->last_matchpos_offset[1];
                context->last_matchpos_offset[1] = context->last_matchpos_offset[0];
                context->last_matchpos_offset[0] = match_pos;
            }
            else
            {
                match_pos = context->last_matchpos_offset[m];

                if (m)
                {
                    context->last_matchpos_offset[m] = context->last_matchpos_offset[0];
                    context->last_matchpos_offset[0] = match_pos;
                }
            }

            match_length += 2;

            do
            {
                dec_mem_window[pos] = dec_mem_window[(pos - match_pos) & context->window_mask];

                if (pos < 257)
                    dec_mem_window[context->window_size + pos] = dec_mem_window[pos];

                pos++;
            } while (--match_length > 0);
        }
    }

    context->bitcount = dec_bitcount;
    context->bitbuf = dec_bitbuf;
    context->input_curpos = dec_input_curpos;

    return pos;
}
static long decode_aligned_offset_block_fast(DECODER_CONTEXT* context, long pos, int amount_to_decode)
{
    uint32_t match_pos;
    uint32_t temp_pos;
    long pos_end;
    long decode_residue;
    int	match_length;
    int c;
    uint32_t dec_bitbuf;
    char dec_bitcount;
    const uint8_t* dec_input_curpos;
    const uint8_t* dec_end_input_pos;
    uint8_t* dec_mem_window;
    uint32_t match_ptr;
    char m;

    dec_bitcount = context->bitcount;
    dec_bitbuf = context->bitbuf;
    dec_input_curpos = context->input_curpos;
    dec_end_input_pos = context->end_input_pos;
    dec_mem_window = context->mem_window;

    pos_end = pos + amount_to_decode;

    while (pos < pos_end)
    {
        DECODE_MAIN_TREE(c);

        if ((c -= 256) < 0)
        {
            dec_mem_window[pos++] = (uint8_t)c;
        }
        else
        {
            if ((match_length = c & LZX_NUM_PRIMARY_LEN) == LZX_NUM_PRIMARY_LEN)
            {
                DECODE_LEN_TREE_NOEOFCHECK(match_length);
            }

            m = (char)(c >> 3);

            if (m > 2)
            {
                if (lzx_extra_bits[m] >= 3)
                {
                    if (lzx_extra_bits[m] - 3)
                    {
                        GET_BITS_NOEOFCHECK(lzx_extra_bits[m] - 3, temp_pos);
                    }
                    else
                    {
                        temp_pos = 0;
                    }

                    match_pos = match_pos_minus2[m] + (temp_pos << 3);

                    DECODE_ALIGNED_NOEOFCHECK(temp_pos);
                    match_pos += temp_pos;
                }
                else
                {
                    if (lzx_extra_bits[m])
                    {
                        GET_BITS_NOEOFCHECK(lzx_extra_bits[m], match_pos);

                        match_pos += match_pos_minus2[m];
                    }
                    else
                    {
                        match_pos = match_pos_minus2[m];
                    }
                }

                context->last_matchpos_offset[2] = context->last_matchpos_offset[1];
                context->last_matchpos_offset[1] = context->last_matchpos_offset[0];
                context->last_matchpos_offset[0] = match_pos;
            }
            else
            {
                match_pos = context->last_matchpos_offset[m];

                if (m)
                {
                    context->last_matchpos_offset[m] = context->last_matchpos_offset[0];
                    context->last_matchpos_offset[0] = match_pos;
                }
            }

            match_length += 2;
            match_ptr = (pos - match_pos) & context->window_mask;

            do
            {
                dec_mem_window[pos++] = dec_mem_window[match_ptr++];
            } while (--match_length > 0);
        }
    }

    context->bitcount = dec_bitcount;
    context->bitbuf = dec_bitbuf;
    context->input_curpos = dec_input_curpos;

    decode_residue = pos - pos_end;

    pos &= context->window_mask;
    context->pos = pos;

    return decode_residue;
}
static int decode_aligned_offset_block(DECODER_CONTEXT* context, long pos, int amount_to_decode)
{
    if (pos < 257)
    {
        long new_pos;
        long amount_to_slowly_decode;

        amount_to_slowly_decode = min(257 - pos, amount_to_decode);

        new_pos = decode_aligned_block_special(context, pos, amount_to_slowly_decode);

        amount_to_decode -= (new_pos - pos);

        context->pos = pos = new_pos;

        if (amount_to_decode <= 0)
            return amount_to_decode;
    }

    return decode_aligned_offset_block_fast(context, pos, amount_to_decode);
}

static long decode_verbatim_block_special(DECODER_CONTEXT* context, long pos, int amount_to_decode)
{
    uint32_t match_pos;
    long pos_end;
    int match_length;
    int c;
    uint32_t dec_bitbuf;
    const uint8_t* dec_input_curpos;
    const uint8_t* dec_end_input_pos;
    uint8_t* dec_mem_window;
    char dec_bitcount;
    char m;

    dec_bitcount = context->bitcount;
    dec_bitbuf = context->bitbuf;
    dec_input_curpos = context->input_curpos;
    dec_end_input_pos = context->end_input_pos;
    dec_mem_window = context->mem_window;

    pos_end = pos + amount_to_decode;

    while (pos < pos_end) {
        DECODE_MAIN_TREE(c);

        if ((c -= 256) < 0) {
            context->mem_window[pos] = (uint8_t)c;
            context->mem_window[context->window_size + pos] = (uint8_t)c;
            pos++;
        }
        else {
            if ((match_length = c & LZX_NUM_PRIMARY_LEN) == LZX_NUM_PRIMARY_LEN) {
                DECODE_LEN_TREE_NOEOFCHECK(match_length);
            }

            m = (char)(c >> 3);

            if (m > 2) {
                if (m > 3) {
                    GET_BITS17_NOEOFCHECK(lzx_extra_bits[m], match_pos);
                    match_pos += match_pos_minus2[m];
                }
                else {
                    match_pos = 1;
                }

                context->last_matchpos_offset[2] = context->last_matchpos_offset[1];
                context->last_matchpos_offset[1] = context->last_matchpos_offset[0];
                context->last_matchpos_offset[0] = match_pos;
            }
            else {
                match_pos = context->last_matchpos_offset[m];

                if (m) {
                    context->last_matchpos_offset[m] = context->last_matchpos_offset[0];
                    context->last_matchpos_offset[0] = match_pos;
                }
            }

            match_length += 2;

            do {
                context->mem_window[pos] = context->mem_window[(pos - match_pos) & context->window_mask];

                if (pos < 257)
                    context->mem_window[context->window_size + pos] = context->mem_window[pos];

                pos++;
            } while (--match_length > 0);
        }
    }

    context->bitcount = dec_bitcount;
    context->bitbuf = dec_bitbuf;
    context->input_curpos = dec_input_curpos;

    return pos;
}
static long decode_verbatim_block_fast(DECODER_CONTEXT* context, long pos, int amount_to_decode)
{
    uint32_t match_pos;
    uint32_t match_ptr;
    long pos_end;
    long decode_residue;
    int match_length;
    int c;
    uint32_t dec_bitbuf;
    const uint8_t* dec_input_curpos;
    const uint8_t* dec_end_input_pos;
    uint8_t* dec_mem_window;
    char dec_bitcount;
    char m;

    dec_bitcount = context->bitcount;
    dec_bitbuf = context->bitbuf;
    dec_input_curpos = context->input_curpos;
    dec_end_input_pos = context->end_input_pos;
    dec_mem_window = context->mem_window;

    pos_end = pos + amount_to_decode;

    while (pos < pos_end)
    {
        DECODE_MAIN_TREE(c); // decode an item from the main tree

        if ((c -= 256) < 0)
        {
            context->mem_window[pos++] = (uint8_t)c;
        }
        else
        {
            // get match length header
            if ((match_length = c & LZX_NUM_PRIMARY_LEN) == LZX_NUM_PRIMARY_LEN)
            {
                // get match length footer if necessary
                DECODE_LEN_TREE_NOEOFCHECK(match_length);
            }

            m = (char)(c >> 3); // get match position slot

            // read any extra bits for the match position
            if (m > 2)
            {
                if (m > 3)
                {
                    GET_BITS17_NOEOFCHECK(lzx_extra_bits[m], match_pos);
                    match_pos += match_pos_minus2[m];
                }
                else
                {
                    match_pos = match_pos_minus2[3];
                }

                context->last_matchpos_offset[2] = context->last_matchpos_offset[1];
                context->last_matchpos_offset[1] = context->last_matchpos_offset[0];
                context->last_matchpos_offset[0] = match_pos;
            }
            else
            {
                match_pos = context->last_matchpos_offset[m];

                if (m)
                {
                    context->last_matchpos_offset[m] = context->last_matchpos_offset[0];
                    context->last_matchpos_offset[0] = match_pos;
                }
            }

            match_length += 2;
            match_ptr = (pos - match_pos) & context->window_mask;

            do
            {
                context->mem_window[pos++] = context->mem_window[match_ptr++];
            } while (--match_length > 0);
        }
    }

    context->bitcount = dec_bitcount;
    context->bitbuf = dec_bitbuf;
    context->input_curpos = dec_input_curpos;

    decode_residue = pos - pos_end;

    pos &= context->window_mask;
    context->pos = pos;

    return decode_residue;
}
static int decode_verbatim_block(DECODER_CONTEXT* context, long pos, int amount_to_decode)
{
    if (pos < 257)
    {
        long new_pos;
        long amount_to_slowly_decode;

        amount_to_slowly_decode = min(257 - pos, amount_to_decode);

        new_pos = decode_verbatim_block_special(context, pos, amount_to_slowly_decode);

        amount_to_decode -= (new_pos - pos);

        context->pos = pos = new_pos;

        if (amount_to_decode <= 0)
            return amount_to_decode;
    }

    return decode_verbatim_block_fast(context, pos, amount_to_decode);
}

static int decode_uncompressed_block(DECODER_CONTEXT* context, long pos, int amount_to_decode)
{
    long pos_end;
    long decode_residue;
    uint32_t pos_start;
    uint32_t end_copy_pos;
    const uint8_t* p;

    pos_start = pos;
    pos_end = pos + amount_to_decode;

    p = context->input_curpos;

    while (pos < pos_end)
    {
        if (p >= context->end_input_pos)
            return -1; // input overflow

        context->mem_window[pos++] = *p++;
    }

    context->input_curpos = p;

    end_copy_pos = min(257, pos_end);

    while (pos_start < end_copy_pos)
    {
        context->mem_window[pos_start + context->window_size] =
            context->mem_window[pos_start];
        pos_start++;
    }

    decode_residue = pos - pos_end;

    pos &= context->window_mask;
    context->pos = pos;

    return (int)decode_residue;
}

static long decode_data(DECODER_CONTEXT* context, int bytes_to_decode)
{
    int amount_can_decode;
    int total_decoded = 0;

    while (bytes_to_decode > 0)
    {
        if (context->decoder_state == DEC_STATE_NEW_BLOCK)
        {
            uint32_t temp1;
            uint32_t temp2;
            uint32_t temp3;
            bool do_translation;

            if (context->first_time_this_group)
            {
                context->first_time_this_group = false;

                do_translation = (bool)get_bits(context, 1);

                if (do_translation)
                {
                    uint32_t high, low;

                    high = get_bits(context, 16);
                    low = get_bits(context, 16);
                    context->current_file_size = (high << 16) | low;
                }
                else
                {
                    context->current_file_size = 0;
                }
            }

            if (context->block_type == LZX_BLOCK_TYPE_UNCOMPRESSED)
            {
                if (context->original_block_size & 1)
                {
                    if (context->input_curpos < context->end_input_pos)
                        context->input_curpos++;
                }

                context->block_type = LZX_BLOCK_TYPE_INVALID;

                init_bitbuf(context);
            }

            context->block_type = get_bits(context, 3);

            temp1 = get_bits(context, 8);
            temp2 = get_bits(context, 8);
            temp3 = get_bits(context, 8);

            context->original_block_size = (temp1 << 16) + (temp2 << 8) + (temp3);
            context->block_size = context->original_block_size;

            switch (context->block_type) {
                case LZX_BLOCK_TYPE_ALIGNED:
                    read_aligned_offset_tree(context);
                    // fall through to VERBATIM

                case LZX_BLOCK_TYPE_VERBATIM:
                    memcpy(context->main_tree_prev_len, context->main_tree_len, LZX_MAIN_TREE_ELEMENTS(context->num_position_slots));
                    memcpy(context->secondary_len_tree_prev_len, context->secondary_len_tree_len, LZX_NUM_SECONDARY_LEN);
                    read_main_and_secondary_trees(context);
                    break;

                case LZX_BLOCK_TYPE_UNCOMPRESSED: {
                    int i;
                    context->input_curpos -= 2;
                    if (context->input_curpos + 4 >= context->end_input_pos)
                        return -1;
                    for (i = 0; i < LZX_NUM_REPEATED_OFFSETS; i++) {
                        context->last_matchpos_offset[i] =
                            ((uint32_t) * (context->input_curpos)) |
                            ((uint32_t) * (context->input_curpos + 1) << 8) |
                            ((uint32_t) * (context->input_curpos + 2) << 16) |
                            ((uint32_t) * (context->input_curpos + 3) << 24);

                        context->input_curpos += 4;
                    }
                } break;

                default:
                    return -1;
            }

            context->decoder_state = DEC_STATE_DECODING;
        }

        while (context->block_size > 0 && bytes_to_decode > 0) {
            int decode_residue;

            amount_can_decode = min(context->block_size, bytes_to_decode);
            if (amount_can_decode == 0)
                return -1;

            switch (context->block_type) {
                case LZX_BLOCK_TYPE_ALIGNED:
                    decode_residue = decode_aligned_offset_block(context, context->pos, amount_can_decode);
                    break;
                case LZX_BLOCK_TYPE_VERBATIM:
                    decode_residue = decode_verbatim_block(context, context->pos, amount_can_decode);
                    break;
                case LZX_BLOCK_TYPE_UNCOMPRESSED:
                    decode_residue = decode_uncompressed_block(context, context->pos, amount_can_decode);
                    break;
                default:
                    return -1;
            }

            context->block_size -= amount_can_decode;
            bytes_to_decode -= amount_can_decode;
            total_decoded += amount_can_decode;
        }

        if (context->block_size == 0) {
            context->decoder_state = DEC_STATE_NEW_BLOCK;
        }

        if (bytes_to_decode == 0) {
            init_bitbuf(context);
        }
    }

    long temp = context->pos;
    if (!temp) {
        temp = context->window_size;
    }

    copy_data_to_output(context, total_decoded, &context->mem_window[temp - total_decoded]);

    return total_decoded;
}
static bool decode_init(DECODER_CONTEXT* context)
{
    uint32_t pos_start;

    context->window_size = LZX_WINDOW_SIZE;
    context->window_mask = context->window_size - 1;

    // allocate memory for the window
    pos_start = 4;
    context->num_position_slots = 4;

    while (1)
    {
        pos_start += 1L << lzx_extra_bits[context->num_position_slots];

        context->num_position_slots++;

        if (pos_start >= context->window_size)
            break;
    }

    // alloc mem window
    context->mem_window = (uint8_t*)malloc(context->window_size + (LZX_MAX_MATCH + 4));
    if (context->mem_window == NULL)
        return false;

    // alloc input buffer
    context->input_buffer = (uint8_t*)malloc(LZX_OUTPUT_SIZE);
    if (context->input_buffer == NULL)
    {
        free(context->mem_window);
        return false;
    }

    // reset decoder state
    memset(context->main_tree_len, 0, LZX_MAIN_TREE_ELEMENTS(context->num_position_slots));
    memset(context->main_tree_prev_len, 0, LZX_MAIN_TREE_ELEMENTS(context->num_position_slots));
    memset(context->secondary_len_tree_len, 0, LZX_NUM_SECONDARY_LEN);
    memset(context->secondary_len_tree_prev_len, 0, LZX_NUM_SECONDARY_LEN);

    // init decoder state
    context->last_matchpos_offset[0] = 1;
    context->last_matchpos_offset[1] = 1;
    context->last_matchpos_offset[2] = 1;
    context->pos = 0;
    context->position_at_start = 0;
    context->decoder_state = DEC_STATE_NEW_BLOCK;
    context->block_size = 0;
    context->block_type = LZX_BLOCK_TYPE_INVALID;
    context->first_time_this_group = true;
    context->current_file_size = 0;
    context->error_condition = false;
    context->instr_pos = 0;
    context->num_cfdata_frames = 0;

    return true;
}

int check_buffer_resize(uint8_t** buffer, uint8_t** buffer_ptr, uint32_t* buffer_size, uint32_t required_size, uint32_t allocation_size)
{
    // Check to see if we have enough space in the output buffer

    uint8_t* new_buffer = NULL;
    uint32_t offset = 0;

    if (required_size == 0)
    {
        return LZX_ERROR_INVALID_DATA;
    }

    // if allocation size is not provided or less than the allocation_size, use required size
    if (allocation_size < required_size)
    {
        allocation_size = required_size;
    }

    // check buffer size
    if (*buffer_ptr + required_size > *buffer + *buffer_size)
    {
        offset = *buffer_ptr - *buffer;

        new_buffer = (uint8_t*)realloc(*buffer, *buffer_size + allocation_size);
        if (new_buffer == NULL)
        {
            return LZX_ERROR_OUT_OF_MEMORY;
        }

        // update buffer pointers.

        *buffer = new_buffer;
        *buffer_ptr = (*buffer + offset);
        *buffer_size += allocation_size;
    }

    return 0;
}

DECODER_CONTEXT* createDecompression()
{
    DECODER_CONTEXT* context;

    context = (DECODER_CONTEXT*)malloc(sizeof(DECODER_CONTEXT));
    if (context == NULL)
    {
        return NULL;
    }

    if (decode_init(context) == false)
    {
        free(context);
        context = NULL;
    }

    return context;
}
void destroyDecompression(DECODER_CONTEXT* context)
{
    if (context != NULL)
    {
        if (context->mem_window != NULL)
        {
            free(context->mem_window);
            context->mem_window = NULL;
        }

        if (context->input_buffer != NULL)
        {
            free(context->input_buffer);
            context->input_buffer = NULL;
        }

        free(context);
    }
}

int decompressBlock(DECODER_CONTEXT* context, const uint8_t* src, uint32_t bytes_compressed, uint8_t* dest, uint32_t* bytes_decompressed)
{
    uint32_t bytes_encoded;
    long bytes_decoded;

    if (bytes_compressed > LZX_OUTPUT_SIZE) {
        return LZX_ERROR_BUFFER_OVERFLOW;
    }

    if (*bytes_decompressed > LZX_CHUNK_SIZE) {
        return LZX_ERROR_BUFFER_OVERFLOW;
    }

    if (bytes_compressed > *bytes_decompressed) {
        return LZX_ERROR_INVALID_DATA;
    }

    // copy src into input buffer
    memcpy(context->input_buffer, src, bytes_compressed);

    context->input_curpos = context->input_buffer;
    context->end_input_pos = (context->input_buffer + bytes_compressed + 4);

    context->output_buffer = dest;

    init_bitbuf(context);

    bytes_encoded = *bytes_decompressed;
    bytes_decoded = decode_data(context, bytes_encoded);

    context->num_cfdata_frames++;

    if (bytes_decoded < 0)
    {
        *bytes_decompressed = 0;
        return LZX_ERROR_FAILED;
    }

    *bytes_decompressed = bytes_decoded;
    context->position_at_start += bytes_decoded;
    return 0;
}
int decompressNextBlock(DECODER_CONTEXT* context, const uint8_t** src, uint32_t* bytes_compressed, uint8_t** dest, uint32_t* bytes_decompressed)
{
    int result;
    LZX_BLOCK* block;

    block = (LZX_BLOCK*)*src;
    *src += sizeof(LZX_BLOCK);

    *bytes_decompressed = block->uncompressedSize;
    *bytes_compressed = block->compressedSize;

    result = decompressBlock(context, *src, *bytes_compressed, *dest, bytes_decompressed);

    *src += *bytes_compressed;
    *dest += *bytes_decompressed;

    return result;
}

int lzxDecompress(const uint8_t* src, const uint32_t src_size, uint8_t** dest, uint32_t* dest_size, uint32_t* decompressed_size)
{
    const uint8_t* src_ptr = NULL;
    uint8_t* dest_ptr = NULL;
    DECODER_CONTEXT* context = NULL;
    uint32_t bytes_decompressed = 0;
    uint32_t bytes_compressed = 0;
    uint32_t total_decompressed_size = 0;
    uint32_t allocated_size = 0;
    int result = 0;

    if (dest_size != NULL)
    {
        allocated_size = *dest_size;
    }

    // Allocate a buffer if one was not provided
    if (*dest == NULL)
    {
        *dest = (uint8_t*)malloc(LZX_CHUNK_SIZE);
        if (*dest == NULL)
        {
            result = LZX_ERROR_OUT_OF_MEMORY;
            goto Cleanup;
        }
        allocated_size = LZX_CHUNK_SIZE;
    }

    // Create a decompression context
    context = createDecompression();
    if (context == NULL)
    {
        result = LZX_ERROR_OUT_OF_MEMORY;
        goto Cleanup;
    }

    src_ptr = src;
    dest_ptr = *dest;

    for (;;) {
        if ((uint32_t)(src_ptr - src) >= src_size) {
            result = 0;
            break;
        }

        // realloc output buffer if needed; allocate 10 times the chunk size so we don't have to realloc too often
        result = check_buffer_resize(dest, &dest_ptr, &allocated_size, LZX_CHUNK_SIZE, LZX_CHUNK_SIZE * 10);
        if (result != 0) {
            goto Cleanup;
        }

        result = decompressNextBlock(context, &src_ptr, &bytes_compressed, &dest_ptr, &bytes_decompressed);
        if (result != 0) {
            goto Cleanup;
        }

        total_decompressed_size += bytes_decompressed;
    }

    if (decompressed_size != NULL) {
        *decompressed_size = total_decompressed_size;
    }
        
Cleanup:

    if (context != NULL) {
        destroyDecompression(context);
        context = NULL;
    }

    return result;
}
uint8_t* lzxDecompressImage(const uint8_t* src, const uint32_t src_size, uint32_t* decompressed_size)
{
    const uint8_t* src_ptr = NULL;
    uint8_t* dest_ptr = NULL;
    uint8_t* dest = NULL;
    DECODER_CONTEXT* context = NULL;
    IMAGE_NT_HEADER* nt = NULL;
    uint32_t bytes_decompressed = 0;
    uint32_t bytes_compressed = 0;
    uint32_t dest_size = 0;
    uint32_t image_size = 0;
    uint32_t total_decompressed_size = 0;
    int result = 0;

    // Allocate a buffer
    dest = (uint8_t*)malloc(LZX_CHUNK_SIZE);
    if (dest == NULL) {
        result = LZX_ERROR_OUT_OF_MEMORY;
        goto Cleanup;
    }
    dest_size = LZX_CHUNK_SIZE;

    // Create a decompression context
    context = createDecompression();
    if (context == NULL) {
        result = LZX_ERROR_OUT_OF_MEMORY;
        goto Cleanup;
    }

    src_ptr = src;
    dest_ptr = dest;

    // decompress the first block to get the nt_headers.
    result = decompressNextBlock(context, &src_ptr, &bytes_compressed, &dest_ptr, &bytes_decompressed);
    if (result != 0) {
        goto Cleanup;
    }

    total_decompressed_size += bytes_decompressed;

    nt = verify_nt_headers(dest, dest_size);
    if (nt == NULL) {
        result = 1;
        goto Cleanup;
    }

    image_size = nt->optional_header.imageSize;
    nt = NULL; // invalid after realloc.

    // realloc output buffer if needed; allocate the total pe image size
    result = check_buffer_resize(&dest, &dest_ptr, &dest_size, image_size, image_size);
    if (result != 0) {
        goto Cleanup;
    }

    for (;;) {
        if ((uint32_t)(src_ptr - src) >= src_size) {
            result = 0;
            break;
        }

        result = decompressNextBlock(context, &src_ptr, &bytes_compressed, &dest_ptr, &bytes_decompressed);
        if (result != 0) {
            goto Cleanup;
        }

        total_decompressed_size += bytes_decompressed;
    }

    if (decompressed_size != NULL)  {
        *decompressed_size = total_decompressed_size;
    }

Cleanup:

    if (context != NULL) {
        destroyDecompression(context);
        context = NULL;
    }

    if (result != 0) {
        if (dest != NULL) {
            free(dest);
            dest = NULL;
        }
    }

    return dest;
}
