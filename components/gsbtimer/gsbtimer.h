#ifndef GSB_TIMER_H
#define GSB_TIMER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"

void timer_callback(TimerHandle_t pxTimer);
void create_timer();
void start_timer();

#endif
