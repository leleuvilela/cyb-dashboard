#pragma once
#include <lvgl.h>
// Init TFT + LVGL + touch. Call once in setup().
void display_init();
// Pump LVGL — call often in loop().
void display_tick();
