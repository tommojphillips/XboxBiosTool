//  sha1.h

// GitHub: https://github.com/XboxDev/xbedump
// 
// credit to XboxDev, and the authors of xbedump
// Michael Steil (mist64)
// Franz

#ifndef SHA1_H
#define SHA1_H

#include <stdint.h>

#define SHA1_DIGEST_LEN 20

/* error codes */
#define SHA_STATUS_SUCCESS 0
#define SHA_STATUS_STATE_NULL 1
#define SHA_STATUS_INPUT_TOO_LONG 2
#define SHA_STATUS_STATE_ERROR 3

// SHA-1 context
typedef struct _SHA1Context {
    uint32_t intermediate_hash[SHA1_DIGEST_LEN / 4U]; // Digest

    uint32_t length_low;    // Message length in bits
    uint32_t length_high;

    short block_index;    // Index into message block array
    uint8_t block[64];    // 512-bit message blocks

    int computed;       // Is the digest computed?
    int corrupted;      // Is the message digest corrupted?
} SHA1Context;

#ifdef __cplusplus
extern "C" {
#endif

int SHA1Reset(SHA1Context* context);
int SHA1Input(SHA1Context* context, const uint8_t* message, uint32_t len);
int SHA1Result(SHA1Context* context, uint8_t digest[SHA1_DIGEST_LEN]);

#ifdef __cplusplus
};
#endif

#endif // _SHA1_H_
