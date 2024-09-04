// File.cpp: File handling functions

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

#include "file.h"
#include "type_defs.h"

#ifdef MEM_TRACKING
#include "mem_tracking.h"
#else
#include <malloc.h>
#endif

UCHAR* readFile(const char* filename, UINT* bytesRead, const UINT expectedSize)
{
	FILE* file = NULL;
	UINT size = 0;

	if (filename == NULL)
		return NULL;

	file = fopen(filename, "rb");
	if (file == NULL)
	{
		printf("Error: Could not open file: %s\n", filename);
		return NULL;
	}

	getFileSize(file, &size);

	if (expectedSize != 0 && size != expectedSize)
	{
		printf("Error: Invalid file size. Expected %d bytes. Got %d bytes\n", expectedSize, size);
		fclose(file);
		return NULL;
	}

	UCHAR* data = (UCHAR*)malloc(size);
	if (data != NULL)
	{
		fread(data, 1, size, file);
		if (bytesRead != NULL) // only set bytesRead if not NULL
		{
			*bytesRead = size;
		}
	}
	fclose(file);

	return data;
}

int writeFile(const char* filename, void* ptr, const UINT bytesToWrite)
{
	FILE* file = NULL;
	UINT bytesWritten = 0;

	if (filename == NULL)
		return 1;

	file = fopen(filename, "wb");
	if (file == NULL)
	{
		printf("Error: Could not open file: %s\n", filename);
		return 1;
	}

	bytesWritten = fwrite(ptr, 1, bytesToWrite, file);
	fclose(file);

	return 0;
}
int writeFileF(const char* filename, void* ptr, const UINT bytesToWrite, const char* name)
{
	static const char SUCCESS_OUT[] = "Wrote %s ( %.2f %s )\n";
	static const char FAIL_OUT[] = "Error failed to write %s\n";
	static const char WRITE_OUT[] = "Writing %s to %s\n";

	static const char* units[] = { "bytes", "kb", "mb", "gb" };
	//static const char* units[] = { "bytes", "kb", };

	int result;
	float bytesF;
	const char* sizeSuffix;
	
	bytesF = (float)bytesToWrite;
	result = 0;
	while (bytesF > 1024.0f && result < (sizeof(units) / sizeof(char*)) - 1)
	{
		bytesF /= 1024.0f;
		result++;
	}
	sizeSuffix = units[result];
	
	printf(WRITE_OUT, name, filename);
	result = writeFile(filename, ptr, bytesToWrite);
	if (result == 0)
	{
		printf(SUCCESS_OUT, filename, bytesF, sizeSuffix);
	}
	else
	{
		printf(FAIL_OUT, name);
	}
	return result;
}

int getFileSize(FILE* file, UINT* fileSize)
{
	if (file == NULL)
		return 1;

	fseek(file, 0, SEEK_END);
	*fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	return 0;
}
bool fileExists(const char* filename)
{
	FILE* file = NULL;

	if (filename == NULL)
		return false;

	file = fopen(filename, "rb");
	if (file == NULL)
		return false;
	fclose(file);
	return true;
}

int deleteFile(const char* filename)
{
	if (filename == NULL)
		return 1;

	if (!fileExists(filename))
		return 0;

	if (remove(filename) != 0)
		return 1;

	return 0;
}
