// lzx.cpp: implements lz decompression algorithm

#include "lzx.h"
#include "type_defs.h"
#include "xbmem.h"
#include "util.h"

#define MAIN_TREE_ELEMENTS              (256 + (context.num_position_slots << 3))

#define MAKE_SIGNATURE(a,b,c,d)         (a + (b<<8) + (c<<16) + (d<<24))
#define LDI_SIGNATURE                   MAKE_SIGNATURE('L','D','I','C')
#define BAD_SIGNATURE                   0UL

#define max(a,b)                        (((a) > (b)) ? (a) : (b))
#define min(a,b)                        (((a) < (b)) ? (a) : (b))

#define TREE_ENC_REP_MIN                    4
#define TREE_ENC_REPZ_FIRST_EXTRA_BITS      4
#define TREE_ENC_REPZ_SECOND_EXTRA_BITS     5
#define TREE_ENC_REP_ZERO_FIRST            16
#define TREE_ENC_REP_SAME_EXTRA_BITS        1
#define NUM_DECODE_SMALL	               20
#define DS_TABLE_BITS		                8
#define MAX_GROWTH                       6144
#define E8_CFDATA_FRAME_THRESHOLD       32768

#define FILL_BUF_FULLCHECK(N) \
{                                    		\
	if (dec_input_curpos >= dec_end_input_pos)	\
        return -1; \
	dec_bitbuf <<= (N);            			\
	dec_bitcount -= (N);                    \
	if (dec_bitcount <= 0)      			\
	{                                 		\
        dec_bitbuf |= ((((ULONG) *dec_input_curpos | (((ULONG) *(dec_input_curpos+1)) << 8))) << (-dec_bitcount)); \
        dec_input_curpos += 2;              \
		dec_bitcount += 16;               	\
    }                                       \
}
#define FILL_BUF_NOEOFCHECK(N) 			\
{                                    	\
	dec_bitbuf <<= (N);            		\
	dec_bitcount -= (N);                \
	if (dec_bitcount <= 0)      		\
	{                                 	\
        dec_bitbuf |= ((((ULONG) *dec_input_curpos | (((ULONG) *(dec_input_curpos+1)) << 8))) << (-dec_bitcount)); \
        dec_input_curpos += 2; \
		dec_bitcount += 16;				\
	}                                   \
}
#define FILL_BUF17_NOEOFCHECK(N)        \
{                                    	\
	dec_bitbuf <<= (N);            		\
	dec_bitcount -= (N);                \
	if (dec_bitcount <= 0)      		\
	{                                 	\
        dec_bitbuf |= ((((ULONG) *dec_input_curpos | (((ULONG) *(dec_input_curpos+1)) << 8))) << (-dec_bitcount)); \
        dec_input_curpos += 2; \
		dec_bitcount += 16;				\
		if (dec_bitcount <= 0) \
		{ \
            dec_bitbuf |= ((((ULONG) *dec_input_curpos | (((ULONG) *(dec_input_curpos+1)) << 8))) << (-dec_bitcount)); \
            dec_input_curpos += 2; \
			dec_bitcount += 16;         \
		} \
	}                                   \
}

#define DECODE_ALIGNED_NOEOFCHECK(j) \
	(j) = context.aligned_table[dec_bitbuf >> (32-ALIGNED_TABLE_BITS)]; \
	FILL_BUF_NOEOFCHECK(context.aligned_len[(j)]);

#define GET_BITS_NOEOFCHECK(N,DEST_VAR) \
{                                               \
   DEST_VAR = dec_bitbuf >> (32-(N));			\
   FILL_BUF_NOEOFCHECK((N));					\
}

#define GET_BITS17_NOEOFCHECK(N,DEST_VAR) \
{                                               \
   DEST_VAR = dec_bitbuf >> (32-(N));			\
   FILL_BUF17_NOEOFCHECK((N));					\
}

#define DECODE_MAIN_TREE(j) \
	j = context.main_tree_table[dec_bitbuf >> (32-MAIN_TREE_TABLE_BITS)];	\
	if (j < 0)															\
	{																	\
        ULONG mask = (1L << (32-1-MAIN_TREE_TABLE_BITS));               \
		do																\
		{																\
	 		j = -j;														\
	 		if (dec_bitbuf & mask)										\
                j = context.main_tree_left_right[j*2+1];           \
			else														\
                j = context.main_tree_left_right[j*2];             \
			mask >>= 1;													\
		} while (j < 0);												\
	}																	\
	FILL_BUF_FULLCHECK(context.main_tree_len[j]);

