// XcodeDecoder.cpp: defines functions for decoding XCODE instructions

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

// std incl
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

// user incl
#include "XcodeInterp.h"
#include "XcodeDecoder.h"
#include "util.h"
#include "str_util.h"
#include "loadini.h"

#ifndef NO_MEM_TRACKING
#include "mem_tracking.h"
#endif

#define IS_NEG_TRUE(x, y) (y < 0 ? true : x == y)

#define XC_WRITE_MASK(i, xc_cmd, xc_addr, addr_mask, xc_data, data_mask) \
	(((uint8_t*)(_ptr+i)) >= _data) && ((_ptr+i)->opcode == xc_cmd && \
	((_ptr+i)->addr & addr_mask) == xc_addr && \
	IS_NEG_TRUE(((_ptr+i)->data  & data_mask), xc_data))

#define XC_WRITE(i, xc_cmd, xc_addr, xc_data) XC_WRITE_MASK(i, xc_cmd, xc_addr, 0xFFFFFFFF, xc_data, 0xFFFFFFFF)

#define XC_WRITE_COMMENT_MASK(comment, xc_cmd, xc_addr, addr_mask, xc_data) \
	if (XC_WRITE_MASK(0, xc_cmd, xc_addr, addr_mask, xc_data, 0xFFFFFFFF)) \
		{ strcat(str, comment); return 0; }

#define XC_WRITE_STATEMENT(xc_cmd, xc_addr, xc_data, statement) \
	if (XC_WRITE(0, xc_cmd, xc_addr, xc_data)) \
		{ statement; return 0; }

#define XC_WRITE_COMMENT(comment, xc_cmd, xc_addr, xc_data) XC_WRITE_STATEMENT(xc_cmd, xc_addr, xc_data, strcat(str, comment))

#define XC_WRITE_COMMENT_LAST_XCODE(comment, xc_cmd, xc_addr, xc_data, xc_prev_cmd, xc_prev_addr, xc_prev_data) \
	if (XC_WRITE(0, xc_cmd, xc_addr, xc_data) && XC_WRITE(-1, xc_prev_cmd, xc_prev_addr, xc_prev_data)) \
		{ strcat(str, comment); return 0; }

#define XC_WRITE_COMMENT_NEXT_XCODE(comment, xc_cmd, xc_addr, xc_data, xc_prev_cmd, xc_prev_addr, xc_prev_data) \
	if (XC_WRITE(0, xc_cmd, xc_addr, xc_data) && XC_WRITE(1, xc_prev_cmd, xc_prev_addr, xc_prev_data)) \
		{ strcat(str, comment); return 0; }

#define XC_WRITE_COMMENT_LAST_2_XCODE(comment, xc_cmd, xc_addr, xc_data, xc_prev_cmd, xc_prev_addr, xc_prev_data, xc_prev1_cmd, xc_prev1_addr, xc_prev1_data) \
	if (XC_WRITE(0, xc_cmd, xc_addr, xc_data) && XC_WRITE(-1, xc_prev_cmd, xc_prev_addr, xc_prev_data) && XC_WRITE(-2, xc_prev1_cmd, xc_prev1_addr, xc_prev1_data)) \
		{ strcat(str, comment); return 0; }

#define XC_WRITE_COMMENT_MASK_LAST_XCODE(comment, xc_cmd, xc_addr, xc_addr_mask, xc_data, xc_prev_cmd, xc_prev_addr, xc_prev_addr_mask, xc_prev_data) \
	if (XC_WRITE_MASK(0, xc_cmd, xc_addr, xc_addr_mask, xc_data, 0xFFFFFFFF) && XC_WRITE_MASK(-1, xc_prev_cmd, xc_prev_addr, xc_prev_addr_mask, xc_prev_data, 0xFFFFFFFF)) \
		{ strcat(str, comment); return 0; }

#define XC_WRITE_COMMENT_MASK_NEXT_XCODE(comment, xc_cmd, xc_addr, xc_addr_mask, xc_data, xc_prev_cmd, xc_prev_addr, xc_prev_addr_mask, xc_prev_data) \
	if (XC_WRITE_COMMENT_MASK(0, xc_cmd, xc_addr, xc_addr_mask, xc_data) && XC_WRITE_COMMENT_MASK(1, xc_prev_cmd, xc_prev_addr, xc_prev_addr_mask, xc_prev_data)) \
		{ strcat(str, comment); return 0; }

