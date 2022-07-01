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
#define JoyBtn 5 // Set gpio5 as JoyBtn pin
#define JoyVRx 6 // Set gpio6 as JoyVRx pin,
                 // mind we connected it to a
                 // digital pin, thus it would
                 // be treated as another btn
// For JoyStick

// For DFPlayer
#include "DFPlayer.h"
#define BUSY_PIN 1 // Busy pin is set to be GPIO1
// For DFPlayer

// For DFPlayer and JoyStick
#include "SC16IS750_Bluepacket.h"
// For DFPlayer and JoyStick

#define ADC_3021_DEV_ADDR 0x4f
#define REG_ADDR 0x7f

char str_buf[100] = {0}; // String buffer for OLED_Display
time_t begin_sec, now_sec;
clock_t clk_cnt_time; // The clk cnt time will be set to zero after every time
                      // the time is set
struct tm ti;
struct tm lastTimeTakeMed;
bool haveNextTime = false;

dfplayer Player;

void mainMenu();
void initMainMenu();
bool dectected = false;

void set_Hour_Min(int *hour, int *min);
void set_day(int year, int month, int *day);
void set_month(int *month);
void set_year(int *year);
void timeSetMenu();
void timeInit();
void showTime();
void medMonitor();
void initSelMenu();
void nextTimeSelMenu();
void textDetect();

int main(void) {
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
  // Setting GPIO for joystick and DFPlayer

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

  mainMenu();

  /*
          sprintf(str_buf, "now playing: %d", sound_track);
          OLED_DisplayString_Flush(str_buf);
          printf("now playing: %d\n", sound_track);
          while (1) {
            int i = get_joystick_state();
            bool playBusy = Player.isBusy(BUSY_PIN);
            if (get_joystick_btn(GPIO5)) {
              if (playBusy)
                Player.pause();
              else
                Player.play();
            }
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
            OLED_SetCursor(1, 0);
            if (playBusy) {
              sprintf(str_buf, "now status: play");
              OLED_DisplayString_Flush(str_buf);
              printf("now status: play\n");
            } else {
              sprintf(str_buf, "now status: pause");
              OLED_DisplayString_Flush(str_buf);
              printf("now status: pause\n");
            }
            board_delay_ms(50);
          }
  */
  return 0;
}

void initMainMenu() {
  OLED_Clear();
  showTime();
  OLED_SetCursor(2, 0);
  sprintf(str_buf, "  time setting");
  OLED_DisplayString(str_buf);

  OLED_SetCursor(3, 0);
  sprintf(str_buf, "  text detect");
  OLED_DisplayString(str_buf);

  OLED_SetCursor(4, 0);
  sprintf(str_buf, "  When to take med");
  OLED_DisplayString(str_buf);
}

void mainMenu() {
  // Init
  enum Option { O1 = 0, O2, O3 };
  enum Option optionPtr = O1;
  bool selBtn = false;
  initMainMenu();
  // Init
  while (1) {
    showTime();
    OLED_SetCursor(optionPtr + 2, 0); // First 2 line is reserved for showtime()
    sprintf(str_buf, " ");
    OLED_DisplayString(str_buf);

    optionPtr += get_joystick_state();
    if (optionPtr > O3)
      optionPtr = O1;
    if (optionPtr < O1)
      optionPtr = O3;

    OLED_SetCursor(optionPtr + 2, 0); // First 2 line is reserved for showtime()
    sprintf(str_buf, ">");
    OLED_DisplayString(str_buf);

    if (get_joystick_btn(JoyBtn)) {
      OLED_Clear();
      showTime();
      /*
      OLED_SetCursor(2, 0);
      sprintf(str_buf, "You select:");
      OLED_DisplayString_Flush(str_buf);
      OLED_SetCursor(3, 0);
      */
      switch (optionPtr) {
      case O1:
        timeSetMenu();
        break;
      case O2:
        textDetect();
        break;
      case O3:
        sprintf(str_buf, "When to take med");
        break;
      default:
        break;
      }
      OLED_SetCursor(7, 0);
      sprintf(str_buf, "<- back");
      OLED_DisplayString_Flush(str_buf);

      while (1) {
        if (get_joystick_btn(JoyVRx)) {
          initMainMenu();
          break;
        }
      }
    } else
      board_delay_ms(200);
  }
}

