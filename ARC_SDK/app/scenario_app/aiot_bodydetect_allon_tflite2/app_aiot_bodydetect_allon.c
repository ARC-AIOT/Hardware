/*
 * app_aiot_facedetect.c
 *
 *  Created on: 2019/6/16
 *      Author: 902447
 */

#include <app_aiot_bodydetect_allon.h>
#include "tflitemicro_algo.h"


extern WE1AppCfg_CustTable_t app_cust_config;
extern WE1AlgoCfg_CustTable_t algo_cust_config;

//datapath boot up reason flag
volatile uint8_t g_bootup_md_detect = 0;
volatile uint8_t g_bootup_pir_detect = 0;

//datapath callback flag
volatile uint8_t g_xdma_abnormal = 0;
volatile uint8_t g_rs_abnormal = 0;
volatile uint8_t g_hog_abnormal = 0;
volatile uint8_t g_rs_frameready = 0;
volatile uint8_t g_hog_frameready = 0;
volatile uint8_t g_md_detect = 0;
volatile uint8_t g_cdm_fifoerror = 0;
volatile uint8_t g_wdt1_timeout = 0;
volatile uint8_t g_wdt2_timeout = 0;
volatile uint8_t g_wdt3_timeout = 0;
volatile int32_t g_inp1bitparer_abnormal = 0;
volatile uint32_t g_dp_event = 0;
volatile uint8_t g_frame_ready = 0;

//HW5x5 +JPEG Enc Frame
volatile uint32_t g_cur_hw5x5jpeg_frame = 0;
volatile uint32_t g_hw5x5jpeg_acc_frame = 0;

uint8_t g_jpeg_total_slot = 0;

CIS_XHSHUTDOWN_INDEX_E g_xshutdown_pin;
IOMUX_INDEX_E g_motion_led_pin;
IOMUX_INDEX_E g_human_led_pin;

//tick cal
volatile uint32_t g_tick_start = 0, g_tick_stop = 0, g_tick_toggle = 0;
volatile uint32_t g_tick_period, g_period;
volatile uint32_t g_tick_sensor_std = 0, g_tick_sensor_stream = 0, g_tick_sensor_toggle = 0;

//error retry time
volatile uint32_t g_hw5x5jpeg_err_retry_cnt = 0;

uint32_t g_image_size = 0;
uint32_t g_wdma1_baseaddr = WE1_DP_WDMA1_OUT_SRAM_ADDR;
uint32_t g_wdma2_baseaddr = WE1_DP_WDMA2_OUT_SRAM_ADDR;
uint32_t g_wdma3_baseaddr = WE1_DP_WDMA3_OUT_SRAM_ADDR;
uint32_t g_jpegautofill_addr = WE1_DP_JPEG_HWAUTOFILL_SRAM_ADDR;
SENSORDPLIB_STREAM_E g_stream_type = SENSORDPLIB_STREAM_NONEAOS;
SENSORDPLIB_HM11B1_HEADER_T info;
volatile APP_STATE_E g_app_cur_state = APP_STATE_INIT;
volatile APP_STATE_E g_app_new_state = APP_STATE_INIT;


void app_1bitparser_err_info();
void app_set_dp_mclk_src();
void app_load_cfg_from_flash();
/**
 * \brief	application flag initial
 *
 * \return	void.
 */
void app_init_flag();

void app_aiot_bodydetect_init();

void app_start_cv();
void app_get_gpio(WE1AppCfg_GPIO_e app_gpio, IOMUX_INDEX_E *gpio_mux);
void app_get_xshutdown(WE1AppCfg_GPIO_e app_gpio, CIS_XHSHUTDOWN_INDEX_E *gpio_mux);
void app_print_cfg();

#ifdef SIM_I2C_CHANGE_STATE
static void adc_rtc_timer_dump(uint32_t val)
{
    APP_STATE_E app_state;
    dbg_printf(DBG_LESS_INFO, "adc_rtc_timer_dump\n");

    app_get_cur_state(&app_state);
    dbg_printf(DBG_LESS_INFO, "adc_timer_fire app_state=%d\n", app_state);

    if (app_state == APP_STATE_FACE_LIVE_HW5X5JPEG)
    {
        dbg_printf(DBG_LESS_INFO, "\n\n\n\nfrom %d to %d\n", app_state, APP_STATE_STOP);
        app_setup_change_state(APP_STATE_STOP);
    }
    else if (app_state == APP_STATE_STOP)
    {
        dbg_printf(DBG_LESS_INFO, "\n\n\n\nfrom %d to %d\n", app_state, APP_STATE_FACE_LIVE_HW5X5JPEG);
        app_start_cv();
    }
    else
    {
        dbg_printf(DBG_LESS_INFO, "\n\n\n\nOthers State=%d\n", app_state);
    }
}
#endif

static int open_spi()
{
	int ret ;
#ifndef SPI_MASTER_SEND
	ret = hx_drv_spi_slv_open();
	dbg_printf(DBG_LESS_INFO, "SPI slave ");
#else
	ret = hx_drv_spi_mst_open();
	dbg_printf(DBG_LESS_INFO, "SPI master ");
#endif
    return ret;
}

void app_set_dp_mclk_src()
{
    if (app_cust_config.we1_driver_cfg.dp_clk_mux == WE1AppCfg_DP_CLK_Mux_RC36M)
    {
        hx_drv_dp_set_dp_clk_src(DP_CLK_SRC_RC_36M);
    }
    else if (app_cust_config.we1_driver_cfg.dp_clk_mux == WE1AppCfg_DP_CLK_Mux_XTAL)
    {
        hx_drv_dp_set_dp_clk_src(DP_CLK_SRC_XTAL_24M_POST);
    }
    if (app_cust_config.we1_driver_cfg.mclk_clk_mux == WE1AppCfg_MCLK_CLK_Mux_XTAL)
    {
        hx_drv_dp_set_mclk_src(DP_MCLK_SRC_INTERNAL, DP_MCLK_SRC_INT_SEL_XTAL);
    }
    else if (app_cust_config.we1_driver_cfg.mclk_clk_mux == WE1AppCfg_MCLK_CLK_Mux_RC36M)
    {
        hx_drv_dp_set_mclk_src(DP_MCLK_SRC_INTERNAL, DP_MCLK_SRC_INT_SEL_RC36M);
    }
}

