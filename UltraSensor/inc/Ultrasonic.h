#include <time.h>
#include <sys/time.h>
#include "SC16IS750_Bluepacket.h"
#define TrigPin GPIO7
#define EchoPin GPIO6

bool detect_obj(float cm, uint32_t repeat);
float detect_distance();
void init_ultra();
