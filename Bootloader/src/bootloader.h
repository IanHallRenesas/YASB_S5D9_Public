/*
 * bootloader.h
 *
 *  Created on: 18 Feb 2021
 *      Author: b3800274
 */

#include "hal_data.h"
#include "sha256.h"
#include <string.h>
#include "port.h"

#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_

#define MAGIC_NUMBER            "YASB"
#define MAGIC_NUMBER_LEN        4

#define SIGNATURE_LEN           (2 * ECC_256_SIGNATURE_R_LENGTH_WORDS)
#define SIGNATURE_LEN_BYTES     (4 * SIGNATURE_LEN)

#define IMAGE_HEADER_SIZE       0x100

#define VERIFY_SUCCESS          0x5A3C
#define VERIFY_FAIL             0

typedef struct bootloader_image_header {
    uint32_t magic_number;
    uint32_t signature[SIGNATURE_LEN];
    uint32_t length;
    uint32_t version;
} bootloader_image_header_t;

extern const uint8_t g_public_key[ECC_256_PUBLIC_KEY_LENGTH_WORDS * sizeof(uint32_t)];

uint16_t verify_image(bootloader_image_header_t * p_image_header, uint8_t * p_public_key);
void boot(void);
void boot_main_application(void);

#endif /* BOOTLOADER_H_ */
