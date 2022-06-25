
#include "joystick.h"
#include <string.h>
#define ADC_3021_DEV_ADDR 0x4f
enum State { Up = 0, Idle = 7, Down = 15 };
uint8_t rw_buf[4];
uint8_t joy_stick_dev_addr = ADC_3021_DEV_ADDR;

int get_joystick_state() {
  memset(rw_buf, '\0', sizeof(rw_buf));
  hx_drv_i2cm_read_data(SS_IIC_0_ID, joy_stick_dev_addr, rw_buf, 2);
  // printf("cur read: %d\n", rw_buf[0]);
  switch ((int)rw_buf[0]) {
  case Idle:
    return 0;
  case Up:
    return 1;
  case Down:
  default:
    return -1;
  }
}

char *show_joystick_state() {
  int state = get_joystick_state();
  switch (state) {
  case 0:
    return "Idle";
  case 1:
    return "Up";
  case -1:
  default:
    return "Down";
  }
}