static const DECODE_SETTING_MAP num_str_fields[] = {
	{ "{hex}", "%x" },
	{ "{hex8}", "%08x" },
	{ "{HEX}", "%X" },
	{ "{HEX8}", "%08X" }
};
static const FIELD_MAP format_map[] = {
	{ "{offset}", DECODE_FIELD_OFFSET },
	{ "{op}", DECODE_FIELD_OPCODE },
	{ "{addr}", DECODE_FIELD_ADDRESS },
	{ "{data}", DECODE_FIELD_DATA },
	{ "{comment}", DECODE_FIELD_COMMENT }
};
static const LOADINI_SETTING settings_map[] = {
	{ "format_str", LOADINI_SETTING_TYPE::STR },
	{ "jmp_str", LOADINI_SETTING_TYPE::STR },
	{ "no_operand_str", LOADINI_SETTING_TYPE::STR },
	{ "num_str", LOADINI_SETTING_TYPE::STR, },
	{ "comment_prefix", LOADINI_SETTING_TYPE::STR},
	{ "label_on_new_line", LOADINI_SETTING_TYPE::BOOL },
	{ "pad", LOADINI_SETTING_TYPE::BOOL },
	{ "opcode_use_result", LOADINI_SETTING_TYPE::BOOL },

	{ xcode_opcode_map[0].str, LOADINI_SETTING_TYPE::STR },
	{ xcode_opcode_map[1].str, LOADINI_SETTING_TYPE::STR },
	{ xcode_opcode_map[2].str, LOADINI_SETTING_TYPE::STR },
	{ xcode_opcode_map[3].str, LOADINI_SETTING_TYPE::STR },
	{ xcode_opcode_map[4].str, LOADINI_SETTING_TYPE::STR },
	{ xcode_opcode_map[5].str, LOADINI_SETTING_TYPE::STR },
	{ xcode_opcode_map[6].str, LOADINI_SETTING_TYPE::STR },
	{ xcode_opcode_map[7].str, LOADINI_SETTING_TYPE::STR },
	{ xcode_opcode_map[8].str, LOADINI_SETTING_TYPE::STR },
	{ xcode_opcode_map[9].str, LOADINI_SETTING_TYPE::STR },
	{ xcode_opcode_map[10].str, LOADINI_SETTING_TYPE::STR },
	{ xcode_opcode_map[11].str, LOADINI_SETTING_TYPE::STR },
	{ xcode_opcode_map[12].str, LOADINI_SETTING_TYPE::STR },
	{ xcode_opcode_map[13].str, LOADINI_SETTING_TYPE::STR },
	{ xcode_opcode_map[14].str, LOADINI_SETTING_TYPE::STR }
};
static const LOADINI_RETURN_MAP cmap = { settings_map, sizeof(settings_map), sizeof(settings_map) / sizeof(LOADINI_SETTING_MAP) };

// remove {entry} from str;
// output: output buffer
// str: input string
// i: start index
// j: end index
// len: length of input string
// m: max length of output buffer
// return 0 if found, 1 if not found, -1 if error
int ll(char* output, char* str, uint32_t i, uint32_t* j, uint32_t len, uint32_t m);

// output: output buffer
// str: input string
// i: start index
// j: end index
// len: length of input string
// return 0 if found, 1 if not found, -1 if error
int ll2(char* output, char* str, uint32_t i, uint32_t& j, uint32_t len);

