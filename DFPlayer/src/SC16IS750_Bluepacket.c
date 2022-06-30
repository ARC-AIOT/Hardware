#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "embARC.h"
#include "embARC_debug.h"
#include "board_config.h"
#include "arc_timer.h"
#include "hardware_config.h"
#include "SC16IS750_Bluepacket.h"

#include "hx_drv_iic_m.h"
#include "hx_drv_iomux.h"
#include "hx_drv_spi_m.h"
#include "dw_spi.h"
#include "dw_spi_hal.h"
#include "rb8762cjf.h"
#include "rw8711_Bluepacket.h"

// volatile int int_flag;
int int_flag;
int rx_a_int_flag, gpio_a_int_flag;
int rx_b_int_flag, gpio_b_int_flag;

uint8_t temp_iir;
uint8_t temp_gpio_state;
uint8_t device_addr;

uint16_t recv_a_index;
uint8_t resp_buf[2048];
uint8_t recv_a_buf[2048];
// uint8_t recv_b_buf[2048];

volatile static DEV_SPI_PTR dev_spi_m_ptr;
DW_SPI_CTRL *spi_ctrl_ptr;
DW_SPI_REG *spi_reg_ptr;

static void IRQ_Callback(void *param) {
  // printf("into %s-%d\r\n", __func__, __LINE__);
  int_flag = 1;
  // printf("into %s-%d\r\n", __func__, __LINE__);
  // IRQ_State(CH_A);
}

int16_t i2cm_write_reg(uint8_t isChA, uint8_t reg_addr, int8_t *data,
                       int16_t data_len) {
  // printf("into %s-%d\r\n", __func__, __LINE__);
  uint8_t reg_buf[2];
  // uint8_t dev_addr;
  uint8_t send_buf[64];
  int8_t ret_i2c;

  memset(reg_buf, '\0', sizeof(reg_buf));
  memset(send_buf, '\0', sizeof(send_buf));

  // dev_addr = slave_dev_addr;
  if (isChA == 1) {
    reg_buf[0] = 0x00 | (reg_addr << 3);
  } else {
    reg_buf[0] = 0x02 | (reg_addr << 3);
  }

  ret_i2c = hx_drv_i2cm_write_data(SC16IS75x_I2C_INTERFACE, device_addr,
                                   reg_buf, 1, data, data_len);

  // printf("ret : %d\r\n", ret_i2c);

  return 0;
}

uint8_t i2cm_read_stream_reg(uint8_t isChA, uint8_t reg_addr, uint8_t *data_buf,
                             uint16_t data_len) {
  // printf("into %s-%d-%d\r\n", __func__, __LINE__, data_len);
  uint8_t reg_buf[2];
  // uint8_t dev_addr;
  uint8_t read_buf[64];
  uint8_t send_buf[16];
  int8_t ret_i2c;
  int i;

  memset(reg_buf, '\0', sizeof(reg_buf));
  memset(read_buf, '\0', sizeof(read_buf));
  memset(send_buf, '\0', sizeof(send_buf));

  // dev_addr = slave_dev_addr;

  if (isChA == 1) {
    reg_buf[0] = 0x00 | (reg_addr << 3);
  } else {
    reg_buf[0] = 0x02 | (reg_addr << 3);
  }

  // printf("reg : %02hhx\r\n", reg_buf[0]);
  ret_i2c = hx_drv_i2cm_write_stop_read(SC16IS75x_I2C_INTERFACE, device_addr,
                                        reg_buf, 1, data_buf, data_len);

  // printf("ret : %d\r\n", ret_i2c);
  // printf("Read value stop: %02hhx\r\n", read_buf[0]);
  /*
  for(i=0; i<data_len; i++)
          printf("%02hhx, ", data_buf[i]);

  printf("\r\n");
  */
  return 0;
}

uint8_t i2cm_read_reg(uint8_t isChA, uint8_t reg_addr) {
  // printf("into %s-%d\r\n", __func__, __LINE__);
  uint8_t reg_buf[2];
  // uint8_t dev_addr;
  uint8_t read_buf[2];
  int8_t ret_i2c;

  memset(reg_buf, '\0', sizeof(reg_buf));
  memset(read_buf, '\0', sizeof(read_buf));

  // dev_addr = slave_dev_addr;

  if (isChA == 1) {
    reg_buf[0] = 0x00 | (reg_addr << 3);
  } else {
    reg_buf[0] = 0x02 | (reg_addr << 3);
  }

  // printf("reg : %02hhx\r\n", reg_buf[0]);
  ret_i2c = hx_drv_i2cm_write_stop_read(SC16IS75x_I2C_INTERFACE, device_addr,
                                        reg_buf, 1, read_buf, 1);

  // printf("ret : %d\r\n", ret_i2c);
  // printf("Read value stop: %02hhx\r\n", read_buf[0]);

  return read_buf[0];
}

