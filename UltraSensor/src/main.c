#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "embARC.h"
#include "embARC_debug.h"
#include "board_config.h"
#include "hardware_config.h"
#include "hx_drv_iic_m.h"
#include <time.h>
#include <errno.h>
#include <sys/time.h>

#include "hx_drv_iic_m.h"
#include "hx_drv_iomux.h"
#include "SC16IS750_Bluepacket.h"

#include "hx_drv_uart.h"
#define uart_buf_size 100

#include "Ultrasonic.h"

char uart_buf[uart_buf_size] = {0};

uint8_t temp_gpio_state = 0;

int main(void) {
  // UART 0 is already initialized with 115200bps
  printf("This is Lab2_SPI_Arduino_GPIO752\r\n");

  HX_GPIOSetup();
  IRQSetup();
  UartInit(SC16IS750_PROTOCOL_SPI);
  init_ultra();
  while (1) {
    if (detect_obj(7.0, 6))
      printf("Detect!!\n");
    else
      printf(":(\n");
    board_delay_ms(1000);
  }

  return 0;
}
