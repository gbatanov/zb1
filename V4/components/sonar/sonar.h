#pragma once

#include "driver/gpio.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define ECHO_PIN_NUM GPIO_NUM_1
#define GPIO_INPUT_LEVEL_ON 1
#define GPIO_INPUT_LEVEL_OFF 0
#define ESP_INTR_FLAG_DEFAULT 0

    typedef void (*esp_switch_callback_t)(uint32_t pin);

    bool switch_driver_init(uint32_t pin, esp_switch_callback_t cb);
    esp_err_t deferred_driver_init(void);

#ifdef __cplusplus
} // extern "C"
#endif
