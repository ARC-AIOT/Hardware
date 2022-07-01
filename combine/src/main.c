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
// For JoyStick

// For DFPlayer
#include "DFPlayer.h"
// For DFPlayer

// For DFPlayer and JoyStick
#include "SC16IS750_Bluepacket.h"
// For DFPlayer and JoyStick

#define ADC_3021_DEV_ADDR 0x4f
#define REG_ADDR 0x7f

int main(void) {
  // Setting GPIO for joystick btn and DFPlayer
  HX_GPIOSetup();
  IRQSetup();

  UartInit(SC16IS750_PROTOCOL_SPI); // Make sure CH_A of funt UartInit in
                                    // SC16IS750_Bluepacket.c has been set up as
                                    // 9600 baud rate, which is DFPlayer's
                                    // working baud rate

  GPIOSetPinMode(SC16IS750_PROTOCOL_SPI, CH_A, GPIO5, INPUT);
  // Setting GPIO for joystick btn and DFPlayer

  // set up dfplayer
  dfplayer Player = Init_DFPlayer(); //　Construct an instance of obj dfplayer
  Player.set_vol(5);
  board_delay_ms(500);
  Player.play();
  board_delay_ms(500);
  int sound_track = 1;
  // set up dfplayer

  // Setting IIC for OLED
  DEV_IIC *iic1_ptr;
  iic1_ptr = hx_drv_i2cm_get_dev(USE_SS_IIC_1);
  iic1_ptr->iic_open(DEV_MASTER_MODE, IIC_SPEED_STANDARD);
  OLED_Init();
  OLED_Clear();
  OLED_SetCursor(0, 0);
  char str_buf[100] = {0}; // String buffer for OLED_Display;
  // Setting IIC for OLED

  sprintf(str_buf, "now playing: %d", sound_track);
  OLED_DisplayString_Flush(str_buf);
  printf("now playing: %d\n", sound_track);

  while (1) {

    int i = get_joystick_state();
    if (i) {
      sound_track += i;
      if (sound_track == 0)
        sound_track = 4;
      if (sound_track > 4)
        sound_track &= 0x03;

      Player.playNum(sound_track);
      board_delay_ms(500);
      sprintf(str_buf, "now playing: %d", sound_track);
      OLED_SetCursor(0, 0);
      OLED_DisplayString_Flush(str_buf);
      printf("now playing: %d\n", sound_track);
    }
  }
  return 0;
}
