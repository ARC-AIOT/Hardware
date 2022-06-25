/*
 * peripheral_cmd_handler.c
 *
 *  Created on: 2020¦~4¤ë9¤é
 *      Author: 903935
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "peripheral_cmd_handler.h"
#include "embARC_debug.h"
#ifndef AUDIO_RECOGNITION
//#include <app_algo.h>
#endif
#include "ccitt_crc16.h"
#ifdef DUT
#include "evt_spiscomm.h"
#endif
#include "event.h"
#include "event_handler_evt.h"
#include "printf_color.h"
#include "hx_drv_iomux.h"
#if defined(USE_STANDALONE) && (defined(OMRON_FD_FR) || defined(OMRON_FD_FR_BD))
#include <standalone_comm/exch_tbl_api.h>
#endif

extern infra_evt_t 	g_event[];

extern uint32_t g_wdma3_baseaddr;
volatile static EN_DATA_TX_CMD_T g_en_data_tx;
volatile static CFG_SUB_CMD_DATA_T g_sub_cmd_cfg;
#ifdef DUT
volatile static DBG_MODE_DATA_T g_dbg_mode_data = {0};
#endif
#if defined(USE_STANDALONE) && (defined(OMRON_FD_FR) || defined(OMRON_FD_FR_BD))
/* global variable used to set FR reg flag into standalone */
volatile bool g_FR_register_flag = false;
#else
extern volatile bool g_FR_register_flag;
#endif
#ifdef CDM_ENABLE
extern uint8_t g_cdm_tolerance;
#endif

volatile bool g_focusing_en = false;

#if defined(QRCODE) || defined(DUT)
volatile bool g_send_every_frame_en = true;
#else
volatile bool g_send_every_frame_en = false;
#endif
volatile bool g_wait_pc = false;

static char* wkup_str[] = {"Others", "Voice Wakeup","FD Wakeup"};

volatile static SPISCOMM_CB_TYPE_E spis_cb_type;

void peripheral_cmd_init()
{
    memset(&g_en_data_tx, 0, sizeof(g_en_data_tx));
#if defined(AIOT_HUMANDETECT_TV_CDM)
    g_en_data_tx.en_jpg = ENABLE;
    g_en_data_tx.en_meta = ENABLE;
    g_en_data_tx.en_voice = ENABLE;
#elif defined(SAMPLE_CODE_RTOS)
    g_en_data_tx.en_raw = ENABLE;
    g_en_data_tx.en_meta = ENABLE;
    g_en_data_tx.en_voice = ENABLE;
#endif
    g_en_data_tx.en_algo = ENABLE;

#if defined(TV01)
     g_sub_cmd_cfg.humn_lv_timeout = 5;  //sec
#elif defined (TV02) || defined(TV04)
    g_sub_cmd_cfg.humn_lv_timeout = 2;  //sec
#elif defined(TV03)
    g_sub_cmd_cfg.humn_lv_timeout = 2;  //sec
#else
    g_sub_cmd_cfg.humn_lv_timeout = 2;  //sec
#endif

    g_sub_cmd_cfg.wakeup_method = OTHERS_WKUP;
#ifdef TV01
    g_sub_cmd_cfg.human_lock_count = 1;  //1//one for tpv demo  //default 0xFF lock count
    g_sub_cmd_cfg.eye_protect_timeout= 5;  //5 for TPV demo //default 3 sec
#endif

    spis_cb_type = PACKET_CMD;

#ifdef DUT
#ifdef EMZA_ALGO
    g_dbg_mode_data.auto_run_max = 1;   //times
#else  // OMRON
    g_dbg_mode_data.auto_run_max = 2;   //times
#endif
#endif
}

