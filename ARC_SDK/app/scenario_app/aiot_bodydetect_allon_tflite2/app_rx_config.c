/*
 * app_rx_config.c
 *
 *  Created on: 2019¦~8¤ë3¤é
 *      Author: 902447
 */

#include <app_rx_config.h>
#include "hx_drv_pmu.h"

extern WE1AppCfg_CustTable_t  app_cust_config;
extern SENSORDPLIB_STREAM_E g_stream_type;

extern uint32_t g_tick_start;
extern uint32_t g_tick_period;
extern uint32_t g_period;
extern uint32_t g_tick_sensor_std;
extern uint32_t g_tick_sensor_stream;
extern uint32_t g_tick_sensor_toggle;
extern IOMUX_INDEX_E g_xshutdown_pin;

/*Sensor and RX related Start*/
int app_sensor_streaming()
{
	dbg_printf(DBG_LESS_INFO,"stream on\n");
	hx_drv_sensorctrl_set_MCLKCtrl(SENSORCTRL_MCLKCTRL_NONAOS);
	app_sensor_xsleep_ctrl(SENSORCTRL_XSLEEP_BY_CPU, 1);
	// stream on
	if(hx_drv_cis_setRegTable(app_cust_config.sensor_streamon_cfg.sensor_stream_cfg, app_cust_config.sensor_streamon_cfg.active_cfg_cnt)!= HX_CIS_NO_ERROR)
	{
		dbg_printf(DBG_LESS_INFO,"stream on by app fail\n");
		return -1;
	}
	return 0;
}

int app_sensor_register_dump()
{
	hx_drv_sensorctrl_set_MCLKCtrl(SENSORCTRL_MCLKCTRL_NONAOS);
	app_sensor_xsleep_ctrl(SENSORCTRL_XSLEEP_BY_CPU, 1);
	if(hx_drv_cis_getRegTable(app_cust_config.sensor_table_cfg.sensor_cfg, app_cust_config.sensor_table_cfg.active_cfg_cnt)!= HX_CIS_NO_ERROR)
	{
		return -1;
	}
	if(hx_drv_cis_getRegTable(app_cust_config.sensor_streamon_cfg.sensor_stream_cfg, app_cust_config.sensor_streamon_cfg.active_cfg_cnt)!= HX_CIS_NO_ERROR)
	{
		return -1;
	}
	return 0;
}

int app_get_sensor_id(uint16_t *sensor_id, uint8_t *rev_id)
{
	uint16_t addr;
	uint8_t val;

	//dbg_printf(DBG_LESS_INFO,"app_get_sensor_id\n");
	hx_drv_sensorctrl_set_MCLKCtrl(SENSORCTRL_MCLKCTRL_NONAOS);
	app_sensor_xsleep_ctrl(SENSORCTRL_XSLEEP_BY_CPU, 1);
	addr= 0x0000;
	if(hx_drv_cis_get_reg(addr, &val)!= HX_CIS_NO_ERROR)
	{
		dbg_printf(DBG_LESS_INFO,"get sensor id fail 1\n");
		return -1;
	}
	*sensor_id = val << 8;

	addr= 0x0001;
	if(hx_drv_cis_get_reg(addr, &val)!= HX_CIS_NO_ERROR)
	{
		dbg_printf(DBG_LESS_INFO,"get sensor id fail 2\n");
		return -1;
	}
	*sensor_id = *sensor_id | val;

	addr= 0x0002;
	if(hx_drv_cis_get_reg(addr, &val)!= HX_CIS_NO_ERROR)
	{
		dbg_printf(DBG_LESS_INFO,"get sensor id fail 3\n");
		return -1;
	}
	*rev_id = val;
	return 0;
}

/**
 * \brief	Sensor strobe on
 *
 * \return	int. (-1: failure, 0: success)
 */
