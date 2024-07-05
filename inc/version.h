// version.h

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

#ifndef XB_BIOS_TOOL_VERSION_H
#define XB_BIOS_TOOL_VERSION_H

#define STRIFY(x) #x
#define STR(x) STRIFY(x)

#define XB_BIOS_TOOL_VER_MAJOR 0
#define XB_BIOS_TOOL_VER_MINOR 1
#define XB_BIOS_TOOL_VER_PATCH 0
#define XB_BIOS_TOOL_VER_BUILD 6

#define XB_BIOS_TOOL_AUTHOR_STR "tommojphillips"
#define XB_BIOS_TOOL_NAME_STR "XbTool"

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

#define XB_BIOS_TOOL_HEADER_STR XB_BIOS_TOOL_NAME_STR " v" \
								XB_BIOS_TOOL_VER_STR " by " \
								XB_BIOS_TOOL_AUTHOR_STR ". " \
								"Bulit: " \
								__DATE__ " at " \
								__TIME__ "\n\n"

#endif // !XB_BIOS_TOOL_VERSION_H
