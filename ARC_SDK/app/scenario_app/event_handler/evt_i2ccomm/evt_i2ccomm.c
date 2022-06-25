/**
 ********************************************************************************************
 *  @file      evt_i2ccomm.c
 *  @details   This file contains all i2c communication event related function
 *  @author    himax/902452
 *  @version   V1.0.0
 *  @date      14-July-2019
 *  @copyright (C) COPYRIGHT, Himax, Inc. ALL RIGHTS RESERVED
 *******************************************************************************************/

/********************************************************************************************
 *  History :
 *  2019.07.17 - Initial version
 *  2019.08.02 - issue fixed : refer to i2c_comm.c issue (1)
 *
 *******************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "embARC_debug.h"
#include "board_config.h"

#include "board.h"
#include "event.h"
#include "event_handler_evt.h"
#include "i2c_comm.h"
#include "evt_i2ccomm.h"
#include "hx_drv_CIS_common.h"
#include "ota.h"
#include "powermode.h"
#include "hx_drv_ckgen.h"

#ifdef ALANGO
#include "vep-api.h"
#endif

#include "arc_udma.h"
#include "arc_dmac.h"
#include "peripheral_cmd_handler.h"
#ifdef AUDIO_RECOGNITION
#include "Spotter.h"
#endif

#ifdef AUDIO
#include "aud_lib.h"
#endif
#ifdef QNB_FACEDETECT
#include "qnb_facedetect.h"
#endif

#ifdef SUPPORT_AREA_JUDGEMENT
    #include "area_judgement.h"
#endif
/****************************************************
 * Constant Definition                              *
 ***************************************************/
#define DATA_SFT_OFFSET_0       0
#define DATA_SFT_OFFSET_8       8
#define DATA_SFT_OFFSET_16      16
#define DATA_SFT_OFFSET_24      24

/***************************************************
 * Static Function Declaration
 **************************************************/
static void i2ccomm_callback_fun_tx(void *param);
static void i2ccomm_callback_fun_rx(void *param);
static void i2ccomm_callback_fun_err(void *param);

static void evt_i2ccomm_cmd_process_sysinfo(void);
static void evt_i2ccomm_cmd_process_reg_rw(void);
static void evt_i2ccomm_cmd_process_gpio_ctrl(void);
static void evt_i2ccomm_cmd_process_sensor_reg_rw(void);

#ifdef QNB_FACEDETECT
static void evt_i2ccomm_cmd_process_quantanb(void);
#endif

static I2CCOMM_ERROR_E i2ccomm_cmd_process_ota_flow(void);
static I2CCOMM_ERROR_E i2ccomm_cmd_process_spi(void);
static I2CCOMM_ERROR_E i2ccomm_cmd_process_audio_test(void);

/****************************************************
 * Variable Declaration                             *
 ***************************************************/
extern infra_evt_t g_event[];

// create r/w buffer with size:I2CCOMM_MAX_WBUF_SIZE for general command
unsigned char gWrite_buf[I2CCOMM_MAX_WBUF_SIZE];
unsigned char gRead_buf[I2CCOMM_MAX_RBUF_SIZE];
bool g_eci_status=0;  //0: on 1:off
volatile bool i2c_cb_tx_flag = false;

I2CCOMM_CFG_T gI2CCOMM_cfg = {
    BOARD_DFSS_IICS_SLVADDR,
    i2ccomm_callback_fun_tx, // the callback function will be override at application level, ex: event_handler
    i2ccomm_callback_fun_rx,  // the callback function will be override at application level, ex: event_handler
    i2ccomm_callback_fun_err,
};

typedef void (*i2ccomm_customer)(void);
i2ccomm_customer i2ccomm_cmd_customer_process = NULL;

/***************************************************
 * Function Implementation
 **************************************************/
EVT_IICCOM_ERR_E evt_i2ccomm_init(void)
{
    I2CCOMM_STATUS_E status = I2CCOMM_STATUS_UNKNOWN;

    hx_lib_i2ccomm_init(gI2CCOMM_cfg);

    // Enable I2C Communication Process
    hx_lib_i2ccomm_status(&status);
    if (status == I2CCOMM_STATUS_OPEN)
        return EVT_IICCOMM_ERR_DRVFAIL;

    // override the callback function to support event handler
    // hx_lib_i2ccomm_register_cb(i2ccomm_callback_fun_rx, i2ccomm_callback_fun_tx);
    hx_lib_i2ccomm_register_cb(NULL,evt_i2ccomm_tx_cb);
    // start i2c communication process
    hx_lib_i2ccomm_start(gRead_buf, I2CCOMM_MAX_RBUF_SIZE);

    return EVT_IICCOMM_NO_ERROR;
}

#ifdef OS_FREERTOS
#include "FreeRTOS.h"
#include "task.h"

static TaskHandle_t i2ccomm_task_handle = NULL;

static void i2ccomm_callback_fun_tx(void *param)
{
    dbg_printf(DBG_LESS_INFO, "[MSG] I2CCOMM TX CB\n");
    i2c_cb_tx_flag = true;
    return;
}

// raise rx event
static void i2ccomm_callback_fun_rx(void *param)
{
    //dbg_printf(DBG_LESS_INFO, "[MSG] I2CCOMM RX CB \n"); // Don't use printf in ISR

    BaseType_t xYieldRequired;

    xYieldRequired = xTaskResumeFromISR(i2ccomm_task_handle);

    if( xYieldRequired == pdTRUE )
    {
        #ifdef SUPPORT_AREA_JUDGEMENT
        board_get_rtc_tick(&i2c_t1_lowtick, &i2c_t1_hightick);
        i2c_counts ++;
        #endif
        portYIELD_FROM_ISR();
    }
    return;
}

static void i2ccomm_callback_fun_err(void *param)
{
    //dbg_printf(DBG_LESS_INFO, "[MSG] I2CCOMM ERR CB \n");
    return;
}

static void i2ccomm_task(void *par)
{
    unsigned char feature = I2CCOMM_FEATURE_MAX;
    while (1)
    {
        vTaskSuspend(NULL); /*!< suspend audio_task */

        unsigned char feature = gRead_buf[I2CFMT_FEATURE_OFFSET];
        //dbg_printf(DBG_LESS_INFO, "[MSG] I2CCOMM_SYS_FEATURE: 0x%x \n", feature);

        switch (feature)
        {
            case I2CCOMM_FEATURE_SYS:
                evt_i2ccomm_cmd_process_sysinfo();
                break;
            case I2CCOMM_WEI_REG_ACCESS:
                evt_i2ccomm_cmd_process_reg_rw();
                break;
            case I2CCOMM_WEI_GPIO_CTRL:
                evt_i2ccomm_cmd_process_gpio_ctrl();
                break;
            case I2CCOMM_SENSOR_REG_ACCESS:
                evt_i2ccomm_cmd_process_sensor_reg_rw();
                break;
            case I2CCOMM_FEATURE_OTA_RESERVED:
                i2ccomm_cmd_process_ota_flow();
                break;
		#ifdef AUDIO_RECOGNITION_TEST
	    	case I2CCOMM_FEATURE_AUDIO_TEST:
                i2ccomm_cmd_process_audio_test();
                break;
		#endif
            case I2CCOMM_FEATURE_SPI:
                i2ccomm_cmd_process_spi();
                break;
            case I2CCOMM_FEATURE_CUSTOMER_MIN ... I2CCOMM_FEATURE_CUSTOMER_MAX:
                i2ccomm_cmd_customer_process();
                break;
            case I2CCOMM_FEATURE_MAX:
                break;
        }

        // clear cmd header
        memset(gRead_buf, 0xFF, 4);

        // Enable read process at end of cmd process for next i2c write command
        // Note: during cmd process, any new i2c write command will be drop
        hx_lib_i2ccomm_enable_read(gRead_buf, I2CCOMM_MAX_RBUF_SIZE);
    }

    vTaskDelete(i2ccomm_task_handle);

}

int i2ccomm_task_init(void)
{
    I2CCOMM_STATUS_E status = I2CCOMM_STATUS_UNKNOWN;

    hx_lib_i2ccomm_init(gI2CCOMM_cfg);

    // Enable I2C Communication Process
    hx_lib_i2ccomm_status(&status);
    if (status == I2CCOMM_STATUS_OPEN)
        return -1;

    // override the callback function to support event handler
    // hx_lib_i2ccomm_register_cb(i2ccomm_callback_fun_rx, i2ccomm_callback_fun_tx);

    // start i2c communication process
    hx_lib_i2ccomm_start(gRead_buf, I2CCOMM_MAX_RBUF_SIZE);

    if (xTaskCreate(i2ccomm_task, "i2ccomm_task", 1024, (void *)2, TSK_PRIOR_HI + 2, &i2ccomm_task_handle)
        != pdPASS) {
        dbg_printf(DBG_LESS_INFO,"create i2ccomm_task error\r\n");
        return -1;
    }

    return 0;
}

uint8_t evt_i2ccomm_tx_cb(void)
{
    #ifndef __GNU__
    dbg_printf(DBG_LESS_INFO, "[MSG] EVENT INFRA_EVT_I2CS_TX: event_i2cs_tx_cb TRIGGERED\n");
    #endif
    return INFRA_EVT_RTN_CLEAR;
}

uint8_t evt_i2ccomm_err_cb(void)
{
    #ifndef __GNU__
    dbg_printf(DBG_LESS_INFO, "[MSG] EVENT INFRA_EVT_I2CS_ERR \n");
    #endif
    return INFRA_EVT_RTN_CLEAR;
}

uint8_t evt_i2ccomm_rx_cb(void)
{
    BaseType_t xYieldRequired;

    xYieldRequired = xTaskResumeFromISR(i2ccomm_task_handle);

    if( xYieldRequired == pdTRUE )
    {
        #ifdef SUPPORT_AREA_JUDGEMENT
        board_get_rtc_tick(&i2c_t1_lowtick, &i2c_t1_hightick);
        i2c_counts ++;
        #endif
        portYIELD_FROM_ISR();
    }
    return INFRA_EVT_RTN_CLEAR;
}