int16_t spim_write_reg(uint8_t isChA, uint8_t reg_addr, int8_t *data,
                       int16_t data_len) {
  // printf("into %s-%d\r\n", __func__, __LINE__);
  uint8_t reg_buf[2];
  // uint8_t dev_addr;
  uint8_t send_buf[64];
  int8_t ret_spi;

  memset(reg_buf, '\0', sizeof(reg_buf));
  memset(send_buf, '\0', sizeof(send_buf));

  // dev_addr = slave_dev_addr;
  if (isChA == 1) {
    send_buf[0] = 0x00 | (reg_addr << 3);
  } else {
    send_buf[0] = 0x02 | (reg_addr << 3);
  }

  memcpy(send_buf + 1, data, data_len);
  /*
          int i;
          for(i=0; i<data_len+1; i++)
                  printf("%02hhx, ", send_buf[i]);

          printf("\r\n");
  */
  // dw_spi_write(dev_spi_m_ptr, data, data_len);

  // hx_drv_iomux_set_outvalue(IOMUX_PGPIO9, 1);
  // hx_drv_iomux_set_outvalue(IOMUX_PGPIO12, 0);
  dw_spi_write(dev_spi_m_ptr, send_buf, data_len + 1);
  // hx_drv_iomux_set_outvalue(IOMUX_PGPIO9, 0);
  // hx_drv_iomux_set_outvalue(IOMUX_PGPIO12, 0);

  // ret_i2c = hx_drv_i2cm_write_data(SC16IS75x_I2C_INTERFACE, device_addr,
  // reg_buf, 1, data, data_len);

  // printf("ret : %d\r\n", ret_i2c);

  return 0;
}

uint8_t spim_read_reg(uint8_t isChA, uint8_t reg_addr) {
  // printf("into %s-%d\r\n", __func__, __LINE__);
  uint8_t reg_buf[2];
  // uint8_t dev_addr;
  uint8_t read_buf[2];
  int8_t ret_spi;

  memset(reg_buf, '\0', sizeof(reg_buf));
  memset(read_buf, '\0', sizeof(read_buf));

  // dev_addr = slave_dev_addr;

  if (isChA == 1) {
    reg_buf[0] = 0x00 | (reg_addr << 3) | 0x80;
  } else {
    reg_buf[0] = 0x02 | (reg_addr << 3) | 0x80;
  }

  ret_spi = dw_spi_write_read(dev_spi_m_ptr, reg_buf, 1, read_buf, 1);

  return read_buf[0];
}

uint8_t spim_read_stream_reg(uint8_t isChA, uint8_t reg_addr, uint8_t *data_buf,
                             uint16_t data_len) {
  // printf("into %s-%d\r\n", __func__, __LINE__);
  uint8_t reg_buf[2];
  // uint8_t dev_addr;
  uint8_t read_buf[2];
  int8_t ret_spi;

  memset(reg_buf, '\0', sizeof(reg_buf));
  memset(read_buf, '\0', sizeof(read_buf));

  // dev_addr = slave_dev_addr;

  if (isChA == 1) {
    reg_buf[0] = 0x00 | (reg_addr << 3) | 0x80;
  } else {
    reg_buf[0] = 0x02 | (reg_addr << 3) | 0x80;
  }

  ret_spi = dw_spi_write_read(dev_spi_m_ptr, reg_buf, 1, data_buf, data_len);

  return read_buf[0];
}

void FIFOEnable(uint8_t interface, uint8_t isChA, uint8_t fifo_enable) {
  printf("into %s-%d\r\n", __func__, __LINE__);
  uint8_t temp_fcr[2];

  memset(temp_fcr, '\0', sizeof(temp_fcr));

  if (interface == SC16IS750_PROTOCOL_I2C) {
    temp_fcr[0] = i2cm_read_reg(isChA, SC16IS750_REG_FCR);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    temp_fcr[0] = spim_read_reg(isChA, SC16IS750_REG_FCR);
  }

  printf("temp fcr : %d- %02hhx\r\n", __LINE__, temp_fcr[0]);

  if (fifo_enable == 0) {
    temp_fcr[0] &= 0xFE;
  } else {
    temp_fcr[0] |= 0x01;
  }

  printf("temp fcr : %d- %02hhx\r\n", __LINE__, temp_fcr[0]);

  if (interface == SC16IS750_PROTOCOL_I2C) {
    i2cm_write_reg(isChA, SC16IS750_REG_FCR, temp_fcr, 1);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    spim_write_reg(isChA, SC16IS750_REG_FCR, temp_fcr, 1);
  }

  return;
}

