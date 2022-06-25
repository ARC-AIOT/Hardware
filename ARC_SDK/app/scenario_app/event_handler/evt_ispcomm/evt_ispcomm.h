/**
 ********************************************************************************************
 *  @file      evt_ispcomm.h
 *  @details   This file contains all event handler related function
 *  @author    himax/902452
 *  @version   V1.0.0
 *  @date      14-July-2019
 *  @copyright (C) COPYRIGHT, Himax, Inc. ALL RIGHTS RESERVED
 *******************************************************************************************/
/**
 * \defgroup	EVT_ISPCOMM	ISPCOMM Event
 * \ingroup	EVENT_HANDLER
 * \brief	ISPCOMM EVENT declaration
 */

#ifndef EVENT_HANDLER_EVT_ISPCOMM_EVT_ISPCOMM_H_
#define EVENT_HANDLER_EVT_ISPCOMM_EVT_ISPCOMM_H_

/****************************************************
 * Constant Definition                              *
 ***************************************************/


/****************************************************
 * ENUM Declaration                                 *
 ***************************************************/
/**
 * \defgroup	ISP_COMM_ENUM	ISP Communication Library Enumeration
 * \ingroup	ISP_COMM
 * \brief	Defines the required enumeration of ISP communication library.
 * @{
 */
/**
 * \enum EVT_IICCOM_ERR_E
 * \brief this enumeration use in isp communication library, define the status of isp communication process.
 */
typedef enum
{
	EVT_ISPCOMM_NO_ERROR	             = 0,	/**< STATUS: No Error*/
	EVT_ISPCOMM_ERR_DRVFAIL	           	    	/**< STATUS: Driver Not Initialize Correctly */
} EVT_ISPCOM_ERR_E;
/** @} */


/****************************************************
 * Function Declaration                             *
 ***************************************************/
/**
 * \defgroup	EVT_ISPCOMM_FUNCDLR	ISP Communication Event Function Declaration
 * \ingroup	EVENT_HANDLER
 * \brief	Contains declarations of ISP Communication Event functions.
 * @{
 */

/**
 * \brief	The function use to get the version of isp communication process
 *
 * Get the version of isp communication process
 * \retval	EVT_ISPCOMM_NO_ERROR	no error
 * \retval	others	error
 */
EVT_ISPCOM_ERR_E evt_ispcomm_init(void);


/**
 * \brief	Callback function for isp communication process
 *
 * Callback function for isp communication process
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_ispcomm_rx_cb(void);  // isp rx event handler

/** @} */	//EVT_ISPCOMM_FUNCDLR

#endif /* EVENT_HANDLER_EVT_ISPCOMM_EVT_ISPCOMM_H_ */
