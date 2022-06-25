/*
 * evt_datapath.c
 *
 *  Created on: 2019/7/30
 *      Author: 902447
 */

#include <stdio.h>
#include <string.h>
#include "embARC_debug.h"

#include "event.h"
#include "event_handler_evt.h"
#include "evt_datapath.h"

/****************************************************
 * Variable Declaration                             *
 ***************************************************/
#define DBG_DP_EVENT_MSG    DBG_LESS_INFO

extern infra_evt_t 	g_event[];

static volatile SENSORDPLIB_STATUS_E g_dplib_event;

static volatile evthandlerdp_CBEvent_t  g_event_cb = NULL;
static ADCC_CFG_T g_evt_adcc_cfg;
static volatile evthandlerGPIO_CBEvent_t  g_gpio_event_cb[IOMUX_SGPIO1 + 1]
														  = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
															 NULL, NULL, NULL, NULL, NULL, NULL, NULL
														  };
static volatile evthandlerADC_CBEvent_t  g_adc_ch0_event_cb = NULL;
static volatile evthandlerADC_CBEvent_t  g_adc_ch1_event_cb = NULL;
static volatile evthandlerADC_CBEvent_t  g_adc_ch2_event_cb = NULL;
static volatile evthandlerADC_CBEvent_t  g_adc_ch3_event_cb = NULL;
static volatile evthandlerADC_CBEvent_t  g_adc_cfg_event_cb = NULL;
static volatile evthandlerADC_CBEvent_t  g_adc_sg_event_cb = NULL;
static volatile evthdlSensorRTC_CBEvent_t  g_sensorRTC_event_cb = NULL;
static volatile evthdlADCRTC_CBEvent_t  g_ADCRTC_event_cb = NULL;
static volatile uint32_t  g_adc_ch0_event = 0;
static volatile uint32_t  g_adc_ch1_event = 0;
static volatile uint32_t  g_adc_ch2_event = 0;
static volatile uint32_t  g_adc_ch3_event = 0;
static volatile uint32_t  g_adc_cfg_event = 0;
static volatile uint32_t g_sensor_rtc_interval = 0;
static volatile uint32_t g_adc_rtc_interval = 0;

static void iomux_pgpio0_cb(void* param);
static void iomux_pgpio1_cb(void* param);
static void iomux_pgpio2_cb(void* param);
static void iomux_pgpio3_cb(void* param);
static void iomux_pgpio4_cb(void* param);
static void iomux_pgpio5_cb(void* param);
static void iomux_pgpio6_cb(void* param);
static void iomux_pgpio7_cb(void* param);
static void iomux_pgpio8_cb(void* param);
static void iomux_pgpio9_cb(void* param);
static void iomux_pgpio10_cb(void* param);
static void iomux_pgpio11_cb(void* param);
static void iomux_pgpio12_cb(void* param);
static void iomux_pgpio13_cb(void* param);
static void iomux_pgpio14_cb(void* param);
static void iomux_sgpio0_cb(void* param);
static void iomux_sgpio1_cb(void* param);
/***************************************************
 * Function Implementation
 **************************************************/
//Periodic Timer Callback
static void sensor_rtc_timer_dump(void *data)
{
//	dbg_printf(DBG_LESS_INFO,"sensor_rtc_timer_dump CB Interval=%d Start\n",g_sensor_rtc_interval);
	infra_evt_raise_ISR(g_event[EVT_INDEX_SENSOR_RTC]);
//	dbg_printf(DBG_LESS_INFO,"sensor_rtc_timer_dump CB Done\n");
}

static void adc_rtc_timer_dump(void *data)
{
	infra_evt_raise_ISR(g_event[EVT_INDEX_ADC_RTC]);
}