uint8_t read_cmd_handler(uint8_t *rx_buf, uint32_t buf_size)
{
    uint8_t i;
    uint32_t command_data = 0;
    uint8_t cfg_cmd = 0, cfg_sub_cmd = 0, dbg_mode = 0;
#if 1
    if(rx_buf[0] != 0xC0 && rx_buf[1] != 0x5A)
        return -1;

// remove debug message, debug message will cause SPI_SLV event lost
//    for(i = 0; i < buf_size; i++)
//    {
//        dbg_printf(DBG_LESS_INFO, "rx_buf[%d]:0x%x \n", i, rx_buf[i]);
//    }

    READ_CMD_DATA_TYPE_E command_type = rx_buf[2];
    switch(command_type)
    {
        #ifdef OMRON_FACE_RECOGNITION
        case TYPE_FR:
            EMBARC_PRINTF("TYPE_FR\n");
            command_data = rx_buf[6]+(rx_buf[5]<<8)+(rx_buf[4]<<16)+(rx_buf[3]<<24);
            switch(command_data)
            {
                case FR_REG:
                    g_FR_register_flag = true;
                #if defined(USE_STANDALONE) && (defined(OMRON_FD_FR) || defined(OMRON_FD_FR_BD))
                    sa_ioctl(SET_FR_REG_FLAG, (void *)&g_FR_register_flag, NULL);
                #endif
                #ifdef TV01
                    //diable send jpg
                    g_en_data_tx.en_jpg = DISABLE;
                #endif
                    break;
                case FR_UNREG:
                    app_algo_albumunRegister();
                    break;
                default:
                    break;
            }
            break;
        #endif
        case TYPE_BASIC:
            EMBARC_PRINTF("TYPE_BASIC\n");
            command_data = rx_buf[6]+(rx_buf[5]<<8)+(rx_buf[4]<<16)+(rx_buf[3]<<24);
            switch(command_data)
            {
                case ALL_ON:
                    g_en_data_tx.en_jpg = ENABLE;
                    g_en_data_tx.en_meta = ENABLE;
                    g_en_data_tx.en_voice = ENABLE;
                    break;

                case START_SEND_JPG:
                    g_en_data_tx.en_jpg = ENABLE;
                    break;

                case START_SEND_META:
                    g_en_data_tx.en_meta = ENABLE;
                    break;

                case START_SEND_VOICE:
                    g_en_data_tx.en_voice = ENABLE;
                    break;

                case START_SEND_PDM:
                    g_en_data_tx.en_pdm = ENABLE;
                    break;

                case START_SEND_ALGO:
                    g_en_data_tx.en_algo = ENABLE;
                    break;

                case ALL_OFF:
                    g_en_data_tx.en_jpg = DISABLE;
                    g_en_data_tx.en_meta = DISABLE;
                    g_en_data_tx.en_voice = DISABLE;
                    g_en_data_tx.en_pdm = DISABLE;
                    break;

                case STOP_SEND_JPG:
                    g_en_data_tx.en_jpg = DISABLE;
                    break;

                case STOP_SEND_META:
                    g_en_data_tx.en_meta = DISABLE;
                    break;

                case STOP_SEND_VOICE:
                    g_en_data_tx.en_voice = DISABLE;
                    break;

                case STOP_SEND_PDM:
                    g_en_data_tx.en_pdm = DISABLE;
                    break;

                case STOP_SEND_ALGO:
                    g_en_data_tx.en_algo = DISABLE;
                    break;

                default:
                    EMBARC_PRINTF("Wrong command data\n");
                    break;
            }
            break;
        case TYPE_CFG:
            EMBARC_PRINTF("TYPE_CFG\n");
            cfg_cmd = rx_buf[3];
            cfg_sub_cmd = rx_buf[4];
            command_data = rx_buf[6] | (rx_buf[5]<<8);
            EMBARC_PRINTF("cfg_cmd = 0x%x, cfg_sug_cmd = 0x%x, command_data = 0x%x\n"
                                    , cfg_cmd, cfg_sub_cmd, command_data);
            switch(cfg_cmd)
            {
                case SET_CFG:
                    set_cfg_sub_cmd(cfg_sub_cmd, command_data);
                    break;
                case GET_CFG:
                    get_cfg_sub_cmd(cfg_sub_cmd, true);
                    break;
			#ifdef CDM_ENABLE
				case SET_CDM:
					g_cdm_tolerance = command_data;
					EMBARC_PRINTF(RED "g_cdm_tolerance = %d\n" NONE, g_cdm_tolerance);
					break;	
			#endif
                default:
                    EMBARC_PRINTF("Wrong cfg command\n");
                    break;
            }
            break;

        case TYPE_DBG:
            EMBARC_PRINTF("TYPE_DBG\n");
            dbg_mode = rx_buf[3];
            command_data = rx_buf[6] | (rx_buf[5]<<8) | (rx_buf[4]<<16);
            EMBARC_PRINTF("dbg_mode = 0x%x, command_data = 0x%x\n"
                                    , dbg_mode, command_data);
            set_debug_mode_cmd(dbg_mode, command_data);
            break;

         default:
            EMBARC_PRINTF("Wrong Command TYPE\n");
            memset(rx_buf,0xFF,buf_size);
            return -1;
            break;
    }

    EMBARC_PRINTF("BASIC CMD stateflag = 0x%x\n", g_en_data_tx.stateflag);

    memset(rx_buf,0xFF,buf_size);

    return 0;
#else
    if((rx_buf[0] == 0xC0 && rx_buf[1] == 0x5A))
    {
    	goto read_command;
    }
	else
	{
		if((rx_buf[0] == 0xA5 && rx_buf[1] == 0xFA))
    		goto read_command;
		else
        return -1;

		return -1;
	}
// remove debug message, debug message will cause SPI_SLV event lost
//    for(i = 0; i < buf_size; i++)
//    {
//        dbg_printf(DBG_LESS_INFO, "rx_buf[%d]:0x%x \n", i, rx_buf[i]);
//    }
read_command:

	if((rx_buf[0] == 0xA5 && rx_buf[1] == 0xFA))
	{
		//***************************smart fan command********************************
		//****************************************************************************
		//  Sync	 Reserve1	Command Data   Data 	Reserve2	Checksum	End
		//buf[0].[1]  buf[2]       buf[3]     buf[4]     buf[5]      buf[6]    buf[7]
		//****************************************************************************
		READ_CMD_DATA_TYPE_E command_type = rx_buf[3];
		switch(command_type)
    	{
			case TYPE_FAN:
				EMBARC_PRINTF("TYPE_FAN\n");
				command_data = rx_buf[4];
				switch(command_data)
	            {
	            	case LITTLE_RED:
						EMBARC_PRINTF("LITTLE_RED\n");
						break;
	            	case OPEN_FAN:
						EMBARC_PRINTF("OPEN_FAN\n");
						break;
					case CLOSE_FAN:
						EMBARC_PRINTF("CLOSE_FAN\n");
						break;
					default:
						EMBARC_PRINTF("default\n");
						break;
				}
				break;

			default:
			   EMBARC_PRINTF("Wrong Command TYPE\n");
			   memset(rx_buf,0xFF,buf_size);
			   return -1;
			   break;
		}
	}
	else if((rx_buf[0] == 0xC0&& rx_buf[1] == 0x5A))
	{
    READ_CMD_DATA_TYPE_E command_type = rx_buf[2];
    switch(command_type)
    {
        #ifdef OMRON_FACE_RECOGNITION
        case TYPE_FR:
            EMBARC_PRINTF("TYPE_FR\n");
            command_data = rx_buf[6]+(rx_buf[5]<<8)+(rx_buf[4]<<16)+(rx_buf[3]<<24);
            switch(command_data)
            {
                case FR_REG:
                    g_FR_register_flag = true;
                    break;
                case FR_UNREG:
                    app_algo_albumunRegister();
                    break;
                default:
                    break;
            }
            break;
        #endif
        case TYPE_BASIC:
            EMBARC_PRINTF("TYPE_BASIC\n");
            command_data = rx_buf[6]+(rx_buf[5]<<8)+(rx_buf[4]<<16)+(rx_buf[3]<<24);
            switch(command_data)
            {
                case ALL_ON:
                    g_en_data_tx.en_jpg = ENABLE;
                    g_en_data_tx.en_meta = ENABLE;
                    g_en_data_tx.en_voice = ENABLE;
                    break;

                case START_SEND_JPG:
                    g_en_data_tx.en_jpg = ENABLE;
                    break;

                case START_SEND_META:
                    g_en_data_tx.en_meta = ENABLE;
                    break;

                case START_SEND_VOICE:
                    g_en_data_tx.en_voice = ENABLE;
                    break;

                case ALL_OFF:
                    g_en_data_tx.en_jpg = DISABLE;
                    g_en_data_tx.en_meta = DISABLE;
                    g_en_data_tx.en_voice = DISABLE;
                    break;

                case STOP_SEND_JPG:
                    g_en_data_tx.en_jpg = DISABLE;
                    break;

                case STOP_SEND_META:
                    g_en_data_tx.en_meta = DISABLE;
                    break;

                case STOP_SEND_VOICE:
                    g_en_data_tx.en_voice = DISABLE;
                    break;

                default:
                    EMBARC_PRINTF("Wrong command data\n");
                    break;
            }
            break;
        case TYPE_CFG:
            EMBARC_PRINTF("TYPE_CFG\n");
            cfg_cmd = rx_buf[3];
            cfg_sub_cmd = rx_buf[4];
            command_data = rx_buf[6] | (rx_buf[5]<<8);
            EMBARC_PRINTF("cfg_cmd = 0x%x, cfg_sug_cmd = 0x%x, command_data = 0x%x\n"
                                    , cfg_cmd, cfg_sub_cmd, command_data);
            switch(cfg_cmd)
            {
                case SET_CFG:
                    set_cfg_sub_cmd(cfg_sub_cmd, command_data);
                    break;
                case GET_CFG:
                    get_cfg_sub_cmd(cfg_sub_cmd, true);
                    break;
                default:
                    EMBARC_PRINTF("Wrong cfg command\n");
                    break;
            }
            break;

        case TYPE_DBG:
            EMBARC_PRINTF("TYPE_DBG\n");
            dbg_mode = rx_buf[3];
            command_data = rx_buf[6] | (rx_buf[5]<<8) | (rx_buf[4]<<16);
            EMBARC_PRINTF("dbg_mode = 0x%x, command_data = 0x%x\n"
                                    , dbg_mode, command_data);
            set_debug_mode_cmd(dbg_mode, command_data);
            break;

         default:
            EMBARC_PRINTF("Wrong Command TYPE\n");
            memset(rx_buf,0xFF,buf_size);
            return -1;
            break;
    }
	}
    EMBARC_PRINTF("BASIC CMD stateflag = 0x%x\n", g_en_data_tx.stateflag);

    memset(rx_buf,0xFF,buf_size);

    return 0;

#endif
}