void FIFOSetTriggerLevel(uint8_t interface, uint8_t rx_fifo, uint8_t length) {
  // printf("into %s-%d\r\n", __func__, __LINE__);
  uint8_t temp_reg[2];
  uint8_t tmp_reg[2];
  uint8_t tmp;
  uint8_t temp_length[2];

  memset(temp_reg, '\0', sizeof(temp_reg));
  memset(temp_length, '\0', sizeof(temp_length));

  // temp_reg[0] = ReadRegister(SC16IS750_REG_MCR);
  if (interface == SC16IS750_PROTOCOL_I2C) {
    temp_reg[0] = i2cm_read_reg(CH_A, SC16IS750_REG_MCR);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    temp_reg[0] = spim_read_reg(CH_A, SC16IS750_REG_MCR);
  }

  temp_reg[0] |= 0x04;

  // WriteRegister(SC16IS750_REG_MCR, temp_reg);

  if (interface == SC16IS750_PROTOCOL_I2C) {
    i2cm_write_reg(CH_A, SC16IS750_REG_MCR, temp_reg, 1);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    spim_write_reg(CH_A, SC16IS750_REG_MCR, temp_reg, 1);
  }

  // SET MCR[2] to '1' to use TLR register or trigger level control in FCR
  // register

  if (interface == SC16IS750_PROTOCOL_I2C) {
    temp_reg[0] = i2cm_read_reg(CH_A, SC16IS750_REG_LCR);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    temp_reg[0] = spim_read_reg(CH_A, SC16IS750_REG_LCR);
  }

  printf("temp efr : %d- %02hhx\r\n", __LINE__, temp_reg[0]);

  tmp = temp_reg[0];
  tmp_reg[0] = 0xBF;

  if (interface == SC16IS750_PROTOCOL_I2C) {
    i2cm_write_reg(CH_A, SC16IS750_REG_LCR, tmp_reg, 1);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    spim_write_reg(CH_A, SC16IS750_REG_LCR, tmp_reg, 1);
  }

  // temp_reg = ReadRegister(SC16IS750_REG_EFR);
  temp_reg[0] = i2cm_read_reg(CH_A, SC16IS750_REG_EFR);
  printf("temp efr : %d- %02hhx\r\n", __LINE__, temp_reg[0]);

  tmp_reg[0] = temp_reg[0] | 0x10;

  if (interface == SC16IS750_PROTOCOL_I2C) {
    i2cm_write_reg(CH_A, SC16IS750_REG_EFR, tmp_reg, 1);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    spim_write_reg(CH_A, SC16IS750_REG_EFR, tmp_reg, 1);
  }

  // WriteRegister(SC16IS750_REG_EFR, temp_reg|0x10); //set ERF[4] to '1' to use
  // the  enhanced features

  if (rx_fifo == 0) {
    // WriteRegister(SC16IS750_REG_TLR, length<<4); //Tx FIFO trigger level
    // setting
    temp_length[0] = length << 4;

    if (interface == SC16IS750_PROTOCOL_I2C) {
      i2cm_write_reg(CH_A, SC16IS750_REG_TLR, temp_length[0], 1);
    } else if (interface == SC16IS750_PROTOCOL_SPI) {
      spim_write_reg(CH_A, SC16IS750_REG_TLR, temp_length[0], 1);
    }

  } else {
    temp_length[0] = length;
    if (interface == SC16IS750_PROTOCOL_I2C) {
      i2cm_write_reg(CH_A, SC16IS750_REG_TLR, temp_length[0], 1);
    } else if (interface == SC16IS750_PROTOCOL_SPI) {
      spim_write_reg(CH_A, SC16IS750_REG_TLR, temp_length[0], 1);
    }
    // WriteRegister(SC16IS750_REG_TLR, length);    //Rx FIFO Trigger level
    // setting
  }

  // WriteRegister(SC16IS750_REG_EFR, temp_reg[0]); //restore EFR register
  if (interface == SC16IS750_PROTOCOL_I2C) {
    i2cm_write_reg(CH_A, SC16IS750_REG_LCR, temp_reg, 1);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    spim_write_reg(CH_A, SC16IS750_REG_LCR, temp_reg, 1);
  }

  temp_reg[0] = tmp;

  if (interface == SC16IS750_PROTOCOL_I2C) {
    i2cm_write_reg(CH_A, SC16IS750_REG_LCR, temp_reg, 1);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    spim_write_reg(CH_A, SC16IS750_REG_LCR, temp_reg, 1);
  }

  return;
}