#define DECODE_LEN_TREE_NOEOFCHECK(matchlen) \
    matchlen = context.secondary_length_tree_table[dec_bitbuf >> (32-SECONDARY_LEN_TREE_TABLE_BITS)]; \
	if (matchlen < 0)                                                	\
	{                                                                	\
        ULONG mask = (1L << (32-1-SECONDARY_LEN_TREE_TABLE_BITS));      \
		do                                                          	\
		{																\
	 		matchlen = -matchlen;                                      	\
	 		if (dec_bitbuf & mask)                                  	\
                matchlen = context.secondary_length_tree_left_right[matchlen*2+1];\
			else                                                        \
                matchlen = context.secondary_length_tree_left_right[matchlen*2];  \
			mask >>= 1;                                                 \
		} while (matchlen < 0);											\
	}																	\
    FILL_BUF_NOEOFCHECK(context.secondary_length_tree_len[matchlen]);      \
	matchlen += NUM_PRIMARY_LENGTHS;

const UCHAR dec_extra_bits[] = {
    0,0,0,0,1,1,2,2,
    3,3,4,4,5,5,6,6,
    7,7,8,8,9,9,10,10,
    11,11,12,12,13,13,14,14,
    15,15,16,16,17,17,17,17,
    17,17,17,17,17,17,17,17,
    17,17,17
};
const long match_pos_minus2[sizeof(dec_extra_bits)] = {
    0 - 2,        1 - 2,        2 - 2,        3 - 2,        4 - 2,        6 - 2,        8 - 2,        12 - 2,
    16 - 2,       24 - 2,       32 - 2,       48 - 2,       64 - 2,       96 - 2,       128 - 2,      192 - 2,
    256 - 2,      384 - 2,      512 - 2,      768 - 2,      1024 - 2,     1536 - 2,     2048 - 2,     3072 - 2,
    4096 - 2,     6144 - 2,     8192 - 2,     12288 - 2,    16384 - 2,    24576 - 2,    32768 - 2,    49152 - 2,
    65536 - 2,    98304 - 2,    131072 - 2,   196608 - 2,   262144 - 2,   393216 - 2,   524288 - 2,   655360 - 2,
    786432 - 2,   917504 - 2,   1048576 - 2,  1179648 - 2,  1310720 - 2,  1441792 - 2,  1572864 - 2,  1703936 - 2,
    1835008 - 2,  1966080 - 2,  2097152 - 2
};

void* lzx_alloc(ULONG size)
{
    return xb_alloc(size);
}
void* lzx_realloc(void* ptr, ULONG size)
{
	return xb_realloc(ptr, size);
}
void lzx_free(void* ptr)
{
    xb_free(ptr);
}
void lzx_cpy(void* dest, const void* src, ULONG size)
{
	xb_cpy(dest, src, size);
}
void lzx_set(void* dest, int value, ULONG size)
{
	xb_set(dest, value, size);
}
void lzx_zero(void* dest, ULONG size)
{
	xb_zero(dest, size);
}

void initialise_decoder_bitbuf(DEC_CONTEXT& context)
{
    const UCHAR* p;

    if (context.block_type == BLOCKTYPE_UNCOMPRESSED)
        return;

    if ((context.input_curpos + sizeof(ULONG)) > context.end_input_pos)
        return;

    p = context.input_curpos;

    context.bitbuf = ((ULONG)p[2] | (((ULONG)p[3]) << 8)) | ((((ULONG)p[0] | (((ULONG)p[1]) << 8))) << 16);

    context.bitcount = 16;
    context.input_curpos += 4;
}

void fillbuf(DEC_CONTEXT& context, int n)
{
    context.bitbuf <<= n;
    context.bitcount -= (char)n;

    if (context.bitcount <= 0)
    {
        if (context.input_curpos >= context.end_input_pos)
        {
            context.error_condition = true;
            return;
        }

        context.bitbuf |= ((((ULONG)*context.input_curpos | (((ULONG) * (context.input_curpos + 1)) << 8))) << (-context.bitcount));
        context.input_curpos += 2;
        context.bitcount += 16;

        if (context.bitcount <= 0)
        {
            if (context.input_curpos >= context.end_input_pos)
            {
                context.error_condition = true;
                return;
            }

            context.bitbuf |= ((((ULONG)*context.input_curpos | (((ULONG) * (context.input_curpos + 1)) << 8))) << (-context.bitcount));
            context.input_curpos += 2;
            context.bitcount += 16;
        }
    }
}

void decode_small(DEC_CONTEXT& context, short& temp, short* small_table, ULONG& mask, UCHAR* small_bitlen, short* leftright_s)
{
    temp = small_table[context.bitbuf >> (32 - DS_TABLE_BITS)];
    if (temp < 0)
    {
        mask = (1L << (32 - 1 - DS_TABLE_BITS));
        do
        {
            temp = -temp;

            if (context.bitbuf & mask)
                temp = leftright_s[2 * temp + 1];
            else
                temp = leftright_s[2 * temp];
            mask >>= 1;
        } while (temp < 0);
    }
    fillbuf(context, small_bitlen[temp]);
}