int app_setup_change_state(APP_STATE_E next_app_state)
{
    g_app_new_state = next_app_state;
    return 0;
}

/*HM11B1 used only*/
void app_1bitparser_err_info()
{
    uint32_t de0_count;

    /*get inp1bitparser fsm*/
    hx_drv_inp1bitparser_get_fsm(&info.fsm);
    /*get inp1bitparser HW hvsize*/
    hx_drv_inp1bitparser_get_HW_hvsize(&info.hw_hsize, &info.hw_vsize);
    /*get inp1bitparser hvsize*/
    hx_drv_inp1bitparser_get_hvsize(&info.sensor_hsize, &info.sensor_vsize);
    /*get inp1bitparser frame len, line len*/
    hx_drv_inp1bitparser_get_framelinelen(&info.frame_len, &info.line_len);
    /*get inp1bitparser again*/
    hx_drv_inp1bitparser_get_again(&info.again);
    /*get inp1bitparser dgain*/
    hx_drv_inp1bitparser_get_dgain(&info.dgain);
    /*get inp1bitparser integration time*/
    hx_drv_inp1bitparser_get_intg(&info.intg);
    /*get inp1bitparser interrupt src*/
    hx_drv_inp1bitparser_get_intsrc(&info.intsrc);
    /*get inp1bitparser fstus*/
    hx_drv_inp1bitparser_get_fstus(&info.fstus);
    /*get inp1bitparser fc*/
    hx_drv_inp1bitparser_get_fc(&info.fc);
    /*get inp1bitparser crc*/
    hx_drv_inp1bitparser_get_crc(&info.sensor_crc, &info.hw_crc);
    hx_drv_inp1bitparser_get_cycle(&info.fs_cycle, &info.fe_cycle);
    hx_drv_inp1bitparser_get_fscycle_err_cnt(&info.fs_cycle_err_cnt);
    hx_drv_inp1bitparser_get_errstatus(&info.err_status);

    dbg_printf(DBG_LESS_INFO, "fsm=%d\n", info.fsm);
    dbg_printf(DBG_LESS_INFO, "hw_hsize=%d,hw_vsize=%d\n", info.hw_hsize, info.hw_vsize);
    dbg_printf(DBG_LESS_INFO, "sensor_hsize=%d,sensor_vsize=%d\n", info.sensor_hsize, info.sensor_vsize);
#if 0
	dbg_printf(DBG_LESS_INFO,"frame_len=0x%x,line_len=0x%x\n",info.frame_len, info.line_len);
	dbg_printf(DBG_LESS_INFO,"again=0x%x\n",info.again);
	dbg_printf(DBG_LESS_INFO,"dgain=0x%x\n",info.dgain);
	dbg_printf(DBG_LESS_INFO,"intg=0x%x\n",info.intg);
	dbg_printf(DBG_LESS_INFO,"intsrc=0x%x\n",info.intsrc);
	dbg_printf(DBG_LESS_INFO,"fstus=0x%x\n",info.fstus);
	dbg_printf(DBG_LESS_INFO,"fc=0x%x\n",info.fc);
#endif
    dbg_printf(DBG_LESS_INFO, "sensor_crc=0x%x,hw_crc=0x%x\n", info.sensor_crc, info.hw_crc);
    dbg_printf(DBG_LESS_INFO, "fs_cycle=%d,fe_cycle=%d\n", info.fs_cycle, info.fe_cycle);
    dbg_printf(DBG_LESS_INFO, "fs_cycle_err_cnt=%d\n", info.fs_cycle_err_cnt);
    dbg_printf(DBG_LESS_INFO, "err_status=%d\n", info.err_status);

    hx_drv_inp1bitparser_clear_int();
    hx_drv_inp1bitparser_set_enable(0);
    hx_drv_edm_get_de_count(0, &de0_count);
    dbg_printf(DBG_LESS_INFO, "de0_count=%d\n", de0_count);

    sensordplib_stop_capture();
    if (app_sensor_standby() != 0)
    {
        dbg_printf(DBG_LESS_INFO, "standby sensor fail 9\n");
    }
    sensordplib_start_swreset();
    sensordplib_stop_swreset_WoSensorCtrl();
    g_inp1bitparer_abnormal = 0;
    app_sensor_xshutdown_toggle();
    if (app_config_sensor_WE1_rx(1, SENSOR_STROBE_REQ) != 0)
    {
        dbg_printf(DBG_LESS_INFO, "re-setup sensor fail\n");
    }
}

