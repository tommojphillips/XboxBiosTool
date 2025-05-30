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
#include <cstdint>
#include <cstdio>
#include <cstring>
#if !__APPLE__
#include <malloc.h>
#else
#include <cstdlib>
#endif

// user incl
#include "XcodeInterp.h"
#include "XcodeDecoder.h"
#include "util.h"
#include "str_util.h"
#include "loadini.h"

#ifdef MEM_TRACKING
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

#define XC_WRITE_COMMENT_NEXT_2_XCODE(comment, xc_cmd, xc_addr, xc_data, xc_prev_cmd, xc_prev_addr, xc_prev_data, xc_prev1_cmd, xc_prev1_addr, xc_prev1_data) \
	if (XC_WRITE(0, xc_cmd, xc_addr, xc_data) && XC_WRITE(1, xc_prev_cmd, xc_prev_addr, xc_prev_data) && XC_WRITE(2, xc_prev1_cmd, xc_prev1_addr, xc_prev1_data)) \
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
const LOADINI_SETTING settings_map[] = {
	{ "format_str", LOADINI_SETTING_TYPE_STR },
	{ "jmp_str", LOADINI_SETTING_TYPE_STR },
	{ "no_operand_str", LOADINI_SETTING_TYPE_STR },
	{ "num_str", LOADINI_SETTING_TYPE_STR, },
	{ "comment_prefix", LOADINI_SETTING_TYPE_STR},
	{ "label_on_new_line", LOADINI_SETTING_TYPE_BOOL },
	{ "pad", LOADINI_SETTING_TYPE_BOOL },
	{ "opcode_use_result", LOADINI_SETTING_TYPE_BOOL },

	{ xcode_opcode_map[0].str, LOADINI_SETTING_TYPE_STR },
	{ xcode_opcode_map[1].str, LOADINI_SETTING_TYPE_STR },
	{ xcode_opcode_map[2].str, LOADINI_SETTING_TYPE_STR },
	{ xcode_opcode_map[3].str, LOADINI_SETTING_TYPE_STR },
	{ xcode_opcode_map[4].str, LOADINI_SETTING_TYPE_STR },
	{ xcode_opcode_map[5].str, LOADINI_SETTING_TYPE_STR },
	{ xcode_opcode_map[6].str, LOADINI_SETTING_TYPE_STR },
	{ xcode_opcode_map[7].str, LOADINI_SETTING_TYPE_STR },
	{ xcode_opcode_map[8].str, LOADINI_SETTING_TYPE_STR },
	{ xcode_opcode_map[9].str, LOADINI_SETTING_TYPE_STR },
	{ xcode_opcode_map[10].str, LOADINI_SETTING_TYPE_STR },
	{ xcode_opcode_map[11].str, LOADINI_SETTING_TYPE_STR },
	{ xcode_opcode_map[12].str, LOADINI_SETTING_TYPE_STR },
	{ xcode_opcode_map[13].str, LOADINI_SETTING_TYPE_STR },
	{ xcode_opcode_map[14].str, LOADINI_SETTING_TYPE_STR }
};
const LOADINI_RETURN_MAP decode_settings_map = { settings_map, sizeof(settings_map), sizeof(settings_map) / sizeof(LOADINI_SETTING_MAP) };

static int ll(char* output, char* str, uint32_t i, uint32_t* j, uint32_t len, uint32_t m);
static int ll2(char* output, char* str, uint32_t i, uint32_t& j, uint32_t len);
static int createLabel(DECODE_CONTEXT* context, uint32_t offset, const char* label_format);
static int createJmp(DECODE_CONTEXT * context, uint32_t xcodeOffset, XCODE * xcode);
static int searchLabel(DECODE_CONTEXT* context, uint32_t offset, LABEL** label);
static int searchJmp(DECODE_CONTEXT* context, uint32_t offset, JMP_XCODE** jmp);
static void walkBranch(DECODE_CONTEXT * context, XcodeInterp * interp);