int app_sensor_strobe_on()
{
	if ( (app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM0360_MODE1) ||
	     (app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM0360_MODE2) ||
	     (app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM0360_MODE3) ||
	     (app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM0360_MODE5) )
	{
		dbg_printf(DBG_LESS_INFO,"strobe on\n");
		hx_drv_sensorctrl_set_MCLKCtrl(SENSORCTRL_MCLKCTRL_NONAOS);
		app_sensor_xsleep_ctrl(SENSORCTRL_XSLEEP_BY_CPU, 1);
		// strobe on
		if (hx_drv_cis_setRegTable(app_cust_config.sensor_strobeon_cfg.sensor_strobe_on_cfg, app_cust_config.sensor_strobeon_cfg.active_cfg_cnt)!= HX_CIS_NO_ERROR)
		{
			dbg_printf(DBG_LESS_INFO,"strobe on by app fail\n");
			return -1;
		}
	}

	return 0;
}

/**
 * \brief	Sensor strobe off
 *
 * \return	int. (-1: failure, 0: success)
 */
int app_sensor_strobe_off()
{
	if ( (app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM0360_MODE1) ||
	     (app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM0360_MODE2) ||
	     (app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM0360_MODE3) ||
	     (app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM0360_MODE5) )
	{
		dbg_printf(DBG_LESS_INFO,"strobe off\n");
		hx_drv_sensorctrl_set_MCLKCtrl(SENSORCTRL_MCLKCTRL_NONAOS);
		app_sensor_xsleep_ctrl(SENSORCTRL_XSLEEP_BY_CPU, 1);
		// strobe on
		if (hx_drv_cis_setRegTable(app_cust_config.sensor_strobeoff_cfg.sensor_strobe_off_cfg, app_cust_config.sensor_strobeoff_cfg.active_cfg_cnt)!= HX_CIS_NO_ERROR)
		{
			dbg_printf(DBG_LESS_INFO,"strobe on by app fail\n");
			return -1;
		}
	}

	return 0;
}

/**
 * \brief	Sensor standby and store tick
 *
 * \return	int. (-1: failure, 0: success)
 */
int app_nonAOS_standby_tick()
{
	if(g_stream_type == SENSORDPLIB_STREAM_NONEAOS)
	{
#ifdef NONEAOS_TOGGLE_STREAM_STANDBY
		app_sensor_xsleep_ctrl(SENSORCTRL_XSLEEP_BY_CPU, 1);
		if(app_sensor_standby() != 0)
		{
			return -1;
		}else{
			g_tick_sensor_std = _arc_aux_read(AUX_TIMER0_CNT);
			g_tick_start = g_tick_sensor_std;
			return 0;
		}
#else
		return 0;
#endif
	}
	return 0;
}

/**
 * \brief	Non-AOS restreaming after 1 frame time
 *
 * \return	int. (-1: failure, 0: success)
 */
int app_nonAOS_restreaming()
{
	if(g_stream_type == SENSORDPLIB_STREAM_NONEAOS)
	{
	//FOR Non-AOS need streaming agaign, Toggle stream and standby should over 1 frame
#ifdef NONEAOS_TOGGLE_STREAM_STANDBY
		app_sensor_xsleep_ctrl(SENSORCTRL_XSLEEP_BY_CPU, 1);
		while(1)
		{
			g_tick_sensor_stream = _arc_aux_read(AUX_TIMER0_CNT);
			g_tick_sensor_toggle = g_tick_sensor_stream - g_tick_sensor_std;
			g_period = g_tick_sensor_toggle/BOARD_SYS_TIMER_MS_CONV;
			if(g_period > SENOSR_TOGGLE_STREAM_LIMITATION_MS)
			{
				dbg_printf(DBG_LESS_INFO,"Sensor toggle Period %d ms\r\n", g_period);
				break;
			}
		}
		if(app_sensor_streaming() !=0)
		{
			dbg_printf(DBG_LESS_INFO,"None-AOS re-streaming fail\r\n");
			return -1;
		}
		sensordplib_set_mclkctrl_xsleepctrl_bySCMode();
        #endif
	}
	return 0;
}


