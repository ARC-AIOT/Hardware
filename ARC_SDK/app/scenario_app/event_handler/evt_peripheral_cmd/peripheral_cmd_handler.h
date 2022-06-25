/*
 * peripheral_cmd_handler.h
 *
 *  Created on: 2020¦~4¤ë9¤é
 *      Author: 903935
 */

#ifndef APP_SCENARIO_APP_AIOT_HUMANDETECT_PERIPHERAL_CMD_HANDLER_H_
#define APP_SCENARIO_APP_AIOT_HUMANDETECT_PERIPHERAL_CMD_HANDLER_H_

#include <stdint.h>
#include <stdbool.h>
#include "spi_slave_protocol.h"

#ifdef TV01
#define GPIO_WAKEUP_PIN  5
#define GPIO_DATA_TX_PIN 4
#elif defined (TV02) || defined(TV04)
#define GPIO_WAKEUP_PIN  4
#define GPIO_DATA_TX_PIN 4
#elif defined TV03
#define GPIO_DATA_TX_PIN 4
#define GPIO_WAKEUP_PIN  5
#else
#define GPIO_DATA_TX_PIN 4
#define GPIO_WAKEUP_PIN  5
#endif

#define EN_ALL   0xFF
#define DIS_ALL  0x00

#define ENABLE   1
#define DISABLE  0

/*typedef enum
{
    JPG         = 0x01,
    BMP         = 0x02,
    VO_CMD      = 0x03,
    NO_HUMAN    = 0x05,
    DBG_MSG     = 0x10,
    WARN_MSG    = 0x11,
    CTRL_MSG    = 0x12,
    META_DATA   = 0x13,
    JPG_L       = 0x14,
    JPG_R       = 0x15,
    BIN_DATA    = 0x20,
    INCORRECT_DATA = 0x3F,
}CMD_DATA_TYPE_E;*/


typedef enum
{
    TYPE_FR          = 0x01,   //Face Register/Unregister
    TYPE_BASIC       = 0x02,   //Start/Stop send JPG, META_DATA, VOICE
    TYPE_CFG         = 0x03,   //Set/Get config
    TYPE_DBG         = 0x04,
    TYPE_FAN		 = 0x81,
}READ_CMD_DATA_TYPE_E;

//command of TYPE_FR
typedef enum
{
    FR_REG         = 0x01,
    FR_UNREG
}READ_FR_CMD_E;

//command of TYPE_BASIC
typedef enum
{
    ALL_ON            = 0x01,
    START_SEND_JPG    = 0x11,
    START_SEND_META   = 0x21,
    START_SEND_VOICE  = 0x31,
    START_SEND_PDM    = 0x41,
    START_SEND_ALGO   = 0x51,

    ALL_OFF           = 0x00,
    STOP_SEND_JPG     = 0x10,
    STOP_SEND_META    = 0x20,
    STOP_SEND_VOICE   = 0x30,
    STOP_SEND_PDM     = 0x40,
    STOP_SEND_ALGO    = 0x50,
}READ_BASIC_CMD_E;

//command of TYPE_CFG
typedef enum
{
    SET_CFG   = 0x00,
    GET_CFG,
    SET_CDM
}READ_CFG_CMD_E;

//sub command of TYPE_CFG
typedef enum
{
    SET_HUMN_LV_TIMEOUT  = 0x00,    //WE1 Receive no human command after N sec delay
    SET_WAKE_UP_METHOD,
    GET_HUMN_LV_TIMEOUT,
    GET_WAKE_UP_METHOD,
    SET_LANGUAGE,
    GET_LANGUAGE,
#ifdef TV01
    SET_HUMAN_COUNT_LOCK,
    GET_HUMAN_COUNT_LOCK,
    SET_EYE_PROTECT_TIMEOUT,
    GET_EYE_PROTECT_TIMEOUT,
#endif
}READ_CFG_SUB_CMD_E;

