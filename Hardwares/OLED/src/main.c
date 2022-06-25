#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "embARC.h"
#include "embARC_debug.h"
#include "board_config.h"
#include "arc_timer.h"
#include "hx_drv_spi_s.h"
#include "spi_slave_protocol.h"
#include "hardware_config.h"

#include "hx_drv_iic_m.h"
#include "synopsys_i2c_oled1306.h"

#include "hx_drv_uart.h"
#define uart_buf_size 100

DEV_UART *uart0_ptr;
char uart_buf[uart_buf_size] = {0};

DEV_IIC *iic1_ptr;

char str_buf[50] = {0};
uint8_t oled_i = 0;
uint32_t msec = 0;
uint32_t sec = 0;

int main(void) {
  // UART 0 is already initialized with 115200bps
  printf("This is Lab2_I2C_OLED\r\n");

  uart0_ptr = hx_drv_uart_get_dev(USE_SS_UART_0);

  sprintf(uart_buf, "I2C0 Init\r\n");
  uart0_ptr->uart_write(uart_buf, strlen(uart_buf));
  board_delay_ms(10);

  iic1_ptr = hx_drv_i2cm_get_dev(USE_SS_IIC_1);
  iic1_ptr->iic_open(DEV_MASTER_MODE, IIC_SPEED_STANDARD);

  sprintf(uart_buf, "OLED Init\r\n");
  uart0_ptr->uart_write(uart_buf, strlen(uart_buf));
  board_delay_ms(10);
  OLED_Init();

  OLED_Clear();
  OLED_SetCursor(0, 0);
  OLED_DisplayString("Welcome To ARC EVK");

  OLED_SetCursor(1, 0);
  for (oled_i = 0; oled_i < 128; oled_i++)
    oledSendData(oled_i);

  OLED_SetCursor(2, 5);
  for (oled_i = 0; oled_i < 20; oled_i++)
    OLED_DisplayChar(0x20 + oled_i);

  OLED_SetCursor(3, 10);
  for (oled_i = 0; oled_i < 13; oled_i++)
    OLED_DisplayChar('A' + oled_i);

  OLED_SetCursor(4, 15);
  for (oled_i = 0; oled_i < 13; oled_i++)
    OLED_DisplayChar('A' + oled_i + 13);

  OLED_SetCursor(5, 20);
  for (oled_i = 0; oled_i < 13; oled_i++)
    OLED_DisplayChar('a' + oled_i);

  OLED_SetCursor(6, 25);
  for (oled_i = 0; oled_i < 13; oled_i++)
    OLED_DisplayChar('a' + oled_i + 13);
  board_delay_ms(1000);

  while (1) {
    sprintf(uart_buf, "While Loop\r\n");
    uart0_ptr->uart_write(uart_buf, strlen(uart_buf));

    sprintf(str_buf, "Time Cnt: %4d.%03d", sec, msec);

    OLED_SetCursor(7, 0);
    OLED_DisplayString(str_buf);
    if (sec % 30 == 0) {
      OLED_Clear_Page(3);
      OLED_SetCursor(3, 0);
      sprintf(str_buf, "Hello %4d", sec);
      OLED_DisplayString(str_buf);
    }
    msec = msec + 20;
    sec = sec + (msec / 1000);
    msec = msec % 1000;
    board_delay_ms(10);
  }
  return 0;
}
