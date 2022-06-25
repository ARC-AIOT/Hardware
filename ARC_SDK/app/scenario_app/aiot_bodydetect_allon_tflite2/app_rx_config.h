/*
 * app_rx_config.h
 *
 *  Created on: 2019¦~8¤ë3¤é
 *      Author: 902447
 */

#ifndef SCENARIO_APP_APP_RX_CONFIG_H_
#define SCENARIO_APP_APP_RX_CONFIG_H_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "embARC.h"
#include "embARC_debug.h"
#include "board_config.h"
#include "arc_timer.h"
#include "hx_drv_spi_s.h"
#include "spi_slave_protocol.h"
#include "hx_drv_iomux.h"
#include "sensor_dp_lib.h"
#include "hx_drv_CIS_common.h"
#include <iot_custom_config.h>
#include <algo_custom_config.h>
#include <app_macro_cfg.h>
#include "app_cis_sensor_defcfg.h"
/**
 * \brief	configure WE-1 Sensor Control and INP related setting and Sensor Configuration
 *
 * \return	int. (-1: failure, 0: success)
 */
int app_config_sensor_WE1_rx(uint8_t sensor_init_required, uint8_t sensor_strobe_req);

/**
 * \brief	configure Sensor all setting without streaming
 *
 * \return	int. (-1: failure, 0: success)
 */
int app_config_sensor_all();

/**
 * \brief	configure Sensor to Standby in application
 *
 * \return	int. (-1: failure, 0: success)
 */
int app_sensor_standby();

/**
 * \brief	configure Sensor to Standby in application
 *
 * \return	int. (-1: failure, 0: success)
 */
int app_sensor_streaming();

/**
 * \brief	Sensor strobe on
 *
 * \return	int. (-1: failure, 0: success)
 */
int app_sensor_strobe_on();

/**
 * \brief	Sensor strobe off
 *
 * \return	int. (-1: failure, 0: success)
 */
int app_sensor_strobe_off();


void app_copy_sensor_default_cfg();

/*
 * sensor ID: 	SENSORDPLIB_SENSOR_HM0360_MODE1
 *				SENSORDPLIB_SENSOR_HM0360_MODE2
 *				SENSORDPLIB_SENSOR_HM0360_MODE3
 *				SENSORDPLIB_SENSOR_HM0360_MODE5
 *				SENSORDPLIB_SENSOR_HM11B1_LSB
 *				SENSORDPLIB_SENSOR_HM11B1_MSB
 * */
void app_sensor_xsleep_ctrl(SENSORCTRL_XSLEEP_CTRL_E xsleep_ctrl, uint8_t val);

void app_rx_set_wlcsp38_sharepin();

void app_sensor_xshutdown_toggle();

/**
 * \brief	configure WE-1 Sensor Control and INP related setting and Sensor MD Configuration
 *
 * \return	int. (-1: failure, 0: success)
 */
int app_config_sensor_md_WE1_rx(uint8_t sensor_init_required, uint8_t sensor_strobe_req);

/**
 * \brief	configure WE-1 Sensor Control and INP related setting and Sensor MD Configuration
 *
 * \return	int. (-1: failure, 0: success)
 */
int app_config_sensor_mddiffres_WE1_rx(uint8_t sensor_init_required, uint8_t sensor_strobe_req);


int app_get_sensor_id(uint16_t *sensor_id, uint8_t *rev_id);

#endif /* SCENARIO_APP_DEMO_SCENARIO_HUMAN_APP_RX_CONFIG_H_ */
