#pragma once

#include "driver/gpio.h"
#include "driver/gptimer.h"

#ifdef __cplusplus
extern "C"
{
#endif


#define GPIO_INPUT_LEVEL_ON 1
#define GPIO_INPUT_LEVEL_OFF 0
#define ESP_INTR_FLAG_DEFAULT 0

typedef void (*sonar_callback_t)(uint32_t pin);

esp_err_t sonar_init(uint32_t,sonar_callback_t);

#ifdef __cplusplus
} // extern "C"
#endif
