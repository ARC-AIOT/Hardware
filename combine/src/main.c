#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "embARC.h"
#include "embARC_debug.h"
#include "board_config.h"
//#include "arc_timer.h"
#include "hardware_config.h"
#include "hx_drv_iic_m.h"

// For HW timer
#include "hx_drv_timer.h"
#define US_TO_S 1000000 // Timer's unit is in us
#define US_TO_MS 1000   // Timer's unit is in us
// For HW timer

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
#define BUSY_PIN 1 // Busy pin is set to be GPIO1
// For DFPlayer

// For DFPlayer and JoyStick
#include "SC16IS750_Bluepacket.h"
// For DFPlayer and JoyStick

// For text detection
#include "synopsys_sdk_camera_drv.h"
#include "text_detection_tools.h"
// For text detection
#include "Ultrasonic.h"

#include "menu.h"

#define ADC_3021_DEV_ADDR 0x4f
#define REG_ADDR 0x7f

char str_buf[100] = {0}; // String buffer for OLED_Display
time_t begin_sec, now_sec;
clock_t clk_cnt_time; // The clk cnt time will be set to zero after every time
                      // the time is set
struct tm ti;
// struct tm lastTimeTakeMed;
bool haveNextTime = false;

dfplayer Player;

// main menu
void main_Menu();
void mainMenuEachLoop();
void timeSetMenu();
void textDetect();
void whenToTake();
// main menu

void set_Hour_Min(int *hour, int *min);
void set_day(int year, int month, int *day);
void set_month(int *month);
void set_year(int *year);
void timeInit();
void showTime();
void medMonitor();
void nextTimeSelMenu();
void showNextTimeToEat();
void readNextTimeToEat();
void getNextTime(int);
void waitNextTimeToEat();

extern uint32_t g_wdma2_baseaddr;
int8_t input_buf[32 * 640] = {0};
uint32_t arr_std[480] = {0};
uint32_t idx[11] = {0};
uint8_t image[640 * 480] = {0};
uint8_t output_img[32 * 640] = {0};
int test[10] = {0};

int main(void) {
  synopsys_camera_init();
  tflitemicro_algo_init();
  // Setting GPIO for joystick btn and DFPlayer
  HX_GPIOSetup();
  IRQSetup();

  UartInit(SC16IS750_PROTOCOL_SPI); // Make sure CH_A of funt UartInit in
                                    // SC16IS750_Bluepacket.c has been set up as
                                    // 9600 baud rate, which is DFPlayer's
                                    // working baud rate
  GPIOSetPinMode(SC16IS750_PROTOCOL_SPI, CH_A, BUSY_PIN, INPUT);
  GPIOSetPinMode(SC16IS750_PROTOCOL_SPI, CH_A, JoyBtn, INPUT);
  GPIOSetPinMode(SC16IS750_PROTOCOL_SPI, CH_A, JoyVRx, INPUT);

  init_ultra();

  // Setting GPIO for joystick, DFPlayer, and Ultrasonic

  // set up dfplayer
  Player = Init_DFPlayer(); //　Construct an instance of obj dfplayer
  Player.set_vol(15);
  board_delay_ms(100);
  // set up dfplayer

  // Setting IIC for OLED
  DEV_IIC *iic1_ptr;
  iic1_ptr = hx_drv_i2cm_get_dev(USE_SS_IIC_1);
  iic1_ptr->iic_open(DEV_MASTER_MODE, IIC_SPEED_STANDARD);
  OLED_Init();
  OLED_Clear();
  // Setting IIC for OLED
  timeInit();
  while (1)
    main_Menu();

  return 0;
}

menu mainMenu = {
    .optionNum = 3,
    .optionText = {"Time setting", "Text detect", "When to take med"},
    .sel = optionSel,
    .setOpt = setOpt,
    .eachLoop = mainMenuEachLoop,
    .renderOpt = renderOpt};

void main_Menu() {
  int sel = mainMenu.sel(mainMenu);
  showTime();
  switch (sel) {
  case 0:
    timeSetMenu();
    break;
  case 1:
    textDetect();
    break;
  case 2:
    whenToTake();
    break;
  default:
    break;
  }
  showTime();
  OLED_SetCursor(7, 0);
  sprintf(str_buf, "<- back");
  OLED_DisplayString_Flush(str_buf);
  while (1) {
    if (get_joystick_btn(JoyVRx)) {
      break;
    }
  }
}

void mainMenuEachLoop() {
  showTime();
  medMonitor();
}

