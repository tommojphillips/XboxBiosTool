// rsa.c:

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
#include <memory.h>

// user incl
#include "rsa.h"

int rsa_verifyPublicKey(uint8_t* data, uint32_t size, uint32_t offset, PUBLIC_KEY** pubkey)
{
	// verify the public key header.

	static const char RSA1_MAGIC[4] = { 'R', 'S', 'A', '1' };
	RSA_HEADER* rsa_header;
	uint32_t pubkey_size;

	if (sizeof(RSA_HEADER) > size || offset + sizeof(RSA_HEADER) > size) // header over run
		return RSA_ERROR_INVALID_DATA;

	rsa_header = (RSA_HEADER*)(data + offset);
	pubkey_size = RSA_PUBKEY_SIZE(rsa_header);

	if (pubkey_size > size || offset + pubkey_size > size) // mod over run
		return RSA_ERROR;

	if (rsa_header->mod_size == 0 || rsa_header->bits == 0 || rsa_header->max_bytes == 0 || rsa_header->exponent == 0)
		return RSA_ERROR;

	if (rsa_header->bits % 8)
		return RSA_ERROR;

	if (RSA_MOD_SIZE(rsa_header) > rsa_header->mod_size)
		return RSA_ERROR;

	if (memcmp(rsa_header->magic, &RSA1_MAGIC, 4) != 0) // no match
		return RSA_ERROR;

	if (pubkey != NULL)
		*pubkey = (PUBLIC_KEY*)(rsa_header);

	return RSA_ERROR_SUCCESS;
}
int rsa_findPublicKey(uint8_t* data, uint32_t size, PUBLIC_KEY** pubkey, uint32_t* offset)
{
	size_t i;
	int result = 0;
	if (data == NULL || pubkey == NULL)
		return RSA_ERROR_INVALID_DATA;

	for (i = 0; i < size - sizeof(RSA_HEADER); ++i) {
		result = rsa_verifyPublicKey(data, size, i, pubkey);
		if (result != RSA_ERROR)
			break;
	}

	if (offset != NULL)
		*offset = i;

	return result;
}