void app_load_cfg_from_flash()
{
    uint32_t apptotal_len = sizeof(app_cust_config);
    uint32_t appcfg_len = 0;
    uint32_t flash_appcfg_addr = FLASH_APPCFG_BASEADDR;
    uint32_t appcfg_sram_addr = (uint32_t)&app_cust_config;
    uint32_t appcfg_content_sram_addr = 0;
    uint32_t appcfg_header_size = 0;
    uint8_t pu8CRC8_appcfg = 0;
    uint16_t pu16CheckSum_appcfg = 0;

    uint32_t algototal_len = sizeof(algo_cust_config);
    uint32_t algocfg_len = 0;
    uint32_t flash_algocfg_addr = FLASH_ALGOCFG_BASEADDR;
    uint32_t algocfg_sram_addr = (uint32_t)&algo_cust_config;
    uint32_t algocfg_content_sram_addr = 0;
    uint32_t algocfg_header_size = 0;
    uint8_t pu8CRC8_algocfg = 0;
    uint16_t pu16CheckSum_algocfg = 0;

    appcfg_header_size += sizeof(app_cust_config.table_header);                /**< Table Header */
    appcfg_header_size += sizeof(app_cust_config.app_table_info);              /**< Application Table information */
    appcfg_header_size += sizeof(app_cust_config.sensor_cfg_table_info);       /**< Sensor Configuration Table information */
    appcfg_header_size += sizeof(app_cust_config.sensor_streamon_table_info);  /**< Sensor Stream On Table information */
    appcfg_header_size += sizeof(app_cust_config.sensor_streamoff_table_info); /**< Sensor Stream Off Table information */
    appcfg_header_size += sizeof(app_cust_config.we1_table_info);              /**< WE-1 Driver Table information */
    appcfg_header_size += sizeof(app_cust_config.soc_com_table_info);          /**< SOC Communication Table information */

    appcfg_len = apptotal_len - appcfg_header_size;
    appcfg_content_sram_addr = appcfg_sram_addr + appcfg_header_size;
    dbg_printf(DBG_LESS_INFO, "header_size=0x%x,apptotal_len=%d,appcfg_len=%d,appcfg_sram_addr=0x%x,AppSRAMContent=0x%x\n", appcfg_header_size, apptotal_len, appcfg_len, appcfg_sram_addr, appcfg_content_sram_addr);

    if (hx_drv_spi_flash_protocol_read(0, flash_appcfg_addr, appcfg_sram_addr, appcfg_len, 4) != 0)
    {
        dbg_printf(DBG_LESS_INFO, "read flash fail flash_appcfg_addr=0x%x,appcfg_sram_addr=0x%x,len=%d\n", flash_appcfg_addr, appcfg_sram_addr, appcfg_len);
        app_iot_table_def_config();
    }
    else
    {
        dbg_printf(DBG_LESS_INFO, "read flash success flash_appcfg_addr=0x%x,appcfg_sram_addr=0x%x,len=%d\n", flash_appcfg_addr, appcfg_sram_addr, appcfg_len);
        HxGetCRC8((uint8_t *)appcfg_content_sram_addr, appcfg_len, &pu8CRC8_appcfg, &pu16CheckSum_appcfg);
        dbg_printf(DBG_LESS_INFO, "pu8CRC8_appcfg=0x%x,pu16CheckSum=0x%x\n", pu8CRC8_appcfg, pu16CheckSum_appcfg);

        if ((app_cust_config.table_header.table_crc != pu8CRC8_appcfg) || (app_cust_config.table_header.table_checksum != pu16CheckSum_appcfg) || (app_cust_config.table_header.table_version != APP_CONFIG_TABLE_VERSION))
        {
            dbg_printf(DBG_LESS_INFO, "App CFG bin have problem load default\n");
            dbg_printf(DBG_LESS_INFO, "app_table_crc=0x%x, cal_appcrc=0x%x\n", app_cust_config.table_header.table_crc, pu8CRC8_appcfg);
            dbg_printf(DBG_LESS_INFO, "app_table_chksum=0x%x, cal_appchksum=0x%x\n", app_cust_config.table_header.table_checksum, pu16CheckSum_appcfg);
            dbg_printf(DBG_LESS_INFO, "app_table_version=0x%x, app_h_version=0x%x\n", app_cust_config.table_header.table_version, APP_CONFIG_TABLE_VERSION);
            app_iot_table_def_config();
        }
    }

    algocfg_header_size += sizeof(algo_cust_config.table_header);    /**< Table Header */
    algocfg_header_size += sizeof(algo_cust_config.algo_table_info); /**< Application Table information */
    algocfg_len = algototal_len - algocfg_header_size;
    algocfg_content_sram_addr = algocfg_sram_addr + algocfg_header_size;
    dbg_printf(DBG_LESS_INFO, "algoheader_size=0x%x,algototal_len=%d,algocfg_len=%d,algocfg_sram_addr=0x%x,AlgoSRAMContent=0x%x\n", algocfg_header_size, algototal_len, algocfg_len, algocfg_sram_addr, algocfg_content_sram_addr);

    if (hx_drv_spi_flash_protocol_read(0, flash_algocfg_addr, algocfg_sram_addr, algocfg_len, 4) != 0)
    {
        dbg_printf(DBG_LESS_INFO, "read flash fail flash_algocfg_addr=0x%x,algocfg_sram_addr=0x%x,len=%d\n", flash_algocfg_addr, algocfg_sram_addr, algocfg_len);
        app_algo_table_def_config();
    }
    else
    {
        dbg_printf(DBG_LESS_INFO, "read flash success flash_algocfg_addr=0x%x,algocfg_sram_addr=0x%x,len=%d\n", flash_algocfg_addr, algocfg_sram_addr, algocfg_len);
        HxGetCRC8((uint8_t *)algocfg_content_sram_addr, algocfg_len, &pu8CRC8_algocfg, &pu16CheckSum_algocfg);
        dbg_printf(DBG_LESS_INFO, "pu8CRC8_algocfg=0x%x,pu16CheckSum_algocfg=0x%x\n", pu8CRC8_algocfg, pu16CheckSum_algocfg);
        if ((algo_cust_config.table_header.table_crc != pu8CRC8_algocfg) || (algo_cust_config.table_header.table_checksum != pu16CheckSum_algocfg) || (algo_cust_config.table_header.table_version != ALGO_CONFIG_TABLE_VERSION))
        {
            dbg_printf(DBG_LESS_INFO, "Algo CFG bin have problem load default\n");
            dbg_printf(DBG_LESS_INFO, "algo_table_crc=0x%x, cal_appcrc=0x%x\n", algo_cust_config.table_header.table_crc, pu8CRC8_algocfg);
            dbg_printf(DBG_LESS_INFO, "algo_table_chksum=0x%x, cal_appchksum=0x%x\n", algo_cust_config.table_header.table_checksum, pu16CheckSum_algocfg);
            dbg_printf(DBG_LESS_INFO, "algo_table_version=0x%x, app_h_version=0x%x\n", algo_cust_config.table_header.table_version, ALGO_CONFIG_TABLE_VERSION);
            app_algo_table_def_config();
        }
    }
}