int app_sensor_standby()
{
	dbg_printf(DBG_LESS_INFO,"stream off by app\n");
	hx_drv_sensorctrl_set_MCLKCtrl(SENSORCTRL_MCLKCTRL_NONAOS);
	app_sensor_xsleep_ctrl(SENSORCTRL_XSLEEP_BY_CPU, 1);
	// stream off
	if(hx_drv_cis_setRegTable(app_cust_config.sensor_streamoff_cfg.sensor_off_cfg,app_cust_config.sensor_streamoff_cfg.active_cfg_cnt)!= HX_CIS_NO_ERROR)
	{
		dbg_printf(DBG_LESS_INFO,"stream off by app fail\n");
		return -1;
	}
	return 0;
}

int app_config_sensor_WE1_rx(uint8_t sensor_init_required, uint8_t sensor_strobe_req)
{
#ifdef APP_DEBUG_PRINT
	dbg_printf(DBG_LESS_INFO,"app_config_sensor_WE1_rx sensor_init_required=%d,sensor_strobe_req=%d\n",sensor_init_required,sensor_strobe_req);
#endif
	if(sensor_init_required == 1)
	{
		app_config_sensor_all();
	}

	if(sensordplib_set_sensorctrl_inp(app_cust_config.sensor_table_cfg.sensor_id
			, app_cust_config.sensor_table_cfg.sensor_stream_type
			, app_cust_config.sensor_table_cfg.sensor_width
			, app_cust_config.sensor_table_cfg.sensor_height
			, app_cust_config.we1_driver_cfg.subsample) !=0)
	{
		dbg_printf(DBG_LESS_INFO,"sensordplib_set_sensorctrl_inp fail\n");
		return -1;
	}
	g_stream_type = app_cust_config.sensor_table_cfg.sensor_stream_type;

	if((app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM11B1_LSB) || (app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM11B1_MSB))
	{
		if(app_cust_config.sensor_table_cfg.sensor_width <=324)
		{
			if(app_cust_config.we1_driver_cfg.dp_clk_mux == WE1AppCfg_DP_CLK_Mux_RC36M)
			{
				hx_drv_inp1bitparser_set_tg2utg(8);//ceil(8*36/36)=8
			}else{
				hx_drv_inp1bitparser_set_tg2utg(6);//ceil(8*24/36)=6
			}
		}else{
			if(app_cust_config.we1_driver_cfg.dp_clk_mux == WE1AppCfg_DP_CLK_Mux_RC36M)
			{
				hx_drv_inp1bitparser_set_tg2utg(5);//ceil(8*36/72)=4 smaller than 5
			}else{
				hx_drv_inp1bitparser_set_tg2utg(5);//ceil(8*24/72)=3 smaller than 5
			}
		}

	}

	if(sensor_init_required == 1)
	{
		if(sensor_strobe_req == 1)
		{
			app_sensor_strobe_on();
		}
		app_sensor_streaming();
	///	app_sensor_register_dump();
	}

	sensordplib_set_mclkctrl_xsleepctrl_bySCMode();

	hx_drv_pmu_set_ctrl(PMU_SEN_INIT, 0);
#ifdef APP_DEBUG_PRINT
	dbg_printf(DBG_LESS_INFO, "Sensor Configure Success\n");
#endif
    return 0;
}


/**
 * \brief	configure WE-1 Sensor Control and INP related setting and Sensor MD Configuration
 *
 * \return	int. (-1: failure, 0: success)
 */
