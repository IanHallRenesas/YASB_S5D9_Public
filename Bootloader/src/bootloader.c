/*
 * bootloader.c
 *
 */
#include "bootloader.h"

// Prototype for function pointer to the image
typedef int (*main_fnptr)(void);

void boot(void)
{
    bool        blank_status;
    ssp_err_t   err;

    // Blank check the update image area to see if there might be a valid update image to process.
    err = blank_check_image_area(UPDATE_IMAGE_START_ADDRESS, &blank_status);
    if (SSP_SUCCESS == err)
    {
        if (false == blank_status)
        {
            // No - update image area not blank.
            // Check if update area contains a valid image.
            if (VERIFY_SUCCESS == verify_image((bootloader_image_header_t *)UPDATE_IMAGE_START_ADDRESS, (uint8_t *)g_public_key))
            {
                //  Yes - valid update image

                uint32_t main_application_version = 0;

                // Is the main application area blank?
                bool main_area_blank_status = false;
                err = blank_check_image_area(MAIN_IMAGE_START_ADDRESS, &main_area_blank_status);
                if (err == SSP_SUCCESS)
                {
                    if (false == main_area_blank_status)
                    {
                        // Main area is not blank.
                        // Verify the application image.
                        if (VERIFY_SUCCESS == verify_image((bootloader_image_header_t *)MAIN_IMAGE_START_ADDRESS, (uint8_t *)g_public_key))
                        {
                            // Main image is valid so the version number in the header can be used.
                            bootloader_image_header_t * p_current_image_header;
                            p_current_image_header = (bootloader_image_header_t *)MAIN_IMAGE_START_ADDRESS;
                            main_application_version = p_current_image_header->version;
                        }
                        else
                        {
                            // Main application failed verification
                            // Set the version to zero so the main area will be erased and replaced with the update
                            //
                            // NOTE:
                            // This is an area of weakness.
                            // This could allow an unwanted down grade to a previous version.
                            // Scenario:
                            //  Valid update image in the upgrade area with a version equal or higher than the
                            //  current version.
                            //  Bootloader starts to erase main application area so this area will fail future
                            //  validation.
                            //  Device is stopped before applying the new image.
                            //  The update image is swapped with one with an earlier version number.
                            //  Restarting the bootloader will result in the application image being invalid and
                            //  erased and the older update being applied.
                            // This is an issue if it is possible to replace the update image without the need of
                            // the application.
                            // When the update is in internal memory then this scenario can be mitigated against.
                            // If the update is in external memory then consider preventing an update if there is not
                            // a valid image in the application area. This does come with a risk of being able to
                            // brick the device.
                            // The trade-off is between recovering from a corrupted application image (from an
                            // interrupted update) against a down grade attack.
                            // To prevent this possible attack at the risk of bricking stop at this point.
                            main_application_version = 0;
                        }
                    }
                    else
                    {
                        // Main area is blank so assume version as 0
                        main_application_version = 0;
                    }
                }

                //  Is new version greater or equal to the current version?
                bootloader_image_header_t * p_update_image_header;
                p_update_image_header = (bootloader_image_header_t *)UPDATE_IMAGE_START_ADDRESS;
                if (p_update_image_header->version >= main_application_version)
                {
                    //  Yes - version number good
                    //  Erase primary application slot
                    err = SSP_SUCCESS;
                    if (false == main_area_blank_status)
                    {
                        err = erase_main_image_area();
                    }

                    if (SSP_SUCCESS == err)
                    {
                        // Copy new image into the primary image area (+ 4 for the length value itself)
                        uint32_t update_size = p_update_image_header->length + SIGNATURE_LEN_BYTES + MAGIC_NUMBER_LEN + 4;

                        err = flash_main_image_from_update_area(UPDATE_IMAGE_START_ADDRESS, update_size);
                        if (SSP_SUCCESS == err)
                        {
                            // Verify new application image
                            if (VERIFY_SUCCESS == verify_image((bootloader_image_header_t *)MAIN_IMAGE_START_ADDRESS, (uint8_t *)g_public_key))
                            {
                                // Verify pass
                                // Erase update image area
                                erase_update_image_area(UPDATE_IMAGE_START_ADDRESS);

                                // Boot new application
                                boot_main_application();
                            }
                            else
                            {
                                // Verify fail
                                // Reboot to attempt update again
                                NVIC_SystemReset();
                            }
                        }
                    }
                }
                else
                {
                    // No - version number bad
                    // Erase update image area
                    erase_update_image_area(UPDATE_IMAGE_START_ADDRESS);

                    // Boot original application (including verify check of this image)
                    if (VERIFY_SUCCESS == verify_image((bootloader_image_header_t *)MAIN_IMAGE_START_ADDRESS, (uint8_t *)g_public_key))
                    {
                        // Verify pass
                        // Erase update image area
                        // Boot new application
                        boot_main_application();
                    }
                    else
                    {
                        // STOP!
                        // Failed to verify main image
                        while(1);
                    }
                }
            }
            else
            {
                // No - invalid update image
                // Erase update image area
                // Boot original application (including verify check of this image)
                erase_update_image_area(UPDATE_IMAGE_START_ADDRESS);
                // Boot original application (including verify check of this image)
                if (VERIFY_SUCCESS == verify_image((bootloader_image_header_t *)MAIN_IMAGE_START_ADDRESS, (uint8_t *)g_public_key))
                {
                    // Verify pass
                    boot_main_application();
                }
                else
                {
                    // STOP!
                    // Failed to verify main image
                    while(1);
                }
            }
        }
        else
        {
            // Update area blank
            // Boot original application (including verify check of this image)
            if (VERIFY_SUCCESS == verify_image((bootloader_image_header_t *)MAIN_IMAGE_START_ADDRESS, (uint8_t *)g_public_key))
            {
                // Verify pass
                boot_main_application();
            }
            else
            {
                // STOP!
                // Failed to verify main image
                while(1);
            }
        }
    }
    else
    {
        // Blank check failed
        // Attempt to boot main image
        if (VERIFY_SUCCESS == verify_image((bootloader_image_header_t *)MAIN_IMAGE_START_ADDRESS, (uint8_t *)g_public_key))
        {
            // Verify pass
            boot_main_application();
        }
        else
        {
            // STOP!
            // Failed to verify main image
            while(1);
        }
    }
}

void boot_main_application(void)
{
    main_fnptr *p_jump_to_app; // Function pointer main that will be used to jump to application

    // Close the ECC driver
    g_sce_ecc_0.p_api->close(g_sce_ecc_0.p_ctrl);

    // Close the SCE
    g_sce.p_api->close(g_sce.p_ctrl);

    /* point to the start reset vector of the new image */
    p_jump_to_app = (main_fnptr*)(MAIN_IMAGE_START_ADDRESS + IMAGE_HEADER_SIZE + 4);

    /* Disable interrupts */
    __disable_irq();

    uint32_t * p_VTOR = (uint32_t *)0xE000ED08U;
    *p_VTOR = (uint32_t)(MAIN_IMAGE_START_ADDRESS + IMAGE_HEADER_SIZE);

    __DSB();

    /* Disable the stack monitor before jumping */
    R_SPMON->MSPMPUCTL = (uint16_t)0x0000;

    /* Set stack here. */
    __set_MSP(*((uint32_t*)(MAIN_IMAGE_START_ADDRESS + IMAGE_HEADER_SIZE)));
    /* Jump to image*/
    (*p_jump_to_app)();

    // Should not get here.
    while(1);
}
