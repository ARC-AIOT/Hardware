/**
 ********************************************************************************************
 *  @file      evt_uartcomm.c
 *  @details   This file contains all uart communication event related function
 *  @author    himax/903730
 *  @version   V1.0.0
 *  @date      14-April-2020
 *  @copyright (C) COPYRIGHT, Himax, Inc. ALL RIGHTS RESERVED
 *******************************************************************************************/

/********************************************************************************************
 *  History :
 *  2020.04.14 - Initial version
 *
 *******************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "arc_builtin.h"
#include "embARC_toolchain.h"
#include "embARC_debug.h"

#include "event.h"
#include "event_handler_evt.h"
#include "evt_uartcomm.h"

#include "board_config.h"
#include "hx_drv_uart.h"
#include "uart_protocol.h"

#include "peripheral_cmd_handler.h"

#ifdef __Xdmac
#include "arc_udma.h"
#endif

/****************************************************
 * Constant Definition                              *
 ***************************************************/
#define UART_PACKET_SIZE (5*1024)

/***************************************************
 * Static Function Declaration
 **************************************************/
static void uart_rx_callback_fun(uint32_t status);

/****************************************************
 * Variable Declaration                             *
 ***************************************************/
extern infra_evt_t g_event[];
#if 1
#define uart_buf_size 7
unsigned char g_uart_rx_buf[uart_buf_size]={0,0,0,0,0,0,0};
#else
#define uart_buf_size 8
unsigned char g_uart_rx_buf[uart_buf_size]={0};
#endif
//unsigned char g_uart_tx_buf[7]={0,0,0,0,0,0,0};
static DEV_UART_PTR dev_uart_comm;

/***************************************************
 * Function Implementation
 **************************************************/
EVT_UARTCOMM_ERR_E evt_uartcomm_init(void)
{
#ifdef TV01
    uint32_t cmd_uart_buad = UART_BAUDRATE_115200;
#else
    uint32_t cmd_uart_buad = UART_BAUDRATE_921600;
#endif

    dev_uart_comm = hx_drv_uart_get_dev(BOARD_COMMAND_UART_ID);
    dev_uart_comm->uart_open(cmd_uart_buad);
    #ifdef UART_PROTOCOL
    dev_uart_comm->uart_control(UART_CMD_SET_TXCB, uart_callback_fun_tx);
    dev_uart_comm->uart_control(UART_CMD_SET_RXCB, uart_rx_callback_fun);
    dev_uart_comm->uart_control(UART_CMD_SET_TRANSFER_MODE,  (void*) UART_TRANSFER_DMA);
    dev_uart_comm->uart_read(g_uart_rx_buf, uart_buf_size);
    dbg_printf(DBG_LESS_INFO,"COMMAND UART ID:%d,  Baud Rate:%d \n", BOARD_COMMAND_UART_ID, cmd_uart_buad);

#ifdef __Xdmac
#if (BOARD_COMMAND_UART_ID == DFSS_UART_0_ID)
    static dma_channel_t 	dma_chn_uart0_rx;
    dmac_init_channel(&dma_chn_uart0_rx);
    dmac_reserve_channel(IO_UART0_DMA_RX, &dma_chn_uart0_rx, DMA_REQ_PERIPHERAL);
#elif (BOARD_COMMAND_UART_ID == DFSS_UART_1_ID)
    static dma_channel_t 	dma_chn_uart1_rx;
    dmac_init_channel(&dma_chn_uart1_rx);
    dmac_reserve_channel(IO_UART1_DMA_RX, &dma_chn_uart1_rx, DMA_REQ_PERIPHERAL);
#else
    #error "no configure proper BOARD_COMMAND_UART_ID"
#endif
#endif
#endif

    return EVT_UARTCOMM_NO_ERROR;
}

uint8_t evt_uartcomm_rx_cb(void)
{
    #ifndef __GNU__
    dbg_printf(DBG_LESS_INFO,"EVENT INFRA_EVT_UART_RX: evt_uartcomm_rx_cb TRIGGERED\n");
    uint8_t i;
    for(i = 0; i< uart_buf_size; i++)
    {
        dbg_printf(DBG_LESS_INFO, "g_uart_rx_buf[%d]:0x%02x \n", i, g_uart_rx_buf[i]);
    }
    #endif

    read_cmd_handler((uint8_t *)g_uart_rx_buf, sizeof(g_uart_rx_buf));

    // clear cmd header
    memset(g_uart_rx_buf, 0xFF, uart_buf_size);
    dev_uart_comm->uart_read(g_uart_rx_buf, uart_buf_size);

    return INFRA_EVT_RTN_CLEAR;
}

// raise uart rx event
static void uart_rx_callback_fun(uint32_t status)
{
    //dbg_printf(DBG_LESS_INFO,"Raise UART RX EVT \n");
    infra_evt_raise_ISR(g_event[EVT_INDEX_UART_RX]);
    return;
}

DEV_UART_PTR get_dev_uart_comm()
{
    return dev_uart_comm;
}