EN_DATA_TX_CMD_T get_cmd_flag()
{
    return g_en_data_tx;
}

void set_cmd_flag(EN_DATA_TX_CMD_T en_data_tx)
{
    g_en_data_tx = en_data_tx;
}

uint32_t set_cfg_sub_cmd(READ_CFG_SUB_CMD_E cfg_sub_cmd, uint16_t data)
{
    switch(cfg_sub_cmd)
    {
        case SET_HUMN_LV_TIMEOUT:
            g_sub_cmd_cfg.humn_lv_timeout = data;
            EMBARC_PRINTF("humn_lv_timeout val = %d sec\n", g_sub_cmd_cfg.humn_lv_timeout);
            break;

        case SET_WAKE_UP_METHOD:
            g_sub_cmd_cfg.wakeup_method = data;
            EMBARC_PRINTF("wakeup_method val = %s(%d)\n", wkup_str[g_sub_cmd_cfg.wakeup_method]
                                                 ,g_sub_cmd_cfg.wakeup_method);
            break;
        case SET_LANGUAGE:
            g_sub_cmd_cfg.language = data;
            EMBARC_PRINTF("language val = %s(%d)\n", wkup_str[g_sub_cmd_cfg.language]
                                                 ,g_sub_cmd_cfg.language);
            break;
#ifdef TV01
        case SET_HUMAN_COUNT_LOCK:
            g_sub_cmd_cfg.human_lock_count = data;
            EMBARC_PRINTF("human_count val = %d people\n", g_sub_cmd_cfg.human_lock_count);
            break;
        case SET_EYE_PROTECT_TIMEOUT:
            g_sub_cmd_cfg.eye_protect_timeout= data;
            EMBARC_PRINTF("eye_protect_timeout val = %d sec\n", g_sub_cmd_cfg.eye_protect_timeout);
            break;
#endif
        default:
            EMBARC_PRINTF("Set sub command wrong\n");
            break;
    }

    return 0;
}

