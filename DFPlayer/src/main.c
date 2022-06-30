#include <stdio.h>
#include <stdlib.h>
#include "embARC.h"
#include "embARC_debug.h"
#include "board_config.h"
#include "arc_timer.h"
#include "hx_drv_spi_s.h"
#include "spi_slave_protocol.h"
#include "hardware_config.h"

#include "hx_drv_iic_m.h"
#include "hx_drv_iomux.h"
#include "SC16IS750_Bluepacket.h"

#include "DFPlayer.h"
#define ARDUINO

// This example code shows how to Setup a DFPlayer
// And play mp3 files.
// Note that sometimes DFP sometimes have a collision with printf funct
// If the obj is created dynamically (via malloc)

int main(void) {

  HX_GPIOSetup();
  IRQSetup();
  UartInit(SC16IS750_PROTOCOL_SPI);

  board_delay_ms(1000); // Wait for setting SPI
  printf("Setting DFPlayer...\n");
  dfplayer Player = Init_DFPlayer();
  Player.set_vol(5);
  board_delay_ms(500);
  Player.play();
  board_delay_ms(500);
  int i = 0;
  while (1) {
    printf("HI %d\n", i);
    board_delay_ms(1000);
  }

  return 0;
}
