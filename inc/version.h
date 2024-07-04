// version.h

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef XB_BIOS_TOOL_VERSION_H
#define XB_BIOS_TOOL_VERSION_H

#define STRIFY(x) #x
#define STR(x) STRIFY(x)

#define XB_BIOS_TOOL_VER_MAJOR 0
#define XB_BIOS_TOOL_VER_MINOR 1
#define XB_BIOS_TOOL_VER_PATCH 0
#define XB_BIOS_TOOL_VER_BUILD 5

#define XB_BIOS_TOOL_AUTHOR_STR "tommojphillips"

#define XB_BIOS_TOOL_VER \
		XB_BIOS_TOOL_VER_MAJOR, \
		XB_BIOS_TOOL_VER_MINOR, \
		XB_BIOS_TOOL_VER_PATCH, \
		XB_BIOS_TOOL_VER_BUILD

#define XB_BIOS_TOOL_VER_STR \
		STR(XB_BIOS_TOOL_VER_MAJOR) "." \
		STR(XB_BIOS_TOOL_VER_MINOR) "." \
		STR(XB_BIOS_TOOL_VER_PATCH) "." \
		STR(XB_BIOS_TOOL_VER_BUILD)

#define XB_BIOS_TOOL_HEADER_STR "XbTool v" \
								XB_BIOS_TOOL_VER_STR " by " \
								XB_BIOS_TOOL_AUTHOR_STR ". " \
								"Bulit: " \
								__DATE__ " at " \
								__TIME__ "\n\n"

#endif // !XB_BIOS_TOOL_VERSION_H
