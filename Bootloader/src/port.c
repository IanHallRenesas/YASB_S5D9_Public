/*
 * port.c
 *
 *  Created on: 15 Mar 2021
 */
#include "port.h"

/*
 * erase_main_image_area()
 *
 * Function to erase the main application image area.
 *
 * IN:
 *  - N/A
 *
 * RETURNS:
 * - SSP_SUCCESS if erasure completes without errors
 * - Error values returned from flash driver if flash operation fails
 *
 *  */
ssp_err_t erase_main_image_area(void)
{
    ssp_err_t err;
    // Internal flash being used
    flash_instance_t            p_flash_local = g_flash;
#if defined _BL_TESTING
    extern const flash_api_t    g_flash_on_flash_hp_test;
    flash_instance_t            flash_local;
    flash_local.p_ctrl  = g_flash.p_ctrl;
    flash_local.p_cfg   = g_flash.p_cfg;
    flash_local.p_api   = &g_flash_on_flash_hp_test;

    p_flash_local       = flash_local;

#endif

    err = p_flash_local.p_api->open(p_flash_local.p_ctrl, p_flash_local.p_cfg);
    if (SSP_SUCCESS != err)
    {
        return err;
    }

    // Erase the main flash area
    err = p_flash_local.p_api->erase(p_flash_local.p_ctrl, (uint32_t const)MAIN_IMAGE_START_ADDRESS, (MAIN_IMAGE_MAX_SIZE) / (MAIN_IMAGE_ERASE_BLOCK_SIZE));
    // err will fall through after closing the flash driver

    // Close the flash driver
    p_flash_local.p_api->close(p_flash_local.p_ctrl);

    return err;
}

/*
 * flash_main_image_from_update_area()
 *
 * Function to program the main flash application image area with the update image.
 * It is assumed the main application image area is already erased.
 *
 * IN:
 * - update_area_start_addr - Address of the update image in memory
 * - length                 - Length (in bytes) of the update image to copy
 *
 * RETURNS:
 * - SSP_SUCCESS if operation passes
 * - SSP_ERR_ASSERTION if update_area_start_addr or length are zero.
 * - Error values returned from flash driver if flash operation fails
 *  */
ssp_err_t flash_main_image_from_update_area(uint32_t update_area_start_addr, uint32_t length)
{
    ssp_err_t err;
    flash_instance_t            p_flash_local = g_flash;
    uint8_t                     flash_page_buffer[MAIN_FLASH_PROGRAMMING_PAGE_SIZE] BSP_ALIGN_VARIABLE_V2(4);
#if defined _BL_TESTING
    extern const flash_api_t    g_flash_on_flash_hp_test;
    flash_instance_t            flash_local;
    flash_local.p_ctrl  = g_flash.p_ctrl;
    flash_local.p_cfg   = g_flash.p_cfg;
    flash_local.p_api   = &g_flash_on_flash_hp_test;

    p_flash_local       = flash_local;
#endif

    if ((0 == update_area_start_addr) || (0 == length))
    {
        return SSP_ERR_ASSERTION;
    }

    err = p_flash_local.p_api->open(p_flash_local.p_ctrl, p_flash_local.p_cfg);
    if (SSP_SUCCESS != err)
    {
        return err;
    }

    uint32_t bytes_to_program = length;

    /* Flash must be programmed in page size */
    /* Check if length is a multiple of the programming page size */
    uint32_t page_overflow = (bytes_to_program % MAIN_FLASH_PROGRAMMING_PAGE_SIZE);
    if (page_overflow != 0)
    {
        // Not a multiple of the programming page size
        bytes_to_program -= page_overflow;
    }

    err = p_flash_local.p_api->write(p_flash_local.p_ctrl, (uint32_t const)update_area_start_addr, (uint32_t const)MAIN_IMAGE_START_ADDRESS, bytes_to_program);

    if (SSP_SUCCESS == err)
    {
        if (page_overflow > 0)
        {
            memset((void *)flash_page_buffer, 0xFF, MAIN_FLASH_PROGRAMMING_PAGE_SIZE);
            memcpy((void *)flash_page_buffer, (void *)(update_area_start_addr + bytes_to_program), page_overflow);
            err = p_flash_local.p_api->write(p_flash_local.p_ctrl, (uint32_t const)flash_page_buffer, (uint32_t const)(MAIN_IMAGE_START_ADDRESS + bytes_to_program), MAIN_FLASH_PROGRAMMING_PAGE_SIZE);
        }
    }

    // Close the flash driver
    p_flash_local.p_api->close(p_flash_local.p_ctrl);

    return err;
}

/*
 * erase_update_image_area()
 *
 * Function to erase the update image area.
 * The complete update image area specified by MAIN_IMAGE_MAX_SIZE is erased.
 * Supports both internal flash and QSPI update areas.
 *
 * IN:
 *  - update_area_start_addr - Address where the update image is located in memory
 *
 * RETURNS:
 * - SSP_SUCCESS if erasure completes without errors
 * - Error values returned from flash driver if flash operation fails
 *
 *  */
