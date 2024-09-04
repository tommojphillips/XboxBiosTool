// lzx.cpp: implements lz decompression algorithm

// std incl
#include <cstdio>
#include <memory.h>

// user incl
#include "lzx.h"
#include "type_defs.h"
#include "nt_headers.h"

#ifdef MEM_TRACKING
#include "mem_tracking.h"
#else
#include <malloc.h>
#endif

#define MAIN_TREE_ELEMENTS              (256 + (context->num_position_slots << 3))

#define MAKE_SIGNATURE(a,b,c,d)         (a + (b<<8) + (c<<16) + (d<<24))
#define LDI_SIGNATURE                   MAKE_SIGNATURE('L','D','I','C')
#define BAD_SIGNATURE                   0UL

#define max(a,b)                        (((a) > (b)) ? (a) : (b))
#define min(a,b)                        (((a) < (b)) ? (a) : (b))

#define TREE_ENC_REP_MIN                4
#define TREE_ENC_REPZ_FIRST_EXTRA_BITS  4
#define TREE_ENC_REPZ_SECOND_EXTRA_BITS 5
#define TREE_ENC_REP_ZERO_FIRST         16
#define TREE_ENC_REP_SAME_EXTRA_BITS    1
#define NUM_DECODE_SMALL	            20
#define DS_TABLE_BITS		            8
#define MAX_GROWTH                      6144
#define E8_CFDATA_FRAME_THRESHOLD       32768

#define LZX_WINDOW_SIZE                 (128*1024)
#define LZX_CHUNK_SIZE                  (32*1024)
#define LZX_WORKSPACE                   (256*1024)

// error codes
#define ERROR_NOT_ENOUGH_MEMORY         1
#define ERROR_BAD_PARAMETERS            2
#define ERROR_BUFFER_OVERFLOW           3
#define ERROR_FAILED                    4
#define ERROR_CONFIGURATION             5

#define ALIGNED_TABLE_BITS		        7
#define ALIGNED_NUM_ELEMENTS	        8
#define MAX_MAIN_TREE_ELEMENTS          672
#define MAIN_TREE_TABLE_BITS            10
#define SECONDARY_LEN_TREE_TABLE_BITS   8

#define NUM_REPEATED_OFFSETS            3
#define NUM_PRIMARY_LENGTHS             7
#define NUM_SECONDARY_LENGTHS           (256 - NUM_PRIMARY_LENGTHS)

enum BLOCK_TYPE {
    BLOCKTYPE_INVALID = 0,
    BLOCKTYPE_VERBATIM = 1,     // normal block
    BLOCKTYPE_ALIGNED = 2,      // aligned offset block
    BLOCKTYPE_UNCOMPRESSED = 3  // uncompressed block
};
enum DEC_STATE {
    DECSTATE_UNKNOWN = 0,
    DECSTATE_START_NEW_BLOCK = 1,
    DECSTATE_DECODING_DATA = 2,
};

typedef struct {
    USHORT compressedSize;
    USHORT uncompressedSize;
} LZX_BLOCK;
typedef struct {
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
    char bitcount;

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
typedef struct {
    ULONG signature;
    DEC_CONTEXT* decoder_context;
} LDI_CONTEXT;

#define FILL_BUF_NOEOFCHECK(N) { \
	dec_bitbuf <<= (N); \
	dec_bitcount -= (N); \
	if (dec_bitcount <= 0) { \
        dec_bitbuf |= ((((ULONG) *dec_input_curpos | (((ULONG) *(dec_input_curpos+1)) << 8))) << (-dec_bitcount)); \
        dec_input_curpos += 2; \
		dec_bitcount += 16; \
	} \
}
#define FILL_BUF17_NOEOFCHECK(N) { \
	dec_bitbuf <<= (N); \
	dec_bitcount -= (N); \
	if (dec_bitcount <= 0) { \
        dec_bitbuf |= ((((ULONG) *dec_input_curpos | (((ULONG) *(dec_input_curpos+1)) << 8))) << (-dec_bitcount)); \
        dec_input_curpos += 2; \
		dec_bitcount += 16; \
		if (dec_bitcount <= 0) { \
            dec_bitbuf |= ((((ULONG) *dec_input_curpos | (((ULONG) *(dec_input_curpos+1)) << 8))) << (-dec_bitcount)); \
            dec_input_curpos += 2; \
			dec_bitcount += 16; \
		} \
	} \
}

