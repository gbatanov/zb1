#ifndef GSB_HC_SR04_H
#define GSB_HC_SR04_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_check.h"
#include "esp_log.h"
#include "driver/gpio.h" // для использования пинов на ввод/вывод
#include "rom/gpio.h"


static const int MAX_DISTANCE = 400;

#define TRIG_PIN_NUM GPIO_NUM_0
#define ECHO_PIN_NUM GPIO_NUM_1

typedef struct{
    int TRIG_PIN;
    int ECHO_PIN;

}HCSR04;

void sonar_task(void *pvParameters);

#endif