void whenToTake() {
  if (haveNextTime) {
    showNextTimeToEat();
    readNextTimeToEat();
  } else {
    OLED_Clear();
    OLED_SetCursor(3, 0);
    sprintf(str_buf, "Please go to");
    OLED_DisplayString_Flush(str_buf);
    OLED_SetCursor(4, 0);
    sprintf(str_buf, "text detect first");
    OLED_DisplayString_Flush(str_buf);
    Player.playFoldNum(4, 3); // 003_請先進行文字辨識.wav
    board_delay_ms(2500);
  }
}

// Text detect functions
void textDetect() {
  enum detectFreq { F1 = 1, F2, F3, F4, F5 };
  enum detectFreq freq = F1;

  synopsys_camera_start_capture();
  board_delay_ms(100);
  uint8_t *img_ptr;
  uint32_t img_width = 640;
  uint32_t img_height = 480;
  img_ptr = (uint8_t *)g_wdma2_baseaddr;

  synopsys_camera_down_scaling(img_ptr, img_width, img_height, &image[0],
                               img_width, img_height);

  freq = text_detection(&image[0], &output_img[0], &arr_std[0], &idx[0],
                        &input_buf[0], &test[0]);
  printf("freq = %d\n", freq);
  int i;
  for (i = 0; i < 10; i++) {
    printf("%d ", idx[i]);
  }
  printf("\n");
  for (i = 0; i < 10; i++) {
    printf("%d ", test[i]);
  }
  printf("\n");
  if (freq == 0) {
    Player.playFoldNum(1, 5); //請重新辨識
    haveNextTime = false;
    board_delay_ms(4000);
    return;
  }
  Player.playFoldNum(1, freq);
  board_delay_ms(4000);

  nextTimeSelMenu();
}

void nextTimeMenuEachLoop() { showTime(); }
void nextTimeSelMenu() {
  Player.playFoldNum(4, 2);
  /* 0004_Instruction/
  0002_請選擇下次吃藥時間.wav
  */
  board_delay_ms(2500);
  showTime();
  menu nextTimeSelMenu = {.optionNum = 4,
                          .optionText = {"After breakfast", "After lunch",
                                         "After dinner", "Before sleep"},
                          .sel = optionSel,
                          .setOpt = setOpt,
                          .eachLoop = nextTimeMenuEachLoop};
  int nT = nextTimeSelMenu.sel(nextTimeSelMenu);
  OLED_Clear();
  showTime();
  OLED_SetCursor(2, 0);
  sprintf(str_buf, "Next time to take med");
  OLED_DisplayString(str_buf);
  OLED_SetCursor(3, 0);
  switch (nT) {
  case 0:
    sprintf(str_buf, "After breakfast");
    break;
  case 1:
    sprintf(str_buf, "After lunch");
    break;
  case 2:
    sprintf(str_buf, "After dinner");
    break;
  case 3:
    sprintf(str_buf, "Before sleep");
    break;
  default:
    break;
  }
  OLED_DisplayString(str_buf);

  Player.playFoldNum(2, nT + 1);
  board_delay_ms(2500);

  /* 0002_next_time/
  T1: 0001_下次吃藥時間早上，飯後服用.wav
  T2: 0002_下次吃藥時間中午，飯後服用.wav
  T3: 0003_下次吃藥時間晚上，飯後服用.wav
  T4: 0004_下次吃藥時間：睡前.wav
  */
  now_sec = begin_sec + (time_t)((clock() - clk_cnt_time) / CLOCKS_PER_SEC);
  ti = *(gmtime(&now_sec));
  // lastTimeTakeMed = ti;
  getNextTime(nT);
  haveNextTime = true;
  OLED_Clear();
  showTime();
  OLED_SetCursor(2, 0);
  sprintf(str_buf, "Put med into box");
  OLED_DisplayString_Flush(str_buf);
  while (1) {
    if (detect_obj(5, 6))
      break;
    Player.playFoldNum(4, 1);
    board_delay_ms(3000);
  }
}

time_t tmpNextSec;
struct tm tmpNextTm;

// Medicine monitor funct
void medMonitor() {
  if (!haveNextTime || detect_obj(10, 6))
    return;
  if (mktime(&ti) < tmpNextSec) {
    waitNextTimeToEat();
  } else {
    nextTimeSelMenu();
    mainMenu.renderOpt(mainMenu);
  }
}