#define DECODE_ALIGNED_NOEOFCHECK(j) \
	(j) = context->aligned_table[dec_bitbuf >> (32-ALIGNED_TABLE_BITS)]; \
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
        ULONG mask = (1L << (32-1-MAIN_TREE_TABLE_BITS)); \
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
        dec_bitbuf |= ((((ULONG) *dec_input_curpos | (((ULONG) *(dec_input_curpos+1)) << 8))) << (-dec_bitcount)); \
        dec_input_curpos += 2; \
		dec_bitcount += 16; \
    } 

#define DECODE_LEN_TREE_NOEOFCHECK(matchlen) \
    matchlen = context->secondary_length_tree_table[dec_bitbuf >> (32-SECONDARY_LEN_TREE_TABLE_BITS)]; \
	if (matchlen < 0) { \
        ULONG mask = (1L << (32-1-SECONDARY_LEN_TREE_TABLE_BITS)); \
		do  { \
	 		matchlen = -matchlen; \
	 		if (dec_bitbuf & mask) \
                matchlen = context->secondary_length_tree_left_right[matchlen*2+1]; \
			else \
                matchlen = context->secondary_length_tree_left_right[matchlen*2]; \
			mask >>= 1; \
		} while (matchlen < 0);	\
	} \
    FILL_BUF_NOEOFCHECK(context->secondary_length_tree_len[matchlen]); \
	matchlen += NUM_PRIMARY_LENGTHS;

static const UCHAR dec_extra_bits[] = {
    0,0,0,0,1,1,2,2,
    3,3,4,4,5,5,6,6,
    7,7,8,8,9,9,10,10,
    11,11,12,12,13,13,14,14,
    15,15,16,16,17,17,17,17,
    17,17,17,17,17,17,17,17,
    17,17,17
};

static const long match_pos_minus2[sizeof(dec_extra_bits)] = {
    0 - 2,        1 - 2,        2 - 2,        3 - 2,        4 - 2,        6 - 2,        8 - 2,        12 - 2,
    16 - 2,       24 - 2,       32 - 2,       48 - 2,       64 - 2,       96 - 2,       128 - 2,      192 - 2,
    256 - 2,      384 - 2,      512 - 2,      768 - 2,      1024 - 2,     1536 - 2,     2048 - 2,     3072 - 2,
    4096 - 2,     6144 - 2,     8192 - 2,     12288 - 2,    16384 - 2,    24576 - 2,    32768 - 2,    49152 - 2,
    65536 - 2,    98304 - 2,    131072 - 2,   196608 - 2,   262144 - 2,   393216 - 2,   524288 - 2,   655360 - 2,
    786432 - 2,   917504 - 2,   1048576 - 2,  1179648 - 2,  1310720 - 2,  1441792 - 2,  1572864 - 2,  1703936 - 2,
    1835008 - 2,  1966080 - 2,  2097152 - 2
};