#else

uint8_t evt_i2ccomm_tx_cb(void)
{
	i2c_cb_tx_flag = true;
    #ifndef __GNU__
    dbg_printf(DBG_LESS_INFO, "[MSG] EVENT INFRA_EVT_I2CS_TX: event_i2cs_tx_cb TRIGGERED\n");
    #endif
    return INFRA_EVT_RTN_CLEAR;
}

uint8_t evt_i2ccomm_err_cb(void)
{
    #ifndef __GNU__
    dbg_printf(DBG_LESS_INFO, "[MSG] EVENT INFRA_EVT_I2CS_ERR \n");
    #endif
    return INFRA_EVT_RTN_CLEAR;
}

uint8_t evt_i2ccomm_rx_cb(void)
{
    unsigned char feature = gRead_buf[I2CFMT_FEATURE_OFFSET];
    //dbg_printf(DBG_LESS_INFO, "[MSG] I2CCOMM_SYS_FEATURE: 0x%x \n", feature);

    switch (feature)
    {
    case I2CCOMM_FEATURE_SYS:
        evt_i2ccomm_cmd_process_sysinfo();
        break;

    case I2CCOMM_WEI_REG_ACCESS:
        evt_i2ccomm_cmd_process_reg_rw();
        break;

    case I2CCOMM_WEI_GPIO_CTRL:
        evt_i2ccomm_cmd_process_gpio_ctrl();
        break;

    case I2CCOMM_SENSOR_REG_ACCESS:
        evt_i2ccomm_cmd_process_sensor_reg_rw();
        break;

	#ifdef QNB_FACEDETECT
    case I2CCOMM_FEATURE_QUANTA_ISH:
        evt_i2ccomm_cmd_process_quantanb();
        break;
	#endif

    case I2CCOMM_FEATURE_OTA_RESERVED:
	//case I2CCOMM_FEATURE_OTA_ALL:
	//case I2CCOMM_FEATURE_OTA_MEM_DSP:
	//case I2CCOMM_FEATURE_OTA_APP:
	//case I2CCOMM_FEATURE_OTA_APP_CONFIG:
	//case I2CCOMM_FEATURE_OTA_EMZA_CONFIG:
	//case I2CCOMM_FEATURE_OTA_CNNLUT:
        i2ccomm_cmd_process_ota_flow();
        break;
	#ifdef AUDIO_RECOGNITION_TEST
	case I2CCOMM_FEATURE_AUDIO_TEST:
        i2ccomm_cmd_process_audio_test();
        break;
	#endif
    case I2CCOMM_FEATURE_SPI:
        i2ccomm_cmd_process_spi();
        break;
    case I2CCOMM_FEATURE_CUSTOMER_MIN ... I2CCOMM_FEATURE_CUSTOMER_MAX:
   		i2ccomm_cmd_customer_process();
        break;
    case I2CCOMM_FEATURE_MAX:
        break;
    }

    // clear cmd header
    memset(gRead_buf, 0xFF, 4);

    // Enable read process at end of cmd process for next i2c write command
    // Note: during cmd process, any new i2c write command will be drop
    hx_lib_i2ccomm_enable_read(gRead_buf, I2CCOMM_MAX_RBUF_SIZE);

    return INFRA_EVT_RTN_CLEAR;
}

// raise tx event
static void i2ccomm_callback_fun_tx(void *param)
{
    //dbg_printf(DBG_LESS_INFO, "[MSG] I2CCOMM TX CB\n");
    infra_evt_raise_ISR(g_event[EVT_INDEX_I2CS_TX]);
    return;
}

// raise rx event
static void i2ccomm_callback_fun_rx(void *param)
{
    //dbg_printf(DBG_LESS_INFO, "[MSG] I2CCOMM RX CB \n");
    infra_evt_raise_ISR(g_event[EVT_INDEX_I2CS_RX]);
    #ifdef SUPPORT_AREA_JUDGEMENT
    board_get_rtc_tick(&i2c_t1_lowtick, &i2c_t1_hightick);
    i2c_counts ++;
    #endif
    return;
}

static void i2ccomm_callback_fun_err(void *param)
{
    //dbg_printf(DBG_LESS_INFO, "[MSG] I2CCOMM ERR CB \n");
    infra_evt_raise_ISR(g_event[EVT_INDEX_I2CS_ERR]);
    return;
}
#endif

// Command process for FEATURE:SYSTEM_INFO
static void evt_i2ccomm_cmd_process_sysinfo(void)
{
    int retval = 0;
    unsigned int data;
    unsigned char cmd;
    unsigned short checksum;

    retval = hx_lib_i2ccomm_validate_checksum(gRead_buf);
    if (retval != I2CCOMM_NO_ERROR)
    {
        dbg_printf(DBG_LESS_INFO, "[Warning] i2c cmd - checksum validation : FAIL\n");
        return;
    }

    cmd = gRead_buf[I2CFMT_COMMAND_OFFSET];
    dbg_printf(DBG_LESS_INFO, "[MSG] I2CCOMM_SYS_CMD: %x\n", cmd);

    switch (cmd)
    {
    case I2CCOMM_CMD_SYS_GET_VER_BSP:
        // prepare write buffer for write process
        gWrite_buf[I2CFMT_FEATURE_OFFSET] = gRead_buf[I2CFMT_FEATURE_OFFSET];
        gWrite_buf[I2CFMT_COMMAND_OFFSET] = I2CCOMM_CMD_SYS_GET_VER_BSP;
        gWrite_buf[I2CFMT_PAYLOADLEN_MSB_OFFSET] = (I2CCOMM_SYS_CMD_PAYLOAD_VER_BSP >> 8) & 0xFF;
        gWrite_buf[I2CFMT_PAYLOADLEN_LSB_OFFSET] = I2CCOMM_SYS_CMD_PAYLOAD_VER_BSP & 0xFF;

        data = hx_board_get_version();
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET] = data & 0xFF;
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 1] = (data >> DATA_SFT_OFFSET_8) & 0xFF;
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 2] = (data >> DATA_SFT_OFFSET_16) & 0xFF;
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 3] = (data >> DATA_SFT_OFFSET_24) & 0xFF;
        hx_lib_i2ccomm_generate_checksum(gWrite_buf, I2CCOMM_HEADER_SIZE + I2CCOMM_SYS_CMD_PAYLOAD_VER_BSP, &checksum);
        if (retval == I2CCOMM_NO_ERROR)
        {
            gWrite_buf[I2CCOMM_HEADER_SIZE + I2CFMT_PAYLOAD_OFFSET] = checksum & 0xFF;
            gWrite_buf[I2CCOMM_HEADER_SIZE + I2CFMT_PAYLOAD_OFFSET + 1] = (checksum >> DATA_SFT_OFFSET_8) & 0xFF;
        }
        else
        {
            dbg_printf(DBG_LESS_INFO, "[Warning] i2c cmd - checksum generation : FAIL\n");
            gWrite_buf[I2CCOMM_HEADER_SIZE + I2CFMT_PAYLOAD_OFFSET] = 0xFF;
            gWrite_buf[I2CCOMM_HEADER_SIZE + I2CFMT_PAYLOAD_OFFSET + 1] = 0xFF;
        }

        // enable write process for i2c master read command
        hx_lib_i2ccomm_enable_write(gWrite_buf);
        break;
    case I2CCOMM_CMD_SYS_GET_VER_I2C:
        // prepare write buffer for write process
        gWrite_buf[I2CFMT_FEATURE_OFFSET] = gRead_buf[I2CFMT_FEATURE_OFFSET];
        gWrite_buf[I2CFMT_COMMAND_OFFSET] = I2CCOMM_CMD_SYS_GET_VER_I2C;
        gWrite_buf[I2CFMT_PAYLOADLEN_MSB_OFFSET] = (I2CCOMM_SYS_CMD_PAYLOAD_VER_I2C >> 8) & 0xFF;
        gWrite_buf[I2CFMT_PAYLOADLEN_LSB_OFFSET] = I2CCOMM_SYS_CMD_PAYLOAD_VER_I2C & 0xFF;

        data = hx_lib_i2ccomm_version();
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET] = data & 0xFF;
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 1] = (data >> DATA_SFT_OFFSET_8) & 0xFF;
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 2] = (data >> DATA_SFT_OFFSET_16) & 0xFF;
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 3] = (data >> DATA_SFT_OFFSET_24) & 0xFF;
        retval = hx_lib_i2ccomm_generate_checksum(gWrite_buf, I2CCOMM_HEADER_SIZE + I2CCOMM_SYS_CMD_PAYLOAD_VER_I2C, &checksum);
        if (retval == I2CCOMM_NO_ERROR)
        {
            gWrite_buf[I2CCOMM_HEADER_SIZE + I2CFMT_PAYLOAD_OFFSET] = checksum & 0xFF;
            gWrite_buf[I2CCOMM_HEADER_SIZE + I2CFMT_PAYLOAD_OFFSET + 1] = (checksum >> DATA_SFT_OFFSET_8) & 0xFF;
        }
        else
        {
            dbg_printf(DBG_LESS_INFO, "[Warning] i2c cmd - checksum generation : FAIL\n");
            gWrite_buf[I2CCOMM_HEADER_SIZE + I2CFMT_PAYLOAD_OFFSET] = 0xFF;
            gWrite_buf[I2CCOMM_HEADER_SIZE + I2CFMT_PAYLOAD_OFFSET + 1] = 0xFF;
        }

        // enable write process for i2c master read command
        hx_lib_i2ccomm_enable_write(gWrite_buf);
        break;
    default:
        break;
    }
}