ULONG getbits(DEC_CONTEXT& context, int n)
{
    ULONG value;

    value = context.bitbuf >> (32 - (n));
    fillbuf(context, n);

    return value;
}
bool handle_beginning_of_uncompressed_block(DEC_CONTEXT& context)
{
    int i;

    context.input_curpos -= 2;

    if (context.input_curpos + 4 >= context.end_input_pos)
        return false;

    for (i = 0; i < NUM_REPEATED_OFFSETS; i++)
    {
        context.last_matchpos_offset[i] =
            ((ULONG) * ((UCHAR*)context.input_curpos)) |
            ((ULONG) * (((UCHAR*)context.input_curpos) + 1) << 8) |
            ((ULONG) * (((UCHAR*)context.input_curpos) + 2) << 16) |
            ((ULONG) * (((UCHAR*)context.input_curpos) + 3) << 24);

        context.input_curpos += 4;
    }

    return true;
}
bool make_table(DEC_CONTEXT& context,int nchar,const UCHAR* bitlen,UCHAR tablebits,short* table,short* leftright)
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
            lzx_set(table, 0, sizeof(USHORT) * (1 << tablebits));
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
        lzx_set(&table[i], 0, sizeof(USHORT) * ((1 << tablebits) - i));
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
            UCHAR i;

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
bool make_table_8bit(DEC_CONTEXT& context, UCHAR bitlen[], UCHAR table[])
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

    lzx_set(table, 0, 1 << 7);

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
bool readRepTree(DEC_CONTEXT& context, int	num_elements, UCHAR* lastlen, UCHAR* len)
{
    ULONG mask;
    int i;
    int consecutive;
    UCHAR small_bitlen[24];
    short small_table[1 << DS_TABLE_BITS];
    short leftright_s[2 * (2 * 24+241)];
    short Temp;

    static const UCHAR Modulo17Lookup[] =
    {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
    };

    for (i = 0; i < NUM_DECODE_SMALL; i++)
    {
        small_bitlen[i] = (UCHAR)getbits(context, 4);
    }

    if (context.error_condition)
        return false;

    if (!make_table(context, NUM_DECODE_SMALL, small_bitlen, DS_TABLE_BITS, small_table, leftright_s))
    {
        return false;
    }

    for (i = 0; i < num_elements; i++)
    {
        decode_small(context, Temp, small_table, mask, small_bitlen, leftright_s);

        if (context.error_condition)
            return false;

        if (Temp == 17) // small number of repeated zeroes
        {
            consecutive = (UCHAR)getbits(context, TREE_ENC_REPZ_FIRST_EXTRA_BITS);
            consecutive += TREE_ENC_REP_MIN;

            if (i + consecutive >= num_elements)
                consecutive = num_elements - i;

            while (consecutive-- > 0)
                len[i++] = 0;

            i--;
        }
        else if (Temp == 18) // large number of repeated zeroes
        {
            consecutive = (UCHAR)getbits(context, TREE_ENC_REPZ_SECOND_EXTRA_BITS);
            consecutive += (TREE_ENC_REP_MIN + TREE_ENC_REP_ZERO_FIRST);

            if (i + consecutive >= num_elements)
                consecutive = num_elements - i;

            while (consecutive-- > 0)
                len[i++] = 0;

            i--;
        }
        else if (Temp == 19) // small number of repeated somethings
        {
            UCHAR	value;

            consecutive = (UCHAR)getbits(context, TREE_ENC_REP_SAME_EXTRA_BITS);
            consecutive += TREE_ENC_REP_MIN;

            if (i + consecutive >= num_elements)
                consecutive = num_elements - i;

            decode_small(context, Temp, small_table, mask, small_bitlen, leftright_s);
            value = Modulo17Lookup[(lastlen[i] - Temp) + 17];

            while (consecutive-- > 0)
                len[i++] = value;

            i--;
        }
        else
        {
            len[i] = Modulo17Lookup[(lastlen[i] - Temp) + 17];
        }
    }

    if (context.error_condition)
        return false;
    else
        return true;
}
bool read_main_and_secondary_trees(DEC_CONTEXT& context)
{
    if (!readRepTree(context, 256, context.main_tree_prev_len, context.main_tree_len))
    {
        return false;
    }

    if (!readRepTree(context, context.num_position_slots * (NUM_PRIMARY_LENGTHS+1), &context.main_tree_prev_len[256], &context.main_tree_len[256]))
    {
        return false;
    }
    
    if (!make_table(context, MAIN_TREE_ELEMENTS, context.main_tree_len, MAIN_TREE_TABLE_BITS,
        context.main_tree_table, context.main_tree_left_right))
    {
        return false;
    }

    if (!readRepTree(context, NUM_SECONDARY_LENGTHS, context.secondary_length_tree_prev_len, context.secondary_length_tree_len))
    {
        return false;
    }

    if (!make_table(context, NUM_SECONDARY_LENGTHS, context.secondary_length_tree_len, SECONDARY_LEN_TREE_TABLE_BITS,
        context.secondary_length_tree_table, context.secondary_length_tree_left_right))
    {
        return false;
    }

    return true;
}
bool read_aligned_offset_tree(DEC_CONTEXT& context)
{
    int i;

    for (i = 0; i < 8; i++)
    {
        context.aligned_len[i] = (UCHAR)getbits(context, 3);
    }

    if (context.error_condition)
        return false;

    if (!make_table_8bit(context, context.aligned_len, (UCHAR*)context.aligned_table))
    {
        return false;
    }

    return true;
}
void decoder_translate_e8(DEC_CONTEXT& context, UCHAR* mem, long bytes)
{
    ULONG   end_instr_pos;
    UCHAR    temp[6];
    UCHAR* mem_backup;

    if (bytes <= 6)
    {
        context.instr_pos += bytes;
        return;
    }

    mem_backup = mem;

    lzx_cpy(temp, &mem[bytes - 6], 6); // backup last 6 bytes
    lzx_set(&mem[bytes - 6], 0xE8, 6); // set last 6 bytes to 0xE8

    end_instr_pos = context.instr_pos + bytes - 10;

    while (1)
    {
        ULONG absolute;
        ULONG offset;

        while (*mem++ != 0xE8)
            context.instr_pos++;

        if (context.instr_pos >= end_instr_pos)
            break;
        
        absolute = ((ULONG)mem[0]) | (((ULONG)mem[1]) << 8) | (((ULONG)mem[2]) << 16) | (((ULONG)mem[3]) << 24);

        if (absolute < context.current_file_size)
        {
            offset = absolute - context.instr_pos;
            mem[0] = (UCHAR)(offset & 255);
            mem[1] = (UCHAR)((offset >> 8) & 255);
            mem[2] = (UCHAR)((offset >> 16) & 255);
            mem[3] = (UCHAR)((offset >> 24) & 255);
        }
        else if ((ULONG)(-(long)absolute) <= context.instr_pos)
        {
            offset = absolute + context.current_file_size;
            mem[0] = (UCHAR)(offset & 255);
            mem[1] = (UCHAR)(offset >> 8) & 255;
            mem[2] = (UCHAR)(offset >> 16) & 255;
            mem[3] = (UCHAR)(offset >> 24) & 255;
        }

        mem += 4;
        context.instr_pos += 5;
    }

    context.instr_pos = end_instr_pos + 10;

    lzx_cpy(&mem_backup[bytes - 6], temp, 6);
}
void copy_data_to_output(DEC_CONTEXT& context, long amount, const UCHAR* data)
{
    if (context.output_buffer == NULL)
        return;

    lzx_cpy(context.output_buffer, data, amount);

    if ((context.current_file_size != 0) && (context.num_cfdata_frames < E8_CFDATA_FRAME_THRESHOLD))
    {
        decoder_translate_e8(context, context.output_buffer, amount);
    }
}
long special_decode_aligned_block(DEC_CONTEXT& context, long pos, int amount_to_decode)
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

    dec_bitcount = context.bitcount;
    dec_bitbuf = context.bitbuf;
    dec_input_curpos = context.input_curpos;
    dec_end_input_pos = context.end_input_pos;
    dec_mem_window = context.mem_window;

    pos_end = pos + amount_to_decode;

    while (pos < pos_end)
    {
        DECODE_MAIN_TREE(c);

        if ((c -= 256) < 0)
        {
            dec_mem_window[pos] = (UCHAR)c;
            dec_mem_window[context.window_size + pos] = (UCHAR)c;
            pos++;
        }
        else
        {
            if ((match_length = c & NUM_PRIMARY_LENGTHS) == NUM_PRIMARY_LENGTHS)
            {
                DECODE_LEN_TREE_NOEOFCHECK(match_length);
            }

            m = c >> 3;

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

                context.last_matchpos_offset[2] = context.last_matchpos_offset[1];
                context.last_matchpos_offset[1] = context.last_matchpos_offset[0];
                context.last_matchpos_offset[0] = match_pos;
            }
            else
            {
                match_pos = context.last_matchpos_offset[m];

                if (m)
                {
                    context.last_matchpos_offset[m] = context.last_matchpos_offset[0];
                    context.last_matchpos_offset[0] = match_pos;
                }
            }

            match_length += MIN_MATCH;

            do
            {
                dec_mem_window[pos] = dec_mem_window[(pos - match_pos) & context.window_mask];

                if (pos < MAX_MATCH)
                    dec_mem_window[context.window_size + pos] = dec_mem_window[pos];

                pos++;
            } while (--match_length > 0);
        }
    }

    context.bitcount = dec_bitcount;
    context.bitbuf = dec_bitbuf;
    context.input_curpos = dec_input_curpos;

    return pos;
}
long fast_decode_aligned_offset_block(DEC_CONTEXT& context, long pos, int amount_to_decode)
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

    dec_bitcount = context.bitcount;
    dec_bitbuf = context.bitbuf;
    dec_input_curpos = context.input_curpos;
    dec_end_input_pos = context.end_input_pos;
    dec_mem_window = context.mem_window;

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

            m = c >> 3;

            if (m > 2)
            {
                if (dec_extra_bits[m] >= 3)
                {
                    if (dec_extra_bits[m] - 3)
                    {
                        /* no need to getbits17 */
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

                context.last_matchpos_offset[2] = context.last_matchpos_offset[1];
                context.last_matchpos_offset[1] = context.last_matchpos_offset[0];
                context.last_matchpos_offset[0] = match_pos;
            }
            else
            {
                match_pos = context.last_matchpos_offset[m];

                if (m)
                {
                    context.last_matchpos_offset[m] = context.last_matchpos_offset[0];
                    context.last_matchpos_offset[0] = match_pos;
                }
            }

            match_length += MIN_MATCH;
            match_ptr = (pos - match_pos) & context.window_mask;

            do
            {
                dec_mem_window[pos++] = dec_mem_window[match_ptr++];
            } while (--match_length > 0);
        }
    }

    context.bitcount = dec_bitcount;
    context.bitbuf = dec_bitbuf;
    context.input_curpos = dec_input_curpos;

    decode_residue = pos - pos_end;

    pos &= context.window_mask;
    context.pos = pos;

    return decode_residue;
}
int decode_aligned_offset_block(DEC_CONTEXT& context, long pos, int amount_to_decode)
{
    if (pos < MAX_MATCH)
    {
        long new_pos;
        long amount_to_slowly_decode;

        amount_to_slowly_decode = min(MAX_MATCH - pos, amount_to_decode);

        new_pos = special_decode_aligned_block(context, pos, amount_to_slowly_decode);

        amount_to_decode -= (new_pos - pos);

        context.pos = pos = new_pos;

        if (amount_to_decode <= 0)
            return amount_to_decode;
    }

    return fast_decode_aligned_offset_block(context, pos, amount_to_decode);
}
long special_decode_verbatim_block(DEC_CONTEXT& context, long pos, int amount_to_decode)
{
    ULONG	match_pos;
    long    pos_end;
    int		match_length;
    int		c;
    ULONG	dec_bitbuf;
    const UCHAR* dec_input_curpos;
    const UCHAR* dec_end_input_pos;
    UCHAR* dec_mem_window;
    char	dec_bitcount;
    char	m;

    dec_bitcount = context.bitcount;
    dec_bitbuf = context.bitbuf;
    dec_input_curpos = context.input_curpos;
    dec_end_input_pos = context.end_input_pos;
    dec_mem_window = context.mem_window;

    pos_end = pos + amount_to_decode;

    while (pos < pos_end)
    {
        DECODE_MAIN_TREE(c);

        if ((c -= 256) < 0)
        {
            context.mem_window[pos] = (UCHAR)c;
            context.mem_window[context.window_size + pos] = (UCHAR)c;
            pos++;
        }
        else
        {
            if ((match_length = c & NUM_PRIMARY_LENGTHS) == NUM_PRIMARY_LENGTHS)
            {
                DECODE_LEN_TREE_NOEOFCHECK(match_length);
            }

            m = c >> 3;

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

                context.last_matchpos_offset[2] = context.last_matchpos_offset[1];
                context.last_matchpos_offset[1] = context.last_matchpos_offset[0];
                context.last_matchpos_offset[0] = match_pos;
            }
            else
            {
                match_pos = context.last_matchpos_offset[m];

                if (m)
                {
                    context.last_matchpos_offset[m] = context.last_matchpos_offset[0];
                    context.last_matchpos_offset[0] = match_pos;
                }
            }

            match_length += MIN_MATCH;

            do
            {
                context.mem_window[pos] = context.mem_window[(pos - match_pos) & context.window_mask];

                if (pos < MAX_MATCH)
                    context.mem_window[context.window_size + pos] = context.mem_window[pos];

                pos++;
            } while (--match_length > 0);
        }
    }

    context.bitcount = dec_bitcount;
    context.bitbuf = dec_bitbuf;
    context.input_curpos = dec_input_curpos;

    return pos;
}
long fast_decode_verbatim_block(DEC_CONTEXT& context, long pos, int amount_to_decode)
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

    dec_bitcount = context.bitcount;
    dec_bitbuf = context.bitbuf;
    dec_input_curpos = context.input_curpos;
    dec_end_input_pos = context.end_input_pos;
    dec_mem_window = context.mem_window;

    pos_end = pos + amount_to_decode;

    while (pos < pos_end)
    {
        
        DECODE_MAIN_TREE(c); // decode an item from the main tree

        if ((c -= 256) < 0)
        {
            context.mem_window[pos++] = (UCHAR)c;
        }
        else
        {
            // get match length header
            if ((match_length = c & NUM_PRIMARY_LENGTHS) == NUM_PRIMARY_LENGTHS)
            {
                // get match length footer if necessary
                DECODE_LEN_TREE_NOEOFCHECK(match_length);
            }

            m = c >> 3; // get match position slot

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
                // update LRU repeated offset list

                context.last_matchpos_offset[2] = context.last_matchpos_offset[1];
                context.last_matchpos_offset[1] = context.last_matchpos_offset[0];
                context.last_matchpos_offset[0] = match_pos;
            }
            else
            {
                match_pos = context.last_matchpos_offset[m];

                if (m)
                {
                    context.last_matchpos_offset[m] = context.last_matchpos_offset[0];
                    context.last_matchpos_offset[0] = match_pos;
                }
            }

            match_length += MIN_MATCH;
            match_ptr = (pos - match_pos) & context.window_mask;

            do
            {
                context.mem_window[pos++] = context.mem_window[match_ptr++];
            } while (--match_length > 0);
        }
    }

    context.bitcount = dec_bitcount;
    context.bitbuf = dec_bitbuf;
    context.input_curpos = dec_input_curpos;

    decode_residue = pos - pos_end;

    pos &= context.window_mask;
    context.pos = pos;

    return decode_residue;
}

