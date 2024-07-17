//  sha1.h

#ifndef SHA1_H
#define SHA1_H

#include "type_defs.h"

enum {
    SHA_STATUS_SUCCESS = 0,     // Success
    SHA_STATUS_STATE_NULL,
    SHA_STATUS_INPUT_TOO_LONG,  // Data too long
    SHA_STATUS_STATE_ERROR,
};

// SHA-1 context
typedef struct _SHA1Context
{
    UINT intermediate_hash[DIGEST_LEN / 4U]; // Digest

    UINT length_low;    // Message length in bits
    UINT length_high;

    short block_index;  // Index into message block array
    UCHAR block[64];    // 512-bit message blocks

    int computed;       // Is the digest computed?
    int corrupted;      // Is the message digest corrupted?
} SHA1Context;

int SHA1Reset(SHA1Context* context);
int SHA1Input(SHA1Context* context, const UCHAR* message, UINT len);
int SHA1Result(SHA1Context* context, UCHAR digest[DIGEST_LEN]);

#endif // _SHA1_H_