CFG_SUB_CMD_DATA_T get_cfg_sub_cmd(READ_CFG_SUB_CMD_E cfg_sub_cmd, bool enable_io)
{
    uint8_t read_status = 0xFF;
    if (!enable_io)
        return g_sub_cmd_cfg;

    switch(cfg_sub_cmd)
    {
        case GET_HUMN_LV_TIMEOUT:
            read_status = transmit_func((uint32_t)&g_sub_cmd_cfg.humn_lv_timeout,
                                        sizeof(g_sub_cmd_cfg.humn_lv_timeout),
                                        DATA_TYPE_HUMN_LV_TIMEOUT);
            EMBARC_PRINTF("send humn_lv_timeout val = %d sec; read_status = %d\n",
                            g_sub_cmd_cfg.humn_lv_timeout, read_status);
            break;

        case GET_WAKE_UP_METHOD:
            read_status = transmit_func((uint32_t)&g_sub_cmd_cfg.wakeup_method,
                                        sizeof(g_sub_cmd_cfg.wakeup_method),
                                        DATA_TYPE_WKUP_METHOD);
            EMBARC_PRINTF("send wakeup_method val = %d; read_status = %d\n",
                            g_sub_cmd_cfg.wakeup_method, read_status);
            break;

        case GET_LANGUAGE:
            read_status = transmit_func((uint32_t)&g_sub_cmd_cfg.language,
                                         sizeof(g_sub_cmd_cfg.language),
                                         DATA_TYPE_LANGUAGE);
            EMBARC_PRINTF("send language val = %d; read_status = %d\n",
                            g_sub_cmd_cfg.language, read_status);
            break;

#ifdef TV01
        case GET_HUMAN_COUNT_LOCK:
            read_status = transmit_func((uint32_t)&g_sub_cmd_cfg.human_lock_count,
                                         sizeof(g_sub_cmd_cfg.human_lock_count),
                                         DATA_TYPE_HUNN_LOCK_COUNT);
            EMBARC_PRINTF("send human_lock_count val = %d; read_status = %d\n",
                            g_sub_cmd_cfg.human_lock_count, read_status);
            break;

        case GET_EYE_PROTECT_TIMEOUT:
            read_status = transmit_func((uint32_t)&g_sub_cmd_cfg.eye_protect_timeout,
                                         sizeof(g_sub_cmd_cfg.eye_protect_timeout),
                                         DATA_TYPE_EYE_PROT_TIMEOUT);
            EMBARC_PRINTF("send eye_protect_timeout val = %d; read_status = %d\n",
                            g_sub_cmd_cfg.eye_protect_timeout, read_status);
            break;
#endif
        default:
            EMBARC_PRINTF("Get sub command wrong\n");
            break;
    }

    return g_sub_cmd_cfg;
}