// Text detect functions
void textDetect() {
  enum dectectFreq { F1 = 1, F2, F3, F4 };
  enum dectectFreq freq = F1;
  // enum nextTime { T1 = 1, T2, T3, T4 };
  // enum nextTime nT = T1;
  /*
  freq = Some_AI_Detect_funct();
  */
  printf("play fold1 F1\n");
  // Player.playFoldNum(1, F1);
  Player.playFoldNum(1, F1);
  board_delay_ms(4000);

  /* 01/
  F1: 0001_辨識為_每日一次，飯後服用_wav;
  F2: 0002_辨識為_每日兩次，飯後服用_.wav
  F3: 0003_辨識為_每日三次，飯後服用_.wav
  F4: 0004_辨識為_每日四次，飯後、睡前服用_.wav
  */
  OLED_Clear();
  showTime();
  OLED_SetCursor(2, 0);
  sprintf(str_buf, "Put med into box");
  OLED_DisplayString_Flush(str_buf);
  Player.playFoldNum(4, 1);
  board_delay_ms(2000);

  /* 0004_Instruction/
  0001_請將藥袋放入盒內.wav
  */
  /* Wait for UltraSensor funct
  while(1){
    if(UltraSensor_detect())
      break;
    while (Player.isBusy(BUSY_PIN)) {}
    playFoldNum(4, 1);
  }
  */
  nextTimeSelMenu();
}

void nextTimeSelMenu() {
  enum nextTime { T1 = 1, T2, T3, T4 };
  enum nextTime nT = T1;
  printf("Into another funct\n");

  Player.playFoldNum(4, 2);
  board_delay_ms(2000);

  /* 0004_Instruction/
  0002_請選擇下次吃藥時間.wav
  */
  initSelMenu();
  while (1) {
    OLED_SetCursor(nT + 2, 0); // First 2 line is reserved for showtime()
    sprintf(str_buf, " ");
    OLED_DisplayString(str_buf);
    nT += get_joystick_state();

    if (nT > T4)
      nT = T1;
    if (nT < T1)
      nT = T4;
    OLED_SetCursor(nT + 2, 0);
    sprintf(str_buf, ">");
    OLED_DisplayString(str_buf);

    if (get_joystick_btn(JoyBtn))
      break;
    else
      board_delay_ms(200);
  }
  OLED_Clear();
  showTime();
  OLED_SetCursor(2, 0);
  sprintf(str_buf, "Next time to take med");
  OLED_DisplayString(str_buf);

  OLED_SetCursor(3, 0);
  switch (nT) {
  case T1:
    sprintf(str_buf, "After breakfast");
    break;
  case T2:
    sprintf(str_buf, "After lunch");
    break;
  case T3:
    sprintf(str_buf, "After dinner");
    break;
  case T4:
    sprintf(str_buf, "Before sleep");
    break;
  default:
    break;
  }
  OLED_DisplayString(str_buf);

  Player.playFoldNum(2, nT);
  board_delay_ms(2000);

  /* 0002_next_time/
  T1: 0001_下次吃藥時間早上，飯後服用.wav
  T2: 0002_下次吃藥時間中午，飯後服用.wav
  T3: 0003_下次吃藥時間晚上，飯後服用.wav
  T4: 0004_下次吃藥時間：睡前.wav
  */
  now_sec = begin_sec + (time_t)((clock() - clk_cnt_time) / CLOCKS_PER_SEC);
  ti = *(gmtime(&now_sec));
  lastTimeTakeMed = ti;
  haveNextTime = true;
}

void initSelMenu() {
  OLED_Clear();
  showTime();
  OLED_SetCursor(3, 0);
  sprintf(str_buf, "  After breakfast");
  OLED_DisplayString(str_buf);

  OLED_SetCursor(4, 0);
  sprintf(str_buf, "  After lunch");
  OLED_DisplayString(str_buf);

  OLED_SetCursor(5, 0);
  sprintf(str_buf, "  After dinner");
  OLED_DisplayString(str_buf);

  OLED_SetCursor(6, 0);
  sprintf(str_buf, "  Before sleep");
  OLED_DisplayString(str_buf);
}

// Medicine monitor funct
void medMonitor() {
  if (!haveNextTime)
    return;
  enum nextTime { T1 = 1, T2, T3, T4 };
  enum nextTime nT = T1;
  /*
  UltraSonic dectect med removed...
  */
  if (difftime(mktime(&ti), mktime(&lastTimeTakeMed)) <= 4 * 60 * 60) {
    OLED_Clear();
    showTime();
    OLED_SetCursor(2, 0);
    sprintf(str_buf, "You have taken medicine before!!");
    OLED_DisplayString_Flush(str_buf);
    playFoldNum(3, 1);
    board_delay_ms(2000);
    playFoldNum(3, 2);
    board_delay_ms(2000);
  } else {
    nextTimeSelMenu();
    /* Wait for UltraSensor funct
    while(1){
      if(UltraSensor_detect())
        break;
      playFoldNum(4, 1);
      board_delay_ms(2000);
    }
    */
  }
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
  *year = 2022;
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
  *month = 1;
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
  *day = 1;
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
  *hour = 0;
  *min = 0;
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
