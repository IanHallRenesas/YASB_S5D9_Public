/*
 * sha256_hal.c
 *
 */

#include "sha256_hal.h"

static const uint8_t sha256_initial_values[] = {
                        0x6au, 0x09u, 0xe6u, 0x67u, 0xbbu, 0x67u, 0xaeu, 0x85u,
                        0x3cu, 0x6eu, 0xf3u, 0x72u, 0xa5u, 0x4fu, 0xf5u, 0x3au,
                        0x51u, 0x0eu, 0x52u, 0x7fu, 0x9bu, 0x05u, 0x68u, 0x8cu,
                        0x1fu, 0x83u, 0xd9u, 0xabu, 0x5bu, 0xe0u, 0xcdu, 0x19u
};

/*
 * Assumes SCE and HASH drivers are open
 *
 * p_ctrl   - Pointer to the hash driver instance
 * p_input  - Pointer to the data to be hashed (data on 32-bit boundary will result in faster hashing)
 * length   - Number of bytes to be hashed
 * p_hash   - SHA256 hash digest of the input data
 *
 * Returns  - SSP_SUCCESS or error from hash HAL driver
 *
 *  */
ssp_err_t sha256_hash(const hash_instance_t * const p_hash_hal, uint8_t *p_input, uint32_t length, uint8_t *p_hash)
{
    uint32_t hash_digest[SHA256_DIGEST_SIZE_BYTES / 4];
    uint32_t temp_data[SHA256_BLOCK_SIZE_BYTES / 4];
    uint32_t remaining_bytes;   /* Remaining bytes after removing multiples of SHA256_BLOCK_SIZE_BYTES */
    uint32_t bytes_to_hash;     /* Bytes to hash in multiples of SHA256_BLOCK_SIZE_BYTES */
    ssp_err_t err;

    /* Calculate the number of bytes that are a multiple of SHA256_BLOCK_SIZE_BYTES */
    bytes_to_hash = (length / SHA256_BLOCK_SIZE_BYTES) * SHA256_BLOCK_SIZE_BYTES;
    remaining_bytes = length % SHA256_BLOCK_SIZE_BYTES;

    /* Initialise the hash digest */
    memcpy((uint8_t*)hash_digest, sha256_initial_values, sizeof(sha256_initial_values));

    /* If the data to be hashed is on a 32-bit boundary it can be hashed in place */
    if (((uint32_t)p_input & (uint32_t)0x03) == 0)
    {
        /*  32-bit boundary so all of bytes_to_hash number pf bytes can be hashed in one operation */
        err = p_hash_hal->p_api->hashUpdate(p_hash_hal->p_ctrl, (uint32_t *)p_input, (bytes_to_hash / 4), (uint32_t *)hash_digest);
        if (SSP_SUCCESS != err)
        {
            return err;
        }
    }
    else
    {
        /*  Input data is not on a 32-bit boundary. Hash in multiples of SHA256_BLOCK_SIZE_BYTES */
        for (uint32_t i=0; i<bytes_to_hash; i+=SHA256_BLOCK_SIZE_BYTES)
        {
            memcpy((void *)temp_data, (void *)((uint32_t)p_input + i), SHA256_BLOCK_SIZE_BYTES);
            err = p_hash_hal->p_api->hashUpdate(p_hash_hal->p_ctrl, temp_data, (SHA256_BLOCK_SIZE_BYTES / 4), (uint32_t *)hash_digest);
            if (SSP_SUCCESS != err)
            {
                return err;
            }
        }
    }

    /* Copy the remaining bytes into the buffer to be hashed */
    if (0 != remaining_bytes)
    {
        memcpy((void *)temp_data, (void *)((uint32_t)p_input + bytes_to_hash), remaining_bytes);
    }

    /* Insert the terminator */
    uint8_t *t;
    t = (uint8_t *)((uint32_t)temp_data + remaining_bytes);
    *t = 0x80;

    /* If there is room for the 8 final bytes add them */
    /* -1 for the terminator */
    if ((SHA256_BLOCK_SIZE_BYTES - remaining_bytes - 1) >= 8)
    {
        /* Room for the final 8 bytes */
        /* Pad with zeros  */
        memset((t + 1), 0, (SHA256_BLOCK_SIZE_BYTES - remaining_bytes - 1 - 8));
    }
    else
    {
        /* Not enough room for the final 8 bytes */
        /* Pad with zeros, update and then add the final 8 bytes */
        memset((t + 1), 0, (SHA256_BLOCK_SIZE_BYTES - remaining_bytes - 1));

        err = p_hash_hal->p_api->hashUpdate(p_hash_hal->p_ctrl, temp_data, (SHA256_BLOCK_SIZE_BYTES / 4), (uint32_t *)hash_digest);
        if (SSP_SUCCESS != err)
        {
            return err;
        }

        memset((void *)temp_data, 0, (SHA256_BLOCK_SIZE_BYTES - 8));
    }

    uint8_t * p_bit_length =  (uint8_t *)((uint32_t)temp_data + (SHA256_BLOCK_SIZE_BYTES - 8));
    uint32_t  bit_length_32bit = (uint32_t)((uint64_t)(length << 3) >> 32);
    p_bit_length[0] = (uint8_t)(bit_length_32bit >> 24);
    p_bit_length[1] = (uint8_t)(bit_length_32bit >> 16);
    p_bit_length[2] = (uint8_t)(bit_length_32bit >> 8);
    p_bit_length[3] = (uint8_t)(bit_length_32bit >> 0);

    bit_length_32bit = (uint32_t)((uint64_t)length << 3);
    p_bit_length[4] = (uint8_t)(bit_length_32bit >> 24);
    p_bit_length[5] = (uint8_t)(bit_length_32bit >> 16);
    p_bit_length[6] = (uint8_t)(bit_length_32bit >> 8);
    p_bit_length[7] = (uint8_t)(bit_length_32bit >> 0);

    /* final update */
    err = p_hash_hal->p_api->hashUpdate(p_hash_hal->p_ctrl, temp_data, (SHA256_BLOCK_SIZE_BYTES / 4), (uint32_t *)hash_digest);
    if (SSP_SUCCESS != err)
    {
        return err;
    }

    memcpy((void *)p_hash, (void *)hash_digest, SHA256_DIGEST_SIZE_BYTES);

    return SSP_SUCCESS;
}