int app_config_sensor_mddiffres_WE1_rx(uint8_t sensor_init_required, uint8_t sensor_strobe_req)
{
#ifdef APP_DEBUG_PRINT
	dbg_printf(DBG_LESS_INFO,"app_config_sensor_md_WE1_rx sensor_init_required=%d, sensor_strobe_req=%d\n",sensor_init_required, sensor_strobe_req);
#endif
	if(sensor_init_required == 1)
	{
		app_sensor_xsleep_ctrl(SENSORCTRL_XSLEEP_BY_CPU, 1);
		// not need set streaming mode keep in standby, it will config streaming mode in sensor_dplib
		if(hx_drv_cis_setRegTable(app_cust_config.sensor_md_table_cfg.sensor_cfg, app_cust_config.sensor_md_table_cfg.active_cfg_cnt)!= HX_CIS_NO_ERROR)
		{
			dbg_printf(DBG_LESS_INFO,"HM01B0 Config by app fail\n");
			return -1;
		}
	}

	if(sensordplib_set_sensorctrl_inp(app_cust_config.sensor_md_table_cfg.sensor_id
			, app_cust_config.sensor_md_table_cfg.sensor_stream_type
			, app_cust_config.sensor_md_table_cfg.sensor_width
			, app_cust_config.sensor_md_table_cfg.sensor_height
			, app_cust_config.we1_driver_cfg.subsample) !=0)
	{
		dbg_printf(DBG_LESS_INFO,"sensordplib_set_sensorctrl_inp fail\n");
		return -1;
	}
	g_stream_type = app_cust_config.sensor_md_table_cfg.sensor_stream_type;

	if((app_cust_config.sensor_md_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM11B1_LSB) || (app_cust_config.sensor_md_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM11B1_MSB))
	{
		if(app_cust_config.sensor_md_table_cfg.sensor_width <=324)
		{
			if(app_cust_config.we1_driver_cfg.dp_clk_mux == WE1AppCfg_DP_CLK_Mux_RC36M)
			{
				hx_drv_inp1bitparser_set_tg2utg(8);//ceil(8*36/36)=8
			}else{
				hx_drv_inp1bitparser_set_tg2utg(6);//ceil(8*24/36)=6
			}
		}else{
			if(app_cust_config.we1_driver_cfg.dp_clk_mux == WE1AppCfg_DP_CLK_Mux_RC36M)
			{
				hx_drv_inp1bitparser_set_tg2utg(5);//ceil(8*36/72)=4 smaller than 5
			}else{
				hx_drv_inp1bitparser_set_tg2utg(5);//ceil(8*24/72)=3 smaller than 5
			}
		}

	}

	if(sensor_init_required == 1)
	{
		dbg_printf(DBG_LESS_INFO,"stream on\n");
		hx_drv_sensorctrl_set_MCLKCtrl(SENSORCTRL_MCLKCTRL_NONAOS);
		app_sensor_xsleep_ctrl(SENSORCTRL_XSLEEP_BY_CPU, 1);
		if(sensor_strobe_req == 1)
		{
			app_sensor_strobe_on();
		}
		// stream on
    	app_sensor_xsleep_ctrl(1, 1);
    	hx_drv_cis_set_reg(0x3024, 0x01, 1);
    	hx_drv_cis_set_reg(0x0100, 0x03, 1);
	}

	sensordplib_set_mclkctrl_xsleepctrl_bySCMode();

	hx_drv_pmu_set_ctrl(PMU_SEN_INIT, 0);
#ifdef APP_DEBUG_PRINT
	dbg_printf(DBG_LESS_INFO, "Sensor Configure Success\n");
#endif
    return 0;
}
/**
 * \brief	configure WE-1 Sensor Control and INP related setting and Sensor MD Configuration
 *
 * \return	int. (-1: failure, 0: success)
 */