int spi_protocol_write(uint32_t addr, uint32_t size, SPI_CMD_DATA_TYPE data_type)
{
#ifndef SPI_MASTER_SEND
    return hx_drv_spi_slv_protocol_write_simple_ex(addr, size, data_type);
#else
    return hx_drv_spi_mst_protocol_write_sp(addr, size, data_type);
#endif
}

uint8_t transmit_func(uint32_t addr, uint32_t size, SPI_CMD_DATA_TYPE data_type)
{
    int read_status = 0xFF;

    dbg_printf(DBG_LESS_INFO,"transmit_func data_type = %d\n",data_type);

#ifndef I2C_PROTOCOL
    hx_drv_iomux_set_outvalue(GPIO_DATA_TX_PIN, 1);
#endif
#ifdef UART_PROTOCOL
    read_status = app_uart_send(addr, size, data_type);
#elif defined I2C_PROTOCOL
    read_status = i2ccomm_cmd_send(addr, size, data_type);
#else
    read_status = hx_drv_spi_slv_protocol_write_simple_ex(addr, size, data_type);
#ifdef EVT_SPISCOMM
    evt_spiscomm_cmd_read();
#endif
#endif

    dbg_printf(DBG_LESS_INFO,"write result %d\n",read_status);

    if((data_type >= DATA_TYPE_HUNN_PRESENT &&
       data_type < DATA_TYPE_VOICE_TIMEOUT) || data_type == DATA_TYPE_TV_STANDBY){
        //EMBARC_PRINTF("board_delay_ms(80)\n");
        board_delay_ms(80);
    }

#ifndef I2C_PROTOCOL
    hx_drv_iomux_set_outvalue(GPIO_DATA_TX_PIN, 0);
#endif

    return read_status;
}