int decode_verbatim_block(DEC_CONTEXT& context, long pos, int amount_to_decode)
{
    if (pos < MAX_MATCH)
    {
        long new_pos;
        long amount_to_slowly_decode;

        amount_to_slowly_decode = min(MAX_MATCH - pos, amount_to_decode);

        new_pos = special_decode_verbatim_block(context, pos, amount_to_slowly_decode);

        amount_to_decode -= (new_pos - pos);

        context.pos = pos = new_pos;

        if (amount_to_decode <= 0)
            return amount_to_decode;
    }

    return fast_decode_verbatim_block(context, pos, amount_to_decode);
}
int decode_uncompressed_block(DEC_CONTEXT& context, long pos, int amount_to_decode)
{
    long	bytes_decoded = 0;
    long	pos_end;
    long	decode_residue;
    ULONG   pos_start;
    ULONG   end_copy_pos;
    const UCHAR* p;

    pos_start = pos;
    pos_end = pos + amount_to_decode;

    p = context.input_curpos;

    while (pos < pos_end)
    {
        if (p >= context.end_input_pos)
            return -1; // input overflow

        context.mem_window[pos++] = *p++;
    }

    context.input_curpos = p;

    end_copy_pos = min(MAX_MATCH, pos_end);

    while (pos_start < end_copy_pos)
    {
        context.mem_window[pos_start + context.window_size] =
            context.mem_window[pos_start];
        pos_start++;
    }

    decode_residue = pos - pos_end;

    pos &= context.window_mask;
    context.pos = pos;

    return (int)decode_residue;
}
long decode_data(DEC_CONTEXT& context, long bytes_to_decode)
{
    ULONG amount_can_decode;
    long total_decoded;

    total_decoded = 0;

    while (bytes_to_decode > 0)
    {
        if (context.decoder_state == DECSTATE_START_NEW_BLOCK)
        {
            ULONG temp1;
            ULONG temp2;
            ULONG temp3;
            bool do_translation;

            if (context.first_time_this_group)
            {
                context.first_time_this_group = false;

                do_translation = (bool)getbits(context, 1);

                if (do_translation)
                {
                    ULONG high, low;

                    high = getbits(context, 16);
                    low = getbits(context, 16);
                    context.current_file_size = (high << 16) | low;
                }
                else
                {
                    context.current_file_size = 0;
                }
            }

            if (context.block_type == BLOCKTYPE_UNCOMPRESSED)
            {
                if (context.original_block_size & 1)
                {
                    if (context.input_curpos < context.end_input_pos)
                        context.input_curpos++;
                }

                context.block_type = BLOCKTYPE_INVALID;

                initialise_decoder_bitbuf(context);
            }

            context.block_type = (BLOCK_TYPE)getbits(context, 3);

            temp1 = getbits(context, 8);
            temp2 = getbits(context, 8);
            temp3 = getbits(context, 8);

            context.block_size = context.original_block_size = (temp1 << 16) + (temp2 << 8) + (temp3);

            switch (context.block_type)
            {
                case BLOCKTYPE_ALIGNED:
                    read_aligned_offset_tree(context);
                    // fall through to VERBATIM

                case BLOCKTYPE_VERBATIM:
                    lzx_cpy(context.main_tree_prev_len, context.main_tree_len, MAIN_TREE_ELEMENTS);
                    lzx_cpy(context.secondary_length_tree_prev_len, context.secondary_length_tree_len, NUM_SECONDARY_LENGTHS);
                    read_main_and_secondary_trees(context);
                    break;

                case BLOCKTYPE_UNCOMPRESSED:
					if (handle_beginning_of_uncompressed_block(context) == false)
						return -1;
					break;

                default:
                    return -1;
            }
            
            context.decoder_state = DECSTATE_DECODING_DATA;
        }

        while (context.block_size > 0 && bytes_to_decode > 0)
        {
            int decode_residue;

            amount_can_decode = min(context.block_size, bytes_to_decode);

            if (amount_can_decode == 0)
                return -1;

            switch (context.block_type)
            {
				case BLOCKTYPE_ALIGNED:
                    decode_residue = decode_aligned_offset_block(context, context.pos, (int)amount_can_decode);
                    break;

				case BLOCKTYPE_VERBATIM:
                    decode_residue = decode_verbatim_block(context, context.pos, (int)amount_can_decode);
					break;

				case BLOCKTYPE_UNCOMPRESSED:
                    decode_residue = decode_uncompressed_block(context, context.pos, (int)amount_can_decode);
					break;

				default:
					return -1;
			}

            context.block_size -= amount_can_decode;
            bytes_to_decode -= amount_can_decode;
            total_decoded += amount_can_decode;
        }

        if (context.block_size == 0)
        {
            context.decoder_state = DECSTATE_START_NEW_BLOCK;
        }

        if (bytes_to_decode == 0)
        {
            initialise_decoder_bitbuf(context);
        }
    }

    long temp = context.pos;
    if (!temp)
    {
        temp = context.window_size;
	}

	copy_data_to_output(context, total_decoded, &context.mem_window[temp - total_decoded]);

    return total_decoded;
}