int XcodeDecoder::load(uint8_t* data, uint32_t size, uint32_t base, const char* ini)
{
	// set up the xcode decoder.
	// load the interpreter, decode settings and context.
	// parse xcodes for labels.

	int result = 0;
	uint32_t lbi;
	bool isLabel = false;
	static const char* label_format = "lb_%02d";

	result = interp.load(data + base, size - base);
	if (result != 0)
		return 1;

	context = createDecodeContext();
	if (context == NULL) {
		return ERROR_OUT_OF_MEMORY;
	}

	context->xcodeBase = base;
	
	memset(context->str_decode, 0, sizeof(context->str_decode));

	result = loadSettings(ini, &context->settings);
	if (result != 0) {
		return result;
	}

	// Fix up the xcode data to the offset for decoding

	// Count the number of labels for allocation

	lbi = 0;
	interp.reset();
	while (interp.interpretNext(context->xcode) == 0) {
		if (context->xcode->opcode != XC_JMP && context->xcode->opcode != XC_JNE)
			continue;

		lbi++;
		context->xcode->data = interp.getOffset() + context->xcode->data;
	}

	// initialize labels; NOTE. More than likely over allocated as labels can be referenced more than once within the xcode table.

	context->labels = (LABEL*)malloc(sizeof(LABEL) * lbi);
	if (context->labels == NULL) {
		return ERROR_OUT_OF_MEMORY;
	}

	context->settings.labelMaxLen = 0;
	while (lbi > 0) {
		context->settings.labelMaxLen++;
		lbi /= 10;
	}
	context->settings.labelMaxLen += strlen(label_format) - 4; // 4 for %02d

	lbi = 0;
	interp.reset();
	while (interp.interpretNext(context->xcode) == 0) {
		if (context->xcode->opcode != XC_JMP && context->xcode->opcode != XC_JNE)
			continue;

		// check if offset is already a label
		isLabel = false;
		for (uint32_t lb = 0; lb < lbi; lb++) {
			LABEL* label = &context->labels[lb];
			if (label == NULL)
				continue;
			if (label->offset == context->xcode->data) {
				isLabel = true;
				break;
			}
		}

		// label does not exist. create one.
		if (!isLabel) {
			LABEL* label = &context->labels[lbi];
			sprintf(label->name, label_format, lbi);
			label->offset = context->xcode->data;
			lbi++;
		}
	}
	context->xcodeSize = interp.getOffset();
	context->xcodeCount = interp.getOffset() / sizeof(XCODE);
	context->labelCount = lbi;

	return 0;
}

