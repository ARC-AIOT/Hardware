/*
 * aiot_bodydetect.h
 *
 *  Created on: 2019¦~6¤ë16¤é
 *      Author: 902447
 */

#ifndef SCENARIO_APP_AIOT_FACEDETECT_H_
#define SCENARIO_APP_AIOT_FACEDETECT_H_
#include <app_aiot_hw5x5jpeg_algo.h>
#include <app_macro_cfg.h>
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
#include "io_config_ext.h"
#include "sensor_dp_lib.h"
#include "hx_drv_CIS_common.h"
#include "apexextensions.h"
#include "hx_drv_spi_m.h"
#include "hx_drv_pmu.h"
#include "powermode.h"
#include "spi_master_protocol.h"
#include <iot_custom_config_crc_chksum.h>
#include <app_rx_config.h>
#include <iot_custom_config.h>
#include <algo_custom_config.h>
#include <iot_custom_defconfig.h>
#include <bodydetect_meta.h>
#include "event.h"
#include "event_handler.h"
#include "event_handler_evt.h"
#include "evt_datapath.h"
#include <app_state.h>

/**
 * \defgroup	DEMO_SCENARIO_HUMAN_APP	Demo Scenario HUMAN Application
 * \ingroup		DEMO_SCENARIO_HUMAN_APP
 * \brief	Demo Scenario HUMAN Application Declaration
 */

/**
 * \defgroup	DEMO_SCENARIO_HUMAN_APP_FUNCDLR	Demo Scenario HUMAN Function Declaration
 * \ingroup	DEMO_SCENARIO_HUMAN_APP
 * \brief	Contains declarations of Demo Scenario Human functions.
 * @{
 */


/**
 * \brief	AIOT Body Detect main
 *
 * \return	void.
 */
void app_aiot_bodydetect_google();



/**
 * \brief	Get Current Application State
 *
 *
 * \return	void.
 */
void app_get_cur_state(APP_STATE_E *app_state);


int app_setup_change_state(APP_STATE_E next_app_state);

#endif /* SCENARIO_APP_AIOT_FACEDETECT_H_ */
