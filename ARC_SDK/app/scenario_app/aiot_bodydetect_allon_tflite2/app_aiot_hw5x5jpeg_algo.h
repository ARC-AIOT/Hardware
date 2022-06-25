/*
 * demo_scenario_humanPIR.h
 *
 *  Created on: 2019¦~8¤ë5¤é
 *      Author: 902447
 */

#ifndef SCENARIO_APP_AIOT_FACEDETECT_HW5X5JPEG_FACE_H_
#define SCENARIO_APP_AIOT_FACEDETECT_HW5X5JPEG_FACE_H_
#include <app_macro_cfg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "embARC.h"
#include "embARC_debug.h"
#include "board_config.h"
#include "io_config_ext.h"
#include "hx_drv_iomux.h"
#include "arc_timer.h"
#include "hx_drv_spi_s.h"
#include "spi_slave_protocol.h"
#include "spi_master_protocol.h"
#include "hx_drv_iomux.h"
#include "sensor_dp_lib.h"
#include "hx_drv_pmu.h"
#include "powermode.h"
#include "hx_drv_CIS_common.h"
#include <app_rx_config.h>
#include "event.h"
#include "event_handler.h"
#include "event_handler_evt.h"
#include "evt_datapath.h"
#include <app_state.h>
#include <bodydetect_meta.h>

uint8_t app_aiot_face_hw5x5jpeg_algo_check(uint32_t img_addr);
void app_aiot_face_hw5x5jpeg_eventhdl_cb(EVT_INDEX_E event);;
void app_aiot_face_setup_hw5x5jpeg_path(uint32_t image_addr);
void app_start_cv();
void app_stop_cv();
void app_init_dpcbflag();
/**
 * \brief	system reset
 *
 * \return	void.
 */
void app_iot_facedetect_systemreset();

#endif /* SCENARIO_APP_AIOT_FACEDETECT_HW5X5JPEG_FACE_H_ */
