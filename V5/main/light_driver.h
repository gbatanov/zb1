
#ifndef LD_H
#define LD_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* light intensity level */
#define LIGHT_DEFAULT_ON  1
#define LIGHT_DEFAULT_OFF 0

// LED strip configuration 8 пин подключен к RGB светодиоду на плате
#define CONFIG_EXAMPLE_STRIP_LED_GPIO   8 
#define CONFIG_EXAMPLE_STRIP_LED_NUMBER 1

void light_driver_set_power(bool power);
void light_driver_set_green(uint8_t power);
void light_driver_set_red(uint8_t power);
void light_driver_set_blue(uint8_t power);
void light_driver_init(bool power);

#ifdef __cplusplus
} // extern "C"
#endif
#endif