int16_t SetBaudrate(uint8_t interface, uint8_t isChA, uint32_t baudrate) {
  printf("into %s-%d\r\n", __func__, __LINE__);

  uint16_t divisor;
  uint8_t prescaler;
  uint32_t actual_baudrate;
  int16_t error;
  uint8_t temp_lcr;
  uint8_t send_buf[2];

  memset(send_buf, '\0', sizeof(send_buf));

  if (interface == SC16IS750_PROTOCOL_I2C) {
    if ((i2cm_read_reg(isChA, SC16IS750_REG_MCR) & 0x80) ==
        0) { // if prescaler==1
      printf("into %s-%d\r\n", __func__, __LINE__);
      prescaler = 1;
    } else {
      printf("into %s-%d\r\n", __func__, __LINE__);
      prescaler = 4;
    }
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    if ((spim_read_reg(isChA, SC16IS750_REG_MCR) & 0x80) ==
        0) { // if prescaler==1
      printf("into %s-%d\r\n", __func__, __LINE__);
      prescaler = 1;
    } else {
      printf("into %s-%d\r\n", __func__, __LINE__);
      prescaler = 4;
    }
  }

  divisor = (SC16IS750_CRYSTCAL_FREQ / prescaler) / (baudrate * 16);

  // temp_lcr = ReadRegister(SC16IS750_REG_LCR);
  if (interface == SC16IS750_PROTOCOL_I2C) {
    temp_lcr = i2cm_read_reg(isChA, SC16IS750_REG_LCR);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    temp_lcr = spim_read_reg(isChA, SC16IS750_REG_LCR);
  }

  temp_lcr |= 0x80;

  send_buf[0] = temp_lcr;

  if (interface == SC16IS750_PROTOCOL_I2C) {
    i2cm_write_reg(isChA, SC16IS750_REG_LCR, send_buf, 1);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    spim_write_reg(isChA, SC16IS750_REG_LCR, send_buf, 1);
  }

  // WriteRegister(SC16IS750_REG_LCR,temp_lcr);

  // write to DLL
  send_buf[0] = (uint8_t)divisor;
  if (interface == SC16IS750_PROTOCOL_I2C) {
    i2cm_write_reg(isChA, SC16IS750_REG_DLL, send_buf, 1);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    spim_write_reg(isChA, SC16IS750_REG_DLL, send_buf, 1);
  }
  // WriteRegister(SC16IS750_REG_DLL,(uint8_t)divisor);

  // write to DLH
  send_buf[0] = (uint8_t)(divisor >> 8);
  if (interface == SC16IS750_PROTOCOL_I2C) {
    i2cm_write_reg(isChA, SC16IS750_REG_DLH, send_buf, 1);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    spim_write_reg(isChA, SC16IS750_REG_DLH, send_buf, 1);
  }

  // WriteRegister(SC16IS750_REG_DLH,(uint8_t)(divisor>>8));

  temp_lcr &= 0x7F;
  send_buf[0] = temp_lcr;

  if (interface == SC16IS750_PROTOCOL_I2C) {
    i2cm_write_reg(isChA, SC16IS750_REG_LCR, send_buf, 1);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    spim_write_reg(isChA, SC16IS750_REG_LCR, send_buf, 1);
  }
  // WriteRegister(SC16IS750_REG_LCR,temp_lcr);

  actual_baudrate = (SC16IS750_CRYSTCAL_FREQ / prescaler) / (16 * divisor);
  error = ((float)actual_baudrate - baudrate) * 1000 / baudrate;

  printf("===== %s =====\r\n", (isChA == 1) ? "UART A" : "UART B");
  printf("Desired baudrate: %d\r\n", baudrate);
  printf("Calculated divisor: %d\r\n", divisor);
  printf("Actual baudrate: %d\r\n", actual_baudrate);
  printf("Baudrate error: %d\r\n", error);

  return error;
}

void HX_GPIOSetup() {
  hx_drv_iomux_set_pmux(IOMUX_PGPIO9, 3);
  hx_drv_iomux_set_pmux(IOMUX_PGPIO12, 3);
  hx_drv_iomux_set_outvalue(IOMUX_PGPIO9, 1);
  hx_drv_iomux_set_outvalue(IOMUX_PGPIO12, 0);
}

void IRQSetup() {
  printf("into %s-%d\r\n", __func__, __LINE__);
  hx_drv_iomux_cb_register(SC16IS75x_WIFI_IRQ, IRQ_Callback);
  hx_drv_iomux_set_intpol(SC16IS75x_WIFI_IRQ, 1);
  int_level_config(SC16IS75x_WIFI_IRQ, 1);
  hx_drv_iomux_set_intenable(IOMUX_PGPIO13, 1);
}

void UART_ReadBytes(uint8_t interface) {
  // printf("into %s-%d\r\n", __func__, __LINE__);
  int8_t tmp;
  uint16_t data_len;
  uint16_t read_index;

  uint8_t isChA = 0;
  uint8_t isChB = 0;
  int i;

  memset(recv_a_buf, '\0', sizeof(recv_a_buf));

  read_index = 0;
  recv_a_index = 0;

  if (int_flag == 1) {
    int_flag = 0;
    IRQ_State(interface, CH_A);

    if (interface == SC16IS750_PROTOCOL_I2C) {
      tmp = i2cm_read_reg(CH_A, SC16IS750_REG_RXLVL);
    } else if (interface == SC16IS750_PROTOCOL_SPI) {
      tmp = spim_read_reg(CH_A, SC16IS750_REG_RXLVL);
    }
    if (tmp > 0) {
      isChA = 1;
    }

    if (isChA == 1 && rx_a_int_flag == 1) {
      if (interface == SC16IS750_PROTOCOL_I2C) {

        while ((tmp = i2cm_read_reg(CH_A, SC16IS750_REG_RXLVL)) != 0) {
          i2cm_read_stream_reg(CH_A, SC16IS750_REG_RHR, recv_a_buf + read_index,
                               tmp);
          memcpy(resp_buf + recv_a_index, recv_a_buf + read_index, tmp);
          read_index += tmp;
          recv_a_index += read_index;
        }

      } else if (interface == SC16IS750_PROTOCOL_SPI) {
        while ((tmp = spim_read_reg(CH_A, SC16IS750_REG_RXLVL)) != 0) {
          spim_read_stream_reg(CH_A, SC16IS750_REG_RHR, recv_a_buf + read_index,
                               tmp);
          memcpy(resp_buf + recv_a_index, recv_a_buf + read_index, tmp);
          read_index += tmp;
          recv_a_index += tmp;
        }
      }
    }
    rx_a_int_flag = 0;
    rx_b_int_flag = 0;
  }

  return;
}