/*application init flag*/
void app_init_flag()
{
#ifdef SUPPORT_HW5X5_ONLY
    g_image_size = app_cust_config.we1_driver_cfg.hw5x5_cfg.hw55_in_width * app_cust_config.we1_driver_cfg.hw5x5_cfg.hw55_in_height;
#else
    g_image_size = app_cust_config.we1_driver_cfg.jpeg_cfg.enc_width * app_cust_config.we1_driver_cfg.jpeg_cfg.enc_height;
#endif

    app_init_dpcbflag();

    //JPEG Decoder
    g_jpeg_total_slot = 0;

    //HW5x5 JPEG Enc
    g_cur_hw5x5jpeg_frame = 0;

    //err count init
    g_hw5x5jpeg_err_retry_cnt = 0;

    if (app_cust_config.app_table_cfg.motion_led_ind != WE1AppCfg_GPIO_IOMUX_NONE)
    {
        app_get_gpio(app_cust_config.app_table_cfg.motion_led_ind, &g_motion_led_pin);
        hx_drv_iomux_set_pmux(g_motion_led_pin, 3);
    }

    if (app_cust_config.app_table_cfg.human_detect_ind != WE1AppCfg_GPIO_IOMUX_NONE)
    {
        app_get_gpio(app_cust_config.app_table_cfg.human_detect_ind, &g_human_led_pin);
        hx_drv_iomux_set_pmux(g_human_led_pin, 3);
    }
}

void app_get_xshutdown(WE1AppCfg_GPIO_e app_gpio, CIS_XHSHUTDOWN_INDEX_E *gpio_mux)
{
    switch (app_gpio)
    {
    case WE1AppCfg_GPIO_IOMUX_SGPIO0:
        *gpio_mux = CIS_XHSUTDOWN_IOMUX_SGPIO0;
        break;
    case WE1AppCfg_GPIO_IOMUX_SGPIO1:
        *gpio_mux = CIS_XHSUTDOWN_IOMUX_SGPIO1;
        break;
    default:
        *gpio_mux = CIS_XHSUTDOWN_IOMUX_NONE;
        break;
    }
}

void app_get_gpio(WE1AppCfg_GPIO_e app_gpio, IOMUX_INDEX_E *gpio_mux)
{
    switch (app_gpio)
    {
    case WE1AppCfg_GPIO_IOMUX_PGPIO0:
        *gpio_mux = IOMUX_PGPIO0;
        break;
    case WE1AppCfg_GPIO_IOMUX_PGPIO1:
        *gpio_mux = IOMUX_PGPIO1;
        break;
    case WE1AppCfg_GPIO_IOMUX_PGPIO2:
        *gpio_mux = IOMUX_PGPIO2;
        break;
    case WE1AppCfg_GPIO_IOMUX_PGPIO3:
        *gpio_mux = IOMUX_PGPIO3;
        break;
    case WE1AppCfg_GPIO_IOMUX_PGPIO4:
        *gpio_mux = IOMUX_PGPIO4;
        break;
    case WE1AppCfg_GPIO_IOMUX_PGPIO5:
        *gpio_mux = IOMUX_PGPIO5;
        break;
    case WE1AppCfg_GPIO_IOMUX_PGPIO6:
        *gpio_mux = IOMUX_PGPIO6;
        break;
    case WE1AppCfg_GPIO_IOMUX_PGPIO7:
        *gpio_mux = IOMUX_PGPIO7;
        break;
    case WE1AppCfg_GPIO_IOMUX_PGPIO8:
        *gpio_mux = IOMUX_PGPIO8;
        break;
    case WE1AppCfg_GPIO_IOMUX_PGPIO9:
        *gpio_mux = IOMUX_PGPIO9;
        break;
    case WE1AppCfg_GPIO_IOMUX_PGPIO10:
        *gpio_mux = IOMUX_PGPIO10;
        break;
    case WE1AppCfg_GPIO_IOMUX_PGPIO11:
        *gpio_mux = IOMUX_PGPIO11;
        break;
    case WE1AppCfg_GPIO_IOMUX_PGPIO12:
        *gpio_mux = IOMUX_PGPIO12;
        break;
    case WE1AppCfg_GPIO_IOMUX_PGPIO13:
        *gpio_mux = IOMUX_PGPIO13;
        break;
    case WE1AppCfg_GPIO_IOMUX_PGPIO14:
        *gpio_mux = IOMUX_PGPIO14;
        break;
    case WE1AppCfg_GPIO_IOMUX_RESERVED:
        *gpio_mux = IOMUX_RESERVED;
        break;
    case WE1AppCfg_GPIO_IOMUX_SGPIO0:
        *gpio_mux = IOMUX_SGPIO0;
        break;
    case WE1AppCfg_GPIO_IOMUX_SGPIO1:
        *gpio_mux = IOMUX_SGPIO1;
        break;
    default:
        break;
    }
}


void app_aiot_bodydetect_init()
{
    int ret = -1;

#ifdef PRINT_CFG_SIZE
    EMBARC_PRINTF("WE1Cfg_CustTable_t size=%d\n", sizeof(WE1Cfg_CustTable_t));
    EMBARC_PRINTF("WE1Cfg_TableHeader_t size=%d\n", sizeof(WE1Cfg_TableHeader_t));
    EMBARC_PRINTF("WE1Cfg_TableInfo_t size=%d\n", sizeof(WE1Cfg_TableInfo_t));
    EMBARC_PRINTF("WE1Cfg_APP_t size=%d\n", sizeof(WE1Cfg_APP_t));
    EMBARC_PRINTF("WE1Cfg_Sensor_t size=%d\n", sizeof(WE1Cfg_Sensor_t));
    EMBARC_PRINTF("WE1Cfg_Sensor_StreamOn_t size=%d\n", sizeof(WE1Cfg_Sensor_StreamOn_t));
    EMBARC_PRINTF("WE1Cfg_Sensor_StreamOff_t size=%d\n", sizeof(WE1Cfg_Sensor_StreamOff_t));
    EMBARC_PRINTF("WE1Cfg_WE1Driver_t size=%d\n", sizeof(WE1Cfg_WE1Driver_t));
    EMBARC_PRINTF("WE1Cfg_Algo_Para_t size=%d\n", sizeof(WE1Cfg_Algo_Para_t));
    EMBARC_PRINTF("WE1Cfg_SOC_COM_t size=%d\n", sizeof(WE1Cfg_SOC_COM_t));
#endif
    app_load_cfg_from_flash();
    app_print_cfg();
    app_init_flag();
    app_set_dp_mclk_src();
    /*TODO read bootup reason*/

    /*TODO according to chip package and sensor ID configure share pin*/
    app_rx_set_wlcsp38_sharepin();
    app_get_xshutdown(app_cust_config.we1_driver_cfg.xshutdown_pin_sel, &g_xshutdown_pin);

    dbg_printf(DBG_LESS_INFO, "[WEI FIRMWARE] Daemon Process\n\tEvent Handler");

    event_handler_init();

    sensordplib_start_swreset();
    sensordplib_stop_swreset_WoSensorCtrl();
    ret = open_spi();
    if (ret != 0)
    {
        dbg_printf(DBG_LESS_INFO, "initial fail\n");
    }
    else
    {
        dbg_printf(DBG_LESS_INFO, " initial done\n");
    }
    //acc flag
    g_hw5x5jpeg_acc_frame = 0;

    g_wdma1_baseaddr = app_cust_config.we1_driver_cfg.wdma1_startaddr;
    g_wdma2_baseaddr = app_cust_config.we1_driver_cfg.wdma2_startaddr;
    g_wdma3_baseaddr = app_cust_config.we1_driver_cfg.wdma3_startaddr;
    g_jpegautofill_addr = app_cust_config.we1_driver_cfg.jpegsize_autofill_startaddr;

    sensordplib_set_xDMA_baseaddrbyapp(g_wdma1_baseaddr, g_wdma2_baseaddr, g_wdma3_baseaddr);
    sensordplib_set_jpegfilesize_addrbyapp(g_jpegautofill_addr);
	//Power Plan
	hx_drv_pmu_set_ctrl(PMU_PWR_PLAN, app_cust_config.we1_driver_cfg.pmu_powerplan);
	tflitemicro_algo_init();
}

