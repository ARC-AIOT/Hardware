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

#include "hx_drv_uart.h"
#define uart_buf_size 100

#include "hx_drv_iic_m.h"
#include "hx_drv_iomux.h"
#include "SC16IS750_Bluepacket.h"

#include "DFPlayer.h"

DEV_UART *uart0_ptr;
DEV_UART *DFP_dev; // UART Ptr of DFPlayer device

char uart_buf[uart_buf_size] = {0};
char str_buf[uart_buf_size] = {0};
char char_buf[10] = {0};
uint8_t char_cnt;
uint8_t uart_enter_flag;

int function_ret;
// This example code shows how to Setup a DFPlayer
// And play mp3 files.
int main(void) {

  HX_GPIOSetup();
  IRQSetup();
  UartInit(SC16IS750_PROTOCOL_SPI);

  board_delay_ms(1000);
  dfplayer *Player = Init_DFPlayer();
  printf("Setting DFPlayer...\n");

  Player->set_vol(15);
  board_delay_ms(500);
  Player->play();
  board_delay_ms(500);

  while (1) {
  }

  return 0;
}