void getNextTime(int nT) {
  struct tm tmpTm;
  tmpTm = ti;
  switch (nT + 1) {
  case 1:
    tmpTm.tm_hour = 8;
    tmpTm.tm_min = 30;
    tmpTm.tm_sec = 0;
    break;
  case 2:
    tmpTm.tm_hour = 12;
    tmpTm.tm_min = 0;
    tmpTm.tm_sec = 0;
    break;
  case 3:
    tmpTm.tm_hour = 18;
    tmpTm.tm_min = 0;
    tmpTm.tm_sec = 0;
    break;
  case 4:
    tmpTm.tm_hour = 22;
    tmpTm.tm_min = 30;
    tmpTm.tm_sec = 0;
    break;
  }
  tmpNextSec = mktime(&tmpTm);
  tmpNextTm = *(gmtime(&tmpNextSec));
}

void readNextTimeToEat() {
  Player.playFoldNum(3, 3); // 你下次的吃藥時間應在
  board_delay_ms(2500);
  if (tmpNextTm.tm_hour < 12)
    Player.playFoldNum(7, 1); // 上午
  else if (tmpNextTm.tm_hour == 12)
    Player.playFoldNum(7, 2); // 中午
  else
    Player.playFoldNum(7, 3); // 下午
  board_delay_ms(900);
  int Hr = tmpNextTm.tm_hour;
  if (Hr > 12)
    Hr -= 12;
  if (Hr == 0)
    Hr = 12;
  Player.playFoldNum(5, Hr);
  board_delay_ms(900);
  if (tmpNextTm.tm_min > 10) {
    Player.playFoldNum(8, (tmpNextTm.tm_min / 10));
    board_delay_ms(900);
    if (tmpNextTm.tm_min % 10 == 0)
      Player.playFoldNum(8, 6);
    else
      Player.playFoldNum(6, tmpNextTm.tm_min % 10);
    board_delay_ms(900);

  } else if (tmpNextTm.tm_min == 10) {
    Player.playFoldNum(6, 10);
  } else {
    Player.playFoldNum(6, 11);
    board_delay_ms(900);
    Player.playFoldNum(6, tmpNextTm.tm_min % 10);
  }
  if (tmpNextTm.tm_min == 0) {
    Player.playFoldNum(8, 6); //分
    board_delay_ms(900);
  } else
    board_delay_ms(900);
}

void showNextTimeToEat() {
  OLED_Clear();
  showTime();
  OLED_SetCursor(2, 0);
  sprintf(str_buf, "You have already");
  OLED_DisplayString_Flush(str_buf);
  OLED_SetCursor(3, 0);
  sprintf(str_buf, "taken med before");
  OLED_DisplayString_Flush(str_buf);
  OLED_SetCursor(4, 0);

  sprintf(str_buf, "Next time to take:");

  OLED_DisplayString_Flush(str_buf);
  OLED_SetCursor(5, 0);
  sprintf(str_buf, "%02d:%02d", tmpNextTm.tm_hour, tmpNextTm.tm_min);
  OLED_DisplayString_Flush(str_buf);
}

void waitNextTimeToEat() {
  Player.playFoldNum(3, 1); // 你已經吃過藥了
  board_delay_ms(2500);
  readNextTimeToEat();
  Player.playFoldNum(3, 2);
  board_delay_ms(2500);
}

// Time set functions
// The time is saved in format struct tm
// tm's structure:
// struct tm {
//   int tm_sec;         /* seconds,  range 0 to 59          */
//   int tm_min;         /* minutes, range 0 to 59           */
//   int tm_hour;        /* hours, range 0 to 23             */
//   int tm_mday;        /* day of the month, range 1 to 31  */
//   int tm_mon;         /* month, range 0 to 11             */
//   int tm_year;        /* The number of years since 1900   */
//   int tm_wday;        /* day of the week, range 0 to 6    */
//   int tm_yday;        /* day in the year, range 0 to 365  */
//   int tm_isdst;       /* daylight saving time             */
// };
// Notice that the tm_year will be true value - 1900
void showTime() {
  now_sec = begin_sec + (time_t)((clock() - clk_cnt_time) / CLOCKS_PER_SEC);
  ti = *(gmtime(&now_sec));

  char time_buf[30];
  OLED_SetCursor(0, 0);
  sprintf(time_buf, "%s", asctime(&ti));
  sprintf(str_buf, "%.10s %d", time_buf, ti.tm_year + 1900);
  OLED_DisplayString_Flush(str_buf);
  OLED_SetCursor(1, 0);
  sprintf(str_buf, "%02d:%02d", ti.tm_hour, ti.tm_min);
  OLED_DisplayString_Flush(str_buf);
}

