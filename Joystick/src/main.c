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

#include "SC16IS750_Bluepacket.h"

volatile static DEV_IIC_PTR dev_iic_m1_ptr;

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

  while (1) {

    printf("Now joystick in : %s\n", show_joystick_state());
    board_delay_ms(10);
    // GPIO5 for button:
    printf("Now joystick btn state : %d\n", get_joystick_btn(GPIO5));
    board_delay_ms(100);
  }
  return 0;
}
