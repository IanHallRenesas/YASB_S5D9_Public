/*
 * port.h
 *
 *  Created on: 15 Mar 2021
 *      Author: b3800274
 */

#ifndef PORT_H_
#define PORT_H_
#include "hal_data.h"
#include <string.h>

/* MCU specific definitions */
#define PK_S5D9
#ifdef PK_S5D9

// Undefine below to use external QSPI flash as the image update area.
//#define UPDATE_USES_QSPI_FLASH

#define INTERNAL_FLASH_START_ADDRESS 0
#define TOTAL_INTERNAL_FLASH_SIZE   (0x00200000)
// The address of the main executable application image in flash
// The first part of this image will be the header.
// So, the actual link address for this application will be offset by the header size (default H'100 bytes)
#define MAIN_IMAGE_START_ADDRESS    (0x00010000)
#define MAIN_IMAGE_ERASE_BLOCK_SIZE (32 * 1024)
#define MAIN_FLASH_PROGRAMMING_PAGE_SIZE (128)
#ifdef  UPDATE_USES_QSPI_FLASH
// All the internal flash (less the bootloader) is avaliable for the application as the update image is in QSPI flash
#define MAIN_IMAGE_MAX_SIZE         (TOTAL_INTERNAL_FLASH_SIZE - MAIN_IMAGE_START_ADDRESS)
#define UPDATE_IMAGE_START_ADDRESS  (0x60000000)
// QSPI - W25Q64FV
#define FLASH_PROGRAMMING_PAGE_SIZE (256)
#define UPDATE_IMAGE_ERASE_BLOCK_SIZE (32 * 1024)
#else
// The main image size is half of the internal flash excluding the Bootloader itself
#define MAIN_IMAGE_MAX_SIZE         ((TOTAL_INTERNAL_FLASH_SIZE - MAIN_IMAGE_START_ADDRESS) / 2)
#define UPDATE_IMAGE_START_ADDRESS  (MAIN_IMAGE_START_ADDRESS + MAIN_IMAGE_MAX_SIZE)
#define UPDATE_IMAGE_ERASE_BLOCK_SIZE (32 * 1024)
#endif /* QSPI Flash */
#define UPDATE_IMAGE_MAX_SIZE       (MAIN_IMAGE_MAX_SIZE)
#define ERASED_STATE                (0xFF)
#endif /* PK_S5D9 */

ssp_err_t erase_main_image_area(void);
ssp_err_t flash_main_image_from_update_area(uint32_t update_area_start_addr, uint32_t length);
ssp_err_t erase_update_image_area(uint32_t update_area_start_addr);
ssp_err_t blank_check_image_area(uint32_t area_start_addr, bool * p_blank_check_result);

#endif /* PORT_H_ */