// Command process for FEATURE:I2CCOMM_REG_CMD
static void evt_i2ccomm_cmd_process_reg_rw(void)
{
    int retval = 0;
    unsigned int addr;
    unsigned int data;
    unsigned char cmd;
    unsigned short checksum;

    retval = hx_lib_i2ccomm_validate_checksum(gRead_buf);
    if (retval != I2CCOMM_NO_ERROR)
    {
        dbg_printf(DBG_LESS_INFO, "[Warning] i2c cmd - checksum validation : FAIL\n");
        return;
    }

    cmd = gRead_buf[I2CFMT_COMMAND_OFFSET];
    addr = (gRead_buf[I2CFMT_PAYLOAD_OFFSET] << DATA_SFT_OFFSET_24) | (gRead_buf[I2CFMT_PAYLOAD_OFFSET + 1] << DATA_SFT_OFFSET_16) |
           (gRead_buf[I2CFMT_PAYLOAD_OFFSET + 2] << DATA_SFT_OFFSET_8) | (gRead_buf[I2CFMT_PAYLOAD_OFFSET + 3]);

    //TODO: check address range to avoid abnormal access

    dbg_printf(DBG_LESS_INFO, "[MSG] I2CCOMM_REG_CMD: %x\n", cmd);

    switch (cmd)
    {
    case I2CCOMM_CMD_AHB_REG_GET:
        // prepare write buffer for write process
        gWrite_buf[I2CFMT_FEATURE_OFFSET] = gRead_buf[I2CFMT_FEATURE_OFFSET];
        gWrite_buf[I2CFMT_COMMAND_OFFSET] = I2CCOMM_CMD_AHB_REG_GET;
        gWrite_buf[I2CFMT_PAYLOADLEN_MSB_OFFSET] = (I2CCOMM_REG_CMD_PAYLOAD_AHB_REG >> 8) & 0xFF;
        gWrite_buf[I2CFMT_PAYLOADLEN_LSB_OFFSET] = I2CCOMM_REG_CMD_PAYLOAD_AHB_REG & 0xFF;

        data = _arc_read_uncached_32((void *)addr);
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET] = (data >> DATA_SFT_OFFSET_24) & 0xFF;
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 1] = (data >> DATA_SFT_OFFSET_16) & 0xFF;
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 2] = (data >> DATA_SFT_OFFSET_8) & 0xFF;
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 3] = data & 0xFF;
        hx_lib_i2ccomm_generate_checksum(gWrite_buf, I2CCOMM_HEADER_SIZE + I2CCOMM_REG_CMD_PAYLOAD_AHB_REG, &checksum);
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 4] = checksum & 0xFF;
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 5] = (checksum >> DATA_SFT_OFFSET_8) & 0xFF;

        // enable write process for i2c master read command
        hx_lib_i2ccomm_enable_write(gWrite_buf);
        break;

    case I2CCOMM_CMD_AHB_REG_SET:
        data = (gRead_buf[I2CFMT_PAYLOAD_OFFSET + 4] << DATA_SFT_OFFSET_24) | (gRead_buf[I2CFMT_PAYLOAD_OFFSET + 5] << DATA_SFT_OFFSET_16) |
               (gRead_buf[I2CFMT_PAYLOAD_OFFSET + 6] << DATA_SFT_OFFSET_8) | (gRead_buf[I2CFMT_PAYLOAD_OFFSET + 7]);

        _arc_write_uncached_32((void *)addr, data);
        break;

    case I2CCOMM_CMD_AUX_REG_GET:
        // prepare write buffer for write process
        gWrite_buf[I2CFMT_FEATURE_OFFSET] = gRead_buf[I2CFMT_FEATURE_OFFSET];
        gWrite_buf[I2CFMT_COMMAND_OFFSET] = I2CCOMM_CMD_AHB_REG_GET;
        gWrite_buf[I2CFMT_PAYLOADLEN_MSB_OFFSET] = (I2CCOMM_REG_CMD_PAYLOAD_AHB_REG >> 8) & 0xFF;
        gWrite_buf[I2CFMT_PAYLOADLEN_LSB_OFFSET] = I2CCOMM_REG_CMD_PAYLOAD_AHB_REG & 0xFF;

        data = _arc_aux_read(addr);
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET] = (data >> DATA_SFT_OFFSET_24) & 0xFF;
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 1] = (data >> DATA_SFT_OFFSET_16) & 0xFF;
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 2] = (data >> DATA_SFT_OFFSET_8) & 0xFF;
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 3] = data & 0xFF;
        hx_lib_i2ccomm_generate_checksum(gWrite_buf, I2CCOMM_HEADER_SIZE + I2CCOMM_REG_CMD_PAYLOAD_AHB_REG, &checksum);
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 4] = checksum & 0xFF;
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 5] = (checksum >> DATA_SFT_OFFSET_8) & 0xFF;

        // enable write process for i2c master read command
        hx_lib_i2ccomm_enable_write(gWrite_buf);
        break;

    case I2CCOMM_CMD_AUX_REG_SET:
        data = (gRead_buf[I2CFMT_PAYLOAD_OFFSET + 4] << DATA_SFT_OFFSET_24) | (gRead_buf[I2CFMT_PAYLOAD_OFFSET + 5] << DATA_SFT_OFFSET_16) |
               (gRead_buf[I2CFMT_PAYLOAD_OFFSET + 6] << DATA_SFT_OFFSET_8) | (gRead_buf[I2CFMT_PAYLOAD_OFFSET + 7]);

        _arc_aux_write(addr, data);
        break;

    default:
        break;
    }
}

// Command process for FEATURE:I2CCOMM_GPIO_CMD
static void evt_i2ccomm_cmd_process_gpio_ctrl(void)
{
    int retval = 0;
    unsigned char cmd;
    unsigned char io;
    unsigned char data;
    unsigned char val = 0x5A;
    unsigned short checksum;

    retval = hx_lib_i2ccomm_validate_checksum(gRead_buf);
    if (retval != I2CCOMM_NO_ERROR)
    {
        dbg_printf(DBG_LESS_INFO, "[Warning] i2c cmd - checksum validation : FAIL\n");
        return;
    }

     cmd = gRead_buf[I2CFMT_COMMAND_OFFSET];
      io = gRead_buf[I2CFMT_PAYLOAD_OFFSET];
    data = gRead_buf[I2CFMT_PAYLOAD_OFFSET + 1];

    dbg_printf(DBG_LESS_INFO, "[MSG] process_gpio_ctrl: 0x%02x, 0x%02x, 0x%02x \n", cmd, io, data);

    switch (cmd)
    {
    case I2CCOMM_CMD_GPIO_MUX:
        hx_drv_iomux_set_pmux(io, data);
        dbg_printf(DBG_LESS_INFO, "[MSG] GPIO_CMD: mux(%d, %d)\n", io, data);
        break;

    case I2CCOMM_CMD_GPIO_SET:
        hx_drv_iomux_set_outvalue(io, data);
        dbg_printf(DBG_LESS_INFO, "[MSG] GPIO_CMD: set(%d, %d)\n", io, data);
        break;

    case I2CCOMM_CMD_GPIO_GET:
        hx_drv_iomux_get_invalue(io, &val);
        dbg_printf(DBG_LESS_INFO, "[MSG] GPIO_CMD: get(%d, %d)\n", io, val);

        // prepare write buffer for write process
        gWrite_buf[I2CFMT_FEATURE_OFFSET] = gRead_buf[I2CFMT_FEATURE_OFFSET];
        gWrite_buf[I2CFMT_COMMAND_OFFSET] = I2CCOMM_CMD_GPIO_GET;
        gWrite_buf[I2CFMT_PAYLOADLEN_LSB_OFFSET] = I2CCOMM_GPIO_CMD_PAYLOAD_GET_VAL & 0xFF;
        gWrite_buf[I2CFMT_PAYLOADLEN_MSB_OFFSET] = (I2CCOMM_GPIO_CMD_PAYLOAD_GET_VAL >> 8) & 0xFF;
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET] = (data == val);

        hx_lib_i2ccomm_generate_checksum(gWrite_buf, I2CCOMM_HEADER_SIZE + I2CCOMM_GPIO_CMD_PAYLOAD_GET_VAL, &checksum);

        // dbg_printf(DBG_LESS_INFO, "[MSG] checksum: 0x%04x\n", checksum);

        if (retval == I2CCOMM_NO_ERROR)
        {
            gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 1] = checksum & 0xFF;
            gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 2] = (checksum >> DATA_SFT_OFFSET_8) & 0xFF;
        }
        else
        {
            dbg_printf(DBG_LESS_INFO, "[Warning] i2c cmd - checksum generation : FAIL\n");
            gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 1] = 0xFF;
            gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 2] = 0xFF;
        }

        // enable write process for i2c master read command
        hx_lib_i2ccomm_enable_write(gWrite_buf);
        break;

    default:
        break;
    }
}

