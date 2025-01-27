/*
 *  Copyright (c) 2022, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file includes the platform-specific initializers.
 *
 */

#include "clock_config.h"
#include "fsl_clock.h"
#include "fsl_device_registers.h"
#include "fsl_port.h"
#include "pin_mux.h"
#if defined(HDI_MODE) && (HDI_MODE == 1)
#include "hdi.h"
#endif
#include "NVM_Interface.h"
#include "RNG_Interface.h"
#include "SecLib.h"
#include "app.h"
#include "board_comp.h"
#include "fsl_component_mem_manager.h"
#include "fsl_os_abstraction.h"
#include "fwk_platform.h"
#include "fwk_platform_ot.h"
#include "platform-k32w1.h"
#include <stdint.h>
#include "utils/uart.h"

#if defined(gAppLowpowerEnabled_d) && (gAppLowpowerEnabled_d > 0)
#include "fsl_pm_core.h"
#include "fwk_platform_extflash.h"
#include "fwk_platform_lowpower.h"
static status_t            ExtFlash_LowpowerCb(pm_event_type_t eventType, uint8_t powerState, void *data);
static pm_notify_element_t ExtFlashLpNotifyGroup = {
    .notifyCallback = ExtFlash_LowpowerCb,
    .data           = NULL,
};
#endif /*defined(gAppLowpowerEnabled_d) && (gAppLowpowerEnabled_d > 0)*/

#if !defined(configUSE_TICKLESS_IDLE) || (defined(configUSE_TICKLESS_IDLE) && (configUSE_TICKLESS_IDLE == 0))
#if defined(gAppLowpowerEnabled_d) && (gAppLowpowerEnabled_d > 0)
#include "PWR_Interface.h"
#include <openthread/tasklet.h>

extern uint64_t PWR_TryEnterLowPower(uint64_t timeoutUs);

#if (defined(gAppButtonCnt_c) && (gAppButtonCnt_c > 0))
#include "fsl_component_button.h"
bool g_bBtnAllowDeviceToSleep = FALSE;
/*Define button handle*/
extern BUTTON_HANDLE_ARRAY_DEFINE(g_buttonHandle, gAppButtonCnt_c);
button_status_t Btn_HandleKeys0(void *buttonHandle, button_callback_message_t *message, void *callbackParam)
{
    switch (message->event)
    {
    case kBUTTON_EventOneClick:
    case kBUTTON_EventShortPress:
    case kBUTTON_EventLongPress:
        g_bBtnAllowDeviceToSleep = TRUE;
        break;

    default:
        break;
    }
    return kStatus_BUTTON_Success;
}
#if (gAppButtonCnt_c > 1)
button_status_t Btn_HandleKeys1(void *buttonHandle, button_callback_message_t *message, void *callbackParam)
{
    switch (message->event)
    {
    case kBUTTON_EventOneClick:
    case kBUTTON_EventShortPress:
    case kBUTTON_EventLongPress:
        g_bBtnAllowDeviceToSleep = FALSE;
        break;

    default:
        break;
    }
    return kStatus_BUTTON_Success;
}
#endif /*gAppButtonCnt_c > 1*/
#endif /*gAppButtonCnt_c > 0*/
#endif /*gAppLowpowerEnabled_d*/
#endif /*!defined(configUSE_TICKLESS_IDLE) || (defined(configUSE_TICKLESS_IDLE) && (configUSE_TICKLESS_IDLE==0))*/

OT_TOOL_WEAK void APP_SysInitHook(void)
{
    /* Intentionally left empty */
}