/*application */
void app_aiot_bodydetect_google()
{
    uint8_t idx = 0;
    uint8_t sensor_init_required = 0;
    uint16_t sensor_id;
    uint8_t rev_id;

    arc_int_disable(BOARD_SYS_TIMER_INTNO); // TIMER0
    arc_int_disable(BOARD_STD_TIMER_INTNO); // TIMER1

    /*inital*/
    app_aiot_bodydetect_init();

    //app FSM
    g_app_cur_state = APP_STATE_INIT;
    g_app_new_state = APP_STATE_INIT;

    /*Sensor Configuration*/
    //hx_drv_pmu_get_ctrl(PMU_SEN_INIT, &sensor_init_required);
    sensor_init_required = 1;
    dbg_printf(DBG_LESS_INFO, "sensor_init_required=%d\n", sensor_init_required);
    if (sensor_init_required == 1)
    {
        hx_drv_cis_init(g_xshutdown_pin, app_cust_config.we1_driver_cfg.mclk_div);
        app_get_sensor_id(&sensor_id, &rev_id);
        dbg_printf(DBG_LESS_INFO, "sensor_id=0x%04x,rev_id=0x%x\n", sensor_id, rev_id);
    }

    /*Sensor Configuration*/
    if (app_config_sensor_WE1_rx(sensor_init_required, SENSOR_STROBE_REQ) != 0)
    {
        dbg_printf(DBG_LESS_INFO, "app_config_sensor_WE1_rx fail\n");
        for (idx = 0; idx < MAX_RECONFIG_SENSOR_TIME; idx++)
        {
            if (app_config_sensor_WE1_rx(1, SENSOR_STROBE_REQ) != 0)
            {
                dbg_printf(DBG_LESS_INFO, "app_config_sensor_WE1_rx fail\n");
            }
            else
            {
                break;
            }
        }
        if (idx >= MAX_RECONFIG_SENSOR_TIME)
        {
            dbg_printf(DBG_LESS_INFO, "Sensor may crash\n");
            app_iot_facedetect_systemreset();
            return;
        }
    }

    /*start CV*/
    app_start_cv();

#ifdef SIM_I2C_CHANGE_STATE
    hx_ADCRTC_evthdl_register_cb(SIM_ADC_RTC_PERIOD, adc_rtc_timer_dump);
#endif

    xprintf("\n\n\n\n\n");
    event_handler_start();

    if (app_sensor_standby() != 0)
    {
        dbg_printf(DBG_LESS_INFO, "standby sensor fail 10\n");
    }

}

