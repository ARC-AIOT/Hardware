/**
 ********************************************************************************************
 *  @file      evt_i2ccomm.h
 *  @details   This file contains all event handler related function
 *  @author    himax/902452
 *  @version   V1.0.0
 *  @date      14-July-2019
 *  @copyright (C) COPYRIGHT, Himax, Inc. ALL RIGHTS RESERVED
 *******************************************************************************************/
/**
 * \defgroup	EVT_I2CCOMM	I2CCOMM Event
 * \ingroup	EVENT_HANDLER
 * \brief	I2CCOMM EVENT declaration
 */

#ifndef EVENT_HANDLER_EVT_I2CCOMM_EVT_I2CCOMM_H_
#define EVENT_HANDLER_EVT_I2CCOMM_EVT_I2CCOMM_H_

/****************************************************
 * Constant Definition                              *
 ***************************************************/
#define EVT_I2CCOMM_SLV_ID	0x62



/****************************************************
 * ENUM Declaration                                 *
 ***************************************************/
/**
 * \defgroup	I2C_COMM_ENUM	I2C Communication Library Enumeration
 * \ingroup	I2C_COMM
 * \brief	Defines the required enumeration of I2C communication library.
 * @{
 */
/**
 * \enum EVT_IICCOM_ERR_E
 * \brief this enumeration use in i2c communication library, define the status of i2c communication process.
 */
typedef enum
{
	EVT_IICCOMM_NO_ERROR	             = 0,	/**< STATUS: No Error*/
	EVT_IICCOMM_ERR_DRVFAIL	           	    	/**< STATUS: Driver Not Initialize Correctly */
} EVT_IICCOM_ERR_E;
/** @} */


/****************************************************
 * Function Declaration                             *
 ***************************************************/
/**
 * \defgroup	EVT_I2CCOMM_FUNCDLR	I2C Communication Event Function Declaration
 * \ingroup	EVENT_HANDLER
 * \brief	Contains declarations of I2C Communication Event functions.
 * @{
 */

#ifdef OS_FREERTOS
int i2ccomm_task_init(void);
#else
/**
 * \brief	The function use to get the version of i2c communication process
 *
 * Get the version of i2c communication process
 * \retval	EVT_IICCOMM_NO_ERROR	no error
 * \retval	others	error
 */
EVT_IICCOM_ERR_E evt_i2ccomm_init(void);
#endif

/**
 * \brief	Callback function for i2c communication write process
 *
 * Callback function for i2c communication write process
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_i2ccomm_tx_cb(void);	// i2c tx event handler


/**
 * \brief	Callback function for i2c communication read process
 *
 * Callback function for i2c communication read process
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_i2ccomm_rx_cb(void);  // i2c rx event handler

/**
 * \brief	Callback function for i2c communication error process
 *
 * Callback function for i2c communication error process
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_i2ccomm_err_cb(void);  // i2c error event handler


void i2ccomm_cmd_customer_register_cb(void *cb_func);

/** @} */	//EVT_I2CCOMM_FUNCDLR

#endif /* EVENT_HANDLER_EVT_I2CCOMM_EVT_I2CCOMM_H_ */