void SetLine(uint8_t interface, uint8_t isChA, uint8_t data_length,
             uint8_t parity_select, uint8_t stop_length) {
  printf("into %s-%d\r\n", __func__, __LINE__);
  uint8_t temp_lcr;
  uint8_t send_buf[2];

  memset(send_buf, '\0', sizeof(send_buf));

  // temp_lcr = ReadRegister(SC16IS750_REG_LCR);
  if (interface == SC16IS750_PROTOCOL_I2C) {
    temp_lcr = i2cm_read_reg(isChA, SC16IS750_REG_LCR);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    temp_lcr = spim_read_reg(isChA, SC16IS750_REG_LCR);
  }

  temp_lcr &= 0xC0; // Clear the lower six bit of LCR (LCR[0] to LCR[5]

  printf("LCR Register: %02hhx\r\n", temp_lcr);

  switch (data_length) { // data length settings
  case 5:
    break;
  case 6:
    temp_lcr |= 0x01;
    break;
  case 7:
    temp_lcr |= 0x02;
    break;
  case 8:
    temp_lcr |= 0x03;
    break;
  default:
    temp_lcr |= 0x03;
    break;
  }

  if (stop_length == 2) {
    temp_lcr |= 0x04;
  }

  switch (parity_select) { // parity selection length settings
  case 0:                  // no parity
    break;
  case 1: // odd parity
    temp_lcr |= 0x08;
    break;
  case 2: // even parity
    temp_lcr |= 0x18;
    break;
  case 3: // force '1' parity
    temp_lcr |= 0x03;
    break;
  case 4: // force '0' parity
    break;
  default:
    break;
  }

  // WriteRegister(SC16IS750_REG_LCR,temp_lcr);
  send_buf[0] = temp_lcr;
  if (interface == SC16IS750_PROTOCOL_I2C) {
    i2cm_write_reg(isChA, SC16IS750_REG_LCR, send_buf, 1);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    spim_write_reg(isChA, SC16IS750_REG_LCR, send_buf, 1);
  }

  printf("LCR Register: %02hhx\r\n", temp_lcr);
  temp_lcr = spim_read_reg(isChA, SC16IS750_REG_LCR);
  printf("LCR Register end: %02hhx\r\n", temp_lcr);
}

void GPIOSetPinMode(uint8_t interface, uint8_t isChA, uint8_t pin_number,
                    uint8_t i_o) {
  printf("into %s-%d\r\n", __func__, __LINE__);
  uint8_t temp_iodir;
  uint8_t send_buf[2];

  memset(send_buf, '\0', sizeof(send_buf));

  // temp_iodir = ReadRegister(SC16IS750_REG_IODIR);
  // temp_iodir = ReadRegister(SC16IS750_REG_IODIR);
  if (interface == SC16IS750_PROTOCOL_I2C) {
    temp_iodir = i2cm_read_reg(isChA, SC16IS750_REG_IODIR);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    temp_iodir = spim_read_reg(isChA, SC16IS750_REG_IODIR);
  }

  if (i_o == OUTPUT) {
    temp_iodir |= (0x01 << pin_number);
  } else {
    temp_iodir &= (uint8_t) ~(0x01 << pin_number);
  }

  send_buf[0] = temp_iodir;
  if (interface == SC16IS750_PROTOCOL_I2C) {
    i2cm_write_reg(isChA, SC16IS750_REG_IODIR, send_buf, 1);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    spim_write_reg(isChA, SC16IS750_REG_IODIR, send_buf, 1);
  }
  // WriteRegister(SC16IS750_REG_IODIR, temp_iodir);

  return;
}

