// File.c: File handling functions

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

#include <stdint.h>
#include <stdbool.h>
#if !__APPLE__
#include <malloc.h>
#else
#include <stdlib.h>
#endif

#include "file.h"
#include "posix_shims.h"

#ifdef MEM_TRACKING
#include "mem_tracking.h"
#endif

uint8_t* readFile(const char* filename, uint32_t* bytesRead, const uint32_t expectedSize) {
	FILE* file = NULL;
	uint32_t size = 0;

	if (filename == NULL)
		return NULL;

	fopen_s(&file, filename, "rb");
	if (file == NULL) {
		printf("Error: could not open file: %s\n", filename);
		return NULL;
	}

	getFileSize(file, &size);

	if (expectedSize != 0 && size != expectedSize) {
		printf("Error: invalid file size. Expected %u bytes. Got %u bytes\n", expectedSize, size);
		fclose(file);
		return NULL;
	}

	uint8_t* data = (uint8_t*)malloc(size);
	if (data != NULL) {
		fread(data, 1, size, file);
		if (bytesRead != NULL) {
			*bytesRead = size;
		}
	}
	fclose(file);

	return data;
}

int writeFile(const char* filename, void* ptr, const uint32_t bytesToWrite) {
	FILE* file = NULL;
	uint32_t bytesWritten = 0;

	if (filename == NULL)
		return 1;

	fopen_s(&file, filename, "wb");
	if (file == NULL) {
		printf("Error: Could not open file: %s\n", filename);
		return 1;
	}

	bytesWritten = fwrite(ptr, 1, bytesToWrite, file);
	fclose(file);

	return 0;
}
int writeFileF(const char* filename, const char* tag, void* ptr, const uint32_t bytesToWrite) {
	static const char SUCCESS_OUT[] = "Writing %s to %s ( %.2f %s )\n";
	static const char FAIL_OUT[] = "Error: Failed to write %s\n";
	static const char* units[] = { "bytes", "kb", "mb", "gb" };

	int result;
	float bytesF;
	const char* sizeSuffix;
	
	bytesF = (float)bytesToWrite;
	result = 0;
	while (bytesF > 1024.0f && result < (sizeof(units) / sizeof(char*)) - 1) {
		bytesF /= 1024.0f;
		result++;
	}
	sizeSuffix = units[result];

	result = writeFile(filename, ptr, bytesToWrite);
	if (result == 0) {
		printf(SUCCESS_OUT, tag, filename, bytesF, sizeSuffix);
	}
	else {
		printf(FAIL_OUT, filename);
	}

	return result;
}

int getFileSize(FILE* file, uint32_t* fileSize) {
	if (file == NULL)
		return 1;

	fseek(file, 0, SEEK_END);
	*fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	return 0;
}
bool fileExists(const char* filename) {
	FILE* file = NULL;

	if (filename == NULL)
		return false;

	fopen_s(&file, filename, "rb");
	if (file == NULL)
		return false;
	fclose(file);
	return true;
}

int deleteFile(const char* filename) {
	if (filename == NULL)
		return 1;

	if (!fileExists(filename))
		return 0;

	if (remove(filename) != 0)
		return 1;

	return 0;
}
