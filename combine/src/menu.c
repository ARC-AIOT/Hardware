#include "menu.h"

menu initMenu(void (*eachLoop)()) {
  menu m = {0, {""}, optionSel, setOpt, eachLoop};
  return m;
};

void setOpt(struct __menu *m, char str[6][22], int optionNum) {
  memcpy(m->optionText, str, sizeof(char) * 6 * 22);
  m->optionNum = optionNum;
}

int optionSel(struct __menu m) {
  char strBuf[22] = "";
  int optionNum = m.optionNum;
  int optionPtr = 0;
  OLED_Clear();
  m.eachLoop();
  for (int i = 0; i < optionNum; i++) {
    OLED_SetCursor(i + 2, 0);
    sprintf(strBuf, "  %s", m.optionText[i]);
    OLED_DisplayString(strBuf);
  }

  while (1) {
    m.eachLoop();
    OLED_SetCursor(optionPtr + 2, 0); // First 2 line is reserved for showtime()
    sprintf(strBuf, " ");
    OLED_DisplayString(strBuf);
    optionPtr += get_joystick_state();
    if (optionPtr > (optionNum - 1)) // -1 : cnt from 0
      optionPtr = 0;
    if (optionPtr < 0)
      optionPtr = optionNum - 1;
    OLED_SetCursor(optionPtr + 2, 0); // First 2 line is reserved for showtime()
    sprintf(strBuf, ">");
    OLED_DisplayString(strBuf);

    if (get_joystick_btn(JoyBtn)) {
      OLED_Clear();
      return optionPtr;
    } else
      board_delay_ms(200);
  }
}