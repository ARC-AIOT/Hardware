/*
 * app_aiot_hw5x5jpeg_algo.c
 *
 *  Created on: 2019/8/5
 *      Author: 902447
 */

#include <app_aiot_hw5x5jpeg_algo.h>
#include "tflitemicro_algo.h"

#define DBG_APP_PRINT_LEVEL  DBG_MORE_INFO
extern WE1AppCfg_CustTable_t app_cust_config;

extern volatile uint32_t g_image_size;
//datapath callback flag
extern uint8_t g_xdma_abnormal;
extern uint8_t g_rs_abnormal;
extern uint8_t g_hog_abnormal;
extern uint8_t g_rs_frameready;
extern uint8_t g_hog_frameready;
extern uint8_t g_md_detect;
extern uint8_t g_cdm_fifoerror;
extern uint8_t g_wdt1_timeout;
extern uint8_t g_wdt2_timeout;
extern uint8_t g_wdt3_timeout;
extern int32_t g_inp1bitparer_abnormal;
extern uint32_t g_dp_event;
extern uint8_t g_frame_ready;

extern uint32_t g_cur_hw5x5jpeg_frame;

extern uint32_t g_hw5x5jpeg_acc_frame;

extern uint32_t g_hw5x5jpeg_err_retry_cnt;

extern uint32_t g_tick_start;
extern uint32_t g_tick_period;
extern uint32_t g_period;
extern uint32_t g_tick_sensor_std;
extern uint32_t g_tick_sensor_stream;
extern uint32_t g_tick_sensor_toggle;

extern uint32_t g_wdma2_baseaddr;
extern uint32_t g_wdma3_baseaddr;
extern uint32_t g_jpegautofill_addr;
extern SENSORDPLIB_STREAM_E g_stream_type;
extern APP_STATE_E g_app_cur_state;
extern APP_STATE_E g_app_new_state;
extern uint8_t g_jpeg_total_slot;

extern IOMUX_INDEX_E g_motion_led_pin;
extern IOMUX_INDEX_E g_human_led_pin;

//tick cal
extern uint32_t g_tick_start;
extern uint32_t g_tick_stop;
extern uint32_t g_tick_toggle;
extern uint32_t g_tick_period;
extern uint32_t g_period;
struct_algoResult algo_result;


volatile uint8_t g_frame_process_done = 0;
/*Error debug variable*/
volatile static uint32_t g_de0_count = 0;
volatile static uint32_t g_convde_count = 0;
volatile static uint16_t g_af_framecnt = 0;
volatile static uint16_t g_be_framecnt = 0;
volatile static uint8_t g_wdma1_fin = 0;
volatile static uint8_t g_wdma2_fin = 0;
volatile static uint8_t g_wdma3_fin = 0;
volatile static uint8_t g_rdma_fin = 0;
volatile static uint8_t g_nframe_end = 0;
volatile static uint8_t g_xdmadone = 0;
volatile static uint32_t g_rtc_cnt = 0;
volatile static uint32_t g_rtc_err_retry_cnt = 0;

volatile static uint32_t g_face_frame = 0;
volatile static uint32_t g_noface_frame = 0;

static void app_aiot_hw5x5jpeg_recapture();

static void sensor_rtc_fire(uint32_t interval);
static void error_debug_report();
static void app_cpu_sleep_at_capture();

static int spi_write(uint32_t addr, uint32_t size, SPI_CMD_DATA_TYPE data_type)
{
#ifndef SPI_MASTER_SEND
	return hx_drv_spi_slv_protocol_write_simple_ex(addr, size, data_type);
#else
	return hx_drv_spi_mst_protocol_write_sp(addr, size, data_type);
#endif
}

void app_init_dpcbflag()
{
    g_xdma_abnormal = 0;
    g_rs_abnormal = 0;
    g_hog_abnormal = 0;
    g_rs_frameready = 0;
    g_hog_frameready = 0;
    g_md_detect = 0;
    g_cdm_fifoerror = 0;
    g_wdt1_timeout = 0;
    g_wdt2_timeout = 0;
    g_wdt3_timeout = 0;
    g_dp_event = 0;
    g_frame_ready = 0;
}

