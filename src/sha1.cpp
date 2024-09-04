// sha1.cpp: defines functions for SHA-1 hashing

// GitHub: https://github.com/XboxDev/xbedump
// License: ??
// 
// credit to XboxDev, and the authors of xbedump
// Michael Steil (mist64)
// Franz

/*  sha1.cpp: defines functions for SHA-1 hashing
 *
 *  Description:
 *      This file implements the Secure Hashing Algorithm 1 as
 *      defined in FIPS PUB 180-1 published April 17, 1995.
 *
 *      The SHA-1, produces a 160-bit message digest for a given
 *      data stream.  It should take about 2**n steps to find a
 *      message with the same digest as a given message and
 *      2**(n/2) to find any two messages with the same digest,
 *      when n is the digest size in bits.  Therefore, this
 *      algorithm can serve as a means of providing a
 *      "fingerprint" for a message.
 *
 *  Portability Issues:
 *      SHA-1 is defined in terms of 32-bit "words".  This code
 *      uses <stdint.h> (included via "sha1.h" to define 32 and 8
 *      bit unsigned integer types.  If your C compiler does not
 *      support 32 bit unsigned integers, this code is not
 *      appropriate.
 *
 *  Caveats:
 *      SHA-1 is designed to work with messages less than 2^64 bits
 *      long.  Although SHA-1 allows a message digest to be generated
 *      for messages of any number of bits less than 2^64, this
 *      implementation only works with messages with a length that is
 *      a multiple of the size of an 8-bit character.
 */

#include "sha1.h"

#define SHA1CircularShift(bits,word) (((word) << (bits)) | ((word) >> (32-(bits))))

void SHA1PadMessage(SHA1Context*); 
void SHA1ProcessMessageBlock(SHA1Context*);

/*
 *  SHA1Reset
 *
 *  Description:
 *      This function will initialize the SHA1Context in preparation
 *      for computing a new SHA1 message digest.
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to reset.
 *
 *  Returns:
 *      sha Error Code.
 */
int SHA1Reset(SHA1Context* context)
{
    if (!context)
    {
        return SHA_STATUS_STATE_NULL;
    }

    context->length_low  = 0;
    context->length_high = 0;
    context->block_index = 0;

    context->intermediate_hash[0] = 0x67452301;
    context->intermediate_hash[1] = 0xEFCDAB89;
    context->intermediate_hash[2] = 0x98BADCFE;
    context->intermediate_hash[3] = 0x10325476;
    context->intermediate_hash[4] = 0xC3D2E1F0;

    context->computed = 0;
    context->corrupted = 0;

    return SHA_STATUS_SUCCESS;
}

/*
 *  SHA1Result
 *
 *  Description:
 *      This function will return the 160-bit message digest into the
 *      Message_Digest array  provided by the caller.
 *      NOTE: The first octet of hash is stored in the 0th element,
 *            the last octet of hash in the 19th element.
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to use to calculate the SHA-1 hash.
 *      Message_Digest: [out]
 *          Where the digest is returned.
 *
 *  Returns:
 *      sha Error Code.
 */
int SHA1Result(SHA1Context* context, UCHAR digest[SHA1_DIGEST_LEN])
{
    int i;

    if (!context || !digest)
    {
        return SHA_STATUS_STATE_NULL;
    }

    if (context->corrupted)
    {
        return context->corrupted;
    }

    if (!context->computed)
    {
        SHA1PadMessage(context);
        for(i=0; i<64; ++i)
        {
            context->block[i] = 0;
        }

        context->length_low = 0;
        context->length_high = 0;
        context->computed = 1;

    }

    for(i = 0; i < SHA1_DIGEST_LEN; ++i)
    {
        digest[i] = (UCHAR)(context->intermediate_hash[i >> 2] >> 8 * (3 - (i & 0x03)));
    }

    return SHA_STATUS_SUCCESS;
}

/*  SHA1Input
 * 
 *  Description:
 *      This function accepts an array of octets as the next portion
 *      of the message.
 *
 *  Parameters:
 *      context: [in/out]
 *          The SHA context to update
 *      message_array: [in]
 *          An array of characters representing the next portion of
 *          the message.
 *      length: [in]
 *          The length of the message in message_array
 *
 *  Returns:
 *      sha Error Code.
 */