bool lzx_decodeInit(DEC_CONTEXT& context, long compression_window_size)
{
    context.window_size = compression_window_size;
    context.window_mask = context.window_size - 1;

    // check the window size
    if (context.window_size & context.window_mask)
        return false;

    // allocate memory for the window
    ULONG pos_start = 4;
    context.num_position_slots = 4;
    while (1)
    {
        pos_start += 1L << dec_extra_bits[context.num_position_slots];

        context.num_position_slots++;

        if (pos_start >= context.window_size)
            break;
    }
    if (!(context.mem_window = (UCHAR*)lzx_alloc(context.window_size + (MAX_MATCH + 4))))
        return false;

    // reset decoder state
    lzx_zero(context.main_tree_len, MAIN_TREE_ELEMENTS);
    lzx_zero(context.main_tree_prev_len, MAIN_TREE_ELEMENTS);
    lzx_zero(context.secondary_length_tree_len, NUM_SECONDARY_LENGTHS);
    lzx_zero(context.secondary_length_tree_prev_len, NUM_SECONDARY_LENGTHS);

    // init decoder state
    context.last_matchpos_offset[0] = 1;
    context.last_matchpos_offset[1] = 1;
    context.last_matchpos_offset[2] = 1;
    context.pos = 0;
    context.position_at_start = 0;
    context.decoder_state = DECSTATE_START_NEW_BLOCK;
    context.block_size = 0;
    context.block_type = BLOCKTYPE_INVALID;
    context.first_time_this_group = true;
    context.current_file_size = 0;
    context.error_condition = false;

    // init decoder translation
    context.instr_pos = 0;
    
    context.num_cfdata_frames = 0;

    return true;
}
int lzx_decode(DEC_CONTEXT* context,
    const UCHAR* compressed_input, long input_size, 
    UCHAR* uncompressed_output, long output_size,
    long* bytes_decoded)
{
    long result;

    context->input_curpos = compressed_input;
    context->end_input_pos = (compressed_input + input_size + 4);

    context->output_buffer = uncompressed_output;

    initialise_decoder_bitbuf(*context);

    result = decode_data(*context, output_size);

    context->num_cfdata_frames++;

    if (result < 0) // error
    {
        *bytes_decoded = 0;
        return 1;
    }
    else
    {
        *bytes_decoded = result;
        context->position_at_start += result;
        return 0;
    }
}