void app_stop_cv()
{
    dbg_printf(DBG_APP_PRINT_LEVEL, "###Stop State###\n");
    evt_dp_clear_sensor_rtc();
    evt_dp_clear_all_dpevent();
    hx_SensorRTC_evthdl_register_cb(0, sensor_rtc_fire);
    sensordplib_stop_capture();
    sensordplib_start_swreset();
    sensordplib_stop_swreset_WoSensorCtrl();
    g_app_cur_state = APP_STATE_STOP;
}

static void app_cpu_sleep_at_capture()
{
#ifdef SUPPORT_CPU_SLEEP_AT_CAPTURE
    dbg_printf(DBG_APP_PRINT_LEVEL, "app_cpu_sleep_at_capture\n");
    PM_CFG_T aCfg;
    PM_CFG_PWR_MODE_E mode = PM_MODE_CPU_SLEEP;

    hx_lib_get_defcfg_bymode(&aCfg, mode);
    hx_lib_pm_mode_set(aCfg);
#endif
}

static void error_debug_report()
{
    hx_drv_edm_get_de_count(0, &g_de0_count);
    hx_drv_edm_get_conv_de_count(&g_convde_count);
    sensordplib_get_xdma_fin(&g_wdma1_fin, &g_wdma2_fin, &g_wdma3_fin, &g_rdma_fin);
    sensordplib_get_xdma_sc_finflag(&g_xdmadone, &g_nframe_end);
    hx_drv_edm_get_frame_count(&g_af_framecnt, &g_be_framecnt);
    dbg_printf(DBG_APP_PRINT_LEVEL, "de0_count=%d, convde_count=%d\n", g_de0_count, g_convde_count);
    dbg_printf(DBG_APP_PRINT_LEVEL, "wdma1_fin=%d,wdma2_fin=%d,wdma3_fin=%d,rdma_fin=%d\n", g_wdma1_fin, g_wdma2_fin, g_wdma3_fin, g_rdma_fin);
    dbg_printf(DBG_APP_PRINT_LEVEL, "nframe_end=%d,xdmadone=%d\n", g_nframe_end, g_xdmadone);
    dbg_printf(DBG_APP_PRINT_LEVEL, "af_framecnt=%d,be_framecnt=%d\n", g_af_framecnt, g_be_framecnt);
}

static void sensor_rtc_fire(uint32_t interval)
{
    g_rtc_cnt++;
    dbg_printf(DBG_APP_PRINT_LEVEL, "g_rtc_cnt=%d\n", g_rtc_cnt);
    sensordplib_get_xdma_sc_finflag(&g_xdmadone, &g_nframe_end);
    if (g_frame_process_done == 0)
    {
        dbg_printf(DBG_APP_PRINT_LEVEL, "Frame Process not done g_app_cur_state=%d, g_app_new_state=%d\n", g_app_cur_state, g_app_new_state);
        dbg_printf(DBG_APP_PRINT_LEVEL, "sensor_rtc_fire xdma_fin_flag=%d,sc_fin_flag=%d,period=%d\n", g_xdmadone, g_nframe_end, interval);
        error_debug_report();
        /*Error Handling*/
        g_rtc_err_retry_cnt++;
        if (g_rtc_err_retry_cnt >= MAX_RTC_NOT_RECAP_ERR_REBOOT_CNT)
        {
            dbg_printf(DBG_APP_PRINT_LEVEL, "\n\n\n\nsensor_rtc_fire overtime CDM reboot\n");
            hx_lib_pm_cpu_rst();
        }
        else if (g_rtc_err_retry_cnt >= MAX_RTC_NOT_RECAP_ERR_RETRY_CNT)
        {
            if (g_app_cur_state == APP_STATE_STOP) //Stop
            {
                dbg_printf(DBG_APP_PRINT_LEVEL, "\n\n\n\nsensor_rtc_fire dp not ready retry Stop\n");
                sensordplib_stop_capture();
                sensordplib_start_swreset();
                sensordplib_stop_swreset_WoSensorCtrl();
                evt_dp_clear_all_dpevent();
                app_stop_cv();
            }
            else
            { //HW5x5
                dbg_printf(DBG_APP_PRINT_LEVEL, "\n\n\n\nsensor_rtc_fire dp not ready retry HW5x5\n");
                sensordplib_stop_capture();
                sensordplib_start_swreset();
                sensordplib_stop_swreset_WoSensorCtrl();
                evt_dp_clear_all_dpevent();
                app_start_cv();
            }
        }
    }
    else
    { //if(g_frame_process_done == 1)
        g_rtc_cnt = 0;
        g_rtc_err_retry_cnt = 0;

        g_frame_process_done = 0;

        if (g_app_cur_state == g_app_new_state)
        {
            if (g_app_cur_state == APP_STATE_STOP)
            {
                dbg_printf(DBG_APP_PRINT_LEVEL, "###change to Stop State###\n");
                app_stop_cv();
            }
            else
            {
                dbg_printf(DBG_APP_PRINT_LEVEL, "###re-trigger capture###\n");
                app_aiot_hw5x5jpeg_recapture();
            }
        }
        else
        {
            if (g_app_new_state == APP_STATE_STOP)
            {
                dbg_printf(DBG_APP_PRINT_LEVEL, "###change to Stop State###\n");
                app_stop_cv();
            }
            else if (g_app_new_state == APP_STATE_FACE_LIVE_HW5X5JPEG)
            {
                dbg_printf(DBG_APP_PRINT_LEVEL, "###change to HW5x5 State###\n");
                app_start_cv();
            }
        }
    }
}