int app_config_sensor_md_WE1_rx(uint8_t sensor_init_required, uint8_t sensor_strobe_req)
{
#ifdef APP_DEBUG_PRINT
	dbg_printf(DBG_LESS_INFO,"app_config_sensor_md_WE1_rx sensor_init_required=%d, sensor_strobe_req=%d\n",sensor_init_required, sensor_strobe_req);
#endif
	if(sensor_init_required == 1)
	{
		app_sensor_xsleep_ctrl(SENSORCTRL_XSLEEP_BY_CPU, 1);
		// not need set streaming mode keep in standby, it will config streaming mode in sensor_dplib
		if(hx_drv_cis_setRegTable(app_cust_config.sensor_md_table_cfg.sensor_cfg, app_cust_config.sensor_md_table_cfg.active_cfg_cnt)!= HX_CIS_NO_ERROR)
		{
			dbg_printf(DBG_LESS_INFO,"HM01B0 Config by app fail\n");
			return -1;
		}
	}

	if(sensordplib_set_sensorctrl_inp(app_cust_config.sensor_md_table_cfg.sensor_id
			, app_cust_config.sensor_md_table_cfg.sensor_stream_type
			, app_cust_config.sensor_md_table_cfg.sensor_width
			, app_cust_config.sensor_md_table_cfg.sensor_height
			, app_cust_config.we1_driver_cfg.subsample) !=0)
	{
		dbg_printf(DBG_LESS_INFO,"sensordplib_set_sensorctrl_inp fail\n");
		return -1;
	}
	g_stream_type = app_cust_config.sensor_md_table_cfg.sensor_stream_type;

	if((app_cust_config.sensor_md_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM11B1_LSB) || (app_cust_config.sensor_md_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM11B1_MSB))
	{
		if(app_cust_config.sensor_md_table_cfg.sensor_width <=324)
		{
			if(app_cust_config.we1_driver_cfg.dp_clk_mux == WE1AppCfg_DP_CLK_Mux_RC36M)
			{
				hx_drv_inp1bitparser_set_tg2utg(8);//ceil(8*36/36)=8
			}else{
				hx_drv_inp1bitparser_set_tg2utg(6);//ceil(8*24/36)=6
			}
		}else{
			if(app_cust_config.we1_driver_cfg.dp_clk_mux == WE1AppCfg_DP_CLK_Mux_RC36M)
			{
				hx_drv_inp1bitparser_set_tg2utg(5);//ceil(8*36/72)=4 smaller than 5
			}else{
				hx_drv_inp1bitparser_set_tg2utg(5);//ceil(8*24/72)=3 smaller than 5
			}
		}

	}

	if(sensor_init_required == 1)
	{
		dbg_printf(DBG_LESS_INFO,"stream on\n");
		hx_drv_sensorctrl_set_MCLKCtrl(SENSORCTRL_MCLKCTRL_NONAOS);
		app_sensor_xsleep_ctrl(SENSORCTRL_XSLEEP_BY_CPU, 1);
		if(sensor_strobe_req == 1)
		{
			app_sensor_strobe_on();
		}
		// stream on
		if(hx_drv_cis_setRegTable(app_cust_config.sensor_md_streamon_cfg.sensor_stream_cfg, app_cust_config.sensor_md_streamon_cfg.active_cfg_cnt)!= HX_CIS_NO_ERROR)
		{
			dbg_printf(DBG_LESS_INFO,"stream on by app fail\n");
			return -1;
		}
	}

	sensordplib_set_mclkctrl_xsleepctrl_bySCMode();

	hx_drv_pmu_set_ctrl(PMU_SEN_INIT, 0);
#ifdef APP_DEBUG_PRINT
	dbg_printf(DBG_LESS_INFO, "Sensor Configure Success\n");
#endif
    return 0;
}

/**
 * \brief	configure Sensor all setting without streaming
 *
 * \return	int. (-1: failure, 0: success)
 */
int app_config_sensor_all()
{
	app_sensor_xsleep_ctrl(SENSORCTRL_XSLEEP_BY_CPU, 1);
	// not need set streaming mode keep in standby, it will config streaming mode in sensor_dplib
	if(hx_drv_cis_setRegTable(app_cust_config.sensor_table_cfg.sensor_cfg, app_cust_config.sensor_table_cfg.active_cfg_cnt)!= HX_CIS_NO_ERROR)
	{
		dbg_printf(DBG_LESS_INFO,"HM01B0 Config by app fail\n");
		return -1;
	}

	return 0;
}

