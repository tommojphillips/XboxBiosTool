// lzx.h

#ifndef XB_BIOS_TOOLS_LXZ_H
#define XB_BIOS_TOOLS_LXZ_H

#include "type_defs.h"

int decompress(const UCHAR* data, const UINT size, UCHAR*& buff, UINT& buffSize, UINT& decompressedSize);

#endif