void timeInit() {
  ti.tm_year = 2022 - 1900; // year since 1900
  ti.tm_mon = 0;            // 0~11, July is 6
  ti.tm_mday = 1;
  ti.tm_hour = 0;
  ti.tm_min = 0;
  ti.tm_sec = 0;
  begin_sec = mktime(&ti);
  clk_cnt_time = clock();
}

void timeSetMenu() {
  int year = ti.tm_year + 1900;
  int month = ti.tm_mon + 1;
  int day = ti.tm_mday;
  int hour = ti.tm_hour;
  int min = ti.tm_min;
  set_year(&year);
  set_month(&month);
  set_day(year, month, &day);
  set_Hour_Min(&hour, &min);
  ti.tm_year = year - 1900;
  ti.tm_mon = month - 1;
  ti.tm_mday = day;
  ti.tm_min = min;
  ti.tm_hour = hour;
  begin_sec = mktime(&ti);
  clk_cnt_time = clock();
}

void set_year(int *year) {
  OLED_Clear();
  while (1) {
    OLED_SetCursor(2, 0);
    sprintf(str_buf, "Years:");
    OLED_DisplayString(str_buf);
    OLED_SetCursor(3, 0);
    sprintf(str_buf, "%d", *year);
    OLED_DisplayString(str_buf);
    *year -= get_joystick_state();
    if (*year < 1900 || *year > 9999)
      *year = 1900;
    board_delay_ms(200);
    if (get_joystick_btn(JoyBtn))
      break;
  }
}

void set_month(int *month) {
  OLED_Clear();
  while (1) {
    OLED_SetCursor(2, 0);
    sprintf(str_buf, "Month:");
    OLED_DisplayString(str_buf);
    OLED_SetCursor(3, 0);
    sprintf(str_buf, "%02d", *month);
    OLED_DisplayString(str_buf);
    *month -= get_joystick_state();
    if (*month < 1)
      *month = 12;
    else if (*month > 12) {
      *month = 1;
    }
    board_delay_ms(200);
    if (get_joystick_btn(JoyBtn))
      break;
  }
}

void set_day(int year, int month, int *day) {
  OLED_Clear();
  bool isSmall = (month == 4) || (month == 6) || (month == 9) || (month == 11);
  bool isLeap = (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0));
  while (1) {
    OLED_SetCursor(2, 0);
    sprintf(str_buf, "Day:");
    OLED_DisplayString(str_buf);
    OLED_SetCursor(3, 0);
    sprintf(str_buf, "%02d", *day);
    OLED_DisplayString(str_buf);
    *day -= get_joystick_state();
    if (month == 2) {
      if (isLeap) {
        if (*day < 1)
          *day = 29;
        else if (*day > 29)
          *day = 1;
      } else {
        if (*day < 1)
          *day = 28;
        else if (*day > 28)
          *day = 1;
      }
    } else if (isSmall) {
      if (*day < 1)
        *day = 30;
      else if (*day > 30)
        *day = 1;
    } else {
      if (*day < 1)
        *day = 31;
      else if (*day > 31)
        *day = 1;
    }
    board_delay_ms(200);
    if (get_joystick_btn(JoyBtn))
      break;
  }
}

void set_Hour_Min(int *hour, int *min) {
  OLED_Clear();
  OLED_SetCursor(2, 0);
  sprintf(str_buf, "Time:");
  OLED_DisplayString(str_buf);
  OLED_SetCursor(3, 0);
  sprintf(str_buf, "%02d:%02d", *hour, *min);
  OLED_DisplayString(str_buf);

  while (1) {
    *hour -= get_joystick_state();
    if (*hour < 0)
      *hour = 23;
    else if (*hour > 23)
      *hour = 0;
    OLED_SetCursor(3, 0);
    sprintf(str_buf, "  :%02d", *min);
    OLED_DisplayString(str_buf);
    board_delay_ms(100);

    OLED_SetCursor(3, 0);
    sprintf(str_buf, "%02d:%02d", *hour, *min);
    OLED_DisplayString(str_buf);
    board_delay_ms(100);
    if (get_joystick_btn(JoyBtn))
      break;
  }
  while (1) {
    *min -= get_joystick_state();
    if (*min < 0)
      *min = 59;
    else if (*min > 59)
      *min = 0;
    OLED_SetCursor(3, 0);
    sprintf(str_buf, "%02d:  ", *hour);
    OLED_DisplayString(str_buf);
    board_delay_ms(100);

    OLED_SetCursor(3, 0);
    sprintf(str_buf, "%02d:%02d", *hour, *min);
    OLED_DisplayString(str_buf);
    board_delay_ms(100);
    if (get_joystick_btn(JoyBtn))
      break;
  }
}