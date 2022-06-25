/*
 * evt_spiscomm.h
 *
 *  Created on: 2020/02/03
 *      Author: 903730
 */


#ifndef EVT_SPIMCOMM_EVT_SPISCOMM_H_
#define EVT_SPIMCOMM_EVT_SPISCOMM_H_

/****************************************************
 * ENUM Declaration                                 *
 ***************************************************/
/**
 * \defgroup	SPIS_COMM_ENUM	SPI slave Communication Library Enumeration
 * \ingroup	SPIS_COMM
 * \brief	Defines the required enumeration of spi slave communication library.
 * @{
 */
/**
 * \enum EVT_SPISCOM_ERR_E
 * \brief this enumeration use in spi slave communication library, define the status of spi slave communication process.
 */
typedef enum
{
	EVT_SPISCOMM_NO_ERROR	             = 0,	/**< STATUS: No Error*/
	EVT_SPISCOMM_ERR_DRVFAIL           	    	/**< STATUS: Driver Not Initialize Correctly */
} EVT_SPISCOM_ERR_E;

typedef enum
{
	EVT_SPISCOMM_CMD_FR	             = 1,	
	EVT_SPISCOMM_CMD_TRANSFER,           	    	
} EVT_SPISCOM_CMD;

/** @} */

/*
 * @brief SPI slave init
 *
 * SPI slave initial for event handler use

 * @return EVT_SPISCOMM_NO_ERROR.
 * */
EVT_SPISCOM_ERR_E evt_spiscomm_init(void);


/**
 * \brief	Callback function for spi slave receive
 *
 * Callback function for spi receive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_spiscomm_rx_cb(void);

/**
 * \brief	set spi driver to read 7bytes command data
 */
void evt_spiscomm_cmd_read(void);


#ifdef DUT
/**
 * \brief	Callback function for spi slave receive
 *          switch function to 7bytes command or 6k data.
 *
 * Callback function for spi receive
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_spiscomm_rx_cb_swtich_cb(void);

/**
 * \brief	Callback function for spi slave receive 
 *          over 7 bytes.(1000 bytes or 6000 bytes)
 *
 * \retval	INFRA_EVT_RTN_CLEAR	to clear event
 */
uint8_t evt_spiscomm_rx_packet_cb(void);

/**
 * \brief	set spi driver to read 6KB or below 6KB data
 *          (current paket size = 6000)
 */
void evt_spiscomm_cmd_read_6k(uint8_t *sram_addr, uint32_t size);

void detect_spi_s_timeout();
#endif

#endif