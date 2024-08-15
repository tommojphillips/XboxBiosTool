// rsa.cpp:

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
#include <cstdio>
#include <memory.h>

// user incl
#include "rsa.h"
#include "type_defs.h"

PUBLIC_KEY* rsaVerifyPublicKey(UCHAR* data)
{
	// verify the public key header.

	static const RSA_HEADER RSA1_HEADER = { { 'R', 'S', 'A', '1' }, 264, 2048, 255, 65537 };

	if (memcmp(data, &RSA1_HEADER, sizeof(RSA_HEADER)) != 0)
	{
		return NULL;
	}

	return (PUBLIC_KEY*)data;
}

int rsaPrintPublicKey(PUBLIC_KEY* pubkey)
{
	const int bytesPerLine = 16;
	char line[bytesPerLine * 3 + 1] = { 0 };

	for (int i = 0; i < sizeof(pubkey->modulus); i += bytesPerLine)
	{
		for (UINT j = 0; j < bytesPerLine; j++)
		{
			if (i + j >= sizeof(pubkey->modulus))
				break;
			sprintf(line + (j * 3), "%02X ", pubkey->modulus[i + j]);
		}
		printf("%s\n", line);
	}

	return 0;
}