void otSysInit(int argc, char *argv[])
{
    bool alreadyInit = false;
#if defined(gAppLowpowerEnabled_d) && (gAppLowpowerEnabled_d > 0)
    status_t status;
#endif /*defined(gAppLowpowerEnabled_d) && (gAppLowpowerEnabled_d > 0)*/

    if ((argc == 1) && (!strcmp(argv[0], "sdk_app")))
    {
        alreadyInit = true;
    }

    if (!alreadyInit)
    {
        /* Init clock config */
        BOARD_InitHardware();

        /* Init Ot Platform */
        PLATFORM_InitOT();

        /* init framework */
        MEM_Init();

#if !defined(gBoardUseFro32k_d) || (gBoardUseFro32k_d == 0)
        /* Make sure OSC32k is ready and select it as clock source */
        PLATFORM_SwitchToOsc32k();
#endif

        /* Hook used to call OT repo application functions*/
        APP_SysInitHook();

        /* Usually called from main function but in case it is compiled for OT repo applications
         * then we call it here in case any hardware like buttons or leds are needed */
        APP_InitServices();

#if defined(gAppLowpowerEnabled_d) && (gAppLowpowerEnabled_d > 0)
        PLATFORM_InitExternalFlash();
        /* Register the low power Notify callback as high priority (kPM_NotifyGroup2) */
        status = PM_RegisterNotify(kPM_NotifyGroup2, &ExtFlashLpNotifyGroup);
        assert(status == kStatus_Success);
        (void)status;
#endif /*defined(gAppLowpowerEnabled_d) && (gAppLowpowerEnabled_d > 0)*/

#if !defined(configUSE_TICKLESS_IDLE) || (defined(configUSE_TICKLESS_IDLE) && (configUSE_TICKLESS_IDLE == 0))
#if defined(gAppLowpowerEnabled_d) && (gAppLowpowerEnabled_d > 0)
#if (defined(gAppButtonCnt_c) && (gAppButtonCnt_c > 0))
        BUTTON_InstallCallback((button_handle_t)g_buttonHandle[0], Btn_HandleKeys0, NULL);
#if (gAppButtonCnt_c > 1)
        BUTTON_InstallCallback((button_handle_t)g_buttonHandle[1], Btn_HandleKeys1, NULL);
#endif /*gAppButtonCnt_c > 1*/
#endif /*gAppButtonCnt_c > 0*/
#endif /*gAppLowpowerEnabled_d*/
#endif /*!defined(configUSE_TICKLESS_IDLE) || (defined(configUSE_TICKLESS_IDLE) && (configUSE_TICKLESS_IDLE==0))*/
    }

    otPlatRadioInit();
    otPlatAlarmInit();
}

bool otSysPseudoResetWasRequested(void)
{
    return false;
}

void otSysDeinit(void)
{
}

OT_TOOL_WEAK void otSysEventSignalPending(void)
{
    /* Intentionally left empty */
}

void otSysProcessDrivers(otInstance *aInstance)
{
    otPlatRadioProcess(aInstance);
    otPlatAlarmProcess(aInstance);
    otPlatUartProcess();

#if !USE_RTOS
    OSA_ProcessTasks();
    NvIdle();
#endif

#if !defined(configUSE_TICKLESS_IDLE) || (defined(configUSE_TICKLESS_IDLE) && (configUSE_TICKLESS_IDLE == 0))
#if defined(gAppLowpowerEnabled_d) && (gAppLowpowerEnabled_d > 0)
#if (defined(gAppButtonCnt_c) && (gAppButtonCnt_c > 0))
    if (g_bBtnAllowDeviceToSleep)
#endif /*(defined(gAppButtonCnt_c) && (gAppButtonCnt_c > 0))*/
    {
        if (otTaskletsArePending(aInstance) == false)
        {
            PWR_TryEnterLowPower(0);
        }
    }
#endif /*defined(gAppLowpowerEnabled_d) && (gAppLowpowerEnabled_d > 0)*/
#endif /*!defined(configUSE_TICKLESS_IDLE) || (defined(configUSE_TICKLESS_IDLE) && (configUSE_TICKLESS_IDLE==0))*/
}

#if defined(gAppLowpowerEnabled_d) && (gAppLowpowerEnabled_d > 0)
static status_t ExtFlash_LowpowerCb(pm_event_type_t eventType, uint8_t powerState, void *data)
{
    status_t ret = kStatus_Success;
    if (powerState < PLATFORM_DEEP_SLEEP_STATE)
    {
        /* Nothing to do when entering WFI or Sleep low power state
            NVIC fully functionnal to trigger upcoming interrupts */
    }
    else
    {
        if (eventType == kPM_EventEnteringSleep)
        {
            PLATFORM_UninitExternalFlash();
        }
        else
        {
            PLATFORM_ReinitExternalFlash();
        }
    }
    return ret;
}
#endif /*defined(gAppLowpowerEnabled_d) && (gAppLowpowerEnabled_d > 0)*/
