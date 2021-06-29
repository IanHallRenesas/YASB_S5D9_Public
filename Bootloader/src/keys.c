/*
 * keys.c
 *
 *  Created on: 19 Feb 2021
 *      Author: b3800274
 */
#include "bootloader.h"

const uint8_t g_public_key[ECC_256_PUBLIC_KEY_LENGTH_WORDS * sizeof(uint32_t)] BSP_ALIGN_VARIABLE(4) = {
                                        0x3a, 0xec, 0xde, 0x7d, 0xac, 0xb9, 0x9b, 0xbc, 0xd9, 0x0b, 0x95, 0xaa, 0xb5, 0x07, 0x94, 0x9f,  \
                                        0xf7, 0x37, 0xac, 0x28, 0x14, 0x75, 0x46, 0x64, 0xb0, 0x6a, 0x31, 0x17, 0x24, 0x42, 0x64, 0x2d,  \
                                        0xf2, 0xc8, 0xa7, 0xfc, 0x33, 0xa4, 0x5e, 0x41, 0xe2, 0x75, 0x8c, 0x8a, 0xba, 0x76, 0x57, 0x54,  \
                                        0x0f, 0xbe, 0x7d, 0x6b, 0xa6, 0x36, 0xda, 0x4b, 0x29, 0x3a, 0xb4, 0xeb, 0x62, 0xc3, 0x29, 0xa0,
};
