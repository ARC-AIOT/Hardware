/*
 * evt_spimcomm.h
 *
 *  Created on: 2019¦~8¤ë20¤é
 *      Author: 902449
 */

#ifndef SCENARIO_APP_EVENT_HANDLER_EVT_SPIMCOMM_EVT_SPIMCOMM_H_
#define SCENARIO_APP_EVENT_HANDLER_EVT_SPIMCOMM_EVT_SPIMCOMM_H_


/****************************************************
 * ENUM Declaration                                 *
 ***************************************************/
/**
 * \defgroup	SPIM_COMM_ENUM	SPI master Communication Library Enumeration
 * \ingroup	SPIM_COMM
 * \brief	Defines the required enumeration of spi master communication library.
 * @{
 */
/**
 * \enum EVT_SPIMCOM_ERR_E
 * \brief this enumeration use in spi master communication library, define the status of spi master communication process.
 */
typedef enum
{
	EVT_SPIMCOMM_NO_ERROR	             = 0,	/**< STATUS: No Error*/
	EVT_SPIMCOMM_ERR_BUSY	           	    	/**< STATUS: SPI MASTER is still in busy status */
} EVT_SPIMCOM_ERR_E;
/** @} */

/*
 * @brief spi master initial
 *
 * spi master initial
 * @return void.
 * */
EVT_SPIMCOM_ERR_E evt_spimcomm_init(void);

/*
 * @brief spi master send data from sram address to slave
 *
 *  spi master send data from sram address to slave
 * @param SRAM_addr image data address in SRAM
 * @param img_size image size
 * @return void.
 * */
EVT_SPIMCOM_ERR_E evt_spimcomm_tx(uint32_t SRAM_addr, uint32_t img_size);
/**
 * \brief	Callback function for spi master to slave transmit
 *
 * Callback function for spi master to slave transmit
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_spimcomm_tx_cb(void);

/**
 * \brief	Callback function for spi master to slave receive
 *
 * Callback function for spi master to slave receive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_spimcomm_rx_cb(void);

#endif /* SCENARIO_APP_EVENT_HANDLER_EVT_SPIMCOMM_EVT_SPIMCOMM_H_ */