int XcodeDecoder::load(uint8_t* data, uint32_t size, uint32_t base, const char* ini) {
	// set up the xcode decoder.
	// load the interpreter, decode settings and context.
	// parse xcodes for labels.

	int result = 0;
	XCODE* xcode = NULL;
	LABEL* label = NULL;
	uint32_t labelArraySize = 0;
	uint32_t jmpArraySize = 0;
	uint32_t jmpCount = 0;
	static const char* label_format = "lb_%02d";

	result = interp.load(data + base, size - base);
	if (result != 0)
		return 1;

	context = createDecodeContext();
	if (context == NULL) {
		return ERROR_OUT_OF_MEMORY;
	}

	context->xcodeBase = base;
	context->jmpCount = 0;
	context->labelCount = 0;
	context->settings.labelMaxLen = 0;
	memset(context->str_decode, 0, sizeof(context->str_decode));

	result = loadSettings(ini, &context->settings);
	if (result != 0) {
		return result;
	}
	
	interp.reset();
	while (interp.interpretNext(xcode) == 0) {
		if (xcode->opcode == XC_JMP || xcode->opcode == XC_JNE) {
			jmpCount++;
		}
	}

	if (jmpCount > 0) {
		// initialize label array;
		labelArraySize = sizeof(LABEL) * jmpCount;
		context->labels = (LABEL*)malloc(labelArraySize);
		if (context->labels == NULL) {
			return ERROR_OUT_OF_MEMORY;
		}
		memset(context->labels, 0, labelArraySize);	

		// initialize jmp array;
		jmpArraySize = sizeof(JMP_XCODE) * jmpCount;
		context->jmps = (JMP_XCODE*)malloc(jmpArraySize);
		if (context->jmps == NULL) {
			return ERROR_OUT_OF_MEMORY;
		}
		memset(context->jmps, 0, jmpArraySize);
	}

	// create labels, jmps
	interp.reset();
	while (interp.interpretNext(xcode) == 0) {
		if (xcode->opcode == XC_JMP || xcode->opcode == XC_JNE) {
			
			// create a label
			if (context->labels != NULL) {
				if (searchLabel(context, interp.offset + xcode->data, &label) == 0) {
					label->references++;
				}
				else {
					createLabel(context, interp.offset + xcode->data, label_format);
				}
			}

			// create a jmp
			if (context->jmps != NULL) {
				createJmp(context, interp.offset - sizeof(XCODE), xcode);
			}
		}
	}

	// init label max size
	uint32_t lbi = context->labelCount;
	while (lbi > 0) {
		context->settings.labelMaxLen++;
		lbi /= 10;
	}
	context->settings.labelMaxLen += strlen(label_format) - 4; // 4 for %02d
	
	// xcode info
	context->xcodeSize = interp.offset;
	context->xcodeCount = interp.offset / sizeof(XCODE);

	return 0;
}
int XcodeDecoder::loadSettings(const char* ini, DECODE_SETTINGS* settings) const {
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
		{ &decode_settings_map.s[0], &settings->format_str},
		{ &decode_settings_map.s[1], &settings->jmp_str },
		{ &decode_settings_map.s[2], &settings->no_operand_str },
		{ &decode_settings_map.s[3], &settings->num_str },
		{ &decode_settings_map.s[4], &settings->comment_prefix},
		{ &decode_settings_map.s[5], &settings->label_on_new_line },
		{ &decode_settings_map.s[6], &settings->pad },
		{ &decode_settings_map.s[7], &settings->opcode_use_result },
		{ &decode_settings_map.s[8], &settings->opcodes[0].str },
		{ &decode_settings_map.s[9], &settings->opcodes[1].str },
		{ &decode_settings_map.s[10], &settings->opcodes[2].str },
		{ &decode_settings_map.s[11], &settings->opcodes[3].str },
		{ &decode_settings_map.s[12], &settings->opcodes[4].str },
		{ &decode_settings_map.s[13], &settings->opcodes[5].str },
		{ &decode_settings_map.s[14], &settings->opcodes[6].str },
		{ &decode_settings_map.s[15], &settings->opcodes[7].str },
		{ &decode_settings_map.s[16], &settings->opcodes[8].str },
		{ &decode_settings_map.s[17], &settings->opcodes[9].str },
		{ &decode_settings_map.s[18], &settings->opcodes[10].str },
		{ &decode_settings_map.s[19], &settings->opcodes[11].str },
		{ &decode_settings_map.s[20], &settings->opcodes[12].str },
		{ &decode_settings_map.s[21], &settings->opcodes[13].str },
		{ &decode_settings_map.s[22], &settings->opcodes[14].str }
	};

	if (ini != NULL) {
		stream = fopen(ini, "r");
		if (stream != NULL) { // only load ini file if it exists
			printf("settings file: %s\n", ini);
			result = loadini(stream, var_map, decode_settings_map.size);
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

		if (str == NULL) {
			result = 1;
			goto Cleanup;
		}
		
		// set default opcode string
		if (settings->opcodes[i].str == NULL) {
			settings->opcodes[i].str = (char*)malloc(strlen(str) + 1);
			if (settings->opcodes[i].str == NULL) {
				result = ERROR_OUT_OF_MEMORY;
				goto Cleanup;
			}
			strcpy((char*)settings->opcodes[i].str, str);
		}

		memcpy(&settings->opcodes[i].field, &opcode, sizeof(OPCODE));

		len = strlen(settings->opcodes[i].str);
		if (len > settings->opcodeMaxLen) {
			settings->opcodeMaxLen = len;
		}
	}

Cleanup:

	if (result == ERROR_INVALID_DATA) {
		printf("Error: key '%s' has invalid value '%s'\n", buf, value);
	}

	return result;
}
int XcodeDecoder::decodeXcodes() {
	int result;

	interp.reset();
	while (interp.interpretNext(context->xcode) == 0) {
		result = decode();
		if (result != 0) {
			if (result == ERROR_BUFFER_OVERFLOW) {
				printf("Error: decode format too large.\n");
			}
			else {
				printf("Error: decoding xcode:\n\t%04X, OP: %02X, ADDR: %04X, DATA: %04X\n",
					(context->xcodeBase + interp.offset - sizeof(XCODE)), context->xcode->opcode, context->xcode->addr, context->xcode->data);
			}
			return result;
		}
	}

	return 0;
}
int XcodeDecoder::decode() {
	uint32_t len;
	uint32_t fmt_len;
	uint32_t op_len = 0;
	uint32_t operand_len = 0;
	uint32_t no_operand_len = 0;
	uint32_t jmp_len = 0;
	LABEL* label;
	JMP_XCODE* jmp;

	char str[64] = {};
	char str_tmp[64] = {};

	// branch checks
	if (context->branch) {
		if (context->xcode->opcode == XC_JNE) {
			walkBranch(context, &interp);
		}
		else if (context->xcode->opcode == XC_JMP) {
			if (searchJmp(context, interp.offset - sizeof(XCODE), &jmp) == 0) {
				switch (jmp->branchable) {
					case JMP_XCODE_BRANCHABLE:
						walkBranch(context, &interp);
						break;
					case JMP_XCODE_NOT_BRANCHABLE:
						fprintf(context->stream, "; took unbranchable jmp!!\n");
						interp.offset += context->xcode->data;
						break;
				}
			}
		}
	}

	// output label
	if (searchLabel(context, interp.offset - sizeof(XCODE), &label) == 0) {
		label->defined = true;
		sprintf(str, "%s:", label->name);
		if (context->settings.label_on_new_line)
			strcat(str, "\n");
	}

	if (!context->settings.label_on_new_line)
		rpad(str, context->settings.labelMaxLen + 2 + 1, ' '); // 2 for ': '
	else
		strcat(str, "\t");
	strcpy(context->str_decode, str);
	str[0] = '\0';

	// output prefix
	if (context->settings.prefix_str != NULL) {
		strcat(context->str_decode, context->settings.prefix_str);
	}

	// output decode format; loop to find the seq of the format.
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

			// append part of the decode str depending on the seq.
			switch (context->settings.format_map[j].type) {
				
				// output OFFSET
				case DECODE_FIELD_OFFSET: {
					sprintf(str_tmp, context->settings.format_map[j].str, "%04x");
					sprintf(str, str_tmp, context->xcodeBase + interp.offset - sizeof(XCODE));
				} break;

				// output OPCODE
				case DECODE_FIELD_OPCODE: {
					const char* str_opcode;
					if (getOpcodeStr(context->settings.opcodes, context->xcode->opcode, str_opcode) != 0)
						return 1;
					sprintf(str, context->settings.format_map[j].str, str_opcode);
					if (context->settings.pad) {
						rpad(str, op_len, ' ');
					}
				} break;

				// output ADDRESS
				case DECODE_FIELD_ADDRESS: {
					switch (context->xcode->opcode) {
						case XC_JMP: {
							if (context->settings.no_operand_str != NULL) {
								strcpy(str_tmp, context->settings.no_operand_str);
								break;
							}

							if (searchLabel(context, interp.offset + context->xcode->data, &label) == 0) {
								sprintf(str_tmp, context->settings.jmp_str, label->name);
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
						} // fall through to default

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
				} break;

				// output DATA
				case DECODE_FIELD_DATA: {
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
							if (context->settings.no_operand_str != NULL) {
								if (searchLabel(context, interp.offset + context->xcode->data, &label) == 0) {
									sprintf(str_tmp, context->settings.jmp_str, label->name);
								}
							}
							break;

						case XC_JNE:							
							if (searchLabel(context, interp.offset + context->xcode->data, &label) == 0) {
								sprintf(str_tmp, context->settings.jmp_str, label->name);
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
				} break;

				// output COMMENT
				case DECODE_FIELD_COMMENT: {
					uint32_t prefixLen = strlen(context->settings.comment_prefix);
					getCommentStr(str_tmp + prefixLen);
					if (str_tmp[prefixLen] != '\0') {
						memcpy(str_tmp, context->settings.comment_prefix, prefixLen);
					}
					sprintf(str, context->settings.format_map[j].str, str_tmp);
				} break;
			}

			if (len + strlen(str) > sizeof(context->str_decode)) {
				return ERROR_BUFFER_OVERFLOW;
			}

			strcpy(context->str_decode + len, str);
			break;
		}
	}

	if (context->stream != NULL) {
		fprintf(context->stream, "%s\n", context->str_decode);
	}

	return 0;
}
int XcodeDecoder::getCommentStr(char* str) {
	// -1 = dont care about field

	XCODE* _ptr = interp.ptr;
	uint8_t* _data = interp.data;

	XC_WRITE_COMMENT("smbus read status", XC_IO_READ, SMB_BASE + 0x00, -1);
	XC_WRITE_COMMENT("smbus clear status\n", XC_IO_WRITE, SMB_BASE + 0x00, 0x10);

	XC_WRITE_COMMENT("smbus read revision register", XC_IO_WRITE, SMB_CMD_REGISTER, 0x01);

	// CX871
	XC_WRITE_COMMENT("CX871 slave address", XC_IO_WRITE, SMB_BASE + 0x04, 0x8A);
	XC_WRITE_COMMENT_NEXT_XCODE("CX871 0xBA = 0x3F",
		XC_IO_WRITE, SMB_CMD_REGISTER, 0xBA,
		XC_IO_WRITE, SMB_VAL_REGISTER, 0x3F);
	XC_WRITE_COMMENT_NEXT_XCODE("CX871 0x6C = 0x46",
		XC_IO_WRITE, SMB_CMD_REGISTER, 0x6C,
		XC_IO_WRITE, SMB_VAL_REGISTER, 0x46);
	XC_WRITE_COMMENT_NEXT_XCODE("CX871 0xB8 = 0x00",
		XC_IO_WRITE, SMB_CMD_REGISTER, 0xB8,
		XC_IO_WRITE, SMB_VAL_REGISTER, 0x00);
	XC_WRITE_COMMENT_NEXT_XCODE("CX871 0xCE = 0x19",
		XC_IO_WRITE, SMB_CMD_REGISTER, 0xCE,
		XC_IO_WRITE, SMB_VAL_REGISTER, 0x19);
	XC_WRITE_COMMENT_NEXT_XCODE("CX871 0xC6 = 0x9C",
		XC_IO_WRITE, SMB_CMD_REGISTER, 0xC6,
		XC_IO_WRITE, SMB_VAL_REGISTER, 0x9C);
	XC_WRITE_COMMENT_NEXT_XCODE("CX871 0x32 = 0x08",
		XC_IO_WRITE, SMB_CMD_REGISTER, 0x32,
		XC_IO_WRITE, SMB_VAL_REGISTER, 0x08);
	XC_WRITE_COMMENT_NEXT_XCODE("CX871 0xC4 = 0x01",
		XC_IO_WRITE, SMB_CMD_REGISTER, 0xC4,
		XC_IO_WRITE, SMB_VAL_REGISTER, 0x01);

	// focus
	XC_WRITE_COMMENT("focus slave address", XC_IO_WRITE, SMB_BASE + 0x04, 0xD4);

	// xcalibur
	XC_WRITE_COMMENT("xcalibur slave address", XC_IO_WRITE, SMB_BASE + 0x04, 0xE1);
	
	XC_WRITE_COMMENT_NEXT_XCODE("report memory type",
		XC_IO_WRITE, SMB_BASE + 0x04, 0x20,
		XC_IO_WRITE, SMB_CMD_REGISTER, 0x13);

	XC_WRITE_COMMENT("smbus set cmd", XC_IO_WRITE, SMB_CMD_REGISTER, -1);
	XC_WRITE_COMMENT("smbus set val", XC_IO_WRITE, SMB_VAL_REGISTER, -1);

	XC_WRITE_COMMENT("smc slave write address", XC_IO_WRITE, SMB_BASE + 0x04, 0x20);
	XC_WRITE_COMMENT("smc slave read address", XC_IO_WRITE, SMB_BASE + 0x04, 0x21);

	XC_WRITE_COMMENT("smbus kickoff", XC_IO_WRITE, SMB_BASE + 0x02, 0x0A);

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
		XC_JNE, 0x10, -18,
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
	XC_WRITE_COMMENT("set nv clk 155 MHz", XC_MEM_WRITE, NV2A_BASE + NV_CLK_REG, 0x11701);

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

	XC_WRITE_COMMENT_LAST_2_XCODE("if nv rev != A2",
		XC_JNE, 0xA1, -1,
		XC_AND_OR, 0xFF, 0,
		XC_MEM_READ, NV2A_BASE, 0);

	XC_WRITE_COMMENT_LAST_2_XCODE("if nv rev != A1",
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

	XC_WRITE_COMMENT_NEXT_2_XCODE("memory pad configuration",
		XC_MEM_WRITE, NV2A_BASE + 0x1230, 0xFFFFFFFF,
		XC_MEM_WRITE, NV2A_BASE + 0x1234, 0xAAAAAAAA,
		XC_MEM_WRITE, NV2A_BASE + 0x1238, 0xAAAAAAAA);

	XC_WRITE_STATEMENT(XC_PCI_WRITE, 0x80000084, -1,
		sprintf(str + strlen(str), "set memory size %d Mb\n", (_ptr->data + 1) / 1024 / 1024));

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
		XC_JMP, 0, 0,
		XC_JMP, 0, 0);

	XC_WRITE_COMMENT_LAST_2_XCODE("don't gen INIT# on powercycle",
		XC_USE_RESULT, 0x04, MCPX_LEG_24,
		XC_AND_OR, 0xFFFFFFFF, 0x400,
		XC_PCI_READ, MCPX_LEG_24, 0);

	XC_WRITE_COMMENT("visor attack prep", XC_MEM_WRITE, 0x00000000, -1);
	XC_WRITE_COMMENT("TEA attack prep", XC_MEM_WRITE, 0x007fd588, -1);

	XC_WRITE_COMMENT("ctrim_A1", XC_MEM_WRITE, 0x0f0010b0, 0x07633451);
	XC_WRITE_COMMENT("ctrim_A2", XC_MEM_WRITE, 0x0f0010b0, 0x07633461);
	XC_WRITE_COMMENT("set ctrim2 ( samsung )", XC_MEM_WRITE, 0x0f0010b8, 0xFFFF0000);
	XC_WRITE_COMMENT("set ctrim2 ( micron )", XC_MEM_WRITE, 0x0f0010b8, 0xEEEE0000);
	XC_WRITE_COMMENT("ctrim continue", XC_MEM_WRITE, 0x0f0010d4, 0x9);
	XC_WRITE_COMMENT("ctrim common", XC_MEM_WRITE, 0x0f0010b4, 0x0);
	
	XC_WRITE_COMMENT("pll_select", XC_MEM_WRITE, 0x0f68050c, 0x000a0400);

	XC_WRITE_COMMENT("quit xcodes", XC_EXIT, 0x806, 0);

	return 0;
}

static int ll(char* output, char* str, uint32_t i, uint32_t* j, uint32_t len, uint32_t m) {
	// remove {entry} from str;
	// output: output buffer
	// str: input string
	// i: start index
	// j: end index
	// len: length of input string
	// m: max length of output buffer
	// return 0 if found, 1 if not found, -1 if error

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
static int ll2(char* output, char* str, uint32_t i, uint32_t& j, uint32_t len) {
	// output: output buffer
	// str: input string
	// i: start index
	// j: end index
	// len: length of input string
	// return 0 if found, 1 if not found, -1 if error

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
static void walkBranch(DECODE_CONTEXT* context, XcodeInterp* interp) {
	// walk the branch and mark jmps as branchable.

	XCODE* xc = NULL;
	JMP_XCODE* jmp = NULL;
	uint32_t offset = 0;
	XCODE* ptr = NULL;
	XcodeInterp::INTERP_STATUS status;
	uint32_t jmpOffset = 0;
	uint32_t endOffset = 0;

	// save off interp state.
	offset = interp->offset;
	ptr = interp->ptr;
	status = interp->status;
	jmpOffset = offset + context->xcode->data;

	//fprintf(context->stream, "; walking branch\n");
	
	if (jmpOffset > offset) {
		// jmp offset is below; 
		// walk from jmp-definition to jmp-offset

		endOffset = jmpOffset;
	}
	else {
		// jmp offset is above or equal; 
		// walk from jmp-offset to jmp-definition

		interp->offset = jmpOffset;
		endOffset = offset;
	}

	while (interp->interpretNext(xc) == 0) {
		if (interp->offset - sizeof(XCODE) == endOffset)
			break;

		if (xc->opcode == XC_JMP) {
			if (searchJmp(context, interp->offset - sizeof(XCODE), &jmp) == 0) {
				jmp->branchable = JMP_XCODE_BRANCHABLE;
				jmp->xcode = xc;
			}
		}
	}

	// restore interp state.
	interp->offset = offset;
	interp->ptr = ptr;
	interp->status = status;
}
static int createJmp(DECODE_CONTEXT* context, uint32_t xcodeOffset, XCODE* xcode) {
	// create a jmp; add to jmp count.

	JMP_XCODE* jmp = &context->jmps[context->jmpCount];
	jmp->branchable = JMP_XCODE_NOT_BRANCHABLE;
	jmp->xcodeOffset = xcodeOffset;
	jmp->xcode = xcode;
	context->jmpCount++;
	return 0;
}
static int searchJmp(DECODE_CONTEXT* context, uint32_t offset, JMP_XCODE** jmp) {
	// search for jmp by xcode offset.

	if (jmp == NULL)
		return 1;
	for (uint32_t i = 0; i < context->jmpCount; i++) {
		JMP_XCODE* jm = &context->jmps[i];
		if (jm->xcodeOffset == offset) {
			*jmp = jm;
			return 0;
		}
	}

	*jmp = NULL;
	return 1;
}
static int createLabel(DECODE_CONTEXT* context, uint32_t offset, const char* label_format) {
	// create a label; add to label count.
	LABEL* label = &context->labels[context->labelCount];
	sprintf(label->name, label_format, context->labelCount);
	label->offset = offset;
	label->references = 1;
	label->defined = false;
	context->labelCount++;
	return 0;
}
static int searchLabel(DECODE_CONTEXT* context, uint32_t offset, LABEL** label) {
	// search for label by xcode offset.

	if (label == NULL)
		return 1;
	for (uint32_t i = 0; i < context->labelCount; i++) {
		LABEL* lb = &context->labels[i];
		if (lb->offset == offset) {
			*label = lb;
			return 0;
		}
	}
	*label = NULL;
	return 1;
}

void initDecodeSettings(DECODE_SETTINGS* settings) {
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
}
DECODE_SETTINGS* createDecodeSettings() {
	DECODE_SETTINGS* settings = (DECODE_SETTINGS*)malloc(sizeof(DECODE_SETTINGS));
	if (settings == NULL)
		return NULL;
	initDecodeSettings(settings);
	return settings;
}
void destroyDecodeSettings(DECODE_SETTINGS* settings) {
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
}
void initDecodeContext(DECODE_CONTEXT* context) {
	memset(context, 0, sizeof(DECODE_CONTEXT));
	context->branch = false;
	initDecodeSettings(&context->settings);
}
DECODE_CONTEXT* createDecodeContext() {
	DECODE_CONTEXT* context = (DECODE_CONTEXT*)malloc(sizeof(DECODE_CONTEXT));
	if (context == NULL)
		return NULL;
	initDecodeContext(context);
	return context;
}
void destroyDecodeContext(DECODE_CONTEXT* context) {
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
}