// Command process for FEATURE:I2CCOMM_SENSOR_REG_ACCESS
static void evt_i2ccomm_cmd_process_sensor_reg_rw(void)
{
    int retval = 0;
    unsigned char cmd;
    unsigned int addr;
    unsigned char val = 0x5A;
    unsigned short checksum;

    retval = hx_lib_i2ccomm_validate_checksum(gRead_buf);
    if (retval != I2CCOMM_NO_ERROR)
    {
        dbg_printf(DBG_LESS_INFO, "[Warning] i2c cmd - sensor reg r/w - checksum validation : FAIL\n");
        return;
    }

    cmd  =  gRead_buf[I2CFMT_COMMAND_OFFSET];
    addr = (gRead_buf[I2CFMT_PAYLOAD_OFFSET] << DATA_SFT_OFFSET_8) | (gRead_buf[I2CFMT_PAYLOAD_OFFSET + 1]);

    switch (cmd)
    {
    case I2CCOMM_CMD_SENSOR_REG_SET:
        val =  gRead_buf[I2CFMT_PAYLOAD_OFFSET + 2];
        dbg_printf(DBG_LESS_INFO, "[MSG] process_sensor_reg_set(0x%04x, 0x%02x) \n", addr, val);
        hx_drv_cis_set_reg(addr, val, 0);
        break;
    case I2CCOMM_CMD_SENSOR_REG_GET:
        hx_drv_cis_get_reg(addr, &val);
        dbg_printf(DBG_LESS_INFO, "[MSG] process_sensor_reg_get(0x%04x, 0x%02x) \n", addr, val);

        // prepare write buffer for write process
        gWrite_buf[I2CFMT_FEATURE_OFFSET] = gRead_buf[I2CFMT_FEATURE_OFFSET];
        gWrite_buf[I2CFMT_COMMAND_OFFSET] = I2CCOMM_CMD_SENSOR_REG_GET;
        gWrite_buf[I2CFMT_PAYLOADLEN_LSB_OFFSET] =  I2CCOMM_SENSOR_CMD_PAYLOAD_GET_VAL & 0xFF;
        gWrite_buf[I2CFMT_PAYLOADLEN_MSB_OFFSET] = (I2CCOMM_SENSOR_CMD_PAYLOAD_GET_VAL >> 8) & 0xFF;
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET] = val;
        hx_lib_i2ccomm_generate_checksum(gWrite_buf, I2CCOMM_HEADER_SIZE + I2CCOMM_SENSOR_CMD_PAYLOAD_GET_VAL, &checksum);

        // dbg_printf(DBG_LESS_INFO, "[MSG] checksum: 0x%04x\n", checksum);
        if (retval == I2CCOMM_NO_ERROR)
        {
            gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 1] = checksum & 0xFF;
            gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 2] = (checksum >> DATA_SFT_OFFSET_8) & 0xFF;
        }
        else
        {
            dbg_printf(DBG_LESS_INFO, "[Warning] i2c cmd - checksum generation : FAIL\n");
            gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 1] = 0xFF;
            gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 2] = 0xFF;
        }

        // enable write process for i2c master read command
        hx_lib_i2ccomm_enable_write(gWrite_buf);
        break;
    default:
        break;
    }
}

#ifdef QNB_FACEDETECT

int indx = 0;

static void evt_i2ccomm_cmd_process_quantanb(void)
{
    int retval = 0;
    unsigned char cmd;
    unsigned short checksum;
    APP_STATE_E app_state;
    uint16_t sensor_width;
    uint16_t sensor_height;
    uint32_t cdm_sensor_rtc;
    uint32_t walkaway_sensor_rtc;
    uint32_t notalone_sensor_rtc;
    uint32_t adaptive_dim_rtc;
#if EMZA_ALGO
#ifdef EMZA_ALGO_FACE_V2
    emza_detection i2c_AlgoRes;
    #if 0
    /* fix warning: unused variable */
    emz_Interface_struct EmzaInterface;
    #endif
#else
    struct_emza_algoResult AlgoRes;
#endif
#endif
    uint16_t value = 0;

    cmd = gRead_buf[I2CFMT_COMMAND_OFFSET];

    // TODO : checksum
    retval = hx_lib_i2ccomm_validate_checksum(gRead_buf);
    if (retval != I2CCOMM_NO_ERROR)
    {
        dbg_printf(DBG_LESS_INFO, "[Warning] i2c cmd - checksum validation : FAIL\n");
        return;
    }

    dbg_printf(DBG_LESS_INFO, "[MSG] I2CCOMM_QUANDA_CMD: %x\n", cmd);
    app_get_cur_state(&app_state);
    app_get_flashcfg_sensor_rtc(&cdm_sensor_rtc, &walkaway_sensor_rtc, &notalone_sensor_rtc, &adaptive_dim_rtc);

    switch (cmd)
    {
    case I2CCOMM_CMD_QUANDA_FD_SET_START:
        if (app_state == APP_STATE_STOP)
        {
            // start Face Detection process
            dbg_printf(DBG_LESS_INFO, "\n\n\n\nfrom %d to %d\n", app_state, APP_STATE_WALK_AWAY_LOCK);
            app_get_flashcfg_sensorres_by_appstate(APP_STATE_WALK_AWAY_LOCK, &sensor_width, &sensor_height);
            app_setup_hw5x5_scenario_path(APP_STATE_WALK_AWAY_LOCK, 0, walkaway_sensor_rtc, sensor_width, sensor_height);
        }
        break;

    case I2CCOMM_CMD_QUANDA_FD_SET_STOP:
        //		if ( app_state == APP_STATE_WALK_AWAY_LOCK || app_state == APP_STATE_NOT_ALONE || app_state == APP_STATE_ADAPTIVE_DIMMING )
        {
            // stop Face Detection process
            dbg_printf(DBG_LESS_INFO, "\n\n\n\nfrom %d to %d\n", app_state, APP_STATE_STOP);
            app_get_flashcfg_sensorres_by_appstate(APP_STATE_STOP, &sensor_width, &sensor_height);
            app_setup_change_state(APP_STATE_STOP, 0, cdm_sensor_rtc, sensor_width, sensor_height);
        }
        break;

    case I2CCOMM_CMD_QUANDA_FD_GET_STATE:
        // prepare write buffer for write process
        gWrite_buf[I2CFMT_FEATURE_OFFSET] = gRead_buf[I2CFMT_FEATURE_OFFSET];
        gWrite_buf[I2CFMT_COMMAND_OFFSET] = I2CCOMM_CMD_QUANDA_FD_GET_STATE;
        gWrite_buf[I2CFMT_PAYLOADLEN_MSB_OFFSET] = (I2CCOMM_QUANTA_CMD_PAYLOAD_STATE >> 8) & 0xFF;
        gWrite_buf[I2CFMT_PAYLOADLEN_LSB_OFFSET] = I2CCOMM_QUANTA_CMD_PAYLOAD_STATE & 0xFF;

        gWrite_buf[I2CFMT_PAYLOAD_OFFSET] = (unsigned char)app_state & 0xFF;
        hx_lib_i2ccomm_generate_checksum(gWrite_buf, I2CCOMM_HEADER_SIZE + I2CCOMM_QUANTA_CMD_PAYLOAD_STATE, &checksum);
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 1] = checksum & 0xFF;
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET + 2] = (checksum >> DATA_SFT_OFFSET_8) & 0xFF;

        // enable write process for i2c master read command
        hx_lib_i2ccomm_enable_write(gWrite_buf);
        break;

    case I2CCOMM_CMD_QUANDA_FD_GET_RESULT:
#if EMZA_ALGO
        // TODO: prepare algorithm result to write buffer
#ifdef EMZA_ALGO_FACE_V2
        app_get_emza_result(&i2c_AlgoRes);
#endif
        // update human/face information to i2c buffer
        memset(gWrite_buf, 0x00, I2CFMT_PAYLOAD_SIZE);

        gWrite_buf[I2CFMT_FEATURE_OFFSET] = gRead_buf[I2CFMT_FEATURE_OFFSET];
        gWrite_buf[I2CFMT_COMMAND_OFFSET] = I2CCOMM_CMD_QUANDA_FD_GET_RESULT;
        gWrite_buf[I2CFMT_PAYLOADLEN_MSB_OFFSET] = (I2CCOMM_QUANTA_CMD_PAYLOAD_RESULT >> 8) & 0xFF;
        gWrite_buf[I2CFMT_PAYLOADLEN_LSB_OFFSET] = I2CCOMM_QUANTA_CMD_PAYLOAD_RESULT & 0xFF;

        indx = 0;
#ifdef EMZA_ALGO_FACE_V2
        if(i2c_AlgoRes.IsFacedetected == true)
		{
			gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = i2c_AlgoRes.IsFacedetected;
            gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = i2c_AlgoRes.NumberofFace;
            value = i2c_AlgoRes.time ;
            gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
            gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = value & 0xFF;

            // unrolling setup wbuf
            if (i2c_AlgoRes.NumberofFace > 0)
            {
                value = i2c_AlgoRes.face[0].FaceID;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = value & 0xFF;

                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = i2c_AlgoRes.face[0].FaceClass;

            	value = i2c_AlgoRes.face[0].detection.x;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = value & 0xFF;
                value = i2c_AlgoRes.face[0].detection.y;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = value & 0xFF;
                value = i2c_AlgoRes.face[0].detection.h;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = value & 0xFF;
                value = i2c_AlgoRes.face[0].detection.w;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = value & 0xFF;

                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = i2c_AlgoRes.face[0].detection.YAWangle;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = i2c_AlgoRes.face[0].detection.confidence ;

                value = i2c_AlgoRes.face[0].detection.distance ;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = value & 0xFF;

            }

            if (i2c_AlgoRes.NumberofFace > 1)
            {
                value = i2c_AlgoRes.face[1].FaceID;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = value & 0xFF;

                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = i2c_AlgoRes.face[1].FaceClass;

                value = i2c_AlgoRes.face[1].detection.x;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = value & 0xFF;
                value = i2c_AlgoRes.face[1].detection.y;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = value & 0xFF;
                value = i2c_AlgoRes.face[1].detection.h;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = value & 0xFF;
                value = i2c_AlgoRes.face[1].detection.w;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = value & 0xFF;

                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = i2c_AlgoRes.face[1].detection.YAWangle;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = i2c_AlgoRes.face[1].detection.confidence ;

                value = i2c_AlgoRes.face[1].detection.distance ;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = value & 0xFF;
            }
#else
        if(AlgoRes.humanPresence == true)
        {
			gWrite_buf[I2CFMT_PAYLOAD_OFFSET] = AlgoRes.humanPresence;
            gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = AlgoRes.num_tracked_human_targets;
            // unrolling setup wbuf
            if (AlgoRes.num_tracked_human_targets > 0)
            {
                value = AlgoRes.ht[0].head_bbox.x;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET +indx++] = value & 0xFF;
                value = AlgoRes.ht[0].head_bbox.y;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = value & 0xFF;
                value = AlgoRes.ht[0].head_bbox.height;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET +indx++] = value & 0xFF;
                value = AlgoRes.ht[0].head_bbox.width;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = value & 0xFF;
            }
            if (AlgoRes.num_tracked_human_targets > 1)
            {
                value = AlgoRes.ht[1].head_bbox.x;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = value & 0xFF;
                value = AlgoRes.ht[1].head_bbox.y;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = value & 0xFF;
                value = AlgoRes.ht[1].head_bbox.height;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = value & 0xFF;
                value = AlgoRes.ht[1].head_bbox.width;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = (value >> 8) & 0xFF;
                gWrite_buf[I2CFMT_PAYLOAD_OFFSET + indx++] = value & 0xFF;
            }
