/*
 * evt_spimcomm.c
 *
 *  Created on: 2019/8/20
 *      Author: 902449
 */
/********************************************************************************************
 *  History :
 *  2019.08.20 - Initial version
 *
 *******************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "embARC_debug.h"
#include "board_config.h"

#include "board.h"
#include "event.h"
#include "event_handler_evt.h"
#include "spi_master_protocol.h"
#include "evt_spimcomm.h"


/****************************************************
 * Constant Definition                              *
 ***************************************************/



/***************************************************
 * Static Function Declaration
 **************************************************/
static void spimcomm_callback_fun_tx(void * param);

static void spimcomm_callback_fun_rx(void * param);

/****************************************************
 * Variable Declaration                             *
 ***************************************************/
extern infra_evt_t 	g_event[];

/***************************************************
 * Function Implementation
 **************************************************/
/*
 * @brief SPI master init
 *
 * SPI master initial for event handler use

 * @return EVT_SPIMCOMM_NO_ERROR.
 * */
/**
 * \brief	Callback function for 1Bit INP Parser Error
 *
 * Callback function for 1Bit INP Parser Error
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
EVT_SPIMCOM_ERR_E evt_spimcomm_init(void)
{
	hx_drv_spi_mst_open();

	hx_drv_spi_mst_protocol_register_tx_cb((spi_mst_protocol_cb_t) spimcomm_callback_fun_tx);
	hx_drv_spi_mst_protocol_register_rx_cb((spi_mst_protocol_cb_t) spimcomm_callback_fun_rx);

	return EVT_SPIMCOMM_NO_ERROR;
}

EVT_SPIMCOM_ERR_E evt_spimcomm_tx(uint32_t SRAM_addr, uint32_t img_size)
{
	if(hx_drv_spi_mst_protocol_write(SRAM_addr,img_size) !=0)
		return EVT_SPIMCOMM_ERR_BUSY;

	return EVT_SPIMCOMM_NO_ERROR;
}

uint8_t evt_spimcomm_tx_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_LESS_INFO,"[MSG]EVENT INFRA_EVT_SPIM_TX: event_spim_tx_cb TRIGGERED\n");
	#endif
//	hx_drv_spi_mst_protocol_write_data();

	return INFRA_EVT_RTN_CLEAR;
}

uint8_t evt_spimcomm_rx_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_LESS_INFO,"[MSG]EVENT INFRA_EVT_SPIM_TX: event_spim_rx_cb TRIGGERED\n");
	#endif
	
	return INFRA_EVT_RTN_CLEAR;
}

// raise tx event
static void spimcomm_callback_fun_tx(void * param)
{
	#ifndef __GNU__
	dbg_printf(DBG_LESS_INFO,"[MSG]SPIMCOMM TX CB\n");
	#endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_SPIM1_TX]);
    return;
}

// raise rx event
static void spimcomm_callback_fun_rx(void * param)
{
	#ifndef __GNU__
	dbg_printf(DBG_LESS_INFO,"[MSG]SPIMCOMM RX CB \n");
	#endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_SPIM1_RX]);
    return;
}
