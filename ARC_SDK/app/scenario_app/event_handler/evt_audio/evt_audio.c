/**
 ********************************************************************************************
 *  @file      evt_audio.c
 *  @details   This file contains all isp communication event related function
 *  @author    himax/902453
 *  @version   V1.0.0
 *  @date      14-July-2019
 *  @copyright (C) COPYRIGHT, Himax, Inc. ALL RIGHTS RESERVED
 *******************************************************************************************/

/********************************************************************************************
 *  History :
 *  2019.07.17 - Initial version
 *
 *******************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "arc_builtin.h"
#include "embARC_toolchain.h"
#include "embARC_debug.h"

#include "event.h"
#include "event_handler_evt.h"
#include "aud_lib.h"
#include "evt_audio.h"
#include "hx_drv_timer.h"
#ifdef  DSPOTTER
#include "DSpotter.h"
#endif
#ifdef SUPPORT_AREA_JUDGEMENT
    #include "area_judgement.h"
    extern struct_algoResultEx AlgoResEx;
#endif
/****************************************************
 * Constant Definition                              *
 ***************************************************/

/***************************************************
 * Static Function Declaration
 **************************************************/
static void audio_rx_callback_fun(uint32_t status);

/****************************************************
 * Variable Declaration                             *
 ***************************************************/
extern infra_evt_t g_event[];

/***************************************************
 * Function Implementation
 **************************************************/
EVT_PDM_ERR_E evt_audio_init(void)
{
    uint32_t mod_base = 0, mod_g1 = 0, mod_g2 = 0;
#ifdef I2S_SOURCE
    hx_lib_audio_set_if(AUDIO_IF_I2S);
#else
	hx_lib_audio_set_if(AUDIO_IF_PDM);
#endif
    hx_lib_audio_init();
    // register the callback function to support event handler
    hx_lib_audio_register_evt_cb(audio_rx_callback_fun);

    return EVT_PDM_NO_ERROR;
}
uint8_t evt_audio_rx_cb(void)
{
    uint32_t audio_buf_addr;
    uint32_t block;

    // dbg_printf(DBG_LESS_INFO,"EVENT EVT_INDEX_AUDIO_RX: evt_audio_rx_cb TRIGGERED\n");

    // TO DO : call audio recognition library
    hx_lib_audio_request_read(&audio_buf_addr, &block);
    #ifdef SUPPORT_AREA_JUDGEMENT
    unsigned char ConsumptionDebug = 0;
    app_algo_get_consumption_debug(&AlgoResEx, &ConsumptionDebug);
    if(ConsumptionDebug == 1)
    {
        aud_blocks = block;
        aud_t2 = board_get_rtc_us();
        aud_t1 = board_get_rtc_us_by_tick(aud_t1_lowtick, aud_t1_hightick);
        aud_t = aud_t2 - aud_t1;
        aud_t_sum += aud_t;
    }
    #endif
    // dbg_printf(DBG_LESS_INFO, "audio_buf_addr = %x, block = %x\n", audio_buf_addr, block);
    // dbg_printf(DBG_LESS_INFO,"Start audio recognition process ...\n");
#ifdef  DSPOTTER
    dspotter_recognize((unsigned char*)audio_buf_addr, (block * hx_lib_audio_get_block_size()));
#endif
    // dbg_printf(DBG_LESS_INFO, " block = %x (%x)\n", block , &block);
    hx_lib_audio_update_idx(&block);

    return INFRA_EVT_RTN_CLEAR;
}

// raise audio rx event
static void audio_rx_callback_fun(uint32_t status)
{
    // dbg_printf(DBG_LESS_INFO,"Raise Audio RX EVT \n");
    infra_evt_raise_ISR(g_event[EVT_INDEX_AUDIO_RX]);
    #ifdef SUPPORT_AREA_JUDGEMENT
    aud_t_counts ++;
    board_get_rtc_tick(&aud_t1_lowtick, &aud_t1_hightick);
    #endif

    return;
}