#endif
        }

        hx_lib_i2ccomm_generate_checksum(gWrite_buf, I2CCOMM_HEADER_SIZE + I2CCOMM_QUANTA_CMD_PAYLOAD_RESULT, &checksum);
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET + I2CCOMM_QUANTA_CMD_PAYLOAD_RESULT] = checksum & 0xFF;
        gWrite_buf[I2CFMT_PAYLOAD_OFFSET + I2CCOMM_QUANTA_CMD_PAYLOAD_RESULT + 1] = (checksum >> DATA_SFT_OFFSET_8) & 0xFF;

        // enable write process for i2c master read command
        hx_lib_i2ccomm_enable_write(gWrite_buf);

#endif	//#if EMZA_ALGO
        break;

    case I2CCOMM_CMD_QUANDA_FD_SET_STANDBY0:
        if (app_state == APP_STATE_WALK_AWAY_LOCK || app_state == APP_STATE_NOT_ALONE || app_state == APP_STATE_ADAPTIVE_DIMMING)
        {
            // CDM and SLEEP1 mode
            dbg_printf(DBG_LESS_INFO, "\n\n\n\nfrom %d to %d\n", app_state, APP_STATE_CDM_PMU);
            app_get_flashcfg_sensorres_by_appstate(APP_STATE_CDM_PMU, &sensor_width, &sensor_height);
            app_setup_change_state(APP_STATE_CDM_PMU, 0, cdm_sensor_rtc, sensor_width, sensor_height);
        }
        else if (app_state == APP_STATE_STOP)
        {
            // CDM and SLEEP1 mode
            dbg_printf(DBG_LESS_INFO, "\n\n\n\nfrom %d to %d\n", app_state, APP_STATE_CDM_PMU);
            app_get_flashcfg_sensorres_by_appstate(APP_STATE_CDM_PMU, &sensor_width, &sensor_height);
            app_setup_cdm_scenario_path(APP_STATE_CDM_PMU, 0, cdm_sensor_rtc, sensor_width, sensor_height);
        }
        break;

    case I2CCOMM_CMD_QUANDA_FD_SET_STANDBY1:
        if (app_state == APP_STATE_WALK_AWAY_LOCK || app_state == APP_STATE_NOT_ALONE || app_state == APP_STATE_ADAPTIVE_DIMMING)
        {
            // CDM and ALL ON mode
            dbg_printf(DBG_LESS_INFO, "\n\n\n\nfrom %d to %d\n", app_state, APP_STATE_CDM_ALLON);
            app_get_flashcfg_sensorres_by_appstate(APP_STATE_CDM_ALLON, &sensor_width, &sensor_height);
            app_setup_change_state(APP_STATE_CDM_ALLON, 0, cdm_sensor_rtc, sensor_width, sensor_height);
        }
        else if (app_state == APP_STATE_STOP)
        {
            // CDM and SLEEP1 mode
            dbg_printf(DBG_LESS_INFO, "\n\n\n\nfrom %d to %d\n", app_state, APP_STATE_CDM_PMU);
            app_get_flashcfg_sensorres_by_appstate(APP_STATE_CDM_PMU, &sensor_width, &sensor_height);
            app_setup_cdm_scenario_path(APP_STATE_CDM_ALLON, 0, cdm_sensor_rtc, sensor_width, sensor_height);
        }
        break;

    default:
        break;
    }
}
#endif

#define ota_data_mem 0x20050000
static uint32_t ota_audio_data_count = 0;
static uint32_t ota_data_length = 0;

#define AUDIO_TEST_ADDRESS 0x20124E70


// Command process for FEATURE:SYSTEM_INFO
static I2CCOMM_ERROR_E i2ccomm_cmd_process_ota_flow(void)
{
    int retval = 0;
    unsigned char cmd;
    unsigned int  data;
    int sec_errcode;
    int count=0,ret=0xFF,group_id=0,cmd_id=0;
    uint32_t data_len=0;
    unsigned short checksum;
    unsigned char *mic=NULL, *mout=NULL;


    cmd = gRead_buf[I2CFMT_COMMAND_OFFSET];
    //DumpHex(gRead_buf,I2CCOMM_MAX_RBUF_SIZE);

    // TODO : checksum
    retval = hx_lib_i2ccomm_validate_checksum(gRead_buf);

    if (retval != I2CCOMM_NO_ERROR)
    {
        set_ota_result(cmd, OTA_ERROR_CHECKSUM);
        return I2CCOMM_ERROR_CHECKSUM;
    }

    switch (cmd)
    {
	case I2CCOMM_CMD_OTA_SENT_RESULT:
		dbg_printf(DBG_LESS_INFO,"===I2CCOMM_CMD_OTA_SENT_RESULT===\n");
		gWrite_buf[I2CFMT_FEATURE_OFFSET] = gRead_buf[I2CFMT_FEATURE_OFFSET];
		gWrite_buf[I2CFMT_COMMAND_OFFSET] = cmd;
		gWrite_buf[I2CFMT_PAYLOADLEN_MSB_OFFSET] = 0x00;
		gWrite_buf[I2CFMT_PAYLOADLEN_LSB_OFFSET] = 0x04;

		data = get_ota_result();
		gWrite_buf[I2CFMT_PAYLOAD_OFFSET] = data & 0xFF;
		gWrite_buf[I2CFMT_PAYLOAD_OFFSET+1] = (data >>  8) & 0xFF;
		gWrite_buf[I2CFMT_PAYLOAD_OFFSET+2] = (data >> 16) & 0xFF;
		gWrite_buf[I2CFMT_PAYLOAD_OFFSET+3] = (data >> 24) & 0xFF;
		retval = hx_lib_i2ccomm_generate_checksum(gWrite_buf, I2CCOMM_HEADER_SIZE+I2CCOMM_SYS_CMD_PAYLOAD_VER_I2C, &checksum);
		gWrite_buf[I2CFMT_PAYLOAD_OFFSET+4] = checksum & 0xFF;
		gWrite_buf[I2CFMT_PAYLOAD_OFFSET+5] = (checksum >>8) & 0xFF;
		// enable write process for i2c master read command
		hx_lib_i2ccomm_enable_write(gWrite_buf);
	break;
	case I2CCOMM_CMD_OTA_JUMP2UPG:
		dbg_printf(DBG_LESS_INFO, "===I2CCOMM_CMD_OTA_JUMP2UPG===\n");
		uint32_t indication_status = 0;
		uint32_t version_value = 0;
		bool udma_status=0;

		//get chip version
		version_value = _arc_read_uncached_32((void *)HW_PMU_CHIP_VER_ADDR);
	//	version_value = HX_BIT_GET(version_value,PMU_VERSION_WIDTH, PMU_VERSION_OFFSET);

		udma_status=dmac_active();
		if(udma_status == 1)
			dmac_deinit();

		if(version_value == CHIP_VER_A)
		{
			indication_status = _arc_read_uncached_32((void *)0xB0000008);
			indication_status = indication_status >> INDICATION_FLAG_OFFSET;
		}
		else
			indication_status = _arc_read_uncached_32((void *)0xB0000070);

		if((indication_status&INDICATION_OTA) != INDICATION_OTA)
		{
			if(version_value == CHIP_VER_A)
			{
				HX_UNCACHE_BIT_SET((uint32_t)0xB0000008,(uint32_t)INDICATION_FLAG_SIZE,(uint32_t)INDICATION_FLAG_OFFSET,(uint32_t)INDICATION_OTA);
				_dma_channel_clear((1 << 12));
				_dma_chan_reset((1 << 12));
				hx_boot_branch(0x20000004);//jump to bootloader
			}
			else
			{
				HX_UNCACHE_BIT_SET((uint32_t)0xB0000070,(uint32_t)INDICATION_FLAG_SIZE,0,(uint32_t)INDICATION_OTA);
				#ifdef AUDIO
				hx_lib_audio_deinit();
				#endif
				hx_lib_pm_cpu_rst();
			}
		}
		break;
		case I2CCOMM_CMD_OTA_TURN_ON_ECI:
			dbg_printf(DBG_LESS_INFO, "Turn on ECI module\n");
			hx_drv_ckgen_set_gated(SYS_SEC, 0);
			g_eci_status=0;
			count=0;
			while(1)
			{
				count++;
				if(count==500000)
					break;
			};
			break;
		case I2CCOMM_CMD_OTA_TURN_OFF_ECI:
			dbg_printf(DBG_LESS_INFO, "Turn off ECI module\n");
			hx_drv_ckgen_set_gated(SYS_SEC, 1);
			g_eci_status=1;
			break;

    case I2CCOMM_CMD_OTA_SEND_CHIP_ID:
        dbg_printf(DBG_LESS_INFO, "===I2CCOMM_CMD_OTA_SEND_CHIP_ID===\n");
        sec_errcode = read_otp_init();
        if (sec_errcode == 0){
            i2c_read_otp_chip_id((uint8_t *)&gWrite_buf,
                                 (uint8_t *)&gRead_buf[I2CFMT_FEATURE_OFFSET],
                                 I2CCOMM_CMD_OTA_SEND_CHIP_ID);
        }else {
            EMBARC_PRINTF("read otp init error. errorcode = 0x%x\n", sec_errcode);
        }
        break;
#ifdef AUDIO_RECOGNITION
	case I2CCOMM_CMD_OTA_TURN_OFF_PDM:
		dbg_printf(DBG_LESS_INFO, "===I2CCOMM_CMD_OTA_TURN_OFF_PDM===\n");
		hx_lib_audio_deinit();
		break;
	case I2CCOMM_CMD_OTA_AUDIO_RECOGNITION_INIT:
		dbg_printf(DBG_LESS_INFO, "===111I2CCOMM_CMD_OTA_AUDIO_RECOGNITION_INIT===\n");
		Spotter_GetResultId(0);
		Spotter_GetResultGroupId(0);
		ota_data_length = gRead_buf[I2CCOMM_HEADER_SIZE] | (gRead_buf[I2CCOMM_HEADER_SIZE+1] << 8) | (gRead_buf[I2CCOMM_HEADER_SIZE+2] << 16)| (gRead_buf[I2CCOMM_HEADER_SIZE+3] << 24);
		dbg_printf(DBG_LESS_INFO, "2222ota_data_length=%x\n",ota_data_length);
		break;
	case I2CCOMM_CMD_OTA_AUDIO_RECOGNITION_RECV_DATA:
		//dbg_printf(DBG_LESS_INFO, "===I2CCOMM_CMD_OTA_AUDIO_RECOGNITION_RECV_DATA===\n");
		data_len = gRead_buf[I2CFMT_PAYLOADLEN_LSB_OFFSET] | (gRead_buf[I2CFMT_PAYLOADLEN_MSB_OFFSET] << 8);
		memcpy((uint8_t *)(AUDIO_TEST_ADDRESS+ota_audio_data_count),&gRead_buf[I2CCOMM_HEADER_SIZE],data_len);
		ota_audio_data_count+=data_len;
		break;
	case I2CCOMM_CMD_OTA_AUDIO_RECOGNITION:
		dbg_printf(DBG_LESS_INFO, "===I2CCOMM_CMD_OTA_AUDIO_RECOGNITION ota_data_length=%x===\n",ota_data_length);
		//DumpHexEX( (void *) AUDIO_TEST_ADDRESS,ota_data_length);
		ret=doRecognizeFile((unsigned char*)AUDIO_TEST_ADDRESS,ota_data_length);  //1 1 channel
		//ret=dspotter_recognize((unsigned char*)AUDIO_TEST_ADDRESS,ota_data_length);
		Spotter_Reset(0);
		ota_data_length=0;
		ota_audio_data_count=0;
		free(mic);
		free(mout);
		//dbg_printf(DBG_LESS_INFO, "Audio detect result gpID=%d cmdID=%d \n", Spotter_GetResultGroupId(0), Spotter_GetResultId(0));
		break;
	case I2CCOMM_CMD_OTA_GET_AUDIO_RESULT:
		dbg_printf(DBG_LESS_INFO,"===I2CCOMM_CMD_OTA_GET_AUDIO_RESULT===\n");
		//DumpHex(gRead_buf,I2CCOMM_MAX_RBUF_SIZE);
		// prepare write buffer for write process
		gWrite_buf[I2CFMT_FEATURE_OFFSET] = gRead_buf[I2CFMT_FEATURE_OFFSET];
		gWrite_buf[I2CFMT_COMMAND_OFFSET] = cmd;
		gWrite_buf[I2CFMT_PAYLOADLEN_MSB_OFFSET] = 0x00;
		gWrite_buf[I2CFMT_PAYLOADLEN_LSB_OFFSET] = 0x04;

		group_id = Spotter_GetResultGroupId(0);
		cmd_id = Spotter_GetResultId(0);
		gWrite_buf[I2CFMT_PAYLOAD_OFFSET] = group_id & 0xFF;
		gWrite_buf[I2CFMT_PAYLOAD_OFFSET+1] = (group_id >>  8) & 0xFF;
		gWrite_buf[I2CFMT_PAYLOAD_OFFSET+2] = cmd_id & 0xFF;
		gWrite_buf[I2CFMT_PAYLOAD_OFFSET+3] = (cmd_id >>  8) & 0xFF;
		retval = hx_lib_i2ccomm_generate_checksum(gWrite_buf, I2CCOMM_HEADER_SIZE+I2CCOMM_SYS_CMD_PAYLOAD_VER_I2C, &checksum);
		gWrite_buf[I2CFMT_PAYLOAD_OFFSET+4] = checksum & 0xFF;
		gWrite_buf[I2CFMT_PAYLOAD_OFFSET+5] = (checksum >>8) & 0xFF;
		// enable write process for i2c master read command
		hx_lib_i2ccomm_enable_write(gWrite_buf);
        break;
#endif
    case I2CCOMM_CMD_OTA_GET_ALGO_ID:
        dbg_printf(DBG_LESS_INFO, "===I2CCOMM_CMD_OTA_GET_ALGO_ID===\n");
        sec_errcode = read_otp_init();
        if (sec_errcode == 0){
            i2c_read_otp_algo_id((uint8_t *)&gWrite_buf,
                                 (uint8_t *)&gRead_buf[I2CFMT_FEATURE_OFFSET],
                                 I2CCOMM_CMD_OTA_GET_ALGO_ID);
        }else {
            EMBARC_PRINTF("read otp init error. errorcode = 0x%x\n", sec_errcode);
        }
        break;
	default:

        break;
    }

    return I2CCOMM_NO_ERROR;
}
#ifdef AUDIO_RECOGNITION_TEST
static uint32_t rece_count = 0;
static uint32_t rece_data_length = 0;
#define FRAME 16*8 // for 16 kHz
#ifdef ALANGO
extern mem_reg_t reg[1];
#endif
static uint32_t data_size;