int XcodeDecoder::loadSettings(const char* ini, DECODE_SETTINGS* settings) const
{
	char* value = NULL;
	uint32_t len = 0;
	uint32_t i = 0;
	uint32_t j = 0;
	uint8_t k = 0;
	int result = 0;
	FILE* stream = NULL;
	char buf[128] = {};

	static const char* default_format_str = "{offset}: {op} {addr} {data} {comment}";
	
	static const LOADINI_SETTING_MAP var_map[] = {
		{ &cmap.s[0], &settings->format_str},
		{ &cmap.s[1], &settings->jmp_str },
		{ &cmap.s[2], &settings->no_operand_str },
		{ &cmap.s[3], &settings->num_str },
		{ &cmap.s[4], &settings->comment_prefix},
		{ &cmap.s[5], &settings->label_on_new_line },
		{ &cmap.s[6], &settings->pad },
		{ &cmap.s[7], &settings->opcode_use_result },
		{ &cmap.s[8], &settings->opcodes[0].str },
		{ &cmap.s[9], &settings->opcodes[1].str },
		{ &cmap.s[10], &settings->opcodes[2].str },
		{ &cmap.s[11], &settings->opcodes[3].str },
		{ &cmap.s[12], &settings->opcodes[4].str },
		{ &cmap.s[13], &settings->opcodes[5].str },
		{ &cmap.s[14], &settings->opcodes[6].str },
		{ &cmap.s[15], &settings->opcodes[7].str },
		{ &cmap.s[16], &settings->opcodes[8].str },
		{ &cmap.s[17], &settings->opcodes[9].str },
		{ &cmap.s[18], &settings->opcodes[10].str },
		{ &cmap.s[19], &settings->opcodes[11].str },
		{ &cmap.s[20], &settings->opcodes[12].str },
		{ &cmap.s[21], &settings->opcodes[13].str },
		{ &cmap.s[22], &settings->opcodes[14].str }
	};

	if (ini != NULL) {
		stream = fopen(ini, "r");
		if (stream != NULL) { // only load ini file if it exists
			printf("settings file: %s\n", ini);
			result = loadini(stream, var_map, cmap.size);
			fclose(stream);
			if (result != 0) { // convert to error code
				result = 1;
				goto Cleanup;
			}
		}
		else {
			printf("settings: default\n");
		}
	}

	// default format_str
	if (settings->format_str == NULL) {		
		settings->format_str = (char*)malloc(strlen(default_format_str) + 1);
		if (settings->format_str == NULL) {
			result = ERROR_OUT_OF_MEMORY;
			goto Cleanup;
		}
		strcpy(settings->format_str, default_format_str);
	}

	len = strlen(settings->format_str);
	k = 1;
	
	for (i = 0; i < len; i++) {
		result = ll2(buf, settings->format_str, i, j, len);
		if (result == ERROR_INVALID_DATA) {
			goto Cleanup;
		}
		else if (result != 0) {
			result = 0;

			if (i == 0) { // if first entry
				if (j > 1) { // if entry is not empty
					settings->prefix_str = (char*)malloc(j + 1);
					if (settings->prefix_str == NULL) {
						result = ERROR_OUT_OF_MEMORY;
						goto Cleanup;
					}
					strcpy(settings->prefix_str, buf);
					i = j - 1;
				}
				else {
					settings->prefix_str = NULL;
				}
			}

			continue;
		}

		for (j = 0; j < sizeof(settings->format_map) / sizeof(DECODE_STR_SETTING_MAP); j++) {
			if (strstr(buf, format_map[j].str) != NULL) {
				result = ll(NULL, buf, 0, NULL, strlen(buf), 2);
				memcpy(buf, "%s", 2);

				settings->format_map[j].str = (char*)malloc(strlen(buf) + 1);
				if (settings->format_map[j].str == NULL) {
					result = ERROR_OUT_OF_MEMORY;
					goto Cleanup;
				}

				strcpy(settings->format_map[j].str, buf);
				settings->format_map[j].seq = k++;
				i += strlen(buf) - 1;
				break;
			}
		}
		if (j == sizeof(settings->format_map) / sizeof(DECODE_STR_SETTING_MAP)) {
			result = ERROR_INVALID_DATA;
			goto Cleanup;
		}
	}

	// jmp_str
	if (settings->jmp_str != NULL) {
		len = strlen(settings->jmp_str);
		for (i = 0; i < len; i++) {
			result = ll(buf, settings->jmp_str, i, &j, len, 2);
			if (result == ERROR_INVALID_DATA) { // parse error
				goto Cleanup;
			}
			else if (result != 0) { // no '{' found, skip
				result = 0;
				continue;
			}

			if (strcmp(buf, "{label}") == 0) {
				memcpy(settings->jmp_str + i, "%s", 2);
			}
			else {
				result = ERROR_INVALID_DATA;
				goto Cleanup;
			}
		}
	}
	else {
		settings->jmp_str = (char*)malloc(6);
		if (settings->jmp_str == NULL) {
			result = ERROR_OUT_OF_MEMORY;
			goto Cleanup;
		}
		strcpy(settings->jmp_str, "%s:");
	}

	// num_str
	if (settings->num_str != NULL) {
		len = strlen(settings->num_str);
		for (i = 0; i < len; ++i) {
			result = ll(buf, settings->num_str, i, &j, len, 2);
			if (result == ERROR_INVALID_DATA) { // parse error
				goto Cleanup;
			}
			else if (result != 0) { // no '{' found
				result = 0;
				continue;
			}

			memcpy(settings->num_str + i, "%s", 2);

			for (j = 0; j < sizeof(num_str_fields) / sizeof(DECODE_SETTING_MAP); ++j) {
				if (strcmp(buf, num_str_fields[j].field) == 0) {
					settings->num_str_format = (char*)malloc(strlen(settings->num_str) - 2 + 8 + 1); // +8 for digits
					if (settings->num_str_format == NULL) {
						result = ERROR_OUT_OF_MEMORY;
						goto Cleanup;
					}
					sprintf(settings->num_str_format, settings->num_str, num_str_fields[j].value);
					break;
				}
			}

			if (j == sizeof(num_str_fields) / sizeof(DECODE_SETTING_MAP)) {
				result = ERROR_INVALID_DATA;
				goto Cleanup;
			}
		}
	}
	else
	{
		settings->num_str = (char*)malloc(3);
		if (settings->num_str == NULL) {
			result = ERROR_OUT_OF_MEMORY;
			goto Cleanup;
		}
		strcpy(settings->num_str, "%s");
	}

	// set default num_str_format
	if (settings->num_str_format == NULL) {
		settings->num_str_format = (char*)malloc(5);
		if (settings->num_str_format == NULL) {
			result = ERROR_OUT_OF_MEMORY;
			goto Cleanup;
		}
		strcpy(settings->num_str_format, "%08x");
	}

	// set default comment_prefix
	if (settings->comment_prefix == NULL) {
		settings->comment_prefix = (char*)malloc(3);
		if (settings->comment_prefix == NULL) {
			result = ERROR_OUT_OF_MEMORY;
			goto Cleanup;
		}
		strcpy(settings->comment_prefix, "; ");
	}

	// opcodes
	settings->opcodeMaxLen = 0;
	k = 0;
	for (i = 0; i < XC_OPCODE_COUNT; ++i) {
		uint8_t opcode = 0;
		const char* str = NULL;

		// find next valid opcode (range 0-255)
		while ((opcode = k) < 255 && getOpcodeStr(xcode_opcode_map, k++, str) != 0);

		if (str == NULL)
		{
			result = 1;
			goto Cleanup;
		}

		if (settings->opcodes[i].str == NULL) // set default opcode string
		{
			settings->opcodes[i].str = (char*)malloc(strlen(str) + 1);
			if (settings->opcodes[i].str == NULL)
			{
				result = ERROR_OUT_OF_MEMORY;
				goto Cleanup;
			}
			strcpy((char*)settings->opcodes[i].str, str);
		}

		memcpy(&settings->opcodes[i].field, &opcode, sizeof(OPCODE));

		len = strlen(settings->opcodes[i].str);
		if (len > settings->opcodeMaxLen)
		{
			settings->opcodeMaxLen = len;
		}
	}

Cleanup:

	if (result == ERROR_INVALID_DATA)
	{
		printf("Error: Key '%s' has invalid value '%s'\n", buf, value);
	}

	return result;
}