void GPIOSetPinState(uint8_t interface, uint8_t isChA, uint8_t pin_number,
                     uint8_t pin_state) {
  uint8_t temp_iostate;
  uint8_t send_buf[2];

  memset(send_buf, '\0', sizeof(send_buf));

  if (int_flag == 1) {
    printf("into %s-%d\r\n", __func__, __LINE__);
    IRQ_State(interface, CH_A);
    if (interface == SC16IS750_PROTOCOL_I2C) {
      temp_iostate = i2cm_read_reg(isChA, SC16IS750_REG_IOSTATE);
    } else if (interface == SC16IS750_PROTOCOL_SPI) {
      temp_iostate = spim_read_reg(isChA, SC16IS750_REG_IOSTATE);
    }
  } else {
    printf("into %s-%d\r\n", __func__, __LINE__);
    if (interface == SC16IS750_PROTOCOL_I2C) {
      temp_iostate = i2cm_read_reg(isChA, SC16IS750_REG_IOSTATE);
    } else if (interface == SC16IS750_PROTOCOL_SPI) {
      temp_iostate = spim_read_reg(isChA, SC16IS750_REG_IOSTATE);
    }
  }

  printf("temp_iostate : %02hhx\r\n", temp_iostate);

  if (pin_state == 1) {
    temp_iostate |= (0x01 << pin_number);
  } else {
    temp_iostate &= (uint8_t) ~(0x01 << pin_number);
  }

  send_buf[0] = temp_iostate;
  if (interface == SC16IS750_PROTOCOL_I2C) {
    i2cm_write_reg(isChA, SC16IS750_REG_IOSTATE, send_buf, 1);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    spim_write_reg(isChA, SC16IS750_REG_IOSTATE, send_buf, 1);
  }
  // WriteRegister(SC16IS750_REG_IOSTATE, temp_iostate);
  printf("iostate-%d,%d : %02hhx\r\n", pin_number, pin_state,
         spim_read_reg(isChA, SC16IS750_REG_IOSTATE));
  return;
}

uint8_t GPIOGetPinState(uint8_t interface, uint8_t isChA, uint8_t pin_number) {
  uint8_t temp_iostate;

  if (interface == SC16IS750_PROTOCOL_I2C) {
    temp_iostate = i2cm_read_reg(isChA, SC16IS750_REG_IOSTATE);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    temp_iostate = i2cm_read_reg(isChA, SC16IS750_REG_IOSTATE);
  }
  // temp_iostate = ReadRegister(SC16IS750_REG_IOSTATE);

  // printf("temp_iostate :%02hhx, %02hhx\r\n", temp_iostate, temp_iostate &
  // (0x01 << pin_number));

  if ((temp_iostate & (0x01 << pin_number)) == 0) {
    return 0;
  }

  return 1;
}

/*
        Setup GPIO Interrupt Enable
*/

void SetPinInterrupt(uint8_t interface, uint8_t isChA, uint8_t pin_number) {
  printf("into %s-%d\r\n", __func__, __LINE__);
  uint8_t temp_iointea;
  uint8_t send_buf[2];

  memset(send_buf, '\0', sizeof(send_buf));

  if (interface == SC16IS750_PROTOCOL_I2C) {
    temp_iointea = i2cm_read_reg(isChA, SC16IS750_REG_IOINTENA);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    temp_iointea = spim_read_reg(isChA, SC16IS750_REG_IOINTENA);
  }
  printf("temp iointea : %02hhx\r\n", temp_iointea);

  temp_iointea |= (0x01 << pin_number);
  printf("temp iointea : %02hhx\r\n", temp_iointea);
  send_buf[0] = temp_iointea;

  // WriteRegister(SC16IS750_REG_IOINTENA, io_int_ena);
  if (interface == SC16IS750_PROTOCOL_I2C) {
    i2cm_write_reg(isChA, SC16IS750_REG_IOINTENA, send_buf, 1);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    spim_write_reg(isChA, SC16IS750_REG_IOINTENA, send_buf, 1);
  }
  return;
}

void InterruptControl(uint8_t interface, uint8_t isChA, uint8_t int_ena) {
  printf("into %s-%d\r\n", __func__, __LINE__);
  uint8_t send_buf[2];
  memset(send_buf, '\0', sizeof(send_buf));

  send_buf[0] = int_ena;
  if (interface == SC16IS750_PROTOCOL_I2C) {
    i2cm_write_reg(isChA, SC16IS750_REG_IER, send_buf, 1);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    spim_write_reg(isChA, SC16IS750_REG_IER, send_buf, 1);
  }
  // WriteRegister(SC16IS750_REG_IER, int_ena);

  return;
}