uint32_t block;

static uint32_t audio_buf_addr;
audio_config_t aud_pdm_cfg;
uint32_t wrap_test = 0;
uint32_t read_test = 0;
#ifdef ALANGO
#define Audio_Measure_ADDRESS 0x20120000
#else
#define Audio_Measure_ADDRESS 0x20036000
#endif
uint32_t audio_Measure =Audio_Measure_ADDRESS;
#define AUDIO_TEST_ADDRESS Audio_Measure_ADDRESS


#define save_measure_data_size (0x20170000-Audio_Measure_ADDRESS)
extern int Check_Spotter;
uint32_t ALANGO_SIZE=0;


#define I2CCOMM_PAYLOAD_SIZE_EX    (5*1024)
#define I2CCOMM_MAX_WBUF_SIZE_EX   (I2CCOMM_HEADER_SIZE + I2CCOMM_PAYLOAD_SIZE_EX + I2CCOMM_CHECKSUM_SIZE)
#define I2CCOMM_MAX_RBUF_SIZE_EX   (I2CCOMM_HEADER_SIZE + I2CCOMM_PAYLOAD_SIZE_EX + I2CCOMM_CHECKSUM_SIZE)
uint8_t g_audio_flag=0;

int i2ccomm_cmd_send(uint32_t SRAM_addr, uint32_t img_size,  uint8_t data_type)
{
    uint8_t buffer_iic[I2CCOMM_MAX_WBUF_SIZE_EX]={0};
    uint32_t iic_imgsize = 0, iic_cur_pos = 0;
    int s_cmd_count = 0;
    DEV_IIC_S_PTR dev_iic_s = hx_drv_i2cs_get_dev();

    i2c_cb_tx_flag = false;

    buffer_iic[0] = gRead_buf[I2CFMT_FEATURE_OFFSET];
    buffer_iic[1] = data_type;

    //send total data size
    iic_imgsize = 4;
    buffer_iic[2] = iic_imgsize & 0xFF;
    buffer_iic[3] = (iic_imgsize >> 8) & 0xFF;
    buffer_iic[4] = img_size&0xff; //data size
    buffer_iic[5] = (img_size>>8)&0xff; //data size
    buffer_iic[6] = (img_size>>16)&0xff; //data size
    buffer_iic[7] = (img_size>>24)&0xff; //data size

    if(I2CCOMM_NO_ERROR != hx_lib_i2ccomm_generate_checksum(buffer_iic,I2CCOMM_HEADER_SIZE+iic_imgsize,&buffer_iic[4+iic_imgsize]))
    {
        dbg_printf(DBG_LESS_INFO,"hx_lib_i2ccomm_generate_checksum error : %s - %d\n",__FUNCTION__,__LINE__);
        return -1;
    }

    if(0 > dev_iic_s->iic_s_write((const char *)buffer_iic,(4+iic_imgsize+2)))
    {
        dbg_printf(DBG_LESS_INFO,"I2C send error : %s - %d\n",__FUNCTION__,__LINE__);
        return -1;
    }

    while(i2c_cb_tx_flag == false)
    {
        board_delay_ms(1);
        /*s_cmd_count +=1;
        if(s_cmd_count >100000)
            return -1;	*/
    }

	dbg_printf(DBG_LESS_INFO,"send data : %s - %d\n",__FUNCTION__,__LINE__);
    //send data
   	iic_imgsize = img_size;
    iic_cur_pos = SRAM_addr;
    while(iic_imgsize > 0)
    {
        i2c_cb_tx_flag = false;
        if(iic_imgsize > I2CCOMM_PAYLOAD_SIZE_EX)
        {
            buffer_iic[2] = 0;
            buffer_iic[3] = 1;
            memcpy(&buffer_iic[4],(void *)iic_cur_pos,I2CCOMM_PAYLOAD_SIZE_EX);
            if(I2CCOMM_NO_ERROR != hx_lib_i2ccomm_generate_checksum(buffer_iic,I2CCOMM_HEADER_SIZE+I2CCOMM_PAYLOAD_SIZE_EX,&buffer_iic[I2CCOMM_HEADER_SIZE+I2CCOMM_PAYLOAD_SIZE_EX]))
            {
                dbg_printf(DBG_LESS_INFO,"hx_lib_i2ccomm_generate_checksum error : %s - %d\n",__FUNCTION__,__LINE__);
                return -1;
            }
            if(0 > dev_iic_s->iic_s_write((const char *)buffer_iic,I2CCOMM_MAX_WBUF_SIZE_EX))
            {
                dbg_printf(DBG_LESS_INFO,"I2C send error : %s - %d\n",__FUNCTION__,__LINE__);
                return -1;
            }
            iic_cur_pos += I2CCOMM_PAYLOAD_SIZE_EX;
            iic_imgsize -= I2CCOMM_PAYLOAD_SIZE_EX;
        }
        else
        {
            buffer_iic[2] = iic_imgsize & 0xFF;
            buffer_iic[3] = (iic_imgsize >> 8) & 0xFF;
            memcpy(&buffer_iic[I2CCOMM_HEADER_SIZE],(void *)iic_cur_pos,iic_imgsize);
            if(I2CCOMM_NO_ERROR != hx_lib_i2ccomm_generate_checksum(buffer_iic,I2CCOMM_HEADER_SIZE+iic_imgsize,&buffer_iic[I2CCOMM_HEADER_SIZE+iic_imgsize]))
            {
                dbg_printf(DBG_LESS_INFO,"hx_lib_i2ccomm_generate_checksum error : %s - %d\n",__FUNCTION__,__LINE__);
                return -1;
            }
            if(0 > dev_iic_s->iic_s_write((const char *)buffer_iic,(I2CCOMM_HEADER_SIZE+iic_imgsize+I2CCOMM_CHECKSUM_SIZE)))
            {
                dbg_printf(DBG_LESS_INFO,"I2C send error : %s - %d\n",__FUNCTION__,__LINE__);
                return -1;
            }
            iic_imgsize = 0;
        }
        while(i2c_cb_tx_flag == false)
        {
            board_delay_ms(1);
           /* s_cmd_count +=1;
            if(s_cmd_count >100000)
            {
            	//dbg_printf(DBG_LESS_INFO,"// wait i2s true %s - %d\n\r",__FUNCTION__,__LINE__);
                return -1;
            }*/
        }
    }

    i2c_cb_tx_flag = false;
    return 0;
}
static void pdm_rx_callback_fun(uint32_t status)
{
    uint32_t pdm_buf_addr;
    uint32_t block;

    hx_lib_audio_request_read(&pdm_buf_addr, &block);
    dbg_printf(DBG_LESS_INFO, "addr:0x%08x, block_num:%d \n", pdm_buf_addr, block);

    if(read_test == 1)
    {
        hx_lib_audio_update_idx(&block);
    }

    if(wrap_test == 0)
    {
        if (block == (aud_pdm_cfg.block_num - 1))
        {
            hx_lib_audio_stop();
            dbg_printf(DBG_LESS_INFO, "PDM buffer full, block = %d\n", block);
        }
    }
    return;
}

