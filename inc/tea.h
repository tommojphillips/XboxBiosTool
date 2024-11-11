// tea.h

// https://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef _TEA_H
#define _TEA_H


#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void tea_encrypt(uint32_t v[2], const uint32_t k[4]);
void tea_decrypt(uint32_t v[2], const uint32_t k[4]);

#ifdef __cplusplus
}
#endif
#endif // !_TEA_H