int XcodeDecoder::decodeXcodes()
{
	int result;

	interp.reset();
	while (interp.interpretNext(context->xcode) == 0)
	{
		result = decode();
		if (result != 0)
		{
			if (result == ERROR_BUFFER_OVERFLOW)
			{
				printf("Error: Decode format too large.\n");
			}
			else
			{
				printf("Error decoding xcode:\n\t%04X, OP: %02X, ADDR: %04X, DATA: %04X\n",
					(context->xcodeBase + interp.getOffset() - sizeof(XCODE)), context->xcode->opcode, context->xcode->addr, context->xcode->data);
			}
			return result;
		}
	}

	return 0;
}

int XcodeDecoder::decode()
{
	uint32_t len;
	uint32_t fmt_len;
	uint32_t op_len = 0;
	uint32_t operand_len = 0;
	uint32_t no_operand_len = 0;
	uint32_t jmp_len = 0;
	
	char str[64] = {};
	char str_tmp[64] = {};

	for (uint32_t lb = 0; lb < context->labelCount; lb++) {
		LABEL* label = &context->labels[lb];
		if (label->offset == interp.getOffset() - sizeof(XCODE)) {
			sprintf(str, "%s:", label->name);
			if (context->settings.label_on_new_line)
				strcat(str, "\n");
			break;
		}
	}

	if (!context->settings.label_on_new_line)
		rpad(str, context->settings.labelMaxLen + 2 + 1, ' '); // 2 for ': '
	else
		strcat(str, "\t");
	strcpy(context->str_decode, str);
	str[0] = '\0';

	// prefix
	if (context->settings.prefix_str != NULL) {
		strcat(context->str_decode, context->settings.prefix_str);
	}

	for (uint32_t seq = 0; seq < sizeof(context->settings.format_map) / sizeof(DECODE_STR_SETTING_MAP); seq++) {
		for (uint32_t j = 0; j < sizeof(context->settings.format_map) / sizeof(DECODE_STR_SETTING_MAP); j++) {
			if (context->settings.format_map[j].seq != seq + 1)
				continue;
			if (context->settings.format_map[j].str == NULL)
				break;

			len = strlen(context->str_decode);
			fmt_len = strlen(context->settings.format_map[j].str) - 2;
			if (context->settings.no_operand_str != NULL) {
				no_operand_len = strlen(context->settings.no_operand_str) + fmt_len + 1;
			}
			op_len = context->settings.opcodeMaxLen + fmt_len + 1;
			operand_len = strlen(context->settings.num_str) - 2 + 8 + fmt_len + 1;
			jmp_len = context->settings.labelMaxLen + strlen(context->settings.jmp_str) - 3 + fmt_len + 1;

			str[0] = '\0';
			memset(str_tmp, 0, sizeof(str_tmp));

			switch (context->settings.format_map[j].type) {
			case DECODE_FIELD_OFFSET:
				sprintf(str_tmp, context->settings.format_map[j].str, "%04x");
				sprintf(str, str_tmp, context->xcodeBase + interp.getOffset() - sizeof(XCODE));
				break;

			case DECODE_FIELD_OPCODE:
				const char* str_opcode;
				if (getOpcodeStr(context->settings.opcodes, context->xcode->opcode, str_opcode) != 0)
					return 1;
				sprintf(str, context->settings.format_map[j].str, str_opcode);
				if (context->settings.pad) {
					rpad(str, op_len, ' ');
				}
				break;

			case DECODE_FIELD_ADDRESS:	
				switch (context->xcode->opcode) {
					case XC_JMP: {
						if (context->settings.no_operand_str != NULL) {
							strcpy(str_tmp, context->settings.no_operand_str);
						}
						else {
							for (uint32_t lb = 0; lb < context->labelCount; lb++) {
								LABEL* label = &context->labels[lb];
								if (label->offset == context->xcode->data) {
									sprintf(str_tmp, context->settings.jmp_str, label->name);
									break;
								}
							}//str_tmp[0] = '\0';
						}						
					} break;

					case XC_USE_RESULT: {
						if (context->settings.opcode_use_result) {
							const char* opcode_str;
							if (getOpcodeStr(context->settings.opcodes, (uint8_t)context->xcode->addr, opcode_str) == 0) {
								strcpy(str_tmp, opcode_str);
								break;
							}
						}
					}
					// fall through

					default: {
						sprintf(str_tmp, context->settings.num_str_format, context->xcode->addr);
						break;
					}
				}

				sprintf(str, context->settings.format_map[j].str, str_tmp);
				if (context->settings.pad) {
					if (context->settings.opcode_use_result && operand_len < op_len)
						operand_len = op_len;
					if (operand_len < no_operand_len)
						operand_len = no_operand_len;
					rpad(str, operand_len, ' ');
				}
				break;

			case DECODE_FIELD_DATA:
				switch (context->xcode->opcode) {
					case XC_MEM_READ:
					case XC_IO_READ:
					case XC_PCI_READ:
					case XC_EXIT:
						if (context->settings.no_operand_str != NULL) {
							strcpy(str_tmp, context->settings.no_operand_str);
						}
						else {
							str_tmp[0] = '\0';
						}
						break;

					case XC_JMP:
						if (context->settings.no_operand_str != NULL)
							goto MakeLabel;
						break;

					case XC_JNE:
						MakeLabel:
						for (uint32_t lb = 0; lb < context->labelCount; lb++) {
							LABEL* label = &context->labels[lb];
							if (label->offset == context->xcode->data) {
								sprintf(str_tmp, context->settings.jmp_str, label->name);
								break;
							}
						}
						break;

					default:
						sprintf(str_tmp, context->settings.num_str_format, context->xcode->data);
						break;
				}

				sprintf(str, context->settings.format_map[j].str, str_tmp);
				if (context->settings.pad) {
					if (operand_len < jmp_len)
						operand_len = jmp_len;
					if (operand_len < no_operand_len)
						operand_len = no_operand_len;
					rpad(str, operand_len, ' ');
				}
				break;

			case DECODE_FIELD_COMMENT:
				uint32_t prefixLen = strlen(context->settings.comment_prefix);
				getCommentStr(str_tmp + prefixLen);
				if (str_tmp[prefixLen] != '\0') {
					memcpy(str_tmp, context->settings.comment_prefix, prefixLen);
				}
				sprintf(str, context->settings.format_map[j].str, str_tmp);
				break;
			}

			if (len + strlen(str) > sizeof(context->str_decode)) {
				return ERROR_BUFFER_OVERFLOW;
			}

			strcpy(context->str_decode + len, str);
			break;
		}
	}

	if (context->stream != NULL)
	{
		fprintf(context->stream, "%s\n", context->str_decode);
	}

	return 0;
}