void app_print_cfg()
{
#ifdef SUPPORT_PRINT_CFG_HEADER
    dbg_printf(DBG_LESS_INFO, "table_version=%d\n", app_cust_config.table_header.table_version);
    dbg_printf(DBG_LESS_INFO, "table_header.totalLen=%d\n", app_cust_config.table_header.totalLen);
    dbg_printf(DBG_LESS_INFO, "table_Len=%d\n", app_cust_config.table_header.table_Len);
    dbg_printf(DBG_LESS_INFO, "table_cate_count=%d\n", app_cust_config.table_header.table_cate_count);
    dbg_printf(DBG_LESS_INFO, "table_crc=0x%x\n", app_cust_config.table_header.table_crc);
    dbg_printf(DBG_LESS_INFO, "table_checksum=0x%x\n", app_cust_config.table_header.table_checksum);

    dbg_printf(DBG_LESS_INFO, "app_table_info.table_type=%d\n", app_cust_config.app_table_info.table_type);
    dbg_printf(DBG_LESS_INFO, "app_table_info.u16Len=%d\n", app_cust_config.app_table_info.u16Len);
    dbg_printf(DBG_LESS_INFO, "app_table_info.u16Offset=%d\n", app_cust_config.app_table_info.u16Offset);

    dbg_printf(DBG_LESS_INFO, "sensor_cfg_table_info.table_type=%d\n", app_cust_config.sensor_cfg_table_info.table_type);
    dbg_printf(DBG_LESS_INFO, "sensor_cfg_table_info.u16Len=%d\n", app_cust_config.sensor_cfg_table_info.u16Len);
    dbg_printf(DBG_LESS_INFO, "sensor_cfg_table_info.u16Offset=%d\n", app_cust_config.sensor_cfg_table_info.u16Offset);

    dbg_printf(DBG_LESS_INFO, "sensor_streamon_table_info.table_type=%d\n", app_cust_config.sensor_streamon_table_info.table_type);
    dbg_printf(DBG_LESS_INFO, "sensor_streamon_table_info.u16Len=%d\n", app_cust_config.sensor_streamon_table_info.u16Len);
    dbg_printf(DBG_LESS_INFO, "sensor_streamon_table_info.u16Offset=%d\n", app_cust_config.sensor_streamon_table_info.u16Offset);

    dbg_printf(DBG_LESS_INFO, "sensor_streamoff_table_info.table_type=%d\n", app_cust_config.sensor_streamoff_table_info.table_type);
    dbg_printf(DBG_LESS_INFO, "sensor_streamoff_table_info.u16Len=%d\n", app_cust_config.sensor_streamoff_table_info.u16Len);
    dbg_printf(DBG_LESS_INFO, "sensor_streamoff_table_info.u16Offset=%d\n", app_cust_config.sensor_streamoff_table_info.u16Offset);

    dbg_printf(DBG_LESS_INFO, "we1_table_info.table_type=%d\n", app_cust_config.we1_table_info.table_type);
    dbg_printf(DBG_LESS_INFO, "we1_table_info.u16Len=%d\n", app_cust_config.we1_table_info.u16Len);
    dbg_printf(DBG_LESS_INFO, "we1_table_info.u16Offset=%d\n", app_cust_config.we1_table_info.u16Offset);

    dbg_printf(DBG_LESS_INFO, "soc_com_table_info.table_type=%d\n", app_cust_config.soc_com_table_info.table_type);
    dbg_printf(DBG_LESS_INFO, "soc_com_table_info.u16Len=%d\n", app_cust_config.soc_com_table_info.u16Len);
    dbg_printf(DBG_LESS_INFO, "soc_com_table_info.u16Offset=%d\n", app_cust_config.soc_com_table_info.u16Offset);
#endif
#ifdef SUPPORT_PRINT_CFG_CONTENT
    dbg_printf(DBG_LESS_INFO, "app_table_cfg.app_type=%d\n", app_cust_config.app_table_cfg.app_type);
    dbg_printf(DBG_LESS_INFO, "app_table_cfg.pmu_sensor_rtc=%d\n", app_cust_config.app_table_cfg.pmu_sensor_rtc);
    dbg_printf(DBG_LESS_INFO, "app_table_cfg.classification_rtc=%d\n", app_cust_config.app_table_cfg.classification_rtc);
    dbg_printf(DBG_LESS_INFO, "app_table_cfg.adc_rtc=%d\n", app_cust_config.app_table_cfg.adc_rtc);
    dbg_printf(DBG_LESS_INFO, "app_table_cfg.classification_detect_max_frame=%d\n", app_cust_config.app_table_cfg.classification_detect_max_frame);
    dbg_printf(DBG_LESS_INFO, "app_table_cfg.detect_monitor_mode=%d\n", app_cust_config.app_table_cfg.detect_monitor_mode);
    dbg_printf(DBG_LESS_INFO, "app_table_cfg.nodetect_monitor_frame=%d\n", app_cust_config.app_table_cfg.nodetect_monitor_frame);
    dbg_printf(DBG_LESS_INFO, "app_table_cfg.cyclic_check_frame=%d\n", app_cust_config.app_table_cfg.cyclic_check_frame);
    dbg_printf(DBG_LESS_INFO, "app_table_cfg.human_detect_ind=%d\n", app_cust_config.app_table_cfg.human_detect_ind);
    dbg_printf(DBG_LESS_INFO, "app_table_cfg.motion_led_ind=%d\n", app_cust_config.app_table_cfg.motion_led_ind);
    dbg_printf(DBG_LESS_INFO, "app_table_cfg.through_cv=%d\n", app_cust_config.app_table_cfg.through_cv);
    dbg_printf(DBG_LESS_INFO, "app_table_cfg.ir_led=%d\n", app_cust_config.app_table_cfg.ir_led);
    dbg_printf(DBG_LESS_INFO, "app_table_cfg.light_support=%d\n", app_cust_config.app_table_cfg.light_support);
    dbg_printf(DBG_LESS_INFO, "app_table_cfg.pdm_support=%d\n", app_cust_config.app_table_cfg.pdm_support);
    dbg_printf(DBG_LESS_INFO, "app_table_cfg.i2s_support=%d\n", app_cust_config.app_table_cfg.i2s_support);

    dbg_printf(DBG_LESS_INFO, "sensor_table_cfg.sensor_id=%d\n", app_cust_config.sensor_table_cfg.sensor_id);
    dbg_printf(DBG_LESS_INFO, "sensor_table_cfg.sensor_width=%d\n", app_cust_config.sensor_table_cfg.sensor_width);
    dbg_printf(DBG_LESS_INFO, "sensor_table_cfg.sensor_height=%d\n", app_cust_config.sensor_table_cfg.sensor_height);
    dbg_printf(DBG_LESS_INFO, "sensor_table_cfg.sensor_color=%d\n", app_cust_config.sensor_table_cfg.sensor_color);
    dbg_printf(DBG_LESS_INFO, "sensor_table_cfg.active_cfg_cnt=%d\n", app_cust_config.sensor_table_cfg.active_cfg_cnt);
    dbg_printf(DBG_LESS_INFO, "sensor_table_cfg.sensor_stream_type=%d\n", app_cust_config.sensor_table_cfg.sensor_stream_type);

    dbg_printf(DBG_LESS_INFO, "sensor_streamon_cfg.active_cfg_cnt=%d\n", app_cust_config.sensor_streamon_cfg.active_cfg_cnt);

    dbg_printf(DBG_LESS_INFO, "sensor_streamoff_cfg.active_cfg_cnt=%d\n", app_cust_config.sensor_streamoff_cfg.active_cfg_cnt);

    dbg_printf(DBG_LESS_INFO, "we1_driver_cfg.chip_package=%d\n", app_cust_config.we1_driver_cfg.chip_package);
    dbg_printf(DBG_LESS_INFO, "we1_driver_cfg.xshutdown_pin_sel=%d\n", app_cust_config.we1_driver_cfg.xshutdown_pin_sel);
    dbg_printf(DBG_LESS_INFO, "we1_driver_cfg.dp_clk_mux=%d\n", app_cust_config.we1_driver_cfg.dp_clk_mux);
    dbg_printf(DBG_LESS_INFO, "we1_driver_cfg.mclk_clk_mux=%d\n", app_cust_config.we1_driver_cfg.mclk_clk_mux);
    dbg_printf(DBG_LESS_INFO, "we1_driver_cfg.mclk_div=%d\n", app_cust_config.we1_driver_cfg.mclk_div);
    dbg_printf(DBG_LESS_INFO, "we1_driver_cfg.wdma1_startaddr=0x%x\n", app_cust_config.we1_driver_cfg.wdma1_startaddr);
    dbg_printf(DBG_LESS_INFO, "we1_driver_cfg.wdma2_startaddr=0x%x\n", app_cust_config.we1_driver_cfg.wdma2_startaddr);
    dbg_printf(DBG_LESS_INFO, "we1_driver_cfg.wdma3_startaddr=0x%x\n", app_cust_config.we1_driver_cfg.wdma3_startaddr);
    dbg_printf(DBG_LESS_INFO, "we1_driver_cfg.jpegsize_autofill_startaddr=0x%x\n", app_cust_config.we1_driver_cfg.jpegsize_autofill_startaddr);
    dbg_printf(DBG_LESS_INFO, "we1_driver_cfg.cyclic_buffer_cnt=%d\n", app_cust_config.we1_driver_cfg.cyclic_buffer_cnt);
    dbg_printf(DBG_LESS_INFO, "we1_driver_cfg.act_wakupCPU_pin_cnt=%d\n", app_cust_config.we1_driver_cfg.act_wakupCPU_pin_cnt);
    dbg_printf(DBG_LESS_INFO, "we1_driver_cfg.wakeupCPU_int_pin[0]=%d\n", app_cust_config.we1_driver_cfg.wakeupCPU_int_pin[0]);
    dbg_printf(DBG_LESS_INFO, "we1_driver_cfg.analoge_pir_ch_sel=%d\n", app_cust_config.we1_driver_cfg.analoge_pir_ch_sel);
    dbg_printf(DBG_LESS_INFO, "we1_driver_cfg.analoge_pir_th=%d\n", app_cust_config.we1_driver_cfg.analoge_pir_th);
    dbg_printf(DBG_LESS_INFO, "we1_driver_cfg.light_sensor_adc_ch_sel=%d\n", app_cust_config.we1_driver_cfg.light_sensor_adc_ch_sel);
    dbg_printf(DBG_LESS_INFO, "we1_driver_cfg.light_sensor_th=%d\n", app_cust_config.we1_driver_cfg.light_sensor_th);
    dbg_printf(DBG_LESS_INFO, "hw2x2_cfg.hw2x2_path=%d\n", app_cust_config.we1_driver_cfg.hw2x2_cfg.hw2x2_path);
    dbg_printf(DBG_LESS_INFO, "hw2x2_cfg.hw_22_process_mode=%d\n", app_cust_config.we1_driver_cfg.hw2x2_cfg.hw_22_process_mode);
    dbg_printf(DBG_LESS_INFO, "hw2x2_cfg.hw_22_mono_round_mode=%d\n", app_cust_config.we1_driver_cfg.hw2x2_cfg.hw_22_mono_round_mode);
    dbg_printf(DBG_LESS_INFO, "hw2x2_cfg.hw_22_crop_stx=%d\n", app_cust_config.we1_driver_cfg.hw2x2_cfg.hw_22_crop_stx);
    dbg_printf(DBG_LESS_INFO, "hw2x2_cfg.hw_22_crop_sty=%d\n", app_cust_config.we1_driver_cfg.hw2x2_cfg.hw_22_crop_sty);
    dbg_printf(DBG_LESS_INFO, "hw2x2_cfg.hw_22_in_width=%d\n", app_cust_config.we1_driver_cfg.hw2x2_cfg.hw_22_in_width);
    dbg_printf(DBG_LESS_INFO, "hw2x2_cfg.hw_22_in_height=%d\n", app_cust_config.we1_driver_cfg.hw2x2_cfg.hw_22_in_height);
    dbg_printf(DBG_LESS_INFO, "cdm_cfg.cdm_enable=%d\n", app_cust_config.we1_driver_cfg.cdm_cfg.cdm_enable);
    dbg_printf(DBG_LESS_INFO, "cdm_cfg.cdm_crop_stx=%d\n", app_cust_config.we1_driver_cfg.cdm_cfg.cdm_crop_stx);
    dbg_printf(DBG_LESS_INFO, "cdm_cfg.cdm_crop_sty=%d\n", app_cust_config.we1_driver_cfg.cdm_cfg.cdm_crop_sty);
    dbg_printf(DBG_LESS_INFO, "cdm_cfg.cdm_in_width=%d\n", app_cust_config.we1_driver_cfg.cdm_cfg.cdm_in_width);
    dbg_printf(DBG_LESS_INFO, "cdm_cfg.cdm_in_height=%d\n", app_cust_config.we1_driver_cfg.cdm_cfg.cdm_in_height);
    dbg_printf(DBG_LESS_INFO, "cdm_cfg.meta_dump=%d\n", app_cust_config.we1_driver_cfg.cdm_cfg.meta_dump);
    dbg_printf(DBG_LESS_INFO, "cdm_cfg.init_map_flag=%d\n", app_cust_config.we1_driver_cfg.cdm_cfg.init_map_flag);
    dbg_printf(DBG_LESS_INFO, "cdm_cfg.ht_packing=%d\n", app_cust_config.we1_driver_cfg.cdm_cfg.ht_packing);
    dbg_printf(DBG_LESS_INFO, "cdm_cfg.cpu_activeflag=%d\n", app_cust_config.we1_driver_cfg.cdm_cfg.cpu_activeflag);
    dbg_printf(DBG_LESS_INFO, "cdm_cfg.cdm_eros_th=%d\n", app_cust_config.we1_driver_cfg.cdm_cfg.cdm_eros_th);
    dbg_printf(DBG_LESS_INFO, "cdm_cfg.cdm_tolerance=%d\n", app_cust_config.we1_driver_cfg.cdm_cfg.cdm_tolerance);
    dbg_printf(DBG_LESS_INFO, "cdm_cfg.cdm_relaxation=%d\n", app_cust_config.we1_driver_cfg.cdm_cfg.cdm_relaxation);
    dbg_printf(DBG_LESS_INFO, "cdm_cfg.cdm_reactance=%d\n", app_cust_config.we1_driver_cfg.cdm_cfg.cdm_reactance);
    dbg_printf(DBG_LESS_INFO, "cdm_cfg.cdm_min_allow_dis=%d\n", app_cust_config.we1_driver_cfg.cdm_cfg.cdm_min_allow_dis);
    dbg_printf(DBG_LESS_INFO, "cdm_cfg.cdm_num_ht_th=%d\n", app_cust_config.we1_driver_cfg.cdm_cfg.cdm_num_ht_th);
    dbg_printf(DBG_LESS_INFO, "cdm_cfg.cdm_num_cons_ht_bin_th_x=%d\n", app_cust_config.we1_driver_cfg.cdm_cfg.cdm_num_cons_ht_bin_th_x);
    dbg_printf(DBG_LESS_INFO, "cdm_cfg.cdm_num_cons_ht_bin_th_y=%d\n", app_cust_config.we1_driver_cfg.cdm_cfg.cdm_num_cons_ht_bin_th_y);
    dbg_printf(DBG_LESS_INFO, "cdm_cfg.cdm_num_ht_vect_th_x=%d\n", app_cust_config.we1_driver_cfg.cdm_cfg.cdm_num_ht_vect_th_x);
    dbg_printf(DBG_LESS_INFO, "cdm_cfg.cdm_num_ht_vect_th_y=%d\n", app_cust_config.we1_driver_cfg.cdm_cfg.cdm_num_ht_vect_th_y);
    dbg_printf(DBG_LESS_INFO, "hw5x5_cfg.hw5x5_path=%d\n", app_cust_config.we1_driver_cfg.hw5x5_cfg.hw5x5_path);
    dbg_printf(DBG_LESS_INFO, "hw5x5_cfg.demos_pattern_mode=%d\n", app_cust_config.we1_driver_cfg.hw5x5_cfg.demos_pattern_mode);
    dbg_printf(DBG_LESS_INFO, "hw5x5_cfg.demos_color_mode=%d\n", app_cust_config.we1_driver_cfg.hw5x5_cfg.demos_color_mode);
    dbg_printf(DBG_LESS_INFO, "hw5x5_cfg.demos_bndmode=%d\n", app_cust_config.we1_driver_cfg.hw5x5_cfg.demos_bndmode);
    dbg_printf(DBG_LESS_INFO, "hw5x5_cfg.demoslpf_roundmode=%d\n", app_cust_config.we1_driver_cfg.hw5x5_cfg.demoslpf_roundmode);
    dbg_printf(DBG_LESS_INFO, "hw5x5_cfg.hw55_crop_stx=%d\n", app_cust_config.we1_driver_cfg.hw5x5_cfg.hw55_crop_stx);
    dbg_printf(DBG_LESS_INFO, "hw5x5_cfg.hw55_crop_sty=%d\n", app_cust_config.we1_driver_cfg.hw5x5_cfg.hw55_crop_sty);
    dbg_printf(DBG_LESS_INFO, "hw5x5_cfg.hw55_in_width=%d\n", app_cust_config.we1_driver_cfg.hw5x5_cfg.hw55_in_width);
    dbg_printf(DBG_LESS_INFO, "hw5x5_cfg.hw55_in_height=%d\n", app_cust_config.we1_driver_cfg.hw5x5_cfg.hw55_in_height);
    dbg_printf(DBG_LESS_INFO, "hw5x5_cfg.fir_lbp_th=%d\n", app_cust_config.we1_driver_cfg.hw5x5_cfg.fir_lbp_th);
    dbg_printf(DBG_LESS_INFO, "hw5x5_cfg.fir_procmode=%d\n", app_cust_config.we1_driver_cfg.hw5x5_cfg.fir_procmode);
    dbg_printf(DBG_LESS_INFO, "hw5x5_cfg.firlpf_bndmode=%d\n", app_cust_config.we1_driver_cfg.hw5x5_cfg.firlpf_bndmode);
    dbg_printf(DBG_LESS_INFO, "jpeg_cfg.jpeg_path=%d\n", app_cust_config.we1_driver_cfg.jpeg_cfg.jpeg_path);
    dbg_printf(DBG_LESS_INFO, "jpeg_cfg.jpeg_enctype=%d\n", app_cust_config.we1_driver_cfg.jpeg_cfg.jpeg_enctype);
    dbg_printf(DBG_LESS_INFO, "jpeg_cfg.jpeg_encqtable=%d\n", app_cust_config.we1_driver_cfg.jpeg_cfg.jpeg_encqtable);
    dbg_printf(DBG_LESS_INFO, "jpeg_cfg.enc_width=%d\n", app_cust_config.we1_driver_cfg.jpeg_cfg.enc_width);
    dbg_printf(DBG_LESS_INFO, "jpeg_cfg.enc_height=%d\n", app_cust_config.we1_driver_cfg.jpeg_cfg.enc_height);
    dbg_printf(DBG_LESS_INFO, "jpeg_cfg.dec_width=%d\n", app_cust_config.we1_driver_cfg.jpeg_cfg.dec_width);
    dbg_printf(DBG_LESS_INFO, "jpeg_cfg.dec_height=%d\n", app_cust_config.we1_driver_cfg.jpeg_cfg.dec_height);
    dbg_printf(DBG_LESS_INFO, "jpeg_cfg.dec_roi_stx=%d\n", app_cust_config.we1_driver_cfg.jpeg_cfg.dec_roi_stx);
    dbg_printf(DBG_LESS_INFO, "jpeg_cfg.dec_roi_sty=%d\n", app_cust_config.we1_driver_cfg.jpeg_cfg.dec_roi_sty);
    dbg_printf(DBG_LESS_INFO, "we1_driver_cfg.pmu_type=%d\n", app_cust_config.we1_driver_cfg.pmu_type);

    dbg_printf(DBG_LESS_INFO, "soc_comm_cfg.comm_type=%d\n", app_cust_config.soc_comm_cfg.comm_type);
#endif
}

void app_get_cur_state(APP_STATE_E *app_state)
{
    *app_state = g_app_cur_state;
}
