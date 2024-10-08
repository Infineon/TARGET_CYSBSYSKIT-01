/***********************************************************************************************//**
 * \file cybsp.c
 *
 * Description:
 * Provides initialization code for starting up the hardware contained on the
 * Infineon board.
 *
 ***************************************************************************************************
 * \copyright
 * Copyright 2018-2022 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **************************************************************************************************/

#include <stdlib.h>
#include "cy_syspm.h"
#include "cy_sysclk.h"
#include "cybsp.h"
#if defined(CY_USING_HAL)
#include "cyhal_hwmgr.h"
#include "cyhal_syspm.h"
#endif

#if !defined (CY_CFG_PWR_SYS_IDLE_MODE) && defined(__MBED__)
#include "mbed_power_mgmt.h"
#endif

// Define the VDDA voltage - this is needed as 'Power' configuration is disabled
// from design.modus for now.
#ifndef CY_CFG_PWR_VDDA_MV
    #define CY_CFG_PWR_VDDA_MV              (3300)
#endif

#if defined(__cplusplus)
extern "C" {
#endif

// The sysclk deep sleep callback is recommended to be the last callback that is executed before
// entry into deep sleep mode and the first one upon exit the deep sleep mode.
// Doing so minimizes the time spent on low power mode entry and exit.
#ifndef CYBSP_SYSCLK_PM_CALLBACK_ORDER
    #define CYBSP_SYSCLK_PM_CALLBACK_ORDER  (255u)
#endif

//--------------------------------------------------------------------------------------------------
// cybsp_register_sysclk_pm_callback
//
// Registers a power management callback that prepares the clock system for entering deep sleep mode
// and restore the clocks upon wakeup from deep sleep.
// NOTE: This is called automatically as part of \ref cybsp_init
//--------------------------------------------------------------------------------------------------
static cy_rslt_t cybsp_register_sysclk_pm_callback(void)
{
    cy_rslt_t                             result                         = CY_RSLT_SUCCESS;
    static cy_stc_syspm_callback_params_t cybsp_sysclk_pm_callback_param = { NULL, NULL };
    static cy_stc_syspm_callback_t        cybsp_sysclk_pm_callback       =
    {
        .callback       = &Cy_SysClk_DeepSleepCallback,
        .type           = CY_SYSPM_DEEPSLEEP,
        .callbackParams = &cybsp_sysclk_pm_callback_param,
        .order          = CYBSP_SYSCLK_PM_CALLBACK_ORDER
    };

    if (!Cy_SysPm_RegisterCallback(&cybsp_sysclk_pm_callback))
    {
        result = CYBSP_RSLT_ERR_SYSCLK_PM_CALLBACK;
    }
    return result;
}


//--------------------------------------------------------------------------------------------------
// cybsp_init
//--------------------------------------------------------------------------------------------------
cy_rslt_t cybsp_init(void)
{
    // Setup hardware manager to track resource usage then initialize all system (clock/power) board
    // configuration
    #if defined(CY_USING_HAL)
    cy_rslt_t result = cyhal_hwmgr_init();

    if (CY_RSLT_SUCCESS == result)
    {
        result = cyhal_syspm_init();
    }
    #else // if defined(CY_USING_HAL)
    cy_rslt_t result = CY_RSLT_SUCCESS;
    #endif // if defined(CY_USING_HAL)

    #ifdef CY_CFG_PWR_VDDA_MV
    if (CY_RSLT_SUCCESS == result)
    {
        #if defined(CY_USING_HAL)
        cyhal_syspm_set_supply_voltage(CYHAL_VOLTAGE_SUPPLY_VDDA, CY_CFG_PWR_VDDA_MV);
        #elif defined(CY_USING_HAL_LITE)
        cyhal_system_set_supply_voltage(CYHAL_VOLTAGE_SUPPLY_VDDA, CY_CFG_PWR_VDDA_MV);
        #endif
    }
    #endif

    init_cycfg_all();

    if (CY_RSLT_SUCCESS == result)
    {
        result = cybsp_register_sysclk_pm_callback();
    }

    #if defined(CY_USING_HAL)
    #if !defined(CY_CFG_PWR_SYS_IDLE_MODE)
    #ifdef __MBED__
    // Disable deep-sleep
    sleep_manager_lock_deep_sleep();
    #else
    cyhal_syspm_lock_deepsleep();
    #endif
    #endif

    // Reserve resources used by NP
    cyhal_resource_inst_t clock1 =
        { .type        = CYHAL_RSC_CLOCK, .block_num = CYHAL_CLOCK_BLOCK_PERIPHERAL_16BIT,
          .channel_num = 0 };
    cyhal_resource_inst_t clock2 =
        { .type        = CYHAL_RSC_CLOCK, .block_num = CYHAL_CLOCK_BLOCK_PERIPHERAL_16BIT,
          .channel_num = 1 };
    if (CY_RSLT_SUCCESS == result)
    {
        result = cyhal_hwmgr_reserve(&clock1);
    }
    if (CY_RSLT_SUCCESS == result)
    {
        result = cyhal_hwmgr_reserve(&clock2);
    }

    // CYHAL_HWMGR_RSLT_ERR_INUSE error code could be returned if any needed for BSP resource was
    // reserved by user previously. Please review the Device Configurator (design.modus) and the BSP
    // reservation list (cyreservedresources.list) to make sure no resources are reserved by both.
    #endif // defined(CY_USING_HAL)
    return result;
}


#if defined(__cplusplus)
}
#endif
