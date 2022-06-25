/*
 * evt_spiscomm.c
 *
 *  Created on: 2020/02/03
 *      Author: 903730
 */
#include <stdio.h>
#include <string.h>
#include "embARC_debug.h"
#include "board_config.h"

#include "board.h"
#include "event.h"
#include "event_handler_evt.h"
#include "hx_drv_spi_s.h"
#include "spi_slave_protocol.h"
#include "evt_spiscomm.h"
#include "sensor_dp_lib.h"
#include "peripheral_cmd_handler.h"
#include "printf_color.h"

/****************************************************
 * Constant Definition                              *
 ***************************************************/

/***************************************************
 * Static Function Declaration
 **************************************************/
static void spiscomm_callback_fun_rx(void);
void reset_spis_var();
void detect_spi_s_timeout();
/****************************************************
 * Variable Declaration                             *
 ***************************************************/
extern infra_evt_t g_event[];

volatile static DEV_SPI_SLV_PTR dev_spi_s_ptr;
unsigned char g_spi_s_rx_buf[7]={0,0,0,0,0,0,0};

#ifdef DUT
#define PACKET_SIZE 6000

extern uint32_t g_wdma3_baseaddr;
uint32_t total_img_size = 0;
static uint32_t s_imgsize = 0;
static uint32_t s_cur_pos = 0;

bool algo_run_flag = false;
static uint32_t pretime = 0;
static bool packet_done = true;
#endif

/***************************************************
 * Function Implementation
 **************************************************/
EVT_SPISCOM_ERR_E evt_spiscomm_init(void)
{
    /* just register callback function for idle interrupt to trigger evt_spiscomm_rx_cb */
    dev_spi_s_ptr = hx_drv_spi_slv_get_dev(USE_DFSS_SPI_SLV_0);
    hx_drv_spi_slv_register_rx_evt_cb(spiscomm_callback_fun_rx);
    return EVT_SPISCOMM_NO_ERROR;
}

uint8_t evt_spiscomm_rx_cb(void)
{
//    dbg_printf(DBG_LESS_INFO, "[MSG] EVENT INFRA_EVT_SPIS_RX: event_spis_rx_cb TRIGGERED\n");

    read_cmd_handler((uint8_t *)g_spi_s_rx_buf, sizeof(g_spi_s_rx_buf));

#if 0    
    unsigned char feature = g_spi_s_rx_buf[0]];
    switch (feature)
    {
        case SPISCOMM_FEATURE_XXX:
            break;
    }
#endif
    // clear cmd header 
    memset(g_spi_s_rx_buf, 0xFF, 7);
#ifdef EVT_SPISCOMM
    evt_spiscomm_cmd_read();
#endif
    return INFRA_EVT_RTN_CLEAR;
}

void evt_spiscomm_cmd_read(void)
{
    dev_spi_s_ptr->spi_read((void *) &g_spi_s_rx_buf[0], 7);
}

// raise rx event
static void spiscomm_callback_fun_rx(void)
{
    infra_evt_raise_ISR(g_event[EVT_INDEX_SPIS_RX]);
    
#ifdef DUT
    //get previous time
    pretime = _arc_aux_read(AUX_RTC_LOW);
#endif

    return;
}

