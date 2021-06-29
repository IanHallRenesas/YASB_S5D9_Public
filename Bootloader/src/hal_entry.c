/* HAL-only entry function */
#include "hal_data.h"
#include "sha256.h"
#include <string.h>
#include "bootloader.h"

void hal_entry(void)
{
    ssp_err_t err;

    // Open the SCE driver
    err = g_sce.p_api->open(g_sce.p_ctrl, g_sce.p_cfg);
    if (SSP_SUCCESS != err)
    {
        // If opening the SCE driver fails, stop. Cannot verify any image without SCE support.
        while(1);
    }

    /* Open the ECC driver  - plaintext key support */
    err = g_sce_ecc_0.p_api->open(g_sce_ecc_0.p_ctrl, g_sce_ecc_0.p_cfg);
    if (SSP_SUCCESS != err)
    {
        // If opening the ECC driver fails, stop. Cannot verify any image without ECC support.
        while(1);
    }

#if defined _BL_TESTING

    // Set P411 as SCI0 Tx pin
    g_ioport.p_api->pinCfg(IOPORT_PORT_04_PIN_11, (IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_SCI0_2_4_6_8));

    err = g_uart0.p_api->open(g_uart0.p_ctrl, g_uart0.p_cfg);
    if (SSP_SUCCESS != err)
    {
        __BKPT(1);
    }

    int Test_bootloader_main(void);
    Test_bootloader_main();

    while(1)
    {
        __NOP();
    }
#endif

    // Run the bootloader to update the main application, boot the main application or stop.
    boot();

    // Should not get here
    while(1);
}