static void app_aiot_hw5x5jpeg_recapture()
{
    dbg_printf(DBG_APP_PRINT_LEVEL, "app_aiot_hw5x5jpeg_recapture()\n");
    app_nonAOS_restreaming();
    sensordplib_retrigger_capture();
    app_cpu_sleep_at_capture();
}

uint8_t app_aiot_face_hw5x5jpeg_algo_check(uint32_t img_addr)
{
    int read_status = 0;
    uint8_t next_jpeg_enc_frameno = 0;
    uint8_t cur_jpeg_enc_frameno = 0;
    uint32_t jpeg_enc_addr = 0;
    uint32_t jpeg_enc_filesize = 0;
    uint32_t de0_count;
    uint32_t convde_count;
    uint16_t af_framecnt;
    uint16_t be_framecnt;
    uint8_t wdma1_fin, wdma2_fin, wdma3_fin, rdma_fin;
    uint8_t ready_flag, nframe_end, xdmadone;
#ifdef USE_TICK
    uint32_t tick_algo_start = 0, tick_algo_stop = 0;
#endif

  //  dbg_printf(DBG_APP_PRINT_LEVEL, "app_aiot_face_hw5x5jpeg_algo_check() \n");

    /*Error handling*/
    if (g_xdma_abnormal != 0)
    {
        sensordplib_stop_capture();
        sensordplib_start_swreset();
        sensordplib_stop_swreset_WoSensorCtrl();
        dbg_printf(DBG_APP_PRINT_LEVEL, "abnormal re-setup path cur_frame=%d,acc=%d,event=%d\n", g_cur_hw5x5jpeg_frame, g_hw5x5jpeg_acc_frame, g_dp_event);
        g_xdma_abnormal = 0;
        g_hw5x5jpeg_err_retry_cnt++;
        //need re-setup configuration
        if (g_hw5x5jpeg_err_retry_cnt < MAX_HW5x5JPEG_ERR_RETRY_CNT)
        {
            app_aiot_face_setup_hw5x5jpeg_path(g_wdma3_baseaddr);
        }
        else
        {
            dbg_printf(DBG_APP_PRINT_LEVEL, "hw5x5jpeg xdma fail overtime\n");
            app_iot_facedetect_systemreset();
        }
    }

    if ((g_wdt1_timeout == 1) || (g_wdt2_timeout == 1) || (g_wdt3_timeout == 1))
    {
        dbg_printf(DBG_APP_PRINT_LEVEL, "EDM WDT timeout event=%d\n", g_dp_event);
        hx_drv_edm_get_de_count(0, &de0_count);
        hx_drv_edm_get_conv_de_count(&convde_count);
        dbg_printf(DBG_APP_PRINT_LEVEL, "de0_count=%d, convde_count=%d\n", de0_count, convde_count);
        sensordplib_get_xdma_fin(&wdma1_fin, &wdma2_fin, &wdma3_fin, &rdma_fin);
        dbg_printf(DBG_APP_PRINT_LEVEL, "wdma1_fin=%d,wdma2_fin=%d,wdma3_fin=%d,rdma_fin=%d\n", wdma1_fin, wdma2_fin, wdma3_fin, rdma_fin);
        sensordplib_get_status(&ready_flag, &nframe_end, &xdmadone);
        dbg_printf(DBG_APP_PRINT_LEVEL, "ready_flag=%d,nframe_end=%d,xdmadone=%d\n", ready_flag, nframe_end, xdmadone);
        hx_drv_edm_get_frame_count(&af_framecnt, &be_framecnt);
        dbg_printf(DBG_APP_PRINT_LEVEL, "af_framecnt=%d,be_framecnt=%d\n", af_framecnt, be_framecnt);

        sensordplib_stop_capture();
        if (app_sensor_standby() != 0)
        {
            dbg_printf(DBG_APP_PRINT_LEVEL, "standby sensor fail 7\n");
        }
        sensordplib_start_swreset();
        sensordplib_stop_swreset_WoSensorCtrl();
        g_wdt1_timeout = 0;
        g_wdt2_timeout = 0;
        g_wdt3_timeout = 0;
        g_hw5x5jpeg_err_retry_cnt++;
        if (g_hw5x5jpeg_err_retry_cnt < MAX_HW5x5JPEG_ERR_RETRY_CNT)
        {
            app_sensor_xshutdown_toggle();
            if (app_config_sensor_WE1_rx(1, SENSOR_STROBE_REQ) != 0)
            {
                dbg_printf(DBG_APP_PRINT_LEVEL, "re-setup sensor fail\n");
            }
            app_aiot_face_setup_hw5x5jpeg_path(g_wdma3_baseaddr);
        }
        else
        {
            dbg_printf(DBG_APP_PRINT_LEVEL, "hw5x5jpeg WDT fail overtime\n");
            app_iot_facedetect_systemreset();
        }
    }

    if (g_inp1bitparer_abnormal != 0)
    {
        dbg_printf(DBG_APP_PRINT_LEVEL, "g_inp1bitparer_err=%d\n", g_dp_event);
        g_hw5x5jpeg_err_retry_cnt++;
        g_inp1bitparer_abnormal = 0;
        if (g_hw5x5jpeg_err_retry_cnt < MAX_HW5x5JPEG_ERR_RETRY_CNT)
        {
            app_1bitparser_err_info();
            if (app_config_sensor_WE1_rx(1, SENSOR_STROBE_REQ) != 0)
            {
                dbg_printf(DBG_APP_PRINT_LEVEL, "re-setup sensor fail\n");
            }
            app_aiot_face_setup_hw5x5jpeg_path(g_wdma3_baseaddr);
        }
        else
        {
            dbg_printf(DBG_APP_PRINT_LEVEL, "HW5x5JPEG 1bitparser Err retry overtime\n");
            app_iot_facedetect_systemreset();
        }
    }

    //Frame ready
    if (g_frame_ready == 1)
    {
        g_hw5x5jpeg_err_retry_cnt = 0;
#ifdef USE_TICK
        g_tick_stop = _arc_aux_read(AUX_TIMER0_CNT);
        g_tick_period = g_tick_stop - g_tick_start;
        g_period = g_tick_period / BOARD_SYS_TIMER_MS_CONV;
   //     dbg_printf(DBG_APP_PRINT_LEVEL, "g_frame_ready \n");
   //     dbg_printf(DBG_APP_PRINT_LEVEL, "[tick] start:%d, stop:%d, period:%d \n", g_tick_start, g_tick_stop, g_tick_period);
        timer_stop(TIMER_0);
        timer_start(TIMER_0, TIMER_CTRL_NH, 0xffffffff); // Set Counter LIMIT to MAX
        g_tick_start = _arc_aux_read(AUX_TIMER0_CNT);
        dbg_printf(DBG_APP_PRINT_LEVEL, "[tick] next frame start:%d \n", g_tick_start);
#else
        g_period = (g_rtc_cnt + 1) * g_cur_sensor_rtc;
        dbg_printf(DBG_APP_PRINT_LEVEL, "period=%d,g_rtc_cnt=%d,g_cur_sensor_rtc=%d\n", g_period, g_rtc_cnt, g_cur_sensor_rtc);
#endif

        dbg_printf(DBG_APP_PRINT_LEVEL, "frame:%d, 5x5JPEG Frame ready, period:%d ms (acc=%d) face=%d, noface=%d\n", g_cur_hw5x5jpeg_frame, g_period, g_hw5x5jpeg_acc_frame, g_face_frame, g_noface_frame);



        //algorithm
        if (app_cust_config.app_table_cfg.through_cv == WE1AppCfg_THROUGH_CV_YES)
        {
#ifdef SUPPORT_SPI_PC_TRANSFER_RAW
            read_status = spi_write(img_addr, g_image_size, DATA_TYPE_RAW_IMG);
            dbg_printf(DBG_APP_PRINT_LEVEL, "HW5x5 write frame result %d, data size %d\n", read_status, g_image_size);
#endif
            hx_drv_xdma_get_WDMA2NextFrameIdx(&next_jpeg_enc_frameno);
            hx_drv_xdma_get_WDMA2_bufferNo(&g_jpeg_total_slot);
            if (next_jpeg_enc_frameno == 0)
            {
                cur_jpeg_enc_frameno = g_jpeg_total_slot - 1;
            }
            else
            {
                cur_jpeg_enc_frameno = next_jpeg_enc_frameno - 1;
            }
            hx_drv_jpeg_get_FillFileSizeToMem(cur_jpeg_enc_frameno, g_jpegautofill_addr, &jpeg_enc_filesize);
            hx_drv_jpeg_get_MemAddrByFrameNo(cur_jpeg_enc_frameno, g_wdma2_baseaddr, &jpeg_enc_addr);
            dbg_printf(DBG_APP_PRINT_LEVEL, "next_jpeg_enc_frameno=%d,g_jpeg_total_slot=%d,jpeg_enc_addr=0x%x jpeg_enc_filesize=0x%x\n",next_jpeg_enc_frameno, g_jpeg_total_slot,jpeg_enc_addr, jpeg_enc_filesize);
#ifdef USE_TICK
            tick_algo_start = _arc_aux_read(AUX_TIMER0_CNT);
#endif
            tflitemicro_algo_run(img_addr, app_cust_config.we1_driver_cfg.jpeg_cfg.enc_width, app_cust_config.we1_driver_cfg.jpeg_cfg.enc_height, &algo_result);
#ifdef USE_TICK
            tick_algo_stop = _arc_aux_read(AUX_TIMER0_CNT);
            dbg_printf(DBG_APP_PRINT_LEVEL, "[cv algo] frame:%d, period:%d (tick)\n", g_cur_hw5x5jpeg_frame, (tick_algo_stop - tick_algo_start));
#endif
            //algo_result
            if (algo_result.humanPresence == true)
            {
                dbg_printf(DBG_APP_PRINT_LEVEL, "### RESULT: OBJECT FOUND ###  \n");

            }
            else
            {
                dbg_printf(DBG_APP_PRINT_LEVEL, "XXX RESULT: OBJECT NOT FOUND XXX \n");

            }
        }
        else
        {
            algo_result.humanPresence = false;
            algo_result.num_tracked_human_targets = 0;
        }


        g_frame_ready = 0;

        dbg_printf(DBG_LESS_INFO, "Frame:%d, human=%d (acc=%d)\n", g_cur_hw5x5jpeg_frame, algo_result.humanPresence, g_hw5x5jpeg_acc_frame);
#if defined(SUPPORT_SPI_PC_TRANSFER_ENC_EVERYFRAME) || defined(SUPPORT_SPI_PC_TRANSFER_ENC_EVERYFRAME_META)
        read_status = 255;
        read_status = spi_write(jpeg_enc_addr, jpeg_enc_filesize, DATA_TYPE_JPG);        
        dbg_printf(DBG_APP_PRINT_LEVEL, "write frame result %d\n",read_status);

#ifdef SUPPORT_SPI_PC_TRANSFER_ENC_EVERYFRAME_META
        read_status = spi_write((uint32_t)&algo_result, sizeof(struct_algoResult), DATA_TYPE_META_DATA);
        dbg_printf(DBG_APP_PRINT_LEVEL, "write meta result %d\n", read_status);
#endif
#endif

        if (algo_result.humanPresence == false)
        {
            g_noface_frame++;

            if (app_cust_config.app_table_cfg.human_detect_ind != WE1AppCfg_GPIO_IOMUX_NONE)
            {
                hx_drv_iomux_set_outvalue(g_human_led_pin, 0);
            }
            return 0;
        }
        else
        {
            g_face_frame++;

            if (app_cust_config.app_table_cfg.human_detect_ind != WE1AppCfg_GPIO_IOMUX_NONE)
            {
                hx_drv_iomux_set_outvalue(g_human_led_pin, 1);
            }
#if 0
			dbg_printf(DBG_APP_PRINT_LEVEL,"num_tracked_human_targets=%d\n",algo_result.num_tracked_human_targets);
			for(uint8_t idx = 0; idx < algo_result.num_tracked_human_targets; idx++)
			{
				dbg_printf(DBG_APP_PRINT_LEVEL,"x[%d]=%d\n", idx,algo_result.ht[idx].head_bbox.x);
				dbg_printf(DBG_APP_PRINT_LEVEL,"y[%d]=%d\n", idx,algo_result.ht[idx].head_bbox.y);
				dbg_printf(DBG_APP_PRINT_LEVEL,"width[%d]=%d\n", idx,algo_result.ht[idx].head_bbox.width);
				dbg_printf(DBG_APP_PRINT_LEVEL,"height[%d]=%d\n", idx,algo_result.ht[idx].head_bbox.height);
			}
#endif
            return 1;
        }

        /////////End//////////////////////
    } //frame ready
    return 0;
}