static I2CCOMM_ERROR_E i2ccomm_cmd_process_audio_test(void)
{
    int retval = 0;
    unsigned char cmd;
    unsigned int  data;
    int count=0,ret=0xFF,group_id=0x00,cmd_id=0x00;
    uint32_t data_len=0;
    unsigned short checksum;
	audio_config_t pdm_cfg;
#ifdef ALANGO
    err_t alango_err={0};
#endif
    uint32_t alango_process_count=0;
    uint32_t temp_length=0;
    uint32_t N=0;
	short spk[FRAME ];
	char input[FRAME*4];
	short mux_out[FRAME*3];

	int32_t e_no = E_OK;
	I2CCOMM_ERROR_E errcode = I2CCOMM_NO_ERROR;
	static DEV_IIC_S_PTR dev_iic_s[SS_IIC_S_NUM] = {0};

	int read_status = -1;

    cmd = gRead_buf[I2CFMT_COMMAND_OFFSET];
    //DumpHex(gRead_buf,I2CCOMM_MAX_RBUF_SIZE);

    // TODO : checksum
    retval = hx_lib_i2ccomm_validate_checksum(gRead_buf);

    if (retval != I2CCOMM_NO_ERROR)
    {
        set_ota_result(cmd, OTA_ERROR_CHECKSUM);
        return I2CCOMM_ERROR_CHECKSUM;
    }

    switch (cmd)
    {
    case I2CCOMM_CMD_AUDIO_INIT_PDM:
		dbg_printf(DBG_LESS_INFO, "===I2CCOMM_CMD_AUDIO_INIT_PDM===\n");

		hx_lib_audio_set_if(AUDIO_IF_PDM);
		hx_lib_audio_init();
		// register the callback function
		//hx_lib_audio_register_evt_cb(pdm_rx_callback_fun);

		break;
	case I2CCOMM_CMD_AUDIO_TURN_OFF_PDM:
		dbg_printf(DBG_LESS_INFO, "===I2CCOMM_CMD_AUDIO_TURN_OFF_PDM===\n");
		hx_lib_audio_deinit();
		break;
	case I2CCOMM_CMD_AUDIO_TEST_RECV_DATA_SIZE:
		dbg_printf(DBG_LESS_INFO, "===I2CCOMM_CMD_AUDIO_TEST_RECV_DATA_SIZE===\n");
		//Spotter_GetResultId(0);
		//Spotter_GetResultGroupId(0);
		rece_data_length = gRead_buf[I2CCOMM_HEADER_SIZE] | (gRead_buf[I2CCOMM_HEADER_SIZE+1] << 8) | (gRead_buf[I2CCOMM_HEADER_SIZE+2] << 16)| (gRead_buf[I2CCOMM_HEADER_SIZE+3] << 24);
		dbg_printf(DBG_LESS_INFO, "file data length=%x\n",rece_data_length);
		break;
	case I2CCOMM_CMD_AUDIO_TEST_RECV_DATA:
		//dbg_printf(DBG_LESS_INFO, "===I2CCOMM_CMD_AUDIO_TEST_RECV_DATA===\n");
		data_len = gRead_buf[I2CFMT_PAYLOADLEN_LSB_OFFSET] | (gRead_buf[I2CFMT_PAYLOADLEN_MSB_OFFSET] << 8);
		memcpy((uint8_t *)(AUDIO_TEST_ADDRESS+rece_count),&gRead_buf[I2CCOMM_HEADER_SIZE],data_len);
		rece_count+=data_len;
		break;
	case I2CCOMM_CMD_AUDIO_TEST_RECOGNITION:
		dbg_printf(DBG_LESS_INFO, "===I2CCOMM_CMD_AUDIO_TEST_RECOGNITION===\n");
		//DumpHexEX( (void *) AUDIO_TEST_ADDRESS,ota_data_length);
		//doRecognizeFile((unsigned char*)AUDIO_TEST_ADDRESS,rece_data_length);// 1 channel
		dspotter_recognize((unsigned char*)AUDIO_TEST_ADDRESS,rece_data_length);

		//dbg_printf(DBG_LESS_INFO, "Audio detect result gpID=%d cmdID=%d \n", Spotter_GetResultGroupId(0), Spotter_GetResultId(0));
		break;
	case I2CCOMM_CMD_AUDIO_GET_RESULT:
		dbg_printf(DBG_LESS_INFO,"===I2CCOMM_CMD_OTA_GET_AUDIO_RESULT===\n");
		//DumpHex(gRead_buf,I2CCOMM_MAX_RBUF_SIZE);
		// prepare write buffer for write process
		gWrite_buf[I2CFMT_FEATURE_OFFSET] = gRead_buf[I2CFMT_FEATURE_OFFSET];
		gWrite_buf[I2CFMT_COMMAND_OFFSET] = cmd;
		gWrite_buf[I2CFMT_PAYLOADLEN_MSB_OFFSET] = 0x00;
		gWrite_buf[I2CFMT_PAYLOADLEN_LSB_OFFSET] = 0x04;
		if(Check_Spotter==0)
		{
			group_id = Spotter_GetResultGroupId(0);
			cmd_id = Spotter_GetResultId(0);
			dbg_printf(DBG_LESS_INFO,"Check_Spotter =0 group_id=%d ,cmd_id=%d \r\n",group_id,cmd_id);
			Check_Spotter =254;
		}
		else
		{
			group_id = 254;
			cmd_id = 254;
			dbg_printf(DBG_LESS_INFO,"Check_Spotter !=0 group_id=%d ,cmd_id=%d \r\n",group_id,cmd_id);
		}
		gWrite_buf[I2CFMT_PAYLOAD_OFFSET] = group_id & 0xFF;
		gWrite_buf[I2CFMT_PAYLOAD_OFFSET+1] = (group_id >>  8) & 0xFF;
		gWrite_buf[I2CFMT_PAYLOAD_OFFSET+2] = cmd_id & 0xFF;
		gWrite_buf[I2CFMT_PAYLOAD_OFFSET+3] = (cmd_id >>  8) & 0xFF;
		retval = hx_lib_i2ccomm_generate_checksum(gWrite_buf, I2CCOMM_HEADER_SIZE+I2CCOMM_SYS_CMD_PAYLOAD_VER_I2C, &checksum);
		gWrite_buf[I2CFMT_PAYLOAD_OFFSET+4] = checksum & 0xFF;
		gWrite_buf[I2CFMT_PAYLOAD_OFFSET+5] = (checksum >>8) & 0xFF;
		// enable write process for i2c master read command
		hx_lib_i2ccomm_enable_write(gWrite_buf);
		Spotter_Reset(0);
		rece_data_length=0;
		rece_count=0;
        break;
#ifdef ALANGO
	case I2CCOMM_CMD_AUDIO_ALANGO_PROCESS:
		dbg_printf(DBG_LESS_INFO, "===I2CCOMM_CMD_AUDIO_ALANGO_PROCESS===\n");

		N = get_frame_size();
		memset(spk,0x00,sizeof(spk));
		//dbg_printf(DBG_LESS_INFO, "ALango separate two channels\n");

		while(temp_length<rece_data_length)
		{
			alango_process_count+=1;
			//dbg_printf(DBG_LESS_INFO, "ALango process count : %d \n",alango_process_count);
			memset(mux_out,0xFF,sizeof(mux_out));

			ALango_separate_two_channel((unsigned char *)AUDIO_TEST_ADDRESS+temp_length,input,N);
			//DumpHexEX((void *) input,2*N);
			//dbg_printf(DBG_LESS_INFO, "vep_process_beammux\n");
			alango_err=vep_process_beammux(reg, input, spk, mux_out);
			//dbg_printf(DBG_LESS_INFO, "alango err=%x memb=%x pid=%x\n",alango_err.err,alango_err.memb,alango_err.pid);
			//DumpHexEX((void *) mux_out, N );
			memcpy((unsigned char *) AUDIO_TEST_ADDRESS + (alango_process_count-1)*N,mux_out,N);
			temp_length += 2*N;

		}
		//dbg_printf(DBG_LESS_INFO,"output_addr:0x%08x, sz1:%d \n", pdm_cfg.buffer_addr + (pdm_cfg.cb_evt_blk * pdm_cfg.block_sz), temp_length/2);
		ALANGO_SIZE=alango_process_count*N;
		dbg_printf(DBG_LESS_INFO, "vep_process_beammux Done size=0x%08x\n",ALANGO_SIZE);
		//DumpHexEX((void *) AUDIO_TEST_ADDRESS,alango_process_count*N);
		break;
#endif
	case I2CCOMM_CMD_AUDIO_ALANGO_DATA:
		dbg_printf(DBG_LESS_INFO, "ALANGO_AUDIO_TEST_ADDRESS 0x%08x ALANGO_SIZE %d\n",AUDIO_TEST_ADDRESS,ALANGO_SIZE);
		read_status = i2ccomm_cmd_send(AUDIO_TEST_ADDRESS, ALANGO_SIZE, I2CCOMM_CMD_AUDIO_ALANGO_DATA);
		if (read_status != 0){
			 dbg_printf(DBG_LESS_INFO,"IIC Transfer failed return %d\n", read_status);
		 } else {
			 dbg_printf(DBG_LESS_INFO,"IIC Transfer succeed return %d\n", read_status);
		 }

	break;

	case I2CCOMM_CMD_AUDIO_PDM_RECORD:
		dbg_printf(DBG_LESS_INFO, "===I2CCOMM_CMD_AUDIO_PDM_RECORD===\n");
		//board_delay_ms(600);
		/*pdm_cfg.sample_rate = AUDIO_SR_16KHZ;
		pdm_cfg.buffer_addr = (uint32_t *) (0x200A0000);
		pdm_cfg.block_num = 40;  //40
		pdm_cfg.block_sz = 8192; //8192
		pdm_cfg.cb_evt_blk = 10;//30

		hx_lib_audio_start(&pdm_cfg);*/

		aud_pdm_cfg.sample_rate = AUDIO_SR_16KHZ;
		aud_pdm_cfg.buffer_addr = (uint32_t *) (0x20000000+36*1024);//0x20009000;
		aud_pdm_cfg.block_num = 23;
		aud_pdm_cfg.block_sz = 4096;
		aud_pdm_cfg.cb_evt_blk = 2;

		hx_lib_audio_start(&aud_pdm_cfg);
		break;
	case I2CCOMM_CMD_AUDIO_PDM_GETBUF:
		dbg_printf(DBG_LESS_INFO, "===I2CCOMM_CMD_AUDIO_PDM_GETBUF===\n");
		hx_lib_audio_request_read(&audio_buf_addr, &block);

		DumpHexEX((void *) audio_buf_addr,block*4096);
		dbg_printf(DBG_LESS_INFO, "audio_buf_addr 0x%08x block %d\n",audio_buf_addr,block);
		break;
	case I2CCOMM_CMD_AUDIO_SEND_DATA:
		dbg_printf(DBG_LESS_INFO,"===I2CCOMM_CMD_OTA_GET_AUDIO_SEND_DATA===\n");


		hx_lib_audio_request_read(&audio_buf_addr, &block);
		data_size =(block<<13);
		dbg_printf(DBG_LESS_INFO, "audio_buf_addr 0x%08x data_size %d\n",audio_buf_addr,data_size);
		read_status = i2ccomm_cmd_send(audio_buf_addr, data_size, I2CCOMM_CMD_AUDIO_SEND_DATA);
		if (read_status != 0){
			 dbg_printf(DBG_LESS_INFO,"IIC Transfer failed return %d\n", read_status);
		 } else {
			 dbg_printf(DBG_LESS_INFO,"IIC Transfer succeed return %d\n", read_status);
		 }
		break;
	case I2CCOMM_CMD_AUDIO_OPEN_Measure:
		dbg_printf(DBG_LESS_INFO,"===I2CCOMM_CMD_GET_AUDIO_OPEN_Measure===\n");
		g_audio_flag=1;
		dbg_printf(DBG_LESS_INFO,"audio_Measure 0x%08x\n", audio_Measure);
		memset((void *)audio_Measure,0x00,save_measure_data_size);
		//DumpHexEX((void *) audio_Measure,0x20);
		break;
	case I2CCOMM_CMD_AUDIO_OFF_Measure:
		dbg_printf(DBG_LESS_INFO,"===I2CCOMM_CMD_GET_AUDIO_OFF_Measure===\n");
		g_audio_flag=0;
		dbg_printf(DBG_LESS_INFO, "Audio_Measure_ADDRESS = 0x%08x\n",audio_Measure);
		read_status = i2ccomm_cmd_send(audio_Measure, save_measure_data_size, I2CCOMM_CMD_AUDIO_OFF_Measure);
		if (read_status != 0){
			 dbg_printf(DBG_LESS_INFO,"IIC Transfer failed return %d\n", read_status);
		 } else {
			 dbg_printf(DBG_LESS_INFO,"IIC Transfer succeed return %d\n", read_status);
		 }
		break;
	case I2CCOMM_CMD_AUDIO_Reset:
		dbg_printf(DBG_LESS_INFO,"===I2CCOMM_CMD_AUDIO_Reset===\n");
			HX_UNCACHE_BIT_SET(0xB0000010, 1, 2, 0);
			HX_UNCACHE_BIT_SET(0xB0000020, 1, 14, 1);
		break;
	default:
        break;
    }

    return I2CCOMM_NO_ERROR;
}
#endif