int SHA1Input(SHA1Context* context, const UCHAR *message, UINT len)
{
    if (!len) return SHA_STATUS_SUCCESS;

    if (!context || !message)  return SHA_STATUS_STATE_NULL;

    if (context->computed)  
    {
        context->corrupted = SHA_STATUS_STATE_ERROR;
        return SHA_STATUS_STATE_ERROR;
    }

    if (context->corrupted)  return context->corrupted;
    
    while(len-- && !context->corrupted)
    {
        if (context->block_index >= 64)
        {
            context->corrupted = SHA_STATUS_INPUT_TOO_LONG;
            return SHA_STATUS_INPUT_TOO_LONG;
        }

    	context->block[context->block_index] = (*message & 0xFF);

        context->block_index++;

        context->length_low += 8;
    	
    	if (context->length_low == 0)
        {
        	context->length_high++;
        	
        	if (context->length_high == 0) // Message is too long
            {
                context->corrupted = SHA_STATUS_INPUT_TOO_LONG;
        	}
    	}

    	if (context->block_index == 64) // Process the message block
        {
        	SHA1ProcessMessageBlock(context);
    	}

    	message++;
    }
        
    return SHA_STATUS_SUCCESS;
}

/*  SHA1ProcessMessageBlock
 *
 *  Description:
 *      This function will process the next 512 bits of the message
 *      stored in the block array.
 */
void SHA1ProcessMessageBlock(SHA1Context *context)
{
    // Constants defined in SHA-1
    const UINT K[] = { 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6 };

    int t;              // Loop counter
    UINT temp;          // Temporary word value
    UINT W[80];         // Word sequence
    UINT A, B, C, D, E; // Word buffers

    for(t = 0; t < 16; t++)
    {
        W[t] = context->block[t * 4] << 24;
        W[t] |= context->block[t * 4 + 1] << 16;
        W[t] |= context->block[t * 4 + 2] << 8;
        W[t] |= context->block[t * 4 + 3];
    }
    
    for(t = 16; t < 80; t++)
    {
       W[t] = SHA1CircularShift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
    }

    A = context->intermediate_hash[0];
    B = context->intermediate_hash[1];
    C = context->intermediate_hash[2];
    D = context->intermediate_hash[3];
    E = context->intermediate_hash[4];

    for(t = 0; t < 20; t++)
    {
        temp =  SHA1CircularShift(5,A) +
                ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);

        B = A;
        A = temp;
    }

    for(t = 20; t < 40; t++)
    {
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 40; t < 60; t++)
    {
        temp = SHA1CircularShift(5,A) +
               ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 60; t < 80; t++)
    {
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    context->intermediate_hash[0] += A;
    context->intermediate_hash[1] += B;
    context->intermediate_hash[2] += C;
    context->intermediate_hash[3] += D;
    context->intermediate_hash[4] += E;
    
    context->block_index = 0;
}

/*  SHA1PadMessage
 *
 *  Description:
 *      According to the standard, the message must be padded to an even
 *      512 bits.  The first padding bit must be a '1'.  The last 64
 *      bits represent the length of the original message.  All bits in
 *      between should be 0.  This function will pad the message
 *      according to those rules by filling the Message_Block array
 *      accordingly.  It will also call the ProcessMessageBlock function
 *      provided appropriately.  When it returns, it can be assumed that
 *      the message digest has been computed.
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to pad
 *      ProcessMessageBlock: [in]
 *          The appropriate SHA*ProcessMessageBlock function
 */
void SHA1PadMessage(SHA1Context *context)
{
  /*Check to see if the current message block is too small to hold
    the initial padding bits and length.  If so, we will pad the
    block, process it, and then continue padding into a second block.*/

    if (context->block_index > 55)
    {
        context->block[context->block_index++] = 0x80;
        while(context->block_index < 64)
        {
            context->block[context->block_index++] = 0;
        }

        SHA1ProcessMessageBlock(context);

        while(context->block_index < 56)
        {
            context->block[context->block_index++] = 0;
        }
    }
    else
    {
        context->block[context->block_index++] = 0x80;
        while(context->block_index < 56)
        {

            context->block[context->block_index++] = 0;
        }
    }

    context->block[56] = (context->length_high >> 24);
    context->block[57] = (UCHAR)(context->length_high >> 16);
    context->block[58] = (UCHAR)(context->length_high >> 8);
    context->block[59] = (UCHAR)(context->length_high);

    context->block[60] = context->length_low >> 24;
    context->block[61] = (UCHAR)(context->length_low >> 16);
    context->block[62] = (UCHAR)(context->length_low >> 8);
    context->block[63] = (UCHAR)(context->length_low);
      
    SHA1ProcessMessageBlock(context);
}  
