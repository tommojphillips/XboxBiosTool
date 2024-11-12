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

#include <stdint.h>
#include <malloc.h>

// user incl
#include "XcodeInterp.h"

#ifdef MEM_TRACKING
#include "mem_tracking.h"
#endif

extern const LOADINI_RETURN_MAP decode_settings_map;

typedef enum : uint8_t {
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
    uint32_t seq;
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

    uint32_t opcodeMaxLen;
    uint32_t labelMaxLen;
    FIELD_MAP opcodes[XC_OPCODE_COUNT];
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
    
    for (int i = 0; i < XC_OPCODE_COUNT; i++) {
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
        settings->format_str = NULL;
    }
    if (settings->jmp_str != NULL) {
        free(settings->jmp_str);
        settings->jmp_str = NULL;
    }
    if (settings->no_operand_str != NULL) {
        free(settings->no_operand_str);
        settings->no_operand_str = NULL;
    }
    if (settings->num_str != NULL) {
        free(settings->num_str);
        settings->num_str = NULL;
    }
    if (settings->num_str_format != NULL) {
        free(settings->num_str_format);
        settings->num_str_format = NULL;
    }
    if (settings->prefix_str != NULL) {
        free(settings->prefix_str);
        settings->prefix_str = NULL;
    }
    if (settings->comment_prefix != NULL) {
        free(settings->comment_prefix);
        settings->comment_prefix = NULL;
    }
    for (int i = 0; i < 5; i++) {
        if (settings->format_map[i].str != NULL) {
            free(settings->format_map[i].str);
            settings->format_map[i].str = NULL;
        }
    }
    for (int i = 0; i < XC_OPCODE_COUNT; i++) {
        if (settings->opcodes[i].str != NULL) {
            free((char*)settings->opcodes[i].str);
        }
    }
};

#define JMP_XCODE_NOT_BRANCHABLE 0
#define JMP_XCODE_BRANCHABLE 1

typedef struct _JMP_XCODE {
    int branchable;
    uint32_t xcodeOffset;
    XCODE* xcode;
} JMP_XCODE;

// DECODE_CONTEXT
typedef struct {
    DECODE_SETTINGS settings;
    JMP_XCODE* jmps;
    LABEL* labels;
    XCODE* xcode;
    FILE* stream;
    uint32_t labelCount;
    uint32_t xcodeCount;
    uint32_t jmpCount;
    uint32_t xcodeSize;
    uint32_t xcodeBase;
    bool branch;
    char str_decode[128];
} DECODE_CONTEXT;
inline void initDecodeContext(DECODE_CONTEXT* context) {
    memset(context, 0, sizeof(DECODE_CONTEXT));
    context->branch = false;
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
    if (context != NULL) {
        context->stream = NULL;
        context->xcode = NULL;

        if (context->labels != NULL) {
            free(context->labels);
            context->labels = NULL;
        }
        context->labelCount = 0;

        if (context->jmps != NULL) {
            free(context->jmps);
            context->jmps = NULL;
        }
        context->jmpCount = 0;

        destroyDecodeSettings(&context->settings);

        free(context);
        context = NULL;
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
	};

    DECODE_CONTEXT* context;
    XcodeInterp interp;

    // load the decoder.
    // data: the xcode data. xcodes should start at data[base].
    // size: the size of the xcode data.
    // ini: the ini file. [OPTIONAL]. if not provided, default settings are used. NULL if not needed.
    int load(uint8_t* data, uint32_t size, uint32_t base, const char* ini);
    int decodeXcodes();
    int decode();

private:
    int loadSettings(const char* ini, DECODE_SETTINGS* settings) const;
    int getCommentStr(char* str);
};

#endif // !XCODE_DECODER_H
