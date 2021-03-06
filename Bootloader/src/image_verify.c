/*
 * image_verify.c
 */
#include "bootloader.h"

/*
 *  # Image format:
    # Magic Number | Signature | Length | Version | Padding | Binary Image
    #
    # Magic number  - 4 bytes
    # Signature     - 64 bytes for ECC-256
    # Length        - 4 bytes
    # Version       - 4 bytes
    # Padding       - Space added to make header up to the specified header_size
    # Binary image  - Image dependent
    #
    # Header is Magic Number, Signature, Length, Version, Padding
    #
    # The Signature is from (and including) the Length field
    # The Length field is from (and including) the Version field
    # So, total size of the image is:
    # Length + 4 (Length field) + 64 (Signature) + 4 (Magic Number)
 *
 *  */

/* Recommended Parameters secp256k1
 *
 *  Curve E: y^2 = x^3 +ax + b
 *
 */
uint8_t domain[ECC_256_DOMAIN_PARAMETER_WITH_ORDER_LENGTH_WORDS * sizeof(uint32_t)] =
{
  /* a */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* b */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07,
  /* p = 2^256-2^64-1 */
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x2F,
  /* n */
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFE, 0xBA, 0xAE, 0xDC, 0xE6, 0xAF, 0x48, 0xA0, 0x3B,
  0xBF, 0xD2, 0x5E, 0x8C, 0xD0, 0x36, 0x41, 0x41
};
/*
 *
 *  Base Point G (uncompressed)
 *
 */
uint8_t generator_point[ECC_256_GENERATOR_POINT_LENGTH_WORDS * sizeof(uint32_t)] =
{
  /* x */
  0x79, 0xBE, 0x66, 0x7E, 0xF9, 0xDC, 0xBB, 0xAC, 0x55, 0xA0, 0x62, 0x95,
  0xCE, 0x87, 0x0B, 0x07, 0x02, 0x9B, 0xFC, 0xDB, 0x2D, 0xCE, 0x28, 0xD9,
  0x59, 0xF2, 0x81, 0x5B, 0x16, 0xF8, 0x17, 0x98,
  /* y */
  0x48, 0x3a, 0xda, 0x77, 0x26, 0xa3, 0xc4, 0x65, 0x5d, 0xa4, 0xfb, 0xfc,
  0x0e, 0x11, 0x08, 0xa8, 0xfd, 0x17, 0xb4, 0x48, 0xa6, 0x85, 0x54, 0x19,
  0x9c, 0x47, 0xd0, 0x8f, 0xfb, 0x10, 0xd4, 0xb8,
 };

/*
 * verify_image()
 *
 * Function to validate an image header. Checks:
 * - Magic number
 * - Length (is not larger than main image space)
 * - ECDSA signature (SHA256)
 *
 * IN:
 * - p_image_header - Pointer to the start of the header information
 * - p_public_key   - Pointer to the public key used to verify the ECC signature
 *
 * RETURNS:
 * - VERIFY_SUCCESS if verification passes
 * - VERIFY_FAIL if any of the verification elements fails
 *
 *  */
uint16_t verify_image(bootloader_image_header_t * p_image_header, uint8_t * p_public_key)
{
    ssp_err_t                   err;
    uint16_t                    res = VERIFY_FAIL;
    uint32_t                    hash[SHA256_DIGEST_SIZE_BYTES / 4];
    r_crypto_data_handle_t      g_msg_digest_handle;
    r_crypto_data_handle_t      ecdsa_public_key_handle;
    r_crypto_data_handle_t      domain_handle = {(uint32_t *)domain, sizeof(domain)/sizeof(uint32_t)};
    r_crypto_data_handle_t      generator_point_handle = {(uint32_t *)generator_point, sizeof(generator_point)/sizeof(uint32_t)};
    r_crypto_data_handle_t      g_ext_sign_r_handle;
    r_crypto_data_handle_t      g_ext_sign_s_handle;

    // Check the magic number
    // This is a simple check which will indicate whether the image header looks correct and worthy or further processing
    if (0 != memcmp((void *)p_image_header, (void *)MAGIC_NUMBER, MAGIC_NUMBER_LEN))
    {
        return VERIFY_FAIL;
    }

    // Check the length in the header doesn't exceed the size of the main application space
    uint32_t new_image_length;
    new_image_length = p_image_header->length + sizeof(p_image_header->length) + sizeof(p_image_header->signature) + sizeof(p_image_header->magic_number);
    if (new_image_length > MAIN_IMAGE_MAX_SIZE)
    {
        return VERIFY_FAIL;
    }

    // Calculate the hash of the image
    sha256_hash(&g_sce_hash_0, (uint8_t *)&p_image_header->length, (p_image_header->length + 4), (uint8_t *)hash);

    // Verify the signature
    g_msg_digest_handle.p_data          = (uint32_t *)hash;
    g_msg_digest_handle.data_length     = ECC_256_MESSAGE_DIGEST_LENGTH_WORDS;
    ecdsa_public_key_handle.p_data      = (uint32_t *)p_public_key;
    ecdsa_public_key_handle.data_length = (ECC_256_PUBLIC_KEY_LENGTH_WORDS);
    g_ext_sign_r_handle.p_data          = (uint32_t *)p_image_header->signature;
    g_ext_sign_r_handle.data_length     = ECC_256_SIGNATURE_R_LENGTH_WORDS;
    g_ext_sign_s_handle.p_data          = (uint32_t *)(p_image_header->signature + ECC_256_SIGNATURE_R_LENGTH_WORDS);
    g_ext_sign_s_handle.data_length     = ECC_256_SIGNATURE_S_LENGTH_WORDS;
    err = g_sce_ecc_0.p_api->verify(g_sce_ecc_0.p_ctrl, &domain_handle, &generator_point_handle, &ecdsa_public_key_handle, &g_msg_digest_handle, &g_ext_sign_r_handle, &g_ext_sign_s_handle);
    if (SSP_SUCCESS != err)
    {
        res = VERIFY_FAIL;
    }
    else
    {
        res = VERIFY_SUCCESS;
    }

    return (res);
}