void app_aiot_face_hw5x5jpeg_eventhdl_cb(EVT_INDEX_E event)
{
    uint8_t human_present = 0;
    uint16_t err;
    g_dp_event = event;

    switch (event)
    {
    case EVT_INDEX_1BITPARSER_ERR: /*reg_inpparser_fs_cycle_error*/
        hx_drv_inp1bitparser_get_errstatus(&err);
        dbg_printf(DBG_APP_PRINT_LEVEL, "err=0x%x\n", err);
        hx_drv_inp1bitparser_clear_int();
        hx_drv_inp1bitparser_set_enable(0);
        g_inp1bitparer_abnormal = 1;
        break;
    case EVT_INDEX_EDM_WDT1_TIMEOUT:
        g_wdt1_timeout = 1;
        break;
    case EVT_INDEX_EDM_WDT2_TIMEOUT:
        g_wdt2_timeout = 1;
        break;
    case EVT_INDEX_EDM_WDT3_TIMEOUT:
        g_wdt3_timeout = 1;
        break;

    case EVT_INDEX_CDM_FIFO_ERR:
        /*
                * error happen need CDM timing & TPG setting
                * 1. SWRESET Datapath
                * 2. restart streaming flow
                */
        g_cdm_fifoerror = 1;

        break;

    case EVT_INDEX_XDMA_WDMA1_ABNORMAL:
    case EVT_INDEX_XDMA_WDMA2_ABNORMAL:
    case EVT_INDEX_XDMA_WDMA3_ABNORMAL:
    case EVT_INDEX_XDMA_RDMA_ABNORMAL:
        /*
                * error happen need
                * 1. SWRESET Datapath
                * 2. restart streaming flow
                */
        g_xdma_abnormal = 1;
        break;

    case EVT_INDEX_RSDMA_ABNORMAL:
        /*
                * error happen need
                * 1. SWRESET RS & RS DMA
                * 2. Re-run flow again
                */
        g_rs_abnormal = 1;
        break;

    case EVT_INDEX_HOGDMA_ABNORMAL:
        /*
                * error happen need
                * 1. SWRESET HOG & HOG DMA
                * 2. Re-run flow again
                */
        g_hog_abnormal = 1;
        break;

    case EVT_INDEX_CDM_MOTION:
        /*
                * app anything want to do
                * */
        g_md_detect = 1;
        break;
    case EVT_INDEX_XDMA_FRAME_READY:
        g_cur_hw5x5jpeg_frame++;
        g_hw5x5jpeg_acc_frame++;

        g_frame_ready = 1;
        break;
    case EVT_INDEX_RSDMA_FINISH:
        g_rs_frameready = 1;
        break;
    case EVT_INDEX_HOGDMA_FINISH:
        g_hog_frameready = 1;
        break;
    case EVT_INDEX_SENSOR_RTC_FIRE:
        break;
    default:
        dbg_printf(DBG_APP_PRINT_LEVEL, "Other Event %d\n", event);
        break;
    }

    dbg_printf(DBG_APP_PRINT_LEVEL, "app_aiot_face_hw5x5jpeg_eventhdl_cb \n");

    human_present = app_aiot_face_hw5x5jpeg_algo_check(g_wdma3_baseaddr);
    g_frame_process_done = 1;
    dbg_printf(DBG_LESS_INFO, "Frame:%d\n", g_cur_hw5x5jpeg_frame);
    //dbg_printf(DBG_APP_PRINT_LEVEL, "HW5x5Jpeg Frame:%d(acc:%d) human present:%d\n", g_cur_hw5x5jpeg_frame, g_hw5x5jpeg_acc_frame, human_present);

    if (app_cust_config.app_table_cfg.classification_rtc == 0)
    {
        if (g_app_cur_state == g_app_new_state)
        {
            app_aiot_hw5x5jpeg_recapture();
        }
        else
        {
            if (g_app_new_state == APP_STATE_STOP)
            {
                app_stop_cv();
            }
        }
    }
    else
    {
        /*wait timer fire recapture*/
    }
}

