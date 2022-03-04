/*
 * sha256_hal.h
 *
 */

#ifndef SHA256_HAL_H_
#define SHA256_HAL_H_

#include "r_hash_api.h"
#include <string.h>

#define SHA256_DIGEST_SIZE_BYTES    32
#define SHA256_BLOCK_SIZE_BYTES     64

ssp_err_t sha256_hash(const hash_instance_t * const p_hash_hal, uint8_t *p_input, uint32_t length, uint8_t *p_hash);

#endif /* SHA256_HAL_H_ */