/*
 * sensor ID: 	SENSORDPLIB_SENSOR_HM0360_MODE1
 *				SENSORDPLIB_SENSOR_HM0360_MODE2
 *				SENSORDPLIB_SENSOR_HM0360_MODE3
 *				SENSORDPLIB_SENSOR_HM0360_MODE5
 *				SENSORDPLIB_SENSOR_HM11B1_LSB
 *				SENSORDPLIB_SENSOR_HM11B1_MSB
 * */
void app_sensor_xsleep_ctrl(SENSORCTRL_XSLEEP_CTRL_E xsleep_ctrl, uint8_t val)
{
	if((app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM0360_MODE1) ||
			(app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM0360_MODE2)	||
			(app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM0360_MODE3) ||
			(app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM0360_MODE5) ||
			(app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM11B1_LSB) ||
			(app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM11B1_MSB))
	{
		if(xsleep_ctrl == SENSORCTRL_XSLEEP_BY_SC)
		{
			hx_drv_sensorctrl_set_xSleepCtrl(xsleep_ctrl);
		}else{
			hx_drv_sensorctrl_set_xSleepCtrl(xsleep_ctrl);
			hx_drv_sensorctrl_set_xSleep(val);
		}
	}
}

void app_rx_set_wlcsp38_sharepin()
{
	if(app_cust_config.we1_driver_cfg.chip_package == WE1AppCfg_CHIP_Package_WLCSP38)
	{
       if((app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM11B1_LSB) ||
				(app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM11B1_MSB))
       {
    	   hx_drv_sensorctrl_set_wlcsp_sharepin(SENSORCTRL_WLCSP_SHAREPIN_HM11B1);
       }else if((app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM01B0_1BITIO_LSB) ||
    		   (app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM01B0_1BITIO_MSB))
       {
    	   hx_drv_sensorctrl_set_wlcsp_sharepin(SENSORCTRL_WLCSP_SHAREPIN_HM01B0);
       }
#if defined(CIS_HM0360_M_REVB_EXTLDO_MCLK12M_1BIT_XSLEEP) || defined(CIS_HM0360_M_REVB_EXTLDO_MCLK24M_1BIT_XSLEEP)
       else if(app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM0360_MODE5)
       {
    	   hx_drv_sensorctrl_set_wlcsp_sharepin(SENSORCTRL_WLCSP_SHAREPIN_HM0360_MCLK);
       }
#endif
#if defined(CIS_HM0360_M_REVB_EXTLDO_OSC24M_1BIT_XSLEEP)
       else if(app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM0360_MODE5)
       {
    	   hx_drv_sensorctrl_set_wlcsp_sharepin(SENSORCTRL_WLCSP_SHAREPIN_HM0360_OSC);
       }
#endif

	}
}

void app_sensor_xshutdown_toggle()
{
	if((app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM1245) ||
	   (app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM2140) ||
	   (app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM0360_MODE1) ||
	   (app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM0360_MODE2) ||
	   (app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM0360_MODE3) ||
	   (app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM0360_MODE5) ||
		(app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM11B1_LSB) ||
		(app_cust_config.sensor_table_cfg.sensor_id == SENSORDPLIB_SENSOR_HM11B1_MSB))
	{
		hx_drv_sensorctrl_set_xShutdown(g_xshutdown_pin, 0);
		board_delay_cycle(1* BOARD_SYS_TIMER_MS_CONV);
		hx_drv_sensorctrl_set_xShutdown(g_xshutdown_pin, 1);
		board_delay_cycle(50* BOARD_SYS_TIMER_US_CONV);
	}
}

/*Sensor and RX related End*/

