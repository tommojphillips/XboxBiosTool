// XcodeDecoder.h: defines functions for decoding XCODE instructions

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

#ifndef XCODE_DECODER_H
#define XCODE_DECODER_H

// user incl
#include "XcodeInterp.h"
#include "type_defs.h"

#ifdef MEM_TRACKING
#include "mem_tracking.h"
#else
#include <malloc.h>
#endif

typedef enum : UCHAR {
    DECODE_FIELD_OFFSET,
    DECODE_FIELD_OPCODE,
    DECODE_FIELD_ADDRESS,
    DECODE_FIELD_DATA,
    DECODE_FIELD_COMMENT
}  DECODE_FIELD;
typedef struct {
    const char* field;
    const char* value;
} DECODE_SETTING_MAP;
typedef struct {
    DECODE_FIELD type;
    UINT seq;
    char* str;
} DECODE_STR_SETTING_MAP;

// DECODE SETTINGS
typedef struct {
    char* format_str;
    char* prefix_str;
    char* jmp_str;
    char* no_operand_str;
    char* num_str;
    char* num_str_format;
    char* comment_prefix;
    DECODE_STR_SETTING_MAP format_map[5];
    bool label_on_new_line;
    bool pad;
    bool opcode_use_result;

    UINT opcodeMaxLen;
    UINT labelMaxLen;
    FIELD_MAP opcodes[XCODE_OPCODE_COUNT];
} DECODE_SETTINGS;
inline void initDecodeSettings(DECODE_SETTINGS* settings) {
    settings->format_str = NULL;
    settings->jmp_str = NULL;
    settings->no_operand_str = NULL;
    settings->num_str = NULL;
    settings->num_str_format = NULL;
    settings->prefix_str = NULL;
    settings->comment_prefix = NULL;

    settings->label_on_new_line = false;
    settings->pad = true;
    settings->opcode_use_result = false;

    settings->labelMaxLen = 0;
    settings->opcodeMaxLen = 0;

    for (int i = 0; i < 5; i++) {
        settings->format_map[i].type = (DECODE_FIELD)i;
        settings->format_map[i].str = NULL;
        settings->format_map[i].seq = 0;
    }

    for (int i = 0; i < XCODE_OPCODE_COUNT; i++) {
        settings->opcodes[i].str = NULL;
        settings->opcodes[i].field = 0;
    }
};
inline DECODE_SETTINGS* createDecodeSettings() {
    DECODE_SETTINGS* settings = (DECODE_SETTINGS*)malloc(sizeof(DECODE_SETTINGS));
    if (settings == NULL)
        return NULL;
    initDecodeSettings(settings);
    return settings;
};
inline void destroyDecodeSettings(DECODE_SETTINGS* settings) {
    if (settings->format_str != NULL) {
        free(settings->format_str);
    }
    if (settings->jmp_str != NULL) {
        free(settings->jmp_str);
    }
    if (settings->no_operand_str != NULL) {
        free(settings->no_operand_str);
    }
    if (settings->num_str != NULL) {
        free(settings->num_str);
    }
    if (settings->num_str_format != NULL) {
        free(settings->num_str_format);
    }
    if (settings->prefix_str != NULL) {
        free(settings->prefix_str);
    }
    if (settings->comment_prefix != NULL) {
        free(settings->comment_prefix);
    }
    for (int i = 0; i < 5; i++) {
        if (settings->format_map[i].str != NULL) {
            free(settings->format_map[i].str);
        }
    }
    for (int i = 0; i < XCODE_OPCODE_COUNT; i++) {
        if (settings->opcodes[i].str != NULL) {
            free((char*)settings->opcodes[i].str);
        }
    }
};

// DECODE_CONTEXT
typedef struct {
    DECODE_SETTINGS settings;
    LABEL* labels;
    XCODE* xcode;
    FILE* stream;
    UINT labelCount;
    UINT xcodeCount;
    UINT xcodeSize;
    UINT xcodeBase;
    char str_decode[128];
} DECODE_CONTEXT;
inline void initDecodeContext(DECODE_CONTEXT* context) {
    context->labels = NULL;
    context->stream = NULL;
    context->xcode = NULL;
    context->xcodeSize = 0;
    context->xcodeCount = 0;
    context->xcodeBase = 0;
    context->labelCount = 0;
    context->str_decode[0] = '\0';

    initDecodeSettings(&context->settings);
};
inline DECODE_CONTEXT* createDecodeContext() {
    DECODE_CONTEXT* context = (DECODE_CONTEXT*)malloc(sizeof(DECODE_CONTEXT));
    if (context == NULL)
        return NULL;
    initDecodeContext(context);
    return context;
};
inline void destroyDecodeContext(DECODE_CONTEXT* context) {
    if (context != NULL)
    {
        context->stream = NULL;
        context->xcode = NULL;

        if (context->labels != NULL)
        {
            free(context->labels);
            context->labels = NULL;
        }
        context->labelCount = 0;

        destroyDecodeSettings(&context->settings);
    }
};

class XcodeDecoder
{
public:
	XcodeDecoder() {
        context = NULL;
	};
	~XcodeDecoder() {
        destroyDecodeContext(context);
        free(context);
	};

    DECODE_CONTEXT* context;
    XcodeInterp interp;

    // load the decoder.
    // data: the xcode data. xcodes should start at data[base].
    // size: the size of the xcode data.
    // ini: the ini file. [OPTIONAL]. if not provided, default settings are used. NULL if not needed.
    int load(UCHAR* data, UINT size, UINT base, const char* ini);
    int decodeXcodes();
    int decode();

private:
    int loadSettings(const char* ini, DECODE_SETTINGS* settings) const;
    int getCommentStr(char* str);
};

#endif // !XCODE_DECODER_H
