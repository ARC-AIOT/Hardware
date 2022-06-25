/**
 ********************************************************************************************
 *  @file      evt_audio.h
 *  @details   This file contains all event handler related function
 *  @author    himax/902453
 *  @version   V1.0.0
 *  @date      14-July-2019
 *  @copyright (C) COPYRIGHT, Himax, Inc. ALL RIGHTS RESERVED
 *******************************************************************************************/
/**
 * \defgroup	EVT_AUDIO	AUDIO Event
 * \ingroup	EVENT_HANDLER
 * \brief	AUDIO EVENT declaration
 */

#ifndef EVENT_HANDLER_EVT_AUDIO_EVT_AUDIO_H_
#define EVENT_HANDLER_EVT_AUDIO_EVT_AUDIO_H_

/****************************************************
 * Constant Definition                              *
 ***************************************************/


/****************************************************
 * ENUM Declaration                                 *
 ***************************************************/
/**
 * \defgroup	AUDIO_PDM_ENUM Audio Library Enumeration
 * \ingroup	AUIDO_PDM
 * \brief	Defines the required enumeration of audio library.
 * @{
 */
/**
 * \enum EVT_PDM_ERR_E
 * \brief this enumeration use in isp communication library, define the status of isp communication process.
 */
typedef enum
{
	EVT_PDM_NO_ERROR	             = 0,	/**< STATUS: No Error*/
	EVT_PDM_ERR_DRVFAIL	           	    	/**< STATUS: Driver Not Initialize Correctly */
} EVT_PDM_ERR_E;
/** @} */


/****************************************************
 * Function Declaration                             *
 ***************************************************/
/**
 * \defgroup	EVT_PDM_FUNCDLR	Audio Event Function Declaration
 * \ingroup	EVENT_HANDLER
 * \brief	Contains declarations of Audio Event functions.
 * @{
 */

/**
 * \brief	The function use to init audio library
 *
 * Init PDM process
 * \retval	EVT_PDM_NO_ERROR	no error
 * \retval	others	error
 */
EVT_PDM_ERR_E evt_audio_init(void);


/**
 * \brief	Callback function for Audio rx
 *
 * Callback function for Audio rx process
 * \retval	INFRA_EVT_RTN_NONE	to keep event alive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_audio_rx_cb(void);  // audio rx event handler

/** @} */	//EVT_PDM_FUNCDLR

#endif /* EVENT_HANDLER_EVT_AUDIO_EVT_AUDIO_H_ */