//command of TYPE_DBG
typedef enum
{
    SET_WE1_INPUT_TYPE  = 0x00,
    RECEIVE_IMG         = 0x01,
    SET_AUTO_RUN_TIMES  = 0x02,
    START_SEND_IMG_RAW  = 0x03,
    STOP_SEND_IMG_RAW   = 0x04,
    START_FOCUSING      = 0x05,
    STOP_FOCUSING       = 0x06,
    START_ALWAYS_SEND   = 0x07,
    STOP_ALWAYS_SEND    = 0x08,
}DGB_MODE_E;
//command of TYPE_FAN
typedef enum
{
	LITTLE_RED					= 0x00,
	OPEN_FAN,
	CLOSE_FAN,
	OPEN_SHAKING_HEAD,
	CLOSE_SHAKING_HEAD,
	INCREASE_WIND_SPEED,
	REDUCE_WIND_SPEED,
	MAX_WIND_SPEED,
	MIN_WIND_SPEED,
	SHUTDOWN_TIME,
	CANCEL_TIMING,
	SLEEP_WIND,
	NATURAL_WIND,
	NORMAL_WIND,
	INCREASE_VOLUME,
	REDUCE_VOLUME,
	CLOSE_VOICE,
	OPEN_VOICE,
	INSTRUCTION_MANUAL,
	SWITCH_MODE,
	Timed_half_hour,
	Timed_one_hour,
	Timed_two_hour,
	Timed_three_hour,
	Timed_four_hour,
	Timed_five_hour,
	Timed_six_hour,
	Timed_seven_hour,
	Timed_eight_hour,
	Open_one_gear,
	Open_two_gear,
	Open_three_gear,
	Open_four_gear,
	Open_five_gear,
	Open_six_gear,
	Open_seven_gear,
	Open_eight_gear,
	Open_nine_gear,
	Open_eleven_gear,
	Open_tweleven_gear,
}READ_FAN_CMD_E;

//wakeup method
typedef enum
{
    OTHERS_WKUP = 0x00,
    VOICE_WKUP,
    FD_WKUP
}WAKEUP_MEOTHD_E;

//input image CIS/From host
typedef enum
{
    FROM_CIS,
    FROM_HOST,
}INPUT_PATH;

//Command or Data SPIS callbacks
typedef enum
{
    PACKET_CMD,
    PACKET_DATA,
}SPISCOMM_CB_TYPE_E;


//Error for calculate checksum
typedef enum
{
    CAL_CRC_SUCCESS,
    CAL_CRC_ERROR = -1,
}CAL_CRC_E;

//enable command data transfer flag
typedef union{
    uint8_t stateflag;
    struct{
        bool en_jpg    : 1;
        bool en_meta   : 1;
        bool en_voice  : 1;
        bool en_raw    : 1;
        bool en_pdm    : 1;
        bool en_algo   : 1;
    };
}EN_DATA_TX_CMD_T;

//sub command data of TYPE_CFG
typedef struct{
    uint16_t humn_lv_timeout;
    uint16_t wakeup_method;
    uint16_t language;
#ifdef TV01
    uint16_t human_lock_count;
    uint16_t eye_protect_timeout;
#endif
}CFG_SUB_CMD_DATA_T;


//sub command data of TYPE_DBG
typedef struct{
    uint32_t sel_img_input;         // 0 : CIS, 1:From host
    uint32_t img_size;
    uint32_t start_flag;            // 1:start, 0:stop
    uint32_t auto_run_max;
}DBG_MODE_DATA_T;


void peripheral_cmd_init();

uint8_t read_cmd_handler(uint8_t *rx_buf, uint32_t buf_size);
//uint8_t write_cmd_handler(uint8_t tx_buf);

EN_DATA_TX_CMD_T get_cmd_flag();
void set_cmd_flag(EN_DATA_TX_CMD_T en_data_tx);

uint32_t set_cfg_sub_cmd(READ_CFG_SUB_CMD_E cfg_sub_cmd, uint16_t data);
CFG_SUB_CMD_DATA_T get_cfg_sub_cmd(READ_CFG_SUB_CMD_E cfg_sub_cmd, bool enable_io);

void set_debug_mode_cmd(DGB_MODE_E dbg_mode, uint32_t data);

#ifdef DUT
void set_debug_mode_cmd(DGB_MODE_E dbg_mode, uint32_t data);
DBG_MODE_DATA_T get_debug_mode_cmd();

CAL_CRC_E cal_validate_checksum(uint8_t *imgbuf, uint32_t length);
void we1_receive_img(uint8_t *imgbuf, uint32_t length);
void we1_transmit_img(uint8_t *imgbuf, uint32_t length);

void set_spiscomm_evt_cb(uint8_t sel);
SPISCOMM_CB_TYPE_E get_spiscomm_evt_cb_type();
#endif

int spi_protocol_write(uint32_t addr, uint32_t size, SPI_CMD_DATA_TYPE data_type);
uint8_t transmit_func(uint32_t addr, uint32_t size, SPI_CMD_DATA_TYPE data_type);

#endif /* APP_SCENARIO_APP_AIOT_HUMANDETECT_PERIPHERAL_CMD_HANDLER_H_ */