#ifdef DUT
void set_debug_mode_cmd(DGB_MODE_E dbg_mode, uint32_t data)
{
    switch(dbg_mode)
    {
        case SET_WE1_INPUT_TYPE:
            g_dbg_mode_data.sel_img_input = data;

            if(data != 0){
                EMBARC_PRINTF("Change mode to Input Raw Image mode\n");
            }
            else{
                EMBARC_PRINTF("Change mode to CIS mode\n");
                if(g_en_data_tx.en_raw != ENABLE)
                    trigger_wdma23_mannually();
            }
            g_en_data_tx.en_raw = DISABLE;
            dbg_printf(DBG_LESS_INFO,"g_dbg_mode_data.sel_img_input = 0x%x\n", g_dbg_mode_data.sel_img_input);
            break;

        case RECEIVE_IMG:
            g_dbg_mode_data.img_size = data;
            g_dbg_mode_data.start_flag = ENABLE;
            dbg_printf(DBG_LESS_INFO,"g_dbg_mode_data.img_size = %d\n", g_dbg_mode_data.img_size);
            we1_receive_img((uint8_t *)g_wdma3_baseaddr, g_dbg_mode_data.img_size);
            break;

        case SET_AUTO_RUN_TIMES:
            g_dbg_mode_data.auto_run_max = data;
            break;

        /*case START_SEND_IMG_RAW:
            //we1_transmit_img((uint8_t *)g_wdma3_baseaddr, g_dbg_mode_data.img_size);
            EMBARC_PRINTF("g_en_data_tx.en_raw = %d\n", g_en_data_tx.en_raw);
            break;*/
        case START_SEND_IMG_RAW:
            g_en_data_tx.en_raw = ENABLE;
            EMBARC_PRINTF("g_en_data_tx.en_raw = %d\n", g_en_data_tx.en_raw);
            break;

        case STOP_SEND_IMG_RAW:
            g_en_data_tx.en_raw = DISABLE;
            break;
        default:
            dbg_printf(DBG_LESS_INFO,"Debug Mode command wrong\n");
            break;
    }
}

DBG_MODE_DATA_T get_debug_mode_cmd()
{
    return g_dbg_mode_data;
}

CAL_CRC_E cal_validate_checksum(uint8_t *imgbuf, uint32_t length)
{
    uint16_t crc_cal = 0 ,checksum = 0;

    crc_cal = crc16_ccitt((char *)imgbuf, length-2);
    checksum = ((imgbuf[length-2] << 8) | imgbuf[length-1]);

    if(crc_cal == checksum){
        EMBARC_PRINTF("Same Image. crc = 0x%x(0x%x)\n", crc_cal, checksum);
        return CAL_CRC_SUCCESS;
    }else{
        EMBARC_PRINTF("Check Sum dosen't match. crc = 0x%x,(0x%x)\n", crc_cal, checksum);

        return CAL_CRC_ERROR;
    }
}

