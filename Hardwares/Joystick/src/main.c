#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "embARC.h"
#include "embARC_debug.h"
#include "board_config.h"
#include "arc_timer.h"
#include "hardware_config.h"

#include "hx_drv_iic_m.h"

#include "joystick.h"

volatile static DEV_IIC_PTR dev_iic_m1_ptr;

#define ADC_3021_DEV_ADDR 0x4f
#define REG_ADDR 0x7f

int main(void) {
  printf("into %s-%d\r\n", __func__, __LINE__);
  uint8_t send_buf[3];
  uint8_t read_buf[2];
  uint8_t i;
  uint8_t ret_i2c;
  uint8_t reg_buf[2];
  uint8_t dev_addr;
  uint8_t rw_buf[4];

  memset(send_buf, '\0', sizeof(send_buf));
  memset(read_buf, '\0', sizeof(read_buf));
  memset(reg_buf, '\0', sizeof(reg_buf));
  memset(rw_buf, '\0', sizeof(rw_buf));

  memset(reg_buf, '\0', sizeof(reg_buf));
  // memset(rw_buf, '\0', sizeof(rw_buf));

  dev_addr = ADC_3021_DEV_ADDR;
  reg_buf[0] = REG_ADDR;

  while (1) {

    
    memset(rw_buf, '\0', sizeof(rw_buf));
    hx_drv_i2cm_read_data(SS_IIC_0_ID, dev_addr, rw_buf, 2);
    printf("ADC 3021: %d, %d\r\n", rw_buf[0], rw_buf[1]);

    board_delay_ms(3000);
  }

  return 0;
}