/**
 * \brief	Setup HW5x5 --> WDMA3 and HW5x5 --> JPEG Encoder --> WDMA2 Configuration by datapath library
 *
 * \param[in]	image_addr	 image content memory start address
 * \return	void.
 */
void app_aiot_face_setup_hw5x5jpeg_path(uint32_t image_addr)
{
    evt_dp_clear_sensor_rtc();
    evt_dp_clear_all_dpevent();

#ifdef USE_TICK
    timer_stop(TIMER_0);
    timer_start(TIMER_0, TIMER_CTRL_NH, 0xffffffff); // Set Counter LIMIT to MAX
    g_tick_start = _arc_aux_read(AUX_TIMER0_CNT);
    dbg_printf(DBG_APP_PRINT_LEVEL, "setup HW5x5 [g_tick_start]:%d \n", g_tick_start);
#endif

    app_init_dpcbflag();

    g_app_cur_state = APP_STATE_FACE_LIVE_HW5X5JPEG;
    g_app_new_state = APP_STATE_FACE_LIVE_HW5X5JPEG;
    g_frame_process_done = 0;
    g_face_frame = 0;
    g_noface_frame = 0;

#ifdef SUPPORT_HW5X5_ONLY
    sensordplib_set_hw5x5_wdma3(app_cust_config.we1_driver_cfg.hw5x5_cfg,
                                NULL);
#else
    /*
        hw5x5_cfg.hw5x5_path=0
        hw5x5_cfg.demos_pattern_mode=0
        hw5x5_cfg.demos_color_mode=1
        hw5x5_cfg.demos_bndmode=2
        hw5x5_cfg.demoslpf_roundmode=0
        hw5x5_cfg.hw55_crop_stx=0
        hw5x5_cfg.hw55_crop_sty=0
        hw5x5_cfg.hw55_in_width=640
        hw5x5_cfg.hw55_in_height=480
        hw5x5_cfg.fir_lbp_th=3
        hw5x5_cfg.fir_procmode=0
        hw5x5_cfg.firlpf_bndmode=1

        jpeg_cfg.jpeg_path=1    
        jpeg_cfg.jpeg_enctype=1
        jpeg_cfg.jpeg_encqtable=1
        jpeg_cfg.enc_width=640
        jpeg_cfg.enc_height=480
        jpeg_cfg.dec_width=640
        jpeg_cfg.dec_height=480
        jpeg_cfg.dec_roi_stx=0
        jpeg_cfg.dec_roi_sty=0    
   
        we1_driver_cfg.cyclic_buffer_cnt=1
    */

    sensordplib_set_int_hw5x5_jpeg_wdma23(app_cust_config.we1_driver_cfg.hw5x5_cfg, app_cust_config.we1_driver_cfg.jpeg_cfg,
                                          app_cust_config.we1_driver_cfg.cyclic_buffer_cnt,
                                          NULL);
#endif

    hx_dplib_evthandler_register_cb(app_aiot_face_hw5x5jpeg_eventhdl_cb, SENSORDPLIB_CB_FUNTYPE_DP);

    sensordplib_set_sensorctrl_start();

    /* app_table_cfg.classification_rtc=0 */
    if (app_cust_config.app_table_cfg.classification_rtc != 0)
    {
        hx_SensorRTC_evthdl_register_cb(app_cust_config.app_table_cfg.classification_rtc, sensor_rtc_fire);
    }

    app_cpu_sleep_at_capture();
}

void app_start_cv()
{
    xprintf("app_start_cv() \n");
    app_aiot_face_setup_hw5x5jpeg_path(g_wdma3_baseaddr);
}

/**
 * \brief	system reset
 *
 * \return	void.
 */
void app_iot_facedetect_systemreset()
{
    dbg_printf(DBG_APP_PRINT_LEVEL, "\n\n\n\n\nxxxx system reset xxxx\n");
    /*TODO*/
    hx_lib_pm_chip_rst(PMU_WE1_POWERPLAN_INTERNAL_LDO);
}