int ldi_createDecompression(UINT& bufferMin, LDI_CONTEXT*& context)
{
    bufferMin = LZX_CHUNK_SIZE + MAX_GROWTH;

    context = (LDI_CONTEXT*)lzx_alloc(sizeof(struct LDI_CONTEXT));
    if (context == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    context->decoder_context = (DEC_CONTEXT*)lzx_alloc(sizeof(DEC_CONTEXT));

    if (context->decoder_context == NULL)
    {
        lzx_free(context);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    context->blockMax = LZX_CHUNK_SIZE; // save the max block size
    context->signature = LDI_SIGNATURE; // mark as valid

    if (lzx_decodeInit(*context->decoder_context, LZX_WINDOW_SIZE) == false)
    {        
        lzx_free(context);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    return 0;
}
int ldi_destroyDecompression(LDI_CONTEXT& context)
{
    if (context.signature != LDI_SIGNATURE) // invalid signature
    {
        return ERROR_BAD_PARAMETERS; 
    }
    
    if (context.decoder_context->mem_window) // destroy signature
    {
        lzx_free(context.decoder_context->mem_window);
        context.decoder_context->mem_window = NULL;
    }

    context.signature = BAD_SIGNATURE;

    lzx_free(context.decoder_context);
    lzx_free(&context);

    return 0;
}
int ldi_decompress(LDI_CONTEXT& context, const UCHAR* pbSrc, UINT cbSrc, UCHAR* pbDst, UINT* pcbResult)
{
    int	result;
    long bytes_to_decode;
    long total_bytes_written = 0;

    if (context.signature != LDI_SIGNATURE) // invalid decompression signature
    {
        return ERROR_BAD_PARAMETERS;   
    }

    if (*pcbResult > context.blockMax) // buffer too small
    {
        return ERROR_BUFFER_OVERFLOW; 
    }

    bytes_to_decode = (long)*pcbResult;

    result = lzx_decode(context.decoder_context, pbSrc, cbSrc, pbDst, bytes_to_decode, &total_bytes_written);

    *pcbResult = (UINT)total_bytes_written;

    if (result)
        return ERROR_FAILED;
    
    return 0;
}

int decompress(const UCHAR* data, const UINT dataSize, UCHAR*& buff, UINT& buffSize, UINT& decompressedSize)
{
    LZX_BLOCK* block;
    const UCHAR* src;
    UCHAR* dest;

    LDI_CONTEXT* context = NULL;

    UINT bytesDecompressed = 0;
    UINT bytesCompressed = 0;
    UINT destSize = 0;
    ULONG totalUncompressedSize = 0;

    if (!SUCCESS(ldi_createDecompression(destSize, context)))
    {
        error("LDICreateDecompression failed\n");
        return 1;
    }

    src = data;
    dest = buff;

    int result = 0;
    int i = 0;

    for (;;i++)
    {
        block = (LZX_BLOCK*)src;    // get the block header
        src += sizeof(LZX_BLOCK*);  // compressed block starts after the header

        // Perform decompression
        bytesDecompressed = block->uncompressedSize;
        bytesCompressed = block->compressedSize;
 
        // Check to see if we have enough space in the output buffer
        if (dest + bytesDecompressed > buff + buffSize)
		{
            // resize the buffer
            buff = (UCHAR*)lzx_realloc(buff, buffSize + LZX_CHUNK_SIZE);
            if (buff == NULL)
			{
				error("Error: Not enough space in the output buffer\n");
				result = 1;
				goto Cleanup;
			}

            // fix the pointers
            dest = buff + totalUncompressedSize;

            // update the buffer size
            buffSize += bytesDecompressed;
		}


        //print("block %02d: %5d\n", i + 1, bytesDecompressed);

        result = ldi_decompress(*context, (UCHAR*)src, bytesCompressed, (UCHAR*)dest, &bytesDecompressed);
        if (!SUCCESS(result))
        {
            error("Error: Block decompress failed. Error code: %d\n", result);
            goto Cleanup;
        }

        // Advance the pointers
        src += bytesCompressed;
        dest += bytesDecompressed;
        totalUncompressedSize += (ULONG)bytesDecompressed;

        // Check to see if we are done
        if ((UINT)(src - data) >= dataSize)
        {
            result = 0;
            break;
        }
    }

    decompressedSize = totalUncompressedSize;

Cleanup:
    
    print("Decompressed %ld bytes (%d blocks)\n", totalUncompressedSize, i+1);
    
    ldi_destroyDecompression(*context);

    return result;
}