void IRQ_State(uint8_t interface, uint8_t isChA) {
  uint8_t irq_src;

  if (interface == SC16IS750_PROTOCOL_I2C) {
    irq_src = i2cm_read_reg(isChA, SC16IS750_REG_IIR);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    irq_src = spim_read_reg(isChA, SC16IS750_REG_IIR);
  }
  temp_iir = irq_src;
  // irq_src = ReadRegister(SC16IS750_REG_IIR);
  // printf("iir : %02hhx\r\n", irq_src);
  // irq_src = (irq_src >> 1);
  irq_src &= 0x3F;

  // printf("iir : %02hhx\r\n", irq_src);

  switch (irq_src) {
  case 0x06: // Receiver Line Status Error
    break;
  case 0x0c: // Receiver time-out interrupt
    //(isChA==1? rx_a_int_flag=1 : rx_b_int_flag=1);
    if (isChA == 1) {
      rx_a_int_flag = 1;
    } else {
      rx_b_int_flag = 1;
    }
    break;
  case 0x04: // RHR interrupt
    if (isChA == 1) {
      rx_a_int_flag = 1;
    } else {
      rx_b_int_flag = 1;
    }
    break;
  case 0x02: // THR interrupt
    break;
  case 0x00: // modem interrupt;
    break;
  case 0x30: // input pin change of state
    temp_gpio_state = (isChA == 1 ? GPIOGetPinState(interface, CH_A, GPIO4)
                                  : GPIOGetPinState(interface, CH_B, GPIO4));
    if (isChA == 1) {
      gpio_a_int_flag = 1;
    } else {
      gpio_b_int_flag = 1;
    }
    break;
  case 0x10: // XOFF
    break;
  case 0x20: // CTS,RTS
    break;
  default:
    break;
  }

  return;
}

void data_receive(uint8_t interface, uint8_t *mes, uint16_t wait) {
  int retry;
  int jumper_flag = 0;

  jumper_flag = 0;
  recv_a_index = 0;
  memset(resp_buf, '\0', sizeof(resp_buf));

  while (1) {
    retry = 0;
    while (int_flag == 0 && jumper_flag == 0) {
      // if(retry>200){
      if (retry > wait) {
        jumper_flag = 1;
        break;
      }
      board_delay_ms(1);
      retry++;
    }

    UART_ReadBytes(interface);
    if (strstr(resp_buf, mes) != NULL) {
      printf("%s", resp_buf);
      break;
    }

    if (jumper_flag == 1) {
      printf("%s", resp_buf);
      break;
    }
    // board_delay_ms(1000);
  }

  board_delay_ms(1000);
  return;
}

void message_receive(uint8_t interface, uint8_t *mes, uint16_t wait) {
  int retry;
  int jumper_flag = 0;

  jumper_flag = 0;
  recv_a_index = 0;
  memset(resp_buf, '\0', sizeof(resp_buf));

  while (1) {
    retry = 0;
    while (int_flag == 0 && jumper_flag == 0) {
      // if(retry>200){
      if (retry > wait) {
        jumper_flag = 1;
        break;
      }
      board_delay_ms(1);
      retry++;
    }

    UART_ReadBytes(interface);
    if (strstr(resp_buf, mes) != NULL) {
      printf("%s", resp_buf);
      break;
    }

    if (jumper_flag == 1) {
      printf("%s", resp_buf);
      break;
    }
    // board_delay_ms(1000);
  }

  board_delay_ms(1000);
  return;
}

void send_cmd(uint8_t interface, uint8_t *cmd, uint16_t cmd_len) {
  if (interface == SC16IS750_PROTOCOL_I2C) {
    i2cm_write_reg(CH_A, SC16IS750_REG_THR, cmd, cmd_len);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    spim_write_reg(CH_A, SC16IS750_REG_THR, cmd, cmd_len);
  }
  return;
}

int16_t TestUART(uint8_t interface) {
  printf("into %s-%d\r\n", __func__, __LINE__);
  int8_t send_buf[64];

  memset(send_buf, '\0', sizeof(send_buf));
  sprintf(send_buf, "\r\n\r\n");
  send_cmd(interface, send_buf, strlen(send_buf));

  while (1) {
    if (int_flag == 1) {
      memset(resp_buf, '\0', sizeof(resp_buf));
      memset(send_buf, '\0', sizeof(send_buf));
      UART_ReadBytes(interface);
      // printf("From Arduino UART : %s\r\n", resp_buf);
      sprintf(send_buf, "Arduino UART : %s\r\n", resp_buf);
      send_cmd(interface, send_buf, strlen(send_buf));
    }
    board_delay_ms(10);
  }

  return 0;
}

