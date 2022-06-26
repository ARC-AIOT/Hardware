#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "embARC.h"
#include "embARC_debug.h"
#include "board_config.h"
#include "arc_timer.h"
#include "hardware_config.h"
#include "hx_drv_iic_m.h"

// For OLED
#include "hx_drv_spi_s.h"
#include "spi_slave_protocol.h"
#include "synopsys_i2c_oled1306.h"
// For OLED

// For JoyStick
#include "joystick.h"
#include "SC16IS750_Bluepacket.h"
// For JoyStick

#define ADC_3021_DEV_ADDR 0x4f
#define REG_ADDR 0x7f

int main(void) {
  printf("into %s-%d\r\n", __func__, __LINE__);
  uint8_t rw_buf[4];
  memset(rw_buf, '\0', sizeof(rw_buf));
  // Setting GPIO for joystick btn
  HX_GPIOSetup();
  IRQSetup();
  UartInit(SC16IS750_PROTOCOL_SPI);
  GPIOSetPinMode(SC16IS750_PROTOCOL_SPI, CH_A, GPIO5, INPUT);
  // Setting GPIO for joystick btn

  // Setting IIC for OLED
  DEV_IIC *iic1_ptr;
  iic1_ptr = hx_drv_i2cm_get_dev(USE_SS_IIC_1);
  iic1_ptr->iic_open(DEV_MASTER_MODE, IIC_SPEED_STANDARD);
  OLED_Init();
  OLED_Clear();
  OLED_SetCursor(0, 0);
  // Setting IIC for OLED
  char str_buf[50] = {0};
  while (1) {
    OLED_SetCursor(0, 0);
    sprintf(str_buf, "stick in %s", show_joystick_state());
    OLED_DisplayString_Flush(str_buf);
    board_delay_ms(10);
    OLED_SetCursor(1, 0);
    sprintf(str_buf, "stick btn : %d\n", get_joystick_btn(GPIO5));
    OLED_DisplayString_Flush(str_buf);
    board_delay_ms(100);
  }
  return 0;
}