void we1_receive_img(uint8_t *imgbuf, uint32_t length)
{
	uint32_t i=0;
    dbg_printf(DBG_LESS_INFO,"imgbuf addr = 0x%x\n", imgbuf);
    //Start to recevie image (ideal case) - SPI salve blocking read
#ifdef UART_PROTOCOL
    dbg_printf(DBG_LESS_INFO,"UART 1 Receive command - under implement\n");
    //simulation only - uart0 receive
	/*for(i=0; i<length; i++)
	{
		*(imgbuf+i)=console_getchar(); // for we1 code, HV-2 todo.
	}*/
#else
    set_spiscomm_evt_cb(PACKET_DATA);
    //hx_drv_spi_slv_protocol_read_simple_ex((uint32_t)imgbuf, &length);
#endif

    dbg_printf(DBG_LESS_INFO,"we1_receive_img done\n");
}

void we1_transmit_img(uint8_t *imgbuf, uint32_t length)
{
    uint32_t read_status = 0xFF;
    uint16_t crc_cal = 0;

    dbg_printf(DBG_LESS_INFO,"start we1_transmit_img\n");
    dbg_printf(DBG_LESS_INFO,"imgbuf addr = 0x%x, length = %d\n", imgbuf, length);

    crc_cal = crc16_ccitt((char *)imgbuf, length-2);
    imgbuf[length-2] = (crc_cal >> 8) & 0xFF;
    imgbuf[length-1] = crc_cal & 0xFF;
    dbg_printf(DBG_LESS_INFO,"crc = 0x%x%x\n", imgbuf[length-2], imgbuf[length-1]);

    read_status = hx_drv_spi_slv_protocol_write_simple_ex((uint32_t)imgbuf, length, DATA_TYPE_RAW_IMG);
    dbg_printf(DBG_LESS_INFO,"1.write frame result %d\n",read_status);
    dbg_printf(DBG_LESS_INFO,"we1_transmit_img done\n");
}

void set_spiscomm_evt_cb(uint8_t sel)
{
    switch(sel){
        case PACKET_CMD:
            spis_cb_type = PACKET_CMD;
            dbg_printf(DBG_LESS_INFO,"set_spiscomm_evt_cb evt_spiscomm_rx_cb_swtich_cb\n");
            infra_evt_set_callback(g_event[EVT_INDEX_SPIS_RX], evt_spiscomm_rx_cb_swtich_cb);
            infra_evt_set_priority(g_event[EVT_INDEX_SPIS_RX], 0);  //EVT_PRIORITY_HIGHEST
            break;
        case PACKET_DATA:
            spis_cb_type = PACKET_DATA;
            dbg_printf(DBG_LESS_INFO,"set_spiscomm_evt_cb evt_spiscomm_rx_packet_cb\n");
            infra_evt_set_callback(g_event[EVT_INDEX_SPIS_RX], evt_spiscomm_rx_packet_cb);
            infra_evt_set_priority(g_event[EVT_INDEX_SPIS_RX], 0);   //EVT_PRIORITY_HIGHEST
            break;
    }
}

SPISCOMM_CB_TYPE_E get_spiscomm_evt_cb_type()
{
    return spis_cb_type;
}
#else
void set_debug_mode_cmd(DGB_MODE_E dbg_mode, uint32_t data)
{
    switch(dbg_mode)
    {
        case SET_WE1_INPUT_TYPE:
            break;

        case RECEIVE_IMG:
            break;

        case SET_AUTO_RUN_TIMES:
            break;

        case START_SEND_IMG_RAW:
            g_en_data_tx.en_raw = ENABLE;
            break;

        case STOP_SEND_IMG_RAW:
            g_en_data_tx.en_raw = DISABLE;
            break;

        case START_FOCUSING:
            g_focusing_en = true;
            g_wait_pc = false;
            break;
            
        case STOP_FOCUSING:
            g_focusing_en = false;
            g_wait_pc = false;
            break;
            
        case START_ALWAYS_SEND:
            g_send_every_frame_en = true;
            g_wait_pc = false;
            break;
            
        case STOP_ALWAYS_SEND:
            g_send_every_frame_en = false;
            g_wait_pc = false;
            break;

        default:
            dbg_printf(DBG_LESS_INFO,"Debug Mode command wrong\n");
            break;
    }
}
#endif

