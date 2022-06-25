#ifndef _ADC_JOYSTICK_H_
#define _ADC_JOYSTICK_H_
#include "hx_drv_iic_m.h"
/*
 *  1 - Up
 *  0 - Idle
 * -1 - Down
 */
int get_joystick_state();

// Print joystick state in String
char *show_joystick_state();
#endif