static void adcc_callback_ch_0(uint32_t status)
{
	g_adc_ch0_event = status;

	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CB_FUN_CH0 : 0x%x\n", status);
    if ( status & ADCC_CH0_HIGH_OUT_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH0 High OUT\n");

    if ( status & ADCC_CH0_HIGH_IN_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH0 High IN\n");

    if ( status & ADCC_CH0_LOW_OUT_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH0 Low OUT\n");

    if ( status & ADCC_CH0_LOW_IN_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH0 Low IN\n");

    if ( status & ADCC_CH0_HIGH_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH0 Init HIGH INT\n");

    if ( status & ADCC_CH0_LOW_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH0 Init LOW INT\n");
    #endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_ADC_CH0]);
    return;
}

static void adcc_callback_ch_1(uint32_t status)
{
	g_adc_ch1_event = status;
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CB_FUN_CH1 : 0x%x\n", status);
    if ( status & ADCC_CH1_HIGH_OUT_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH1 High OUT\n");

    if ( status & ADCC_CH1_HIGH_IN_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH1 High IN\n");

    if ( status & ADCC_CH1_LOW_OUT_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH1 Low OUT\n");

    if ( status & ADCC_CH1_LOW_IN_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH1 Low IN\n");

    if ( status & ADCC_CH1_HIGH_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH1 Init HIGH INT\n");

    if ( status & ADCC_CH1_LOW_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH1 Init LOW INT\n");
    #endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_ADC_CH1]);
    return;
}

static void adcc_callback_ch_2(uint32_t status)
{
	g_adc_ch2_event = status;
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CB_FUN_CH2 : 0x%x\n", status);
    if ( status & ADCC_CH2_HIGH_OUT_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH2 High OUT\n");

    if ( status & ADCC_CH2_HIGH_IN_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH2 High IN\n");

    if ( status & ADCC_CH2_LOW_OUT_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH2 Low OUT\n");

    if ( status & ADCC_CH2_LOW_IN_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH2 Low IN\n");

    if ( status & ADCC_CH2_HIGH_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH2 Init HIGH INT\n");

    if ( status & ADCC_CH2_LOW_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH2 Init LOW INT\n");
    #endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_ADC_CH2]);
    return;
}

static void adcc_callback_ch_3(uint32_t status)
{
	g_adc_ch3_event = status;
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CB_FUN_CH3 : 0x%x\n", status);
    if ( status & ADCC_CH3_HIGH_OUT_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH3 High OUT\n");

    if ( status & ADCC_CH3_HIGH_IN_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH3 High IN\n");

    if ( status & ADCC_CH3_LOW_OUT_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH3 Low OUT\n");

    if ( status & ADCC_CH3_LOW_IN_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH3 Low IN\n");

    if ( status & ADCC_CH3_HIGH_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH3 Init HIGH INT\n");

    if ( status & ADCC_CH3_LOW_INT_BIT )
    	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CH3 Init LOW INT\n");
    #endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_ADC_CH3]);
    return;
}

static void adcc_constrain_cb(uint32_t status)
{
    g_adc_cfg_event = status;
    #ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"ADCC CB_FUN\n");
    dbg_printf(DBG_DP_EVENT_MSG,"Interrupt Status Code : %x\n", status);
    #endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_ADC_CFG_INT]);
    return;
}

static void adcc_sg_cb(uint32_t status)
{
    g_adc_cfg_event = status;
    #ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"ADCC Single Shot CB_FUN\n");
	dbg_printf(DBG_DP_EVENT_MSG,"Interrupt Status Code : %x\n", status);
	#endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_ADC_SG_INT]);
    return;
}

static void iomux_pgpio0_cb(void* param)
{
	uint8_t aValue;

	hx_drv_iomux_get_invalue(IOMUX_PGPIO0, &aValue);
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"iomux_pgpio0_cb %d\n", aValue);
	#endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_PGPIO0]);
}

static void iomux_pgpio1_cb(void* param)
{
	uint8_t aValue;

	hx_drv_iomux_get_invalue(IOMUX_PGPIO1, &aValue);
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"iomux_pgpio1_cb %d\n", aValue);
	#endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_PGPIO1]);
}

static void iomux_pgpio2_cb(void* param)
{
	uint8_t aValue;

	hx_drv_iomux_get_invalue(IOMUX_PGPIO2, &aValue);
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"iomux_pgpio2_cb %d\n", aValue);
	#endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_PGPIO2]);
}

static void iomux_pgpio3_cb(void* param)
{
	uint8_t aValue;

	hx_drv_iomux_get_invalue(IOMUX_PGPIO3, &aValue);
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"iomux_pgpio3_cb %d\n", aValue);
	#endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_PGPIO3]);
}

static void iomux_pgpio4_cb(void* param)
{
	uint8_t aValue;

	hx_drv_iomux_get_invalue(IOMUX_PGPIO4, &aValue);
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"iomux_pgpio4_cb %d\n", aValue);
	#endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_PGPIO4]);
}

static void iomux_pgpio5_cb(void* param)
{
	uint8_t aValue;

	hx_drv_iomux_get_invalue(IOMUX_PGPIO5, &aValue);
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"iomux_pgpio5_cb %d\n", aValue);
	#endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_PGPIO5]);
}

static void iomux_pgpio6_cb(void* param)
{
	uint8_t aValue;

	hx_drv_iomux_get_invalue(IOMUX_PGPIO6, &aValue);
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"iomux_pgpio6_cb %d\n", aValue);
	#endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_PGPIO6]);
}

static void iomux_pgpio7_cb(void* param)
{
	uint8_t aValue;

	hx_drv_iomux_get_invalue(IOMUX_PGPIO7, &aValue);
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"iomux_pgpio7_cb %d\n", aValue);
	#endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_PGPIO7]);
}

static void iomux_pgpio8_cb(void* param)
{
	uint8_t aValue;

	hx_drv_iomux_get_invalue(IOMUX_PGPIO8, &aValue);
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"iomux_pgpio8_cb %d\n", aValue);
	#endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_PGPIO8]);
}

static void iomux_pgpio9_cb(void* param)
{
	uint8_t aValue;

	hx_drv_iomux_get_invalue(IOMUX_PGPIO9, &aValue);
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"iomux_pgpio9_cb %d\n", aValue);
	#endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_PGPIO9]);
}

static void iomux_pgpio10_cb(void* param)
{
	uint8_t aValue;

	hx_drv_iomux_get_invalue(IOMUX_PGPIO10, &aValue);
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"iomux_pgpio10_cb %d\n", aValue);
	#endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_PGPIO10]);
}

static void iomux_pgpio11_cb(void* param)
{
	uint8_t aValue;

	hx_drv_iomux_get_invalue(IOMUX_PGPIO11, &aValue);
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"iomux_pgpio11_cb %d\n", aValue);
	#endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_PGPIO11]);
}

static void iomux_pgpio12_cb(void* param)
{
	uint8_t aValue;

	hx_drv_iomux_get_invalue(IOMUX_PGPIO12, &aValue);
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"iomux_pgpio12_cb %d\n", aValue);
	#endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_PGPIO12]);
}

static void iomux_pgpio13_cb(void* param)
{
	uint8_t aValue;

	hx_drv_iomux_get_invalue(IOMUX_PGPIO13, &aValue);
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"iomux_pgpio13_cb %d\n", aValue);
	#endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_PGPIO13]);
}

static void iomux_pgpio14_cb(void* param)
{
	uint8_t aValue;

	hx_drv_iomux_get_invalue(IOMUX_PGPIO14, &aValue);
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"iomux_pgpio14_cb %d\n", aValue);
	#endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_PGPIO14]);
}

static void iomux_sgpio0_cb(void* param)
{
	uint8_t aValue;

	hx_drv_iomux_get_invalue(IOMUX_SGPIO0, &aValue);
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"iomux_sgpio0_cb %d\n", aValue);
	#endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_SGPIO0]);
}

static void iomux_sgpio1_cb(void* param)
{
	uint8_t aValue;

	hx_drv_iomux_get_invalue(IOMUX_SGPIO1, &aValue);
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"iomux_sgpio1_cb %d\n", aValue);
	#endif
	infra_evt_raise_ISR(g_event[EVT_INDEX_SGPIO1]);
}

static void event_dplib_cb(SENSORDPLIB_STATUS_E event)
{
	g_dplib_event = event;
//	dbg_printf(DBG_LESS_INFO,"event_dplib_cb %d\n", event);
	switch(event)
	{
	case SENSORDPLIB_STATUS_ERR_FS_HVSIZE:
	case SENSORDPLIB_STATUS_ERR_FE_TOGGLE:
	case SENSORDPLIB_STATUS_ERR_FD_TOGGLE:
	case SENSORDPLIB_STATUS_ERR_FS_TOGGLE:
	case SENSORDPLIB_STATUS_ERR_BLANK_ERR:/*reg_inpparser_stall_error*/
	case SENSORDPLIB_STATUS_ERR_CRC_ERR:  /*reg_inpparser_crc_error*/
	case SENSORDPLIB_STATUS_ERR_FE_ERR:  /*reg_inpparser_fe_cycle_error*/
	case SENSORDPLIB_STATUS_ERR_HSIZE_ERR:  /*reg_inpparser_hsize_error*/
	case SENSORDPLIB_STATUS_ERR_FS_ERR:  /*reg_inpparser_fs_cycle_error*/
		infra_evt_raise_ISR(g_event[EVT_INDEX_1BITPARSER_ERR]);
		break;
	case SENSORDPLIB_STATUS_EDM_WDT1_TIMEOUT:
		//dbg_printf(DBG_DP_EVENT_MSG,"CB WDT1 Timeout\n");
		infra_evt_raise_ISR(g_event[EVT_INDEX_EDM_WDT1_TIMEOUT]);
		break;
	case SENSORDPLIB_STATUS_EDM_WDT2_TIMEOUT:
		infra_evt_raise_ISR(g_event[EVT_INDEX_EDM_WDT2_TIMEOUT]);
		break;
	case SENSORDPLIB_STATUS_EDM_WDT3_TIMEOUT:
		//dbg_printf(DBG_DP_EVENT_MSG,"CB WDT3 Timeout\n");
		infra_evt_raise_ISR(g_event[EVT_INDEX_EDM_WDT3_TIMEOUT]);
		break;
	case SENSORDPLIB_STATUS_SENSORCTRL_WDT_OUT:
		/*
		 * TODO error happen need check sensor
		 * 1. SWRESET Datapath
		 * 2. restart streaming flow
		 */
		//dbg_printf(DBG_DP_EVENT_MSG,"WDT OUT %d\n", event);
		break;

	case SENSORDPLIB_STATUS_CDM_FIFO_OVERFLOW:
	case SENSORDPLIB_STATUS_CDM_FIFO_UNDERFLOW:
		/*
		 * error happen need CDM timing & TPG setting
		 * 1. SWRESET Datapath
		 * 2. restart streaming flow
		 */
		//dbg_printf(DBG_DP_EVENT_MSG,"CDM OverFlow/Underflow %d\n", event);
		infra_evt_raise_ISR(g_event[EVT_INDEX_CDM_FIFO_ERR]);

		break;

	case SENSORDPLIB_STATUS_XDMA_WDMA1_ABNORMAL1:
	case SENSORDPLIB_STATUS_XDMA_WDMA1_ABNORMAL2:
	case SENSORDPLIB_STATUS_XDMA_WDMA1_ABNORMAL3:
	case SENSORDPLIB_STATUS_XDMA_WDMA1_ABNORMAL4:
	case SENSORDPLIB_STATUS_XDMA_WDMA1_ABNORMAL5:
	case SENSORDPLIB_STATUS_XDMA_WDMA1_ABNORMAL6:
	case SENSORDPLIB_STATUS_XDMA_WDMA1_ABNORMAL7:
	case SENSORDPLIB_STATUS_XDMA_WDMA1_ABNORMAL8:
	case SENSORDPLIB_STATUS_XDMA_WDMA1_ABNORMAL9:
		infra_evt_raise_ISR(g_event[EVT_INDEX_XDMA_WDMA1_ABNORMAL]);

		break;

	case SENSORDPLIB_STATUS_XDMA_WDMA2_ABNORMAL1:
	case SENSORDPLIB_STATUS_XDMA_WDMA2_ABNORMAL2:
	case SENSORDPLIB_STATUS_XDMA_WDMA2_ABNORMAL3:
	case SENSORDPLIB_STATUS_XDMA_WDMA2_ABNORMAL4:
	case SENSORDPLIB_STATUS_XDMA_WDMA2_ABNORMAL5:
	case SENSORDPLIB_STATUS_XDMA_WDMA2_ABNORMAL6:
	case SENSORDPLIB_STATUS_XDMA_WDMA2_ABNORMAL7:
		infra_evt_raise_ISR(g_event[EVT_INDEX_XDMA_WDMA2_ABNORMAL]);

		break;

	case SENSORDPLIB_STATUS_XDMA_WDMA3_ABNORMAL1:
	case SENSORDPLIB_STATUS_XDMA_WDMA3_ABNORMAL2:
	case SENSORDPLIB_STATUS_XDMA_WDMA3_ABNORMAL3:
	case SENSORDPLIB_STATUS_XDMA_WDMA3_ABNORMAL4:
	case SENSORDPLIB_STATUS_XDMA_WDMA3_ABNORMAL5:
	case SENSORDPLIB_STATUS_XDMA_WDMA3_ABNORMAL6:
	case SENSORDPLIB_STATUS_XDMA_WDMA3_ABNORMAL7:
	case SENSORDPLIB_STATUS_XDMA_WDMA3_ABNORMAL8:
	case SENSORDPLIB_STATUS_XDMA_WDMA3_ABNORMAL9:
		/*
		 * error happen need
		 * 1. SWRESET Datapath
		 * 2. restart streaming flow
		 */
		//dbg_printf(DBG_DP_EVENT_MSG,"WDMA123 Abnormal %d\n", event);
		infra_evt_raise_ISR(g_event[EVT_INDEX_XDMA_WDMA3_ABNORMAL]);

		break;

	case SENSORDPLIB_STATUS_XDMA_RDMA_ABNORMAL1:
	case SENSORDPLIB_STATUS_XDMA_RDMA_ABNORMAL2:
	case SENSORDPLIB_STATUS_XDMA_RDMA_ABNORMAL3:
	case SENSORDPLIB_STATUS_XDMA_RDMA_ABNORMAL4:
	case SENSORDPLIB_STATUS_XDMA_RDMA_ABNORMAL5:
		//dbg_printf(DBG_DP_EVENT_MSG,"RDMA Abnormal %d\n", event);
		infra_evt_raise_ISR(g_event[EVT_INDEX_XDMA_RDMA_ABNORMAL]);
		break;

	case SENSORDPLIB_STATUS_RSDMA_ABNORMAL1:
	case SENSORDPLIB_STATUS_RSDMA_ABNORMAL2:
	case SENSORDPLIB_STATUS_RSDMA_ABNORMAL3:
	case SENSORDPLIB_STATUS_RSDMA_ABNORMAL4:
	case SENSORDPLIB_STATUS_RSDMA_ABNORMAL5:
	case SENSORDPLIB_STATUS_RSDMA_ABNORMAL6:
	case SENSORDPLIB_STATUS_RSDMA_ABNORMAL7:
		/*
		 * error happen need
		 * 1. SWRESET RS & RS DMA
		 * 2. Re-run flow again
		 */
		//dbg_printf(DBG_DP_EVENT_MSG,"RSDMA Abnormal %d\n", event);
		infra_evt_raise_ISR(g_event[EVT_INDEX_RSDMA_ABNORMAL]);
		break;

	case SENSORDPLIB_STATUS_HOGDMA_ABNORMAL1:
	case SENSORDPLIB_STATUS_HOGDMA_ABNORMAL2:
	case SENSORDPLIB_STATUS_HOGDMA_ABNORMAL3:
	case SENSORDPLIB_STATUS_HOGDMA_ABNORMAL4:
	case SENSORDPLIB_STATUS_HOGDMA_ABNORMAL5:
	case SENSORDPLIB_STATUS_HOGDMA_ABNORMAL6:
	case SENSORDPLIB_STATUS_HOGDMA_ABNORMAL7:
		/*
		 * error happen need
		 * 1. SWRESET HOG & HOG DMA
		 * 2. Re-run flow again
		 */
		//dbg_printf(DBG_DP_EVENT_MSG,"HOGDMA Abnormal %d\n", event);
		infra_evt_raise_ISR(g_event[EVT_INDEX_HOGDMA_ABNORMAL]);
		break;

	case SENSORDPLIB_STATUS_CDM_MOTION_DETECT:
		infra_evt_raise_ISR(g_event[EVT_INDEX_CDM_MOTION]);
		break;
	case SENSORDPLIB_STATUS_XDMA_FRAME_READY:
		infra_evt_raise_ISR(g_event[EVT_INDEX_XDMA_FRAME_READY]);
		break;
	case SENSORDPLIB_STATUS_XDMA_WDMA1_FINISH:
	case SENSORDPLIB_STATUS_XDMA_WDMA2_FINISH:
	case SENSORDPLIB_STATUS_XDMA_WDMA3_FINISH:
	case SENSORDPLIB_STATUS_XDMA_RDMA_FINISH:
		break;

	case SENSORDPLIB_STATUS_RSDMA_FINISH:
		infra_evt_raise_ISR(g_event[EVT_INDEX_RSDMA_FINISH]);
		break;
	case SENSORDPLIB_STATUS_HOGDMA_FINISH:
		infra_evt_raise_ISR(g_event[EVT_INDEX_HOGDMA_FINISH]);
		break;
	case SENSORDPLIB_STATUS_TIMER_FIRE_APP_NOTREADY:
		infra_evt_raise_ISR(g_event[EVT_INDEX_SENSOR_RTC_FIRE]);
		break;
	case SENSORDPLIB_STATUS_TIMER_FIRE_APP_READY:
		infra_evt_raise_ISR(g_event[EVT_INDEX_SENSOR_RTC_FIRE]);
		break;
	default:
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"Other Event %d\n", event);
		#endif
		break;
	}
//	dbg_printf(DBG_LESS_INFO,"event_dplib_cb %d Done\n", event);
}


/**
 * \brief	clear datapath related event
 *
 * clear datapath related event
 */
void evt_dp_clear_all_dpevent()
{
	//dbg_printf(DBG_LESS_INFO,"evt_dp_clear_all_dpevent\n");
   for(uint8_t idx = EVT_INDEX_1BITPARSER_ERR; idx<=EVT_INDEX_HOGDMA_FINISH ;idx++)
   {
		infra_evt_clear(g_event[idx]);
		infra_evt_clear_ISR_pendingevent(g_event[idx]);
   }
}

/**
 * \brief	clear All GPIO Interrupt event
 *
 * clear GPIO Interrupt related event
 */
void evt_dp_clear_allgpio()
{
	dbg_printf(DBG_LESS_INFO,"evt_dp_clear_allgpio\n");

	infra_evt_clear(g_event[EVT_INDEX_PGPIO0]);
	infra_evt_clear_ISR_pendingevent(g_event[EVT_INDEX_PGPIO0]);

	infra_evt_clear(g_event[EVT_INDEX_PGPIO1]);
	infra_evt_clear_ISR_pendingevent(g_event[EVT_INDEX_PGPIO1]);

	infra_evt_clear(g_event[EVT_INDEX_PGPIO2]);
	infra_evt_clear_ISR_pendingevent(g_event[EVT_INDEX_PGPIO2]);

	infra_evt_clear(g_event[EVT_INDEX_PGPIO3]);
	infra_evt_clear_ISR_pendingevent(g_event[EVT_INDEX_PGPIO3]);

	infra_evt_clear(g_event[EVT_INDEX_PGPIO4]);
	infra_evt_clear_ISR_pendingevent(g_event[EVT_INDEX_PGPIO4]);

	infra_evt_clear(g_event[EVT_INDEX_PGPIO5]);
	infra_evt_clear_ISR_pendingevent(g_event[EVT_INDEX_PGPIO5]);

	infra_evt_clear(g_event[EVT_INDEX_PGPIO6]);
	infra_evt_clear_ISR_pendingevent(g_event[EVT_INDEX_PGPIO6]);

	infra_evt_clear(g_event[EVT_INDEX_PGPIO7]);
	infra_evt_clear_ISR_pendingevent(g_event[EVT_INDEX_PGPIO7]);

	infra_evt_clear(g_event[EVT_INDEX_PGPIO8]);
	infra_evt_clear_ISR_pendingevent(g_event[EVT_INDEX_PGPIO8]);

	infra_evt_clear(g_event[EVT_INDEX_PGPIO9]);
	infra_evt_clear_ISR_pendingevent(g_event[EVT_INDEX_PGPIO9]);

	infra_evt_clear(g_event[EVT_INDEX_PGPIO10]);
	infra_evt_clear_ISR_pendingevent(g_event[EVT_INDEX_PGPIO10]);

	infra_evt_clear(g_event[EVT_INDEX_PGPIO11]);
	infra_evt_clear_ISR_pendingevent(g_event[EVT_INDEX_PGPIO11]);

	infra_evt_clear(g_event[EVT_INDEX_PGPIO12]);
	infra_evt_clear_ISR_pendingevent(g_event[EVT_INDEX_PGPIO12]);

	infra_evt_clear(g_event[EVT_INDEX_PGPIO13]);
	infra_evt_clear_ISR_pendingevent(g_event[EVT_INDEX_PGPIO13]);

	infra_evt_clear(g_event[EVT_INDEX_PGPIO14]);
	infra_evt_clear_ISR_pendingevent(g_event[EVT_INDEX_PGPIO14]);

	infra_evt_clear(g_event[EVT_INDEX_SGPIO0]);
	infra_evt_clear_ISR_pendingevent(g_event[EVT_INDEX_SGPIO0]);

	infra_evt_clear(g_event[EVT_INDEX_SGPIO1]);
	infra_evt_clear_ISR_pendingevent(g_event[EVT_INDEX_SGPIO1]);

}

/**
 * \brief	clear Digital PIR Interrupt event
 *
 * clear Digital PIR Interrupt related event
 */
void evt_dp_clear_sensor_rtc()
{
	//dbg_printf(DBG_LESS_INFO,"evt_dp_clear_sensor_rtc\n");

	infra_evt_clear(g_event[EVT_INDEX_SENSOR_RTC_FIRE]);
	infra_evt_clear_ISR_pendingevent(g_event[EVT_INDEX_SENSOR_RTC_FIRE]);
}

/**
 * \brief	Callback function for 1Bit INP Parser Error
 *
 * Callback function for 1Bit INP Parser Error
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_1bitParser_err_cb(void)
{
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_1BITPARSER_ERR TRIGGERED\n");

	if(g_event_cb != NULL)
	{
		g_event_cb(EVT_INDEX_1BITPARSER_ERR);
	}else{
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_1BITPARSER_ERR g_event_cb == NULL\n");
	}
	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for CDM fifo Error
 *
 * Callback function for CDM fifo Error
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_CDM_fifo_err_cb(void)
{
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_CDM_FIFO_ERR TRIGGERED\n");

	if(g_event_cb != NULL)
	{
		g_event_cb(EVT_INDEX_CDM_FIFO_ERR);
	}else{
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_CDM_FIFO_ERR g_event_cb == NULL\n");
	}
	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for WDMA1 Error
 *
 * Callback function for WDMA1 Error
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_WDMA1_err_cb(void)
{
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_XDMA_WDMA1_ABNORMAL TRIGGERED\n");

	if(g_event_cb != NULL)
	{
		g_event_cb(EVT_INDEX_XDMA_WDMA1_ABNORMAL);
	}else{
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_XDMA_WDMA1_ABNORMAL g_event_cb == NULL\n");
	}
	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for WDMA2 Error
 *
 * Callback function for WDMA2 Error
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_WDMA2_err_cb(void)
{
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_XDMA_WDMA2_ABNORMAL TRIGGERED\n");

	if(g_event_cb != NULL)
	{
		g_event_cb(EVT_INDEX_XDMA_WDMA2_ABNORMAL);
	}else{
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_XDMA_WDMA2_ABNORMAL g_event_cb == NULL\n");
	}
	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for WDMA3 Error
 *
 * Callback function for WDMA3 Error
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_WDMA3_err_cb(void)
{
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_XDMA_WDMA3_ABNORMAL TRIGGERED\n");

	if(g_event_cb != NULL)
	{
		g_event_cb(EVT_INDEX_XDMA_WDMA3_ABNORMAL);
	}else{
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_XDMA_WDMA3_ABNORMAL g_event_cb == NULL\n");
	}
	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for RDMA Error
 *
 * Callback function for RDMA Error
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_RDMA_err_cb(void)
{
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_XDMA_RDMA_ABNORMAL TRIGGERED\n");

	if(g_event_cb != NULL)
	{
		g_event_cb(EVT_INDEX_XDMA_RDMA_ABNORMAL);
	}else{
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_XDMA_RDMA_ABNORMAL g_event_cb == NULL\n");
	}
	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for RSDMA Error
 *
 * Callback function for RSDMA Error
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_RSDMA_err_cb(void)
{
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_RSDMA_ABNORMAL TRIGGERED\n");

	if(g_event_cb != NULL)
	{
		g_event_cb(EVT_INDEX_RSDMA_ABNORMAL);
	}else{
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_RSDMA_ABNORMAL g_event_cb == NULL\n");
	}
	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for HOGDMA Error
 *
 * Callback function for HOGDMA Error
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_HOGDMA_err_cb(void)
{
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_HOGDMA_ABNORMAL TRIGGERED\n");

	if(g_event_cb != NULL)
	{
		g_event_cb(EVT_INDEX_HOGDMA_ABNORMAL);
	}else{
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_HOGDMA_ABNORMAL g_event_cb == NULL\n");
	}
	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for EDM WDT1 Error
 *
 * Callback function for EDM WDT1 Error
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_EDM_WDT1_err_cb(void)
{
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_EDM_WDT1_TIMEOUT TRIGGERED\n");

	if(g_event_cb != NULL)
	{
		g_event_cb(EVT_INDEX_EDM_WDT1_TIMEOUT);
	}else{
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_EDM_WDT1_TIMEOUT g_event_cb == NULL\n");
	}
	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for EDM WDT2 Error
 *
 * Callback function for EDM WDT2 Error
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_EDM_WDT2_err_cb(void)
{
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_EDM_WDT2_TIMEOUT TRIGGERED\n");

	if(g_event_cb != NULL)
	{
		g_event_cb(EVT_INDEX_EDM_WDT2_TIMEOUT);
	}else{
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_EDM_WDT2_TIMEOUT g_event_cb == NULL\n");
	}
	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for EDM WDT3 Error
 *
 * Callback function for EDM WDT3 Error
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_EDM_WDT3_err_cb(void)
{
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_EDM_WDT3_TIMEOUT TRIGGERED\n");

	if(g_event_cb != NULL)
	{
		g_event_cb(EVT_INDEX_EDM_WDT3_TIMEOUT);
	}else{
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_EDM_WDT3_TIMEOUT g_event_cb == NULL\n");
	}
	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for CDM Motion
 *
 * Callback function for CDM Motion
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_CDM_Motion_cb(void)
{
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_CDM_MOTION TRIGGERED\n");

	if(g_event_cb != NULL)
	{
		g_event_cb(EVT_INDEX_CDM_MOTION);
	}else{
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_CDM_MOTION g_event_cb == NULL\n");
	}
	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for RS Frame Ready
 *
 * Callback function for RS Frame Ready
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_RSDMA_Ready_cb(void)
{
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_RSDMA_FINISH TRIGGERED\n");

	if(g_event_cb != NULL)
	{
		g_event_cb(EVT_INDEX_RSDMA_FINISH);
	}else{
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_RSDMA_FINISH g_event_cb == NULL\n");
	}
	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for HOG Frame Ready
 *
 * Callback function for HOG Frame Ready
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_HOGDMA_Ready_cb(void)
{
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_HOGDMA_FINISH TRIGGERED\n");

	if(g_event_cb != NULL)
	{
		g_event_cb(EVT_INDEX_HOGDMA_FINISH);
	}else{
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_HOGDMA_FINISH g_event_cb == NULL\n");
	}
	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for Sensor RTC fire
 *
 * Callback function for Sensor RTC fire
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_Sensor_RTC_Fire_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_SENSOR_RTC_FIRE TRIGGERED\n");
	#endif
	if(g_event_cb != NULL)
	{
		g_event_cb(EVT_INDEX_SENSOR_RTC_FIRE);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_SENSOR_RTC_FIRE g_event_cb == NULL\n");
		#endif
	}
	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for XDMA Frame Ready
 *
 * Callback function for XDMA Frame Ready
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_xDMA_FrameReady_cb(void)
{
	//dbg_printf(DBG_DP_EVENT_MSG,"EVENT evt_xDMA_FrameReady_cb TRIGGERED\n");

	if(g_event_cb != NULL)
	{
		g_event_cb(EVT_INDEX_XDMA_FRAME_READY);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_XDMA_FRAME_READY g_event_cb == NULL\n");
		#endif
	}
	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for PGPIO0 Interrupt
 *
 * Callback function for PGPIO0 Interrupt
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_PGPIO0_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_PGPIO0 TRIGGERED\n");
	#endif
	if(g_gpio_event_cb[IOMUX_PGPIO0] != NULL)
	{
		g_gpio_event_cb[IOMUX_PGPIO0](1);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_PGPIO0 g_gpio_event_cb[IOMUX_PGPIO0] == NULL\n");
		#endif	
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for PGPIO1 Interrupt
 *
 * Callback function for PGPIO1 Interrupt
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_PGPIO1_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_PGPIO1 TRIGGERED\n");
	#endif
	if(g_gpio_event_cb[IOMUX_PGPIO1] != NULL)
	{
		g_gpio_event_cb[IOMUX_PGPIO1](1);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_PGPIO1 g_gpio_event_cb[IOMUX_PGPIO1] == NULL\n");
		#endif	
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for PGPIO2 Interrupt
 *
 * Callback function for PGPIO2 Interrupt
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_PGPIO2_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_PGPIO2 TRIGGERED\n");
	#endif
	if(g_gpio_event_cb[IOMUX_PGPIO2] != NULL)
	{
		g_gpio_event_cb[IOMUX_PGPIO2](1);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_PGPIO2 g_gpio_event_cb[IOMUX_PGPIO2] == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for PGPIO3 Interrupt
 *
 * Callback function for PGPIO3 Interrupt
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_PGPIO3_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_PGPIO3 TRIGGERED\n");
	#endif
	if(g_gpio_event_cb[IOMUX_PGPIO3] != NULL)
	{
		g_gpio_event_cb[IOMUX_PGPIO3](1);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_PGPIO3 g_gpio_event_cb[IOMUX_PGPIO3] == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for PGPIO4 Interrupt
 *
 * Callback function for PGPIO4 Interrupt
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_PGPIO4_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_PGPIO4 TRIGGERED\n");
	#endif
	if(g_gpio_event_cb[IOMUX_PGPIO4] != NULL)
	{
		g_gpio_event_cb[IOMUX_PGPIO4](1);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_PGPIO4 g_gpio_event_cb[IOMUX_PGPIO4] == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for PGPIO5 Interrupt
 *
 * Callback function for PGPIO5 Interrupt
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_PGPIO5_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_PGPIO5 TRIGGERED\n");
	#endif
	if(g_gpio_event_cb[IOMUX_PGPIO5] != NULL)
	{
		g_gpio_event_cb[IOMUX_PGPIO5](1);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_PGPIO5 g_gpio_event_cb[IOMUX_PGPIO5] == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for PGPIO6 Interrupt
 *
 * Callback function for PGPIO6 Interrupt
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_PGPIO6_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_PGPIO6 TRIGGERED\n");
	#endif
	if(g_gpio_event_cb[IOMUX_PGPIO6] != NULL)
	{
		g_gpio_event_cb[IOMUX_PGPIO6](1);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_PGPIO6 g_gpio_event_cb[IOMUX_PGPIO6] == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for PGPIO7 Interrupt
 *
 * Callback function for PGPIO7 Interrupt
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_PGPIO7_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_PGPIO7 TRIGGERED\n");
	#endif
	if(g_gpio_event_cb[IOMUX_PGPIO7] != NULL)
	{
		g_gpio_event_cb[IOMUX_PGPIO7](1);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_PGPIO7 g_gpio_event_cb[IOMUX_PGPIO7] == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for PGPIO8 Interrupt
 *
 * Callback function for PGPIO8 Interrupt
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_PGPIO8_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_PGPIO8 TRIGGERED\n");
	#endif
	if(g_gpio_event_cb[IOMUX_PGPIO8] != NULL)
	{
		g_gpio_event_cb[IOMUX_PGPIO8](1);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_PGPIO8 g_gpio_event_cb[IOMUX_PGPIO8] == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for PGPIO9 Interrupt
 *
 * Callback function for PGPIO9 Interrupt
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_PGPIO9_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_PGPIO9 TRIGGERED\n");
	#endif
	if(g_gpio_event_cb[IOMUX_PGPIO9] != NULL)
	{
		g_gpio_event_cb[IOMUX_PGPIO9](1);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_PGPIO9 g_gpio_event_cb[IOMUX_PGPIO9] == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for PGPIO10 Interrupt
 *
 * Callback function for PGPIO10 Interrupt
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_PGPIO10_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_PGPIO10 TRIGGERED\n");
	#endif
	if(g_gpio_event_cb[IOMUX_PGPIO10] != NULL)
	{
		g_gpio_event_cb[IOMUX_PGPIO10](1);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_PGPIO10 g_gpio_event_cb[IOMUX_PGPIO10] == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for PGPIO11 Interrupt
 *
 * Callback function for PGPIO11 Interrupt
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_PGPIO11_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_PGPIO11 TRIGGERED\n");
	#endif
	if(g_gpio_event_cb[IOMUX_PGPIO11] != NULL)
	{
		g_gpio_event_cb[IOMUX_PGPIO11](1);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_PGPIO11 g_gpio_event_cb[IOMUX_PGPIO11] == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for PGPIO12 Interrupt
 *
 * Callback function for PGPIO12 Interrupt
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_PGPIO12_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_PGPIO12 TRIGGERED\n");
	#endif
	if(g_gpio_event_cb[IOMUX_PGPIO12] != NULL)
	{
		g_gpio_event_cb[IOMUX_PGPIO12](1);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_PGPIO12 g_gpio_event_cb[IOMUX_PGPIO12] == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for PGPIO13 Interrupt
 *
 * Callback function for PGPIO13 Interrupt
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_PGPIO13_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_PGPIO13 TRIGGERED\n");
	#endif
	if(g_gpio_event_cb[IOMUX_PGPIO13] != NULL)
	{
		g_gpio_event_cb[IOMUX_PGPIO13](1);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_PGPIO13 g_gpio_event_cb[IOMUX_PGPIO13] == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for PGPIO14 Interrupt
 *
 * Callback function for PGPIO14 Interrupt
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_PGPIO14_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_PGPIO14 TRIGGERED\n");
	#endif
	if(g_gpio_event_cb[IOMUX_PGPIO14] != NULL)
	{
		g_gpio_event_cb[IOMUX_PGPIO14](1);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_PGPIO14 g_gpio_event_cb[IOMUX_PGPIO14] == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}


/**
 * \brief	Callback function for SGPIO0 Interrupt
 *
 * Callback function for SGPIO0 Interrupt
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_SGPIO0_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_SGPIO0 TRIGGERED\n");
	#endif
	if(g_gpio_event_cb[IOMUX_SGPIO0] != NULL)
	{
		g_gpio_event_cb[IOMUX_SGPIO0](1);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_SGPIO0 g_gpio_event_cb[IOMUX_SGPIO0] == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for SGPIO0 Interrupt
 *
 * Callback function for SGPIO1 Interrupt
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_SGPIO1_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_SGPIO0 TRIGGERED\n");
	#endif
	if(g_gpio_event_cb[IOMUX_SGPIO1] != NULL)
	{
		g_gpio_event_cb[IOMUX_SGPIO1](1);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_SGPIO1 g_gpio_event_cb[IOMUX_SGPIO1] == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for ADC Channel 0
 *
 * Callback function for ADC Channel 0
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_ADC_Ch0_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_ADC_CH0 TRIGGERED\n");
	#endif
	if(g_adc_ch0_event_cb != NULL)
	{
		g_adc_ch0_event_cb(g_adc_ch0_event);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_ADC_CH0 g_adc_ch0_event_cb == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for ADC Channel 1
 *
 * Callback function for ADC Channel 1
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_ADC_Ch1_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_ADC_CH1 TRIGGERED\n");
	#endif
	if(g_adc_ch1_event_cb != NULL)
	{
		g_adc_ch1_event_cb(g_adc_ch1_event);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_ADC_CH1 g_adc_ch1_event_cb == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for ADC Channel 2
 *
 * Callback function for ADC Channel 2
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_ADC_Ch2_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_ADC_CH2 TRIGGERED\n");
	#endif
	if(g_adc_ch2_event_cb != NULL)
	{
		g_adc_ch2_event_cb(g_adc_ch2_event);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_ADC_CH2 g_adc_ch2_event_cb == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for ADC Channel 3
 *
 * Callback function for ADC Channel 3
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_ADC_Ch3_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_ADC_CH3 TRIGGERED\n");
	#endif
	if(g_adc_ch3_event_cb != NULL)
	{
		g_adc_ch3_event_cb(g_adc_ch3_event);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_ADC_CH2 g_adc_ch3_event_cb == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for ADC CFG
 *
 * Callback function for ADC CFG
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_ADC_CFG_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_ADC_CFG TRIGGERED\n");
	#endif
	if(g_adc_cfg_event_cb != NULL)
	{
		g_adc_cfg_event_cb(g_adc_cfg_event);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_ADC_CFG g_adc_cfg_event_cb == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for Sensor RTC
 *
 * Callback function for Sensor RTC
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_SensorRTC_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_SENSOR_RTC TRIGGERED\n");
	#endif
	if(g_sensorRTC_event_cb != NULL)
	{
		g_sensorRTC_event_cb(g_sensor_rtc_interval);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_SENSOR_RTC g_sensorRTC_event_cb == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}

/**
 * \brief	Callback function for ADC RTC
 *
 * Callback function for ADC RTC
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_ADCRTC_cb(void)
{
	#ifndef __GNU__
	dbg_printf(DBG_DP_EVENT_MSG,"EVENT EVT_INDEX_ADC_RTC TRIGGERED\n");
	#endif
	if(g_ADCRTC_event_cb != NULL)
	{
		g_ADCRTC_event_cb(g_adc_rtc_interval);
	}else{
		#ifndef __GNU__
		dbg_printf(DBG_LESS_INFO,"EVT_INDEX_ADC_RTC g_ADCRTC_event_cb == NULL\n");
		#endif
	}

	return INFRA_EVT_RTN_CLEAR;
}


/*
 * @brief register DP lib DataPath Status callback
 *
 * register DP lib DataPath Status callback
 * you can use different callback for DP, HOG and RS
 * @param cb_event dp library callback event function
 * @param type register library callback function type (Sensor DP, HOG, RS, JPEG_DEC)
 * @return void.
 * */
void hx_dplib_evthandler_register_cb(evthandlerdp_CBEvent_t  cb_event, SENSORDPLIB_CB_FUNTYPE_E cb_type)
{
	g_event_cb = cb_event;
	hx_dplib_register_cb(event_dplib_cb, cb_type);
}

/*
 * @brief register GPIO Interrupt callback
 *
 * register GPIO Interrupt callback
 * @param aIndex  GPIO Interrupt pin index
 * @param cb_event GPIO Interrupt callback event function
 * @return void.
 * */
void hx_gpio_evthandler_register_cb(IOMUX_INDEX_E aIndex, evthandlerGPIO_CBEvent_t  cb_event)
{
	g_gpio_event_cb[aIndex] = cb_event;
	dbg_printf(DBG_LESS_INFO,"hx_gpio_evthandler_register_cb aIndex=%d\n",aIndex);
	switch(aIndex)
	{
	case IOMUX_PGPIO0:
		hx_drv_iomux_cb_register(aIndex, iomux_pgpio0_cb);
		break;
	case IOMUX_PGPIO1:
		hx_drv_iomux_cb_register(aIndex, iomux_pgpio1_cb);
		break;
	case IOMUX_PGPIO2:
		hx_drv_iomux_cb_register(aIndex, iomux_pgpio2_cb);
		break;
	case IOMUX_PGPIO3:
		hx_drv_iomux_cb_register(aIndex, iomux_pgpio3_cb);
		break;
	case IOMUX_PGPIO4:
		hx_drv_iomux_cb_register(aIndex, iomux_pgpio4_cb);
		break;
	case IOMUX_PGPIO5:
		hx_drv_iomux_cb_register(aIndex, iomux_pgpio5_cb);
		break;
	case IOMUX_PGPIO6:
		hx_drv_iomux_cb_register(aIndex, iomux_pgpio6_cb);
		break;
	case IOMUX_PGPIO7:
		hx_drv_iomux_cb_register(aIndex, iomux_pgpio7_cb);
		break;
	case IOMUX_PGPIO8:
		hx_drv_iomux_cb_register(aIndex, iomux_pgpio8_cb);
		break;
	case IOMUX_PGPIO9:
		hx_drv_iomux_cb_register(aIndex, iomux_pgpio9_cb);
		break;
	case IOMUX_PGPIO10:
		hx_drv_iomux_cb_register(aIndex, iomux_pgpio10_cb);
		break;
	case IOMUX_PGPIO11:
		hx_drv_iomux_cb_register(aIndex, iomux_pgpio11_cb);
		break;
	case IOMUX_PGPIO12:
		hx_drv_iomux_cb_register(aIndex, iomux_pgpio12_cb);
		break;
	case IOMUX_PGPIO13:
		hx_drv_iomux_cb_register(aIndex, iomux_pgpio13_cb);
		break;
	case IOMUX_PGPIO14:
		hx_drv_iomux_cb_register(aIndex, iomux_pgpio14_cb);
		break;
	case IOMUX_SGPIO0:
		hx_drv_iomux_cb_register(aIndex, iomux_sgpio0_cb);
		break;
	case IOMUX_SGPIO1:
		hx_drv_iomux_cb_register(aIndex, iomux_sgpio1_cb);
		break;
	default:
		break;
	}

}

/*
 * @brief register ADC init
 *
 * register ADC Interrupt callback
 * @param cb0_event ADC Channel 0 callback event function
 * @param cb1_event ADC Channel 1 callback event function
 * @param cb2_event ADC Channel 2 callback event function
 * @param cb3_event ADC Channel 3 callback event function
 * @param cfg_event ADC configure callback event function
 * @return void.
 * */
void hx_ADC_evthandler_init(evthandlerADC_CBEvent_t  ch0_event
		,evthandlerADC_CBEvent_t  ch1_event
		,evthandlerADC_CBEvent_t  ch2_event
		,evthandlerADC_CBEvent_t  ch3_event
		,evthandlerADC_CBEvent_t  cfg_event
		,evthandlerADC_CBEvent_t sg_event)
{
	g_adc_ch0_event_cb = ch0_event;
	g_adc_ch1_event_cb = ch1_event;
	g_adc_ch2_event_cb = ch2_event;
	g_adc_ch3_event_cb = ch3_event;
	g_adc_cfg_event_cb = cfg_event;
	g_adc_sg_event_cb = sg_event;

	/*TODO*/
	hx_drv_adcc_get_defCfg(&g_evt_adcc_cfg);

	hx_drv_adcc_init(&g_evt_adcc_cfg);

	hx_drv_adcc_register_cb(ADCC_CH0_CB, adcc_callback_ch_0);
	hx_drv_adcc_register_cb(ADCC_CH1_CB, adcc_callback_ch_1);
	hx_drv_adcc_register_cb(ADCC_CH2_CB, adcc_callback_ch_2);
	hx_drv_adcc_register_cb(ADCC_CH3_CB, adcc_callback_ch_3);
	hx_drv_adcc_register_cb(ADCC_CONSTRAIN_CB, adcc_constrain_cb);
	hx_drv_adcc_register_cb(ADCC_SG_CB, adcc_sg_cb);
}

/*
 * @brief register ADC Interrupt callback
 *
 * register ADC Interrupt callback
 * @param cbxx ADC Channel xx
 * @param cbxx_event ADC Channel xx callback event function
 * @return void.
 * */
void hx_ADCCh_evthandler_register_cb(ADCC_CALLBACK_E  chxx
		,evthandlerADC_CBEvent_t  chxx_event)
{
   switch(chxx)
   {
   case ADCC_CH0_CB:
		g_adc_ch0_event_cb = chxx_event;
	   break;
   case ADCC_CH1_CB:
		g_adc_ch1_event_cb = chxx_event;
	   break;
   case ADCC_CH2_CB:
		g_adc_ch2_event_cb = chxx_event;
	   break;
   case ADCC_CH3_CB:
		g_adc_ch3_event_cb = chxx_event;
	   break;
   case ADCC_CONSTRAIN_CB:
	   g_adc_cfg_event_cb = chxx_event;
	   break;
   case ADCC_SG_CB:
	   g_adc_sg_event_cb = chxx_event;
	   break;
   default:
	   break;
   }
}

/*
 * @brief register Sensor RTC Interrupt callback
 *
 * register Sensor RTC Interrupt callback
 * @param rtc_interval Sensor RTC interval(ms)
 * @param cb_event Sensor RTC Interrupt callback event function
 * @return void.
 * */
void hx_SensorRTC_evthdl_register_cb(uint32_t rtc_interval, evthdlSensorRTC_CBEvent_t  cb_event)
{
	if(rtc_interval == 0)
	{
		g_sensor_rtc_interval = 0;
		hx_drv_rtc_stop(RTC_SENSOR);
		g_sensorRTC_event_cb = NULL;
	}else{
		g_sensor_rtc_interval = rtc_interval;
		hx_drv_rtc_start(RTC_SENSOR, RTC_MODE_RTC, rtc_interval , sensor_rtc_timer_dump);
		g_sensorRTC_event_cb = cb_event;
	}
}

/*
 * @brief register ADC RTC Interrupt callback
 *
 * register ADC RTC Interrupt callback
 * @param rtc_interval ADC RTC interval(ms)
 * @param cb_event ADC RTC Interrupt callback event function
 * @return void.
 * */
void hx_ADCRTC_evthdl_register_cb(uint32_t rtc_interval, evthdlADCRTC_CBEvent_t  cb_event)
{
	if(rtc_interval == 0)
	{
		g_adc_rtc_interval = 0;
		hx_drv_rtc_stop(RTC_ADC);
		g_ADCRTC_event_cb = NULL;
	}else{
		g_adc_rtc_interval = rtc_interval;
		hx_drv_rtc_start(RTC_ADC, RTC_MODE_RTC, rtc_interval , adc_rtc_timer_dump);
		g_ADCRTC_event_cb = cb_event;
	}
}