static I2CCOMM_ERROR_E i2ccomm_cmd_process_spi(void)
{
    unsigned char all_on[]          = {0xC0, 0x5A, 0x02, 0x00, 0x00, 0x00, 0x01};
    unsigned char start_jpeg[]      = {0xC0, 0x5A, 0x02, 0x00, 0x00, 0x00, 0x11};
    unsigned char start_metadata[]  = {0xC0, 0x5A, 0x02, 0x00, 0x00, 0x00, 0x21};
    unsigned char start_voice[]     = {0xC0, 0x5A, 0x02, 0x00, 0x00, 0x00, 0x31};
    unsigned char start_pdm[]       = {0xC0, 0x5A, 0x02, 0x00, 0x00, 0x00, 0x41};
    unsigned char start_algo[]      = {0xC0, 0x5A, 0x02, 0x00, 0x00, 0x00, 0x51};
    unsigned char all_off[]         = {0xC0, 0x5A, 0x02, 0x00, 0x00, 0x00, 0x00};
    unsigned char stop_jpeg[]       = {0xC0, 0x5A, 0x02, 0x00, 0x00, 0x00, 0x10};
    unsigned char stop_metadata[]   = {0xC0, 0x5A, 0x02, 0x00, 0x00, 0x00, 0x20};
    unsigned char stop_voice[]      = {0xC0, 0x5A, 0x02, 0x00, 0x00, 0x00, 0x30};
    unsigned char stop_pdm[]        = {0xC0, 0x5A, 0x02, 0x00, 0x00, 0x00, 0x40};
    unsigned char start_rawimg[]    = {0xC0, 0x5A, 0x04, 0x03, 0x00, 0x00, 0x00};
    unsigned char stop_rawimg[]     = {0xC0, 0x5A, 0x04, 0x04, 0x00, 0x00, 0x40};
    unsigned char stop_algo[]       = {0xC0, 0x5A, 0x02, 0x00, 0x00, 0x00, 0x50};

    unsigned char *p;

    int retval = 0;
    unsigned char cmd;
    uint32_t reg_val;

    cmd = gRead_buf[I2CFMT_COMMAND_OFFSET];
    //DumpHex(gRead_buf,I2CCOMM_MAX_RBUF_SIZE);

    // TODO : checksum
    retval = hx_lib_i2ccomm_validate_checksum(gRead_buf);

    if (retval != I2CCOMM_NO_ERROR)
    {
        // set_ota_result(cmd, OTA_ERROR_CHECKSUM);
        return I2CCOMM_ERROR_CHECKSUM;
    }

    if(cmd & SPI_ENABLE_ALL)
    {
        if((cmd & SPI_ALL_ON) == SPI_ALL_ON)
        {
            p = all_on;
        }
        else
        {
            p = all_off;
        }
        read_cmd_handler(p, 7);

        if((cmd & SPI_ENABLE_JPEG) == SPI_ENABLE_JPEG)
        {
            p = start_jpeg;
        }
        else
        {
            p = stop_jpeg;
        }
        read_cmd_handler(p, 7);

        if((cmd & SPI_ENABLE_METADATA) == SPI_ENABLE_METADATA)
        {
            p = start_metadata;
        }
        else
        {
            p = stop_metadata;
        }
        read_cmd_handler(p, 7);

        if((cmd & SPI_ENABLE_VOICE) == SPI_ENABLE_VOICE)
        {
            p = start_voice;
        }
        else
        {
            p = stop_voice;
        }
        read_cmd_handler(p, 7);

        if((cmd & SPI_ENABLE_PDM) == SPI_ENABLE_PDM)
        {
            p = start_pdm;
        }
        else
        {
            p = stop_pdm;
        }
        read_cmd_handler(p, 7);

        if((cmd & SPI_ENABLE_RAWIMG) == SPI_ENABLE_RAWIMG)
        {
            p = start_rawimg;
        }
        else
        {
            p = stop_rawimg;
        }
        read_cmd_handler(p, 7);
    }

    if((cmd & SPI_DISABLE_ALGO) == SPI_DISABLE_ALGO)
    {
        p = stop_algo;
    }
    else
    {
        p = start_algo;
    }
    read_cmd_handler(p, 7);

    if((cmd & SPI_ALL_ON) == 0)
    {
        p = all_off;
        read_cmd_handler(p, 7);
    }
    return I2CCOMM_NO_ERROR;
}

void i2ccomm_cmd_customer_register_cb(void *cb_func)
{
	i2ccomm_cmd_customer_process = cb_func;
}