int16_t StartTestCMD(uint8_t interface) {
  printf("into %s-%d\r\n", __func__, __LINE__);
  int8_t send_buf[64];

  // Clean cmd line
  memset(send_buf, '\0', sizeof(send_buf));
  sprintf(send_buf, "\r\n\r\n");
  send_cmd(interface, send_buf, strlen(send_buf));
  message_receive(interface, "\r\n\n#", 200);
  //---------------------------------------
  // Disable Echo
  memset(send_buf, '\0', sizeof(send_buf));
  sprintf(send_buf, AT_CMD_UART_ECHO_CTRL, WIFI_ECHO_DISABLE);
  send_cmd(interface, send_buf, strlen(send_buf));
  message_receive(interface, "\r\n\n#", 200);
  //---------------------------------------
  // Send AT
  memset(send_buf, '\0', sizeof(send_buf));
  sprintf(send_buf, AT_CMD);
  send_cmd(interface, send_buf, strlen(send_buf));
  message_receive(interface, "\r\n\n#", 200);
  //---------------------------------------
  // Show fw version
  memset(send_buf, '\0', sizeof(send_buf));
  sprintf(send_buf, AT_CMD_SHOW_FW_VER);
  send_cmd(interface, send_buf, strlen(send_buf));
  message_receive(interface, "\r\n\n#", 200);
  //---------------------------------------
  // UART Configuration
  memset(send_buf, '\0', sizeof(send_buf));
  sprintf(send_buf, AT_CMD_SHOW_UART_CONFIG);
  send_cmd(interface, send_buf, strlen(send_buf));
  message_receive(interface, "\r\n\n#", 200);
  //---------------------------------------
  // Wi-Fi Info
  memset(send_buf, '\0', sizeof(send_buf));
  sprintf(send_buf, AT_CMD_SHOW_WIFI_INFO);
  send_cmd(interface, send_buf, strlen(send_buf));
  message_receive(interface, "\r\n\n#", 500);

  while (1) {
    printf("set High\r\n");
    GPIOSetPinState(interface, CH_A, GPIO0, HIGH);
    GPIOSetPinState(interface, CH_A, GPIO1, HIGH);
    GPIOSetPinState(interface, CH_A, GPIO2, HIGH);
    GPIOSetPinState(interface, CH_A, GPIO5, HIGH);
    GPIOSetPinState(interface, CH_A, GPIO6, HIGH);
    GPIOSetPinState(interface, CH_A, GPIO7, HIGH);

    // send_buf[0]=0xff;
    // spim_write_reg(CH_A, SC16IS750_REG_IOSTATE, send_buf, 1);
    board_delay_ms(3000);

    printf("set Low\r\n");

    GPIOSetPinState(interface, CH_A, GPIO0, LOW);
    GPIOSetPinState(interface, CH_A, GPIO1, LOW);
    GPIOSetPinState(interface, CH_A, GPIO2, LOW);
    GPIOSetPinState(interface, CH_A, GPIO5, LOW);
    GPIOSetPinState(interface, CH_A, GPIO6, LOW);
    GPIOSetPinState(interface, CH_A, GPIO7, LOW);

    // send_buf[0]=0x10;
    // spim_write_reg(CH_A, SC16IS750_REG_IOSTATE, send_buf, 1);
    board_delay_ms(3000);
  }

  return 0;
}

int16_t UartInit(uint8_t interface) {
  printf("into %s-%d\r\n", __func__, __LINE__);
  int8_t send_buf[64];
  int8_t tmp;
  uint8_t read_buf[64];
  int i;
  int ret;

  int_flag = 0;
  rx_a_int_flag = 0;
  rx_b_int_flag = 0;
  gpio_a_int_flag = 0;
  gpio_b_int_flag = 0;
  temp_iir = 0;
  temp_gpio_state = 0;

  if (interface == SC16IS750_PROTOCOL_I2C) {
    // I2C Init
    hx_drv_i2cm_deinit(SS_IIC_0_ID);
    hx_drv_i2cm_init(SS_IIC_0_ID, IIC_SPEED_FAST);
  } else if (interface == SC16IS750_PROTOCOL_SPI) {
    // SPI Init
    dev_spi_m_ptr = hx_drv_spi_mst_get_dev(SC16IS75x_SPI_INTERFACE);
    ret = dev_spi_m_ptr->spi_open(DEV_MASTER_MODE, 1000000);
    ret = dev_spi_m_ptr->spi_control(SPI_CMD_MST_DSEL_DEV, (void *)0);

    printf("SPI de-select\r\n");
    ret = dev_spi_m_ptr->spi_control(SPI_CMD_MST_SEL_DEV, (void *)0);
    // ret = dev_spi_m_ptr->spi_control(SPI_CMD_SET_CLK_MODE, SPI_CLK_MODE_2);
    // info = &(dev_spi_m_ptr->spi_info);
    // spi_ctrl_ptr = (DW_SPI_CTRL_PTR)(info->spi_ctrl);
    spi_ctrl_ptr = (DW_SPI_CTRL_PTR)(dev_spi_m_ptr->spi_info.spi_ctrl);
    spi_reg_ptr = (DW_SPI_REG *)(spi_ctrl_ptr->dw_spi_regs);
  }

  device_addr = (SC16IS750_ADDRESS_AA >> 1);

  memset(send_buf, '\0', sizeof(send_buf));
  memset(read_buf, '\0', sizeof(read_buf));
  // SetBaudrate(interface, CH_A, 115200);
  SetBaudrate(interface, CH_A, 9600); // DFPlayer can only run under 9600
  SetLine(interface, CH_A, 8, 0, 1);

  // SetBaudrate(interface, CH_B, 115200);
  // SetLine(interface, CH_B, 8, 0, 1);

  InterruptControl(interface, CH_A, SC16IS750_INT_RHR);
  // InterruptControl(interface, CH_B, SC16IS750_INT_RHR);

  FIFOEnable(interface, CH_A, 1);
  // FIFOEnable(interface, CH_B, 1);

  // FIFOSetTriggerLevel(1, 16);

  return 0;
}