int XcodeDecoder::getCommentStr(char* str)
{
	// -1 = dont care about field

	XCODE* _ptr = interp.getPtr();
	uint32_t _offset = interp.getOffset();
	uint8_t* _data = interp.getData();

	XC_WRITE_COMMENT("smbus read status", XC_IO_READ, SMB_BASE + 0x00, -1);
	XC_WRITE_COMMENT("smbus clear status", XC_IO_WRITE, SMB_BASE + 0x00, 0x10);

	XC_WRITE_COMMENT("smbus read revision register", XC_IO_WRITE, SMB_BASE + 0x08, 0x01);

	XC_WRITE_COMMENT("smbus set cmd", XC_IO_WRITE, SMB_BASE + 0x08, -1);
	XC_WRITE_COMMENT("smbus set val", XC_IO_WRITE, SMB_BASE + 0x06, -1);

	XC_WRITE_COMMENT("smbus kickoff", XC_IO_WRITE, SMB_BASE + 0x02, 0x0A);

	XC_WRITE_COMMENT("smc slave write addr", XC_IO_WRITE, SMB_BASE + 0x04, 0x20);
	XC_WRITE_COMMENT("smc slave read addr", XC_IO_WRITE, SMB_BASE + 0x04, 0x21);

	XC_WRITE_COMMENT("871 encoder slave addr", XC_IO_WRITE, SMB_BASE + 0x04, 0x8A);
	XC_WRITE_COMMENT("focus encoder slave addr", XC_IO_WRITE, SMB_BASE + 0x04, 0xD4);
	XC_WRITE_COMMENT("xcalibur encoder slave addr", XC_IO_WRITE, SMB_BASE + 0x04, 0xE1);

	// mcpx v1.0 io bar
	XC_WRITE_COMMENT("read io bar (B02) MCPX v1.0", XC_PCI_READ, MCPX_1_0_IO_BAR, 0);
	XC_WRITE_COMMENT("set io bar (B02) MCPX v1.0", XC_PCI_WRITE, MCPX_1_0_IO_BAR, MCPX_IO_BAR_VAL);
	XC_WRITE_COMMENT_LAST_XCODE("jmp if (B02) MCPX v1.0",
		XC_JNE, MCPX_IO_BAR_VAL, -1,
		XC_PCI_READ, MCPX_1_1_IO_BAR, 0);

	// mcpx v1.1 io bar
	XC_WRITE_COMMENT("read io bar (C03) MCPX v1.1", XC_PCI_READ, MCPX_1_1_IO_BAR, 0);
	XC_WRITE_COMMENT("set io bar (C03) MCPX v1.1", XC_PCI_WRITE, MCPX_1_1_IO_BAR, MCPX_IO_BAR_VAL);
	XC_WRITE_COMMENT_LAST_XCODE("jmp if (C03) MCPX v1.1",
		XC_JNE, MCPX_IO_BAR_VAL, -1,
		XC_PCI_READ, MCPX_1_0_IO_BAR, 0);

	// spin loop
	XC_WRITE_COMMENT_LAST_XCODE("spin until smbus is ready",
		XC_JNE, 0x10, _offset - (sizeof(XCODE) * 2),
		XC_IO_READ, SMB_BASE + 0x00, 0);

	XC_WRITE_COMMENT("disable the tco timer", XC_IO_WRITE, 0x8049, 0x08);
	XC_WRITE_COMMENT("KBDRSTIN# in gpio mode", XC_IO_WRITE, 0x80D9, 0);
	XC_WRITE_COMMENT("disable PWRBTN#", XC_IO_WRITE, 0x8026, 0x01);

	XC_WRITE_COMMENT_MASK("turn off secret rom", XC_PCI_WRITE, 0x80000880, 0xF000FFFF, 0x2);

	XC_WRITE_COMMENT("enable io space", XC_PCI_WRITE, 0x80000804, 0x03);

	XC_WRITE_COMMENT("enable internal graphics", XC_PCI_WRITE, 0x8000F04C, 0x01);
	XC_WRITE_COMMENT("setup secondary bus 1", XC_PCI_WRITE, 0x8000F018, 0x10100);
	XC_WRITE_COMMENT("smbus is bad, flatline clks", XC_PCI_WRITE, 0x8000036C, 0x1000000);

	XC_WRITE_COMMENT("set nv reg base", XC_PCI_WRITE, 0x80010010, NV2A_BASE);
	XC_WRITE_COMMENT("reload nv reg base", XC_PCI_WRITE, 0x8000F020, 0xFDF0FD00);

	// nv clk
	XC_WRITE_COMMENT("set nv clk 155 MHz ( rev == A1 )", XC_MEM_WRITE, NV2A_BASE + NV_CLK_REG, 0x11701);

	XC_WRITE_STATEMENT(XC_MEM_WRITE, NV2A_BASE + NV_CLK_REG, -1,
		uint32_t base = 16667;
		uint32_t nvclk = base * ((_ptr->data & 0xFF00) >> 8);
		nvclk /= 1 << ((_ptr->data & 0x70000) >> 16);
		nvclk /= _ptr->data & 0xFF; nvclk /= 1000;
		sprintf(str + strlen(str), "set nv clk %dMHz (@ %.3fMHz)", nvclk, (float)(base / 1000.00f)));

	// nv gpu revision
	XC_WRITE_COMMENT_NEXT_XCODE("get nv rev",
		XC_MEM_READ, NV2A_BASE, 0,
		XC_AND_OR, 0xFF, 0);

	XC_WRITE_COMMENT_LAST_2_XCODE("jmp if nv rev >= A2 ( >= DVT4 )",
		XC_JNE, 0xA1, -1,
		XC_AND_OR, 0xFF, 0,
		XC_MEM_READ, NV2A_BASE, 0);

	XC_WRITE_COMMENT_LAST_2_XCODE("jmp if nv rev < A2 ( < DVT4 )",
		XC_JNE, 0xA2, -1,
		XC_AND_OR, 0xFF, 0,
		XC_MEM_READ, NV2A_BASE, 0);

	// ROM pad
	XC_WRITE_COMMENT_NEXT_XCODE("configure pad for micron",
		XC_MEM_WRITE, NV2A_BASE + 0x1214, 0x28282828,
		XC_MEM_WRITE, NV2A_BASE + 0x122C, 0x88888888);

	XC_WRITE_COMMENT_NEXT_XCODE("configure pad for samsung",
		XC_MEM_WRITE, NV2A_BASE + 0x1214, 0x09090909,
		XC_MEM_WRITE, NV2A_BASE + 0x122C, 0xAAAAAAAA);

	XC_WRITE_STATEMENT(XC_PCI_WRITE, 0x80000084, -1,
		sprintf(str + strlen(str), "set memory size %d Mb", (_ptr->data + 1) / 1024 / 1024));

	XC_WRITE_COMMENT("set extbank bit (00000F00)", XC_MEM_WRITE, NV2A_BASE + 0x100200, 0x03070103);
	XC_WRITE_COMMENT("clear extbank bit (00000F00)", XC_MEM_WRITE, NV2A_BASE + 0x100200, 0x03070003);

	XC_WRITE_COMMENT("clear scratch pad (mem type)", XC_PCI_WRITE, 0x8000103C, 0);
	XC_WRITE_COMMENT("clear scratch pad (mem result)", XC_PCI_WRITE, 0x8000183C, 0);

	XC_WRITE_COMMENT_MASK("mem test pattern 1", XC_MEM_WRITE, 0x00555508, 0x00FFFF0F, MEMTEST_PATTERN1);
	XC_WRITE_COMMENT_MASK("mem test pattern 2", XC_MEM_WRITE, 0x00555508, 0x00FFFF0F, MEMTEST_PATTERN2);
	XC_WRITE_COMMENT_MASK("mem test pattern 3", XC_MEM_WRITE, 0x00555508, 0x00FFFF0F, MEMTEST_PATTERN3);

	XC_WRITE_COMMENT_MASK_LAST_XCODE("does dram exist?",
		XC_JNE, MEMTEST_PATTERN2, 0xFFFFFFFF, -1,
		XC_MEM_READ, 0x00555508, 0x00FFFF0F, 0);

	XC_WRITE_COMMENT_NEXT_XCODE("15ns delay by performing jmps",
		XC_JMP, 0, _offset,
		XC_JMP, 0, _offset + sizeof(XCODE));

	XC_WRITE_COMMENT_LAST_2_XCODE("don't gen INIT# on powercycle",
		XC_USE_RESULT, 0x04, MCPX_LEG_24,
		XC_AND_OR, 0xFFFFFFFF, 0x400,
		XC_PCI_READ, MCPX_LEG_24, 0);

	XC_WRITE_COMMENT("visor attack prep", XC_MEM_WRITE, 0x00000000, -1);
	XC_WRITE_COMMENT("TEA attack prep", XC_MEM_WRITE, 0x007fd588, -1);

	XC_WRITE_COMMENT("quit xcodes", XC_EXIT, 0x806, 0);

	return 0;
}

