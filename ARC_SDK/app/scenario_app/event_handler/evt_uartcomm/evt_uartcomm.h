/*
 * evt_uartcomm.h
 *
 *  Created on: 2020/04/14
 *      Author: 903730
 */

#ifndef EVENT_HANDLER_EVT_UARTCOMM_EVT_UART_H_
#define EVENT_HANDLER_EVT_UARTCOMM_EVT_UART_H_

#include "hx_drv_uart.h"
#include "spi_slave_protocol.h"

/****************************************************
 * ENUM Declaration                                 *
 ***************************************************/
/**
 * \defgroup	UART_COMM_ENUM	UART Communication Library Enumeration
 * \ingroup	UART_COMM_ENUM
 * \brief	Defines the required enumeration of uart communication library.
 * @{
 */
/**
 * \enum EVT_UARTCOMM_ERR_E
 * \brief this enumeration use in uart communication library, define the status of uart communication process.
 */
typedef enum
{
    EVT_UARTCOMM_NO_ERROR           = 0,    /**< STATUS: No Error*/
    EVT_UARTCOMM_ERR_DRVFAIL                /**< STATUS: Driver Not Initialize Correctly */
} EVT_UARTCOMM_ERR_E;



/** @} */
/*
 * @brief Uart init
 *
 * Uart initial for event handler use

 * @return EVT_UARTCOMM_ERR_E.
 * */
EVT_UARTCOMM_ERR_E evt_uartcomm_init(void);


/**
 * \brief	Callback function for uart receive
 *
 * Callback function for uart receive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_uartcomm_rx_cb(void);

DEV_UART_PTR get_dev_uart_comm();

#endif