ssp_err_t erase_update_image_area(uint32_t update_area_start_addr)
{
    ssp_err_t err = SSP_ERR_ASSERTION;

    if (0 == update_area_start_addr)
    {
        return SSP_ERR_ASSERTION;
    }

    // if the start address is not in internal flash it is assumed it is in QSPI flash
    if (!(update_area_start_addr < (INTERNAL_FLASH_START_ADDRESS + TOTAL_INTERNAL_FLASH_SIZE)))
    {
#ifdef  UPDATE_USES_QSPI_FLASH
        qspi_instance_t            p_qspi_local = g_qspi;
#if defined _BL_TESTING
        extern const qspi_api_t    g_qspi_on_qspi_test;
        qspi_instance_t           qspi_local;
        qspi_local.p_ctrl  = g_qspi.p_ctrl;
        qspi_local.p_cfg   = g_qspi.p_cfg;
        qspi_local.p_api   = &g_qspi_on_qspi_test;

        p_qspi_local       = qspi_local;
#endif
        // Open the QSPI driver
        err = p_qspi_local.p_api->open(p_qspi_local.p_ctrl, p_qspi_local.p_cfg);
        if (SSP_SUCCESS != err)
        {
            return err;
        }

        // Erase the flash area
        // QSPI flash erased in blocks of 32kB
        uint8_t * p_erase_addr = (uint8_t *)update_area_start_addr;
        for (uint32_t i=0; i<(UPDATE_IMAGE_MAX_SIZE) / (UPDATE_IMAGE_ERASE_BLOCK_SIZE); i++)
        {
            err = p_qspi_local.p_api->erase(p_qspi_local.p_ctrl, p_erase_addr, UPDATE_IMAGE_ERASE_BLOCK_SIZE);
            if (SSP_SUCCESS != err)
            {
                break;
            }

            // Wait for the operation to complete
            bool in_progress = true;
            while(true == in_progress)
            {
                err = p_qspi_local.p_api->statusGet(p_qspi_local.p_ctrl, &in_progress);
                if (SSP_SUCCESS != err)
                {
                    break;
                }
            }

            p_erase_addr += UPDATE_IMAGE_ERASE_BLOCK_SIZE;
        }

        // Close the qspi driver
        p_qspi_local.p_api->close(p_qspi_local.p_ctrl);
#endif
    }
    else
    {
        // Internal flash being used
        flash_instance_t            p_flash_local = g_flash;
#if defined _BL_TESTING
        extern const flash_api_t    g_flash_on_flash_hp_test;
        flash_instance_t            flash_local;
        flash_local.p_ctrl  = g_flash.p_ctrl;
        flash_local.p_cfg   = g_flash.p_cfg;
        flash_local.p_api   = &g_flash_on_flash_hp_test;

        p_flash_local       = flash_local;
#endif
        err = p_flash_local.p_api->open(p_flash_local.p_ctrl, p_flash_local.p_cfg);
        if (SSP_SUCCESS != err)
        {
            return err;
        }

        // Erase the flash area
        err = p_flash_local.p_api->erase(p_flash_local.p_ctrl, (uint32_t const)update_area_start_addr, (UPDATE_IMAGE_MAX_SIZE) / (UPDATE_IMAGE_ERASE_BLOCK_SIZE));
        // err will fall through after closing the flash driver

        // Close the flash driver
        p_flash_local.p_api->close(p_flash_local.p_ctrl);
    }

    return err;
}

/*
 * blank_check_image_area()
 *
 * Function to perform a blank check of an image area.
 * Complete blank check of UPDATE_IMAGE_MAX_SIZE bytes performed.
 * As the application area max size can be no larger than UPDATE_IMAGE_MAX_SIZE this size is used for both areas.
 *
 * IN:
 *  - area_start_addr       - Start address of the area to be blank checked
 *  - p_blank_check_result  - Pointer to the boolean variable to store the result - true (blank), false (not-blank)
 *
 * RETURNS:
 * - SSP_SUCCESS if blank check completes without errors
 * - Error values returned from flash driver if flash operation fails
 *
 *  */
ssp_err_t blank_check_image_area(uint32_t area_start_addr, bool * p_blank_check_result)
{
    ssp_err_t err = SSP_SUCCESS;

    if ((NULL == p_blank_check_result) || (0 == area_start_addr))
    {
        return SSP_ERR_ASSERTION;
    }

    *p_blank_check_result = false;

    // if the start address is not in internal flash it is assumed it is in QSPI flash
    if (!(area_start_addr < (INTERNAL_FLASH_START_ADDRESS + TOTAL_INTERNAL_FLASH_SIZE)))
    {
#ifdef  UPDATE_USES_QSPI_FLASH
        // QSPI code here

        // blank check
        *p_blank_check_result = true;
        uint8_t *p_src;
        p_src = (uint8_t *)area_start_addr;
        for (uint32_t i=0; i<UPDATE_IMAGE_MAX_SIZE; i++)
        {
            if (*p_src != ERASED_STATE)
            {
                *p_blank_check_result = false;
                break;
            }

            p_src++;
        }
#endif
    }
    else
    {
        // Internal flash being used
        flash_instance_t            p_flash_local = g_flash;
#if defined _BL_TESTING
        extern const flash_api_t    g_flash_on_flash_hp_test;
        flash_instance_t            flash_local;
        flash_local.p_ctrl  = g_flash.p_ctrl;
        flash_local.p_cfg   = g_flash.p_cfg;
        flash_local.p_api   = &g_flash_on_flash_hp_test;

        p_flash_local       = flash_local;
#endif
        err = p_flash_local.p_api->open(p_flash_local.p_ctrl, p_flash_local.p_cfg);
        if (SSP_SUCCESS != err)
        {
            return err;
        }

        // Blank check the update image area
        flash_result_t f_result;
        err = p_flash_local.p_api->blankCheck(p_flash_local.p_ctrl, (uint32_t const)area_start_addr, (uint32_t const)UPDATE_IMAGE_MAX_SIZE, &f_result);
        if (SSP_SUCCESS != err)
        {
            // Error - err value will be returned
        }
        else
        {
            // Check the blank status
            if (FLASH_RESULT_BLANK == f_result)
            {
                *p_blank_check_result = true;
            }
        }

        // Close the flash driver
        p_flash_local.p_api->close(p_flash_local.p_ctrl);
    }

    return err;
}

