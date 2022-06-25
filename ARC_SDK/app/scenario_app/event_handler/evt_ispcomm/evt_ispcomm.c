/**
 ********************************************************************************************
 *  @file      evt_ispcomm.c
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
#include "embARC_debug.h"

#include "event.h"
#include "event_handler_evt.h"
#include "isp_comm.h"
#include "evt_ispcomm.h"
#include "hx_drv_iomux.h"

#ifdef QNB_FACEDETECT
#include "qnb_facedetect.h"
#include "qnb_rx_config.h"
#endif

/****************************************************
 * Constant Definition                              *
 ***************************************************/
#define ISP_GPIO0		IOMUX_PGPIO8
#define ISP_GPIO1		IOMUX_PGPIO9
#define I2C_MUX_GPIO	IOMUX_PGPIO3

/***************************************************
 * Static Function Declaration
 **************************************************/
static void ispcomm_callback_fun_rx(void * param);


/****************************************************
 * Variable Declaration                             *
 ***************************************************/
extern infra_evt_t 	g_event[];

ISPCOMM_CFG_T gISPCOMM_cfg = {
	ISPCOMM_STATUS_UNINIT,	/*!< ISPCOMM state */
	ISP_GPIO0,
	ISP_GPIO1,
	I2C_MUX_GPIO,
	ispcomm_callback_fun_rx
};


/***************************************************
 * Function Implementation
 **************************************************/
EVT_ISPCOM_ERR_E evt_ispcomm_init(void)
{
	// ISPCOMM_STATUS_E status = ISPCOMM_STATUS_UNKNOWN;

	// Init ISP Communication lib
	hx_lib_ispcomm_init(&gISPCOMM_cfg);

	// start isp communication process
	hx_lib_ispcomm_start();

	return EVT_ISPCOMM_NO_ERROR;
}


uint8_t evt_ispcomm_rx_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_LESS_INFO,"EVENT INFRA_EVT_ISP_RX: event_isp_rx_cb TRIGGERED\n");
	#endif

	hx_lib_ispcomm_process(&gISPCOMM_cfg);

	return INFRA_EVT_RTN_CLEAR;
}


// raise rx event
static void ispcomm_callback_fun_rx(void * param)
{

#ifdef ISP_HANDSHAKE_WORKAROUND
	uint8_t value;

	hx_drv_iomux_get_invalue(gISPCOMM_cfg.isp_gpio0, &value);

	if ( value == 1 )
	{
		// stop EMZA algorithm and switch sensor control to IPU, keep xsleep always high
    	app_sensor_xsleep_ctrl(SENSORCTRL_XSLEEP_BY_CPU, 1);
		hx_drv_iomux_set_outvalue(gISPCOMM_cfg.i2c_mux_gpio, value);
		hx_drv_iomux_set_outvalue(gISPCOMM_cfg.isp_gpio1, value);
	}
#endif

	infra_evt_raise_ISR(g_event[EVT_INDEX_ISP_RX]);
    return;
}