int ll(char* output, char* str, uint32_t i, uint32_t* j, uint32_t len, uint32_t m)
{
	if (str[i] != '{')
		return 1;

	uint32_t k = i + 1;
	while (k < len && str[k] != '}') {
		k++;
	}

	if (j != NULL) {
		*j = k;
	}

	if (str[k] != '}') {
		return ERROR_INVALID_DATA;
	}

	if (output != NULL) {
		memcpy(output, str + i, k - i + 1);
		output[k - i + 1] = '\0';
	}

	if (m > 0) {
		memmove(str + i + m, str + k + 1, len - k);
	}

	return 0;
}
int ll2(char* output, char* str, uint32_t i, uint32_t& j, uint32_t len)
{
	if (str[i] != '{') {
		j = i;
		
		while (j < len && str[j] != '{') {
			j++;
		}

		memcpy(output, str + i, j - i);
		output[j - i] = '\0';

		return 1;
	}

	j = i;
	while (j < len && str[j] != '}') {
		j++;
	}

	if (str[j] != '}') {
		return ERROR_INVALID_DATA;
	}

	uint32_t k = j;
	while (k + 1 < len && str[k + 1] != '{') {
		k++;
	}

	if (k + 1 < len) {
		j = k;
	}

	memcpy(output, str + i, j - i + 1);
	output[j - i + 1] = '\0';

	return 0;
}

const LOADINI_RETURN_MAP getDecodeSettingsMap()
{
	return cmap;
}
