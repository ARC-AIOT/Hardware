#include "UltraSonic.h"

void init_ultra() {
  GPIOSetPinMode(SC16IS750_PROTOCOL_SPI, CH_A, TrigPin, OUTPUT);
  GPIOSetPinMode(SC16IS750_PROTOCOL_SPI, CH_A, EchoPin, INPUT);
}

float detect_distance() {
  uint64_t startRec, endRec;
  startRec = endRec = 0;
  float cm = 0;
  GPIOSetPinState(SC16IS750_PROTOCOL_SPI, CH_A, TrigPin, LOW);
  board_delay_us(1);
  GPIOSetPinState(SC16IS750_PROTOCOL_SPI, CH_A, TrigPin, HIGH);
  board_delay_us(11);
  GPIOSetPinState(SC16IS750_PROTOCOL_SPI, CH_A, TrigPin, LOW);

  while (GPIOGetPinState(SC16IS750_PROTOCOL_SPI, CH_A, EchoPin) == LOW)
    startRec = board_get_cur_us();
  while (GPIOGetPinState(SC16IS750_PROTOCOL_SPI, CH_A, EchoPin) == HIGH)
    endRec = board_get_cur_us();

  uint64_t duration = endRec - startRec;
  // printf("start:%llu end:%llu duration: %llu\n", startRec, endRec, duration);
  cm = ((float)duration) / 2.;
  cm = cm * 0.034;
  // printf("%f cm\n", cm);
  return cm;
}

bool detect_obj(float cm, uint32_t repeat) {
  float avg = 0;
  int success = 0;
  for (int i = 0; i < repeat; ++i)
    if (detect_distance() <= cm) {
      success++;
      board_delay_ms(30);
    }
  return success > (repeat >> 1);
}