#ifdef DUT
uint8_t evt_spiscomm_rx_cb_swtich_cb(void)
{
//    dbg_printf(DBG_LESS_INFO, "[MSG] EVENT INFRA_EVT_SPIS_RX: event_spis_rx_cb TRIGGERED\n");
    uint32_t img_size = 0;
    DBG_MODE_DATA_T dbg_data = {0};
        
    read_cmd_handler((uint8_t *)g_spi_s_rx_buf, sizeof(g_spi_s_rx_buf));

    // clear cmd header 
    memset(g_spi_s_rx_buf, 0xFF, 7);
    evt_spiscomm_cmd_read();

    if(get_spiscomm_evt_cb_type() == PACKET_CMD)
    {   
        #ifndef __GNU__
        dbg_printf(DBG_LESS_INFO, "PACKET_CMD\n");
        #endif
        memset(g_spi_s_rx_buf, 0xFF, 7);
        evt_spiscomm_cmd_read();
    } else {
        #ifndef __GNU__
        dbg_printf(DBG_LESS_INFO, "PACKET_DATA\n");
        #endif
        dbg_data = get_debug_mode_cmd();
        total_img_size = dbg_data.img_size;
        if(dbg_data.img_size > PACKET_SIZE){
            img_size = PACKET_SIZE;
            s_cur_pos += PACKET_SIZE;
            s_imgsize = PACKET_SIZE;
            total_img_size -= PACKET_SIZE;
        }else{
            img_size = dbg_data.img_size;
            total_img_size -= img_size;
        }
        evt_spiscomm_cmd_read_6k((uint8_t *)g_wdma3_baseaddr,img_size);
    }

    return INFRA_EVT_RTN_CLEAR;
}

uint8_t evt_spiscomm_rx_packet_cb(void)
{
    uint32_t buf_addr = 0;
    DBG_MODE_DATA_T dbg_data = get_debug_mode_cmd();
    CAL_CRC_E crc_err = CAL_CRC_ERROR;
    
//    dbg_printf(DBG_LESS_INFO, "[MSG] EVENT INFRA_EVT_SPIS_RX: evt_spiscomm_rx_switch_packet_cb TRIGGERED\n");

    buf_addr = g_wdma3_baseaddr + s_cur_pos;
    if(total_img_size > PACKET_SIZE)
    {
        total_img_size -= PACKET_SIZE;
        s_cur_pos += PACKET_SIZE;
        s_imgsize = PACKET_SIZE;
    }
    else if(total_img_size == 0)
    {
        EMBARC_PRINTF("WE1 Receive Data Done\n");
        EMBARC_PRINTF("Calculate checksum\n");
        crc_err = cal_validate_checksum((uint8_t *)g_wdma3_baseaddr, dbg_data.img_size);  //last 2bytes are Check sum   
        if (crc_err == CAL_CRC_SUCCESS)
        {
            trigger_wdma23_mannually();
            algo_run_flag = true;
        }
        else
        {
            EMBARC_PRINTF("CAL_CRC_ERROR\n");
            algo_run_flag = false;            
        }
        reset_spis_var();
        return INFRA_EVT_RTN_CLEAR;
    }
    else
    {
        total_img_size -= dbg_data.img_size % PACKET_SIZE;
        s_cur_pos += dbg_data.img_size % PACKET_SIZE;
        s_imgsize = dbg_data.img_size % PACKET_SIZE;
        packet_done = false;
    }
    
    evt_spiscomm_cmd_read_6k((uint8_t *)buf_addr, s_imgsize);
    
    return INFRA_EVT_RTN_CLEAR;
}

void reset_spis_var()
{
    memset(g_spi_s_rx_buf, 0xFF, 7);  
    evt_spiscomm_cmd_read();
    set_spiscomm_evt_cb(PACKET_CMD);   
    s_imgsize = 0;
    s_cur_pos = 0;
    packet_done = true;
}

void evt_spiscomm_cmd_read_6k(uint8_t *sram_addr, uint32_t size)
{
    dev_spi_s_ptr->spi_read((void *)sram_addr, size);
}

void detect_spi_s_timeout()
{
    uint32_t interval = 0;
    uint32_t cur_time = 0;
    uint32_t timeout = 2000; //ms
    
    interval = _arc_aux_read(AUX_RTC_LOW) - pretime;

    cur_time = interval/400000U;    // convert to ms

    if(cur_time > timeout && !packet_done){
        EMBARC_PRINTF(LIGHT_GREEN "spi_s timeout > %d ms\n" NONE, timeout);
        reset_spis_var();
        send_timeout_result();
    }
}

#endif