void init_decoder_bitbuf(DEC_CONTEXT* context)
{
    const UCHAR* p;

    if (context->block_type == BLOCKTYPE_UNCOMPRESSED)
        return;

    if ((context->input_curpos + sizeof(ULONG)) > context->end_input_pos)
        return;

    p = context->input_curpos;

    context->bitbuf = ((ULONG)p[2] | (((ULONG)p[3]) << 8)) | ((((ULONG)p[0] | (((ULONG)p[1]) << 8))) << 16);

    context->bitcount = 16;
    context->input_curpos += 4;
}
void fillbuf(DEC_CONTEXT* context, int n)
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

        context->bitbuf |= ((((ULONG)*context->input_curpos | (((ULONG)*(context->input_curpos + 1)) << 8))) << (-context->bitcount));
        context->input_curpos += 2;
        context->bitcount += 16;

        if (context->bitcount <= 0)
        {
            if (context->input_curpos >= context->end_input_pos)
            {
                context->error_condition = true;
                return;
            }

            context->bitbuf |= ((((ULONG)*context->input_curpos | (((ULONG)*(context->input_curpos + 1)) << 8))) << (-context->bitcount));
            context->input_curpos += 2;
            context->bitcount += 16;
        }
    }
}
void decode_small(DEC_CONTEXT* context, short& temp, short* small_table, ULONG& mask, UCHAR* small_bitlen, short* leftright_s)
{
    temp = small_table[context->bitbuf >> (32 - DS_TABLE_BITS)];
    if (temp < 0)
    {
        mask = (1L << (32 - 1 - DS_TABLE_BITS));
        do
        {
            temp = -temp;

            if (context->bitbuf & mask)
                temp = leftright_s[2 * temp + 1];
            else
                temp = leftright_s[2 * temp];
            mask >>= 1;
        } while (temp < 0);
    }
    fillbuf(context, small_bitlen[temp]);
}
UINT get_bits(DEC_CONTEXT* context, int n)
{
    UINT value;

    value = context->bitbuf >> (32 - (n));
    fillbuf(context, n);

    return value;
}
bool make_table(int nchar,const UCHAR* bitlen,UCHAR tablebits,short* table,short* leftright)
{
    UINT i;
    int ch;
    short* p;
    UINT count[17], weight[17], start[18];
    int avail;
    UINT nextcode;
    UINT k;
    UCHAR len;
    UCHAR jutbits;

    for (i = 1; i <= 16; i++)
        count[i] = 0;

    for (i = 0; i < (UINT)nchar; i++)
        count[bitlen[i]]++;

    start[1] = 0;

    for (i = 1; i <= 16; i++)
        start[i + 1] = start[i] + (count[i] << (16 - i));

    if (start[17] != 65536)
    {
        if (start[17] == 0)
        {
            memset(table, 0, sizeof(USHORT) * (1 << tablebits));
            return true;
        }
        else // bad table
        {
            return false;
        }
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
        memset(&table[i], 0, sizeof(USHORT) * ((1 << tablebits) - i));
    }

    avail = nchar;

    for (ch = 0; ch < nchar; ch++)
    {
        if ((len = bitlen[ch]) == 0)
            continue;

        nextcode = start[len] + weight[len];

        if (len <= tablebits)
        {
            if (nextcode > (UINT) (1 << tablebits)) // bad table
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

                if ((signed short)k < 0)
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
bool make_table_8bit(UCHAR bitlen[], UCHAR table[])
{
    USHORT count[17], weight[17], start[18];
    USHORT i;
    USHORT nextcode;
    UCHAR len;
    UCHAR ch;

    for (i = 1; i <= 16; i++)
        count[i] = 0;

    for (i = 0; i < 8; i++)
        count[bitlen[i]]++;

    start[1] = 0;

    for (i = 1; i <= 16; i++)
        start[i + 1] = start[i] + (count[i] << (16 - i));

    if (start[17] != 0) // bad table
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

        if (nextcode > (1 << 7)) // bad table
            return false;

        for (i = start[len]; i < nextcode; i++)
            table[i] = ch;

        start[len] = nextcode;
    }

    return true;
}
bool read_rep_tree(DEC_CONTEXT* context, int num_elements, UCHAR* lastlen, UCHAR* len)
{
    ULONG mask;
    int i;
    int consecutive;
    UCHAR small_bitlen[24];
    short small_table[1 << DS_TABLE_BITS];
    short leftright_s[2 * (2 * 24+241)];
    short temp;

    static const UCHAR moduloLookup[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
    };

    for (i = 0; i < NUM_DECODE_SMALL; i++)
    {
        small_bitlen[i] = (UCHAR)get_bits(context, 4);
    }

    if (context->error_condition)
        return false;

    if (!make_table(NUM_DECODE_SMALL, small_bitlen, DS_TABLE_BITS, small_table, leftright_s))
    {
        return false;
    }

    for (i = 0; i < num_elements; i++)
    {
        decode_small(context, temp, small_table, mask, small_bitlen, leftright_s);

        if (context->error_condition)
            return false;

        if (temp == 17) // small number of repeated zeroes
        {
            consecutive = (UCHAR)get_bits(context, TREE_ENC_REPZ_FIRST_EXTRA_BITS);
            consecutive += TREE_ENC_REP_MIN;

            if (i + consecutive >= num_elements)
                consecutive = num_elements - i;

            while (consecutive-- > 0)
                len[i++] = 0;

            i--;
        }
        else if (temp == 18) // large number of repeated zeroes
        {
            consecutive = (UCHAR)get_bits(context, TREE_ENC_REPZ_SECOND_EXTRA_BITS);
            consecutive += (TREE_ENC_REP_MIN + TREE_ENC_REP_ZERO_FIRST);

            if (i + consecutive >= num_elements)
                consecutive = num_elements - i;

            while (consecutive-- > 0)
                len[i++] = 0;

            i--;
        }
        else if (temp == 19) // small number of repeated somethings
        {
            UCHAR	value;

            consecutive = (UCHAR)get_bits(context, TREE_ENC_REP_SAME_EXTRA_BITS);
            consecutive += TREE_ENC_REP_MIN;

            if (i + consecutive >= num_elements)
                consecutive = num_elements - i;

            decode_small(context, temp, small_table, mask, small_bitlen, leftright_s);
            value = moduloLookup[(lastlen[i] - temp + 17) % 17];

            while (consecutive-- > 0)
                len[i++] = value;

            i--;
        }
        else
        {
            len[i] = moduloLookup[(lastlen[i] - temp + 17) % 17];
        }
    }

    if (context->error_condition)
        return false;
    else
        return true;
}
bool read_main_and_secondary_trees(DEC_CONTEXT* context)
{
    if (!read_rep_tree(context, 256, context->main_tree_prev_len, context->main_tree_len))
    {
        return false;
    }

    if (!read_rep_tree(context, context->num_position_slots * (NUM_PRIMARY_LENGTHS+1), &context->main_tree_prev_len[256], &context->main_tree_len[256]))
    {
        return false;
    }
    
    if (!make_table(MAIN_TREE_ELEMENTS, context->main_tree_len, MAIN_TREE_TABLE_BITS,
        context->main_tree_table, context->main_tree_left_right))
    {
        return false;
    }

    if (!read_rep_tree(context, NUM_SECONDARY_LENGTHS, context->secondary_length_tree_prev_len, context->secondary_length_tree_len))
    {
        return false;
    }

    if (!make_table(NUM_SECONDARY_LENGTHS, context->secondary_length_tree_len, SECONDARY_LEN_TREE_TABLE_BITS,
        context->secondary_length_tree_table, context->secondary_length_tree_left_right))
    {
        return false;
    }

    return true;
}
bool read_aligned_offset_tree(DEC_CONTEXT* context)
{
    int i;

    for (i = 0; i < 8; i++)
    {
        context->aligned_len[i] = (UCHAR)get_bits(context, 3);
    }

    if (context->error_condition)
        return false;

    if (!make_table_8bit(context->aligned_len, (UCHAR*)context->aligned_table))
    {
        return false;
    }

    return true;
}
void decoder_translate_e8(DEC_CONTEXT* context, UCHAR* mem, long bytes)
{
    ULONG end_instr_pos;
    UCHAR temp[6];
    UCHAR* mem_backup;

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
        ULONG absolute;
        ULONG offset;

        while (*mem++ != 0xE8)
            context->instr_pos++;

        if (context->instr_pos >= end_instr_pos)
            break;
        
        absolute = ((ULONG)mem[0]) | (((ULONG)mem[1]) << 8) | (((ULONG)mem[2]) << 16) | (((ULONG)mem[3]) << 24);

        if (absolute < context->current_file_size)
        {
            offset = absolute - context->instr_pos;
            mem[0] = (UCHAR)(offset & 255);
            mem[1] = (UCHAR)((offset >> 8) & 255);
            mem[2] = (UCHAR)((offset >> 16) & 255);
            mem[3] = (UCHAR)((offset >> 24) & 255);
        }
        else if ((ULONG)(-(long)absolute) <= context->instr_pos)
        {
            offset = absolute + context->current_file_size;
            mem[0] = (UCHAR)(offset & 255);
            mem[1] = (UCHAR)(offset >> 8) & 255;
            mem[2] = (UCHAR)(offset >> 16) & 255;
            mem[3] = (UCHAR)(offset >> 24) & 255;
        }

        mem += 4;
        context->instr_pos += 5;
    }

    context->instr_pos = end_instr_pos + 10;

    memcpy(&mem_backup[bytes - 6], temp, 6);
}
void copy_data_to_output(DEC_CONTEXT* context, long amount, const UCHAR* data)
{
    if (context->output_buffer == NULL)
        return;

    memcpy(context->output_buffer, data, amount);

    if ((context->current_file_size != 0) && (context->num_cfdata_frames < E8_CFDATA_FRAME_THRESHOLD))
    {
        decoder_translate_e8(context, context->output_buffer, amount);
    }
}

long special_decode_aligned_block(DEC_CONTEXT* context, long pos, int amount_to_decode)
{
    ULONG match_pos;
    ULONG temp_pos;
    long pos_end;
    int	match_length;
    int	c;
    ULONG dec_bitbuf;
    char dec_bitcount;
    const UCHAR* dec_input_curpos;
    const UCHAR* dec_end_input_pos;
    UCHAR* dec_mem_window;
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
            dec_mem_window[pos] = (UCHAR)c;
            dec_mem_window[context->window_size + pos] = (UCHAR)c;
            pos++;
        }
        else
        {
            if ((match_length = c & NUM_PRIMARY_LENGTHS) == NUM_PRIMARY_LENGTHS)
            {
                DECODE_LEN_TREE_NOEOFCHECK(match_length);
            }

            m = (char)(c >> 3);

            if (m > 2)
            {
                if (dec_extra_bits[m] >= 3)
                {
                    if (dec_extra_bits[m] - 3)
                    {
                        GET_BITS_NOEOFCHECK(dec_extra_bits[m] - 3, temp_pos);
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
                    if (dec_extra_bits[m])
                    {
                        GET_BITS_NOEOFCHECK(dec_extra_bits[m], match_pos);

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
long fast_decode_aligned_offset_block(DEC_CONTEXT* context, long pos, int amount_to_decode)
{
    ULONG match_pos;
    ULONG temp_pos;
    long pos_end;
    long decode_residue;
    int	match_length;
    int c;
    ULONG dec_bitbuf;
    char dec_bitcount;
    const UCHAR* dec_input_curpos;
    const UCHAR* dec_end_input_pos;
    UCHAR* dec_mem_window;
    ULONG match_ptr;
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
            dec_mem_window[pos++] = (UCHAR)c;
        }
        else
        {
            if ((match_length = c & NUM_PRIMARY_LENGTHS) == NUM_PRIMARY_LENGTHS)
            {
                DECODE_LEN_TREE_NOEOFCHECK(match_length);
            }

            m = (char)(c >> 3);

            if (m > 2)
            {
                if (dec_extra_bits[m] >= 3)
                {
                    if (dec_extra_bits[m] - 3)
                    {
                        GET_BITS_NOEOFCHECK(dec_extra_bits[m] - 3, temp_pos);
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
                    if (dec_extra_bits[m])
                    {
                        GET_BITS_NOEOFCHECK(dec_extra_bits[m], match_pos);

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
int decode_aligned_offset_block(DEC_CONTEXT* context, long pos, int amount_to_decode)
{
    if (pos < 257)
    {
        long new_pos;
        long amount_to_slowly_decode;

        amount_to_slowly_decode = min(257 - pos, amount_to_decode);

        new_pos = special_decode_aligned_block(context, pos, amount_to_slowly_decode);

        amount_to_decode -= (new_pos - pos);

        context->pos = pos = new_pos;

        if (amount_to_decode <= 0)
            return amount_to_decode;
    }

    return fast_decode_aligned_offset_block(context, pos, amount_to_decode);
}

long special_decode_verbatim_block(DEC_CONTEXT* context, long pos, int amount_to_decode)
{
    ULONG match_pos;
    long pos_end;
    int match_length;
    int c;
    ULONG dec_bitbuf;
    const UCHAR* dec_input_curpos;
    const UCHAR* dec_end_input_pos;
    UCHAR* dec_mem_window;
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
        DECODE_MAIN_TREE(c);

        if ((c -= 256) < 0)
        {
            context->mem_window[pos] = (UCHAR)c;
            context->mem_window[context->window_size + pos] = (UCHAR)c;
            pos++;
        }
        else
        {
            if ((match_length = c & NUM_PRIMARY_LENGTHS) == NUM_PRIMARY_LENGTHS)
            {
                DECODE_LEN_TREE_NOEOFCHECK(match_length);
            }

            m = (char)(c >> 3);

            if (m > 2)
            {
                if (m > 3)
                {
                    GET_BITS17_NOEOFCHECK(dec_extra_bits[m], match_pos);
                    match_pos += match_pos_minus2[m];
                }
                else
                {
                    match_pos = 1;
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
long fast_decode_verbatim_block(DEC_CONTEXT* context, long pos, int amount_to_decode)
{
    ULONG match_pos;
    ULONG match_ptr;
    long pos_end;
    long decode_residue;
    int match_length;
    int c;
    ULONG dec_bitbuf;
    const UCHAR* dec_input_curpos;
    const UCHAR* dec_end_input_pos;
    UCHAR* dec_mem_window;
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
            context->mem_window[pos++] = (UCHAR)c;
        }
        else
        {
            // get match length header
            if ((match_length = c & NUM_PRIMARY_LENGTHS) == NUM_PRIMARY_LENGTHS)
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
                    GET_BITS17_NOEOFCHECK(dec_extra_bits[m], match_pos);
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
int decode_verbatim_block(DEC_CONTEXT* context, long pos, int amount_to_decode)
{
    if (pos < 257)
    {
        long new_pos;
        long amount_to_slowly_decode;

        amount_to_slowly_decode = min(257 - pos, amount_to_decode);

        new_pos = special_decode_verbatim_block(context, pos, amount_to_slowly_decode);

        amount_to_decode -= (new_pos - pos);

        context->pos = pos = new_pos;

        if (amount_to_decode <= 0)
            return amount_to_decode;
    }

    return fast_decode_verbatim_block(context, pos, amount_to_decode);
}

int decode_uncompressed_block(DEC_CONTEXT* context, long pos, int amount_to_decode)
{
    long pos_end;
    long decode_residue;
    ULONG pos_start;
    ULONG end_copy_pos;
    const UCHAR* p;

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

long decode_data(DEC_CONTEXT* context, int bytes_to_decode)
{
    int amount_can_decode;
    int total_decoded = 0;

    while (bytes_to_decode > 0)
    {
        if (context->decoder_state == DECSTATE_START_NEW_BLOCK)
        {
            ULONG temp1;
            ULONG temp2;
            ULONG temp3;
            bool do_translation;

            if (context->first_time_this_group)
            {
                context->first_time_this_group = false;

                do_translation = (bool)get_bits(context, 1);

                if (do_translation)
                {
                    ULONG high, low;

                    high = get_bits(context, 16);
                    low = get_bits(context, 16);
                    context->current_file_size = (high << 16) | low;
                }
                else
                {
                    context->current_file_size = 0;
                }
            }

            if (context->block_type == BLOCKTYPE_UNCOMPRESSED)
            {
                if (context->original_block_size & 1)
                {
                    if (context->input_curpos < context->end_input_pos)
                        context->input_curpos++;
                }

                context->block_type = BLOCKTYPE_INVALID;

                init_decoder_bitbuf(context);
            }

            context->block_type = (BLOCK_TYPE)get_bits(context, 3);

            temp1 = get_bits(context, 8);
            temp2 = get_bits(context, 8);
            temp3 = get_bits(context, 8);

            context->block_size = context->original_block_size = (temp1 << 16) + (temp2 << 8) + (temp3);

            switch (context->block_type)
            {
                case BLOCKTYPE_ALIGNED:
                    read_aligned_offset_tree(context);
                    // fall through to VERBATIM

                case BLOCKTYPE_VERBATIM:
                    memcpy(context->main_tree_prev_len, context->main_tree_len, MAIN_TREE_ELEMENTS);
                    memcpy(context->secondary_length_tree_prev_len, context->secondary_length_tree_len, NUM_SECONDARY_LENGTHS);
                    read_main_and_secondary_trees(context);
                    break;

                case BLOCKTYPE_UNCOMPRESSED:					
                    int i;
                    context->input_curpos -= 2;
                    if (context->input_curpos + 4 >= context->end_input_pos)
                        return -1;
                    for (i = 0; i < NUM_REPEATED_OFFSETS; i++)
                    {
                        context->last_matchpos_offset[i] =
                            ((ULONG)*context->input_curpos) |
                            ((ULONG)*(context->input_curpos + 1) << 8) |
                            ((ULONG)*(context->input_curpos + 2) << 16) |
                            ((ULONG)*(context->input_curpos + 3) << 24);

                        context->input_curpos += 4;
                    }
					break;

                default:
                    return -1;
            }
            
            context->decoder_state = DECSTATE_DECODING_DATA;
        }

        while (context->block_size > 0 && bytes_to_decode > 0)
        {
            int decode_residue;

            amount_can_decode = min(context->block_size, bytes_to_decode);
            if (amount_can_decode == 0)
                return -1;

            switch (context->block_type)
            {
				case BLOCKTYPE_ALIGNED:
                    decode_residue = decode_aligned_offset_block(context, context->pos, amount_can_decode);
                    break;
				case BLOCKTYPE_VERBATIM:
                    decode_residue = decode_verbatim_block(context, context->pos, amount_can_decode);
					break;
				case BLOCKTYPE_UNCOMPRESSED:
                    decode_residue = decode_uncompressed_block(context, context->pos, amount_can_decode);
					break;
				default:
					return -1;
			}

            context->block_size -= amount_can_decode;
            bytes_to_decode -= amount_can_decode;
            total_decoded += amount_can_decode;
        }

        if (context->block_size == 0)
        {
            context->decoder_state = DECSTATE_START_NEW_BLOCK;
        }

        if (bytes_to_decode == 0)
        {
            init_decoder_bitbuf(context);
        }
    }

    long temp = context->pos;
    if (!temp)
    {
        temp = context->window_size;
	}

	copy_data_to_output(context, total_decoded, &context->mem_window[temp - total_decoded]);

    return total_decoded;
}
bool lzx_decodeInit(DEC_CONTEXT* context, long compression_window_size)
{
    context->window_size = compression_window_size;
    context->window_mask = context->window_size - 1;

    // check the window size
    if (context->window_size & context->window_mask)
        return false;

    // allocate memory for the window
    ULONG pos_start = 4;
    context->num_position_slots = 4;
    while (1)
    {
        pos_start += 1L << dec_extra_bits[context->num_position_slots];

        context->num_position_slots++;

        if (pos_start >= context->window_size)
            break;
    }
    if (!(context->mem_window = (UCHAR*)malloc(context->window_size + (257 + 4))))
        return false;

    // reset decoder state
    memset(context->main_tree_len, 0, MAIN_TREE_ELEMENTS);
    memset(context->main_tree_prev_len, 0, MAIN_TREE_ELEMENTS);
    memset(context->secondary_length_tree_len, 0, NUM_SECONDARY_LENGTHS);
    memset(context->secondary_length_tree_prev_len, 0, NUM_SECONDARY_LENGTHS);

    // init decoder state
    context->last_matchpos_offset[0] = 1;
    context->last_matchpos_offset[1] = 1;
    context->last_matchpos_offset[2] = 1;
    context->pos = 0;
    context->position_at_start = 0;
    context->decoder_state = DECSTATE_START_NEW_BLOCK;
    context->block_size = 0;
    context->block_type = BLOCKTYPE_INVALID;
    context->first_time_this_group = true;
    context->current_file_size = 0;
    context->error_condition = false;
    context->instr_pos = 0;    
    context->num_cfdata_frames = 0;

    return true;
}
int createDecompression(UINT& bufferMin, LDI_CONTEXT*& context)
{
    bufferMin = LZX_CHUNK_SIZE + MAX_GROWTH;

    context = (LDI_CONTEXT*)malloc(sizeof(LDI_CONTEXT));
    if (context == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    context->decoder_context = (DEC_CONTEXT*)malloc(sizeof(DEC_CONTEXT));
    if (context->decoder_context == NULL)
    {
        free(context);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    context->signature = LDI_SIGNATURE; // mark as valid

    if (lzx_decodeInit(context->decoder_context, LZX_WINDOW_SIZE) == false)
    {        
        free(context);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    return 0;
}
int destroyDecompression(LDI_CONTEXT* context)
{
    if (context->signature != LDI_SIGNATURE)
    {
        return ERROR_BAD_PARAMETERS;
    }
    
    if (context->decoder_context->mem_window)
    {
        free(context->decoder_context->mem_window);
        context->decoder_context->mem_window = NULL;
    }

    context->signature = BAD_SIGNATURE;

    free(context->decoder_context);
    free(context);

    return 0;
}
int decompressBlock(LDI_CONTEXT* context, const UCHAR* src, UINT srcSize, UCHAR* dest, UINT& decompressedBytes)
{
    int	result;
    UINT bytes_to_decode;

    if (context->signature != LDI_SIGNATURE) // invalid decompression signature
    {
        return ERROR_BAD_PARAMETERS;   
    }

    if (decompressedBytes > LZX_CHUNK_SIZE)
    {
        return ERROR_BUFFER_OVERFLOW; 
    }

    bytes_to_decode = decompressedBytes;

    context->decoder_context->input_curpos = src;
    context->decoder_context->end_input_pos = (src + srcSize + 4);
    context->decoder_context->output_buffer = dest;

    init_decoder_bitbuf(context->decoder_context);

    result = decode_data(context->decoder_context, bytes_to_decode);

    context->decoder_context->num_cfdata_frames++;

    if (result < 0) // error
    {
        decompressedBytes = 0;
        return 1;
    }

    decompressedBytes = result;
    context->decoder_context->position_at_start += result;
    return 0;
}

int decompressNextBlock(const UCHAR*& src, UCHAR*& dest, UINT& bytesDecompressed, UINT& bytesCompressed, LDI_CONTEXT* context)
{
    int result;
    LZX_BLOCK* block;
    
    block = (LZX_BLOCK*)src;
    src += sizeof(LZX_BLOCK);

    bytesDecompressed = block->uncompressedSize;
    bytesCompressed = block->compressedSize;
        
    result = decompressBlock(context, src, bytesCompressed, dest, bytesDecompressed);
    if (result != 0)
    {
        return result;
    }

    src += bytesCompressed;
    dest += bytesDecompressed;
    return 0;
}

int decompress(const UCHAR* data, const UINT size, UCHAR*& buff, UINT& buffSize, UINT* decompressedSize)
{
    const UCHAR* src;
    UCHAR* dest;

    LDI_CONTEXT* context = NULL;

    UINT bytesDecompressed = 0;
    UINT bytesCompressed = 0;
    UINT destSize = 0;
    UINT totalDecompressedSize = 0;

    int result = 0;
    int i = 0;

    result = createDecompression(destSize, context);
    if (result != 0)
    {
        goto Cleanup;
    }

    if (buff == NULL || buffSize == 0)
    {
        buff = (UCHAR*)malloc(LZX_CHUNK_SIZE);
        if (buff == NULL)
        {
            result = 1;
            goto Cleanup;
        }
        buffSize = LZX_CHUNK_SIZE;
    }

    src = data;
    dest = buff;
    
    for (;; i++)
    {
        if ((UINT)(src - data) >= size)
        {
            result = 0;
            break;
        }

        // Check to see if we have enough space in the output buffer
        if (dest + LZX_CHUNK_SIZE > buff + buffSize)
        {
            // resize the buffer
            UCHAR* new_buff = (UCHAR*)realloc(buff, buffSize + LZX_CHUNK_SIZE);
            if (new_buff == NULL)
            {
                result = 1;
                goto Cleanup;
            }
            buff = new_buff;
            buffSize += LZX_CHUNK_SIZE;
            dest = (buff + totalDecompressedSize);
        }

        if (decompressNextBlock(src, dest, bytesDecompressed, bytesCompressed, context) != 0)
		{
			result = 1;
			goto Cleanup;
		}
        totalDecompressedSize += bytesDecompressed;

        //printf("block %d: %5d -> %d\n", i + 1, bytesCompressed, bytesDecompressed);
    }

    if (decompressedSize != NULL)
    {
        *decompressedSize = totalDecompressedSize;
    }

    printf("Decompressed %ld bytes (%d blocks)\n", totalDecompressedSize, i);

Cleanup:

    destroyDecompression(context);
    
    if (result != 0)
    {
        printf("Error: Decompression failed. Error code: %d\n", result);       
    }

    return result;
}
UCHAR* decompressImg(const UCHAR* data, const UINT size, UINT* decompressedSize)
{
    const UCHAR* src = NULL;
    UCHAR* dest = NULL;
    UCHAR* buff = NULL;
    LDI_CONTEXT* context = NULL;

    UINT bytesDecompressed = 0;
    UINT bytesCompressed = 0;
    UINT destSize = 0;
    UINT buffSize = 0;
    UINT totalDecompressedSize = 0;

    int result = 0;
    int i = 1;

    IMAGE_NT_HEADER* nt;
    UINT ntSize;

    result = createDecompression(destSize, context);
    if (result != 0)
    {
        goto Cleanup;
    }

    buff = (UCHAR*)malloc(LZX_CHUNK_SIZE);
    if (buff == NULL)
	{
		result = 1;
		goto Cleanup;
	}
    buffSize = LZX_CHUNK_SIZE;

    src = data;
    dest = buff;

    // decompress the first block to get the nt_headers.

    result = decompressNextBlock(src, dest, bytesDecompressed, bytesCompressed, context);
    if (result != 0)
	{
		goto Cleanup;
	}

    totalDecompressedSize += bytesDecompressed;

    nt = verify_nt_headers(buff, buffSize);
    if (nt == NULL)
    {
        result = 1;
        goto Cleanup;
    }
    ntSize = nt->optional_header.imageSize;
    nt = NULL; // invalid after realloc.
        
    if (ntSize > buffSize)
    {
        UCHAR* new_buff = (UCHAR*)realloc(buff, ntSize);
        if (new_buff == NULL)
        {
            result = 1;
            goto Cleanup;
        }
        buff = new_buff;
        buffSize = ntSize;
        dest = (buff + bytesDecompressed);
    }

    for (;; i++)
    {
        if ((UINT)(src - data) >= size)
        {
            result = 0;
            break;
        }
        
        result = decompressNextBlock(src, dest, bytesDecompressed, bytesCompressed, context);
        if (result != 0)
        {
            goto Cleanup;
        }

        totalDecompressedSize += bytesDecompressed;
    }

    printf("Decompressed %ld bytes (%d blocks)\n", totalDecompressedSize, i);

    if (totalDecompressedSize != ntSize)
    {
        printf("Error: Decompressed size does not match image size\n");
        result = 1;
    }

Cleanup:

    destroyDecompression(context);

    if (result != 0)
	{
        printf("Error: Decompression failed. Error code: %d\n", result);
		free(buff);
		return NULL;
	}

    if (decompressedSize != NULL)
    {
        *decompressedSize = totalDecompressedSize;
    }
    
    return buff;
}
