// File.cpp: File handling functions

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#include "file.h"
#include "util.h"
#include "type_defs.h"
#include "xbmem.h"

UCHAR* readFile(const char* filename, UINT* bytesRead, const int expectedSize)
{
	FILE* file = NULL;
	UINT size = 0;

	if (filename == NULL)
	{
		error("Error: filename is NULL\n");
		return NULL;
	}

	file = fopen(filename, "rb");
	if (file == NULL)
	{
		error("Error: Could not open file: %s\n", filename);
		return NULL;
	}

	getFileSize(file, &size);

	if (expectedSize != -1 && size != expectedSize)
	{
		error("Error: Invalid file size. Expected %d bytes. Got %d bytes\n", expectedSize, size);
		fclose(file);
		return NULL;
	}

	UCHAR* data = (UCHAR*)xb_alloc(size);
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
	{
		error("Error: filename is NULL\n");
		return 1;
	}

	file = fopen(filename, "wb");
	if (file == NULL)
	{
		error("Error: Could not open file: %s\n", filename);
		return 1;
	}

	bytesWritten = fwrite(ptr, 1, bytesToWrite, file);
	fclose(file);

	return 0;
}

int getFileSize(FILE* file, UINT* fileSize)
{
	if (file == NULL)
	{
		return 1;
	}

	fseek(file, 0, SEEK_END);
	*fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	return 0;
}

int deleteFile(const char* filename)
{
	if (filename == NULL)
	{
		error("Error: filename is NULL\n");
		return 1;
	}

	if (remove(filename) != 0)
	{
		error("Error: Could not delete file: %s\n", filename);
		return 1;
	}

	return 0;
}
