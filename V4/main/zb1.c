// 2024 GSB zb1 v0.4.3
//

#include "settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp_check.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "iot_button.h"
#include "driver/gpio.h" // для использования пинов на ввод/вывод
#include "rom/gpio.h"

#ifdef USE_TEMP_CHIP
#include "temp_chip.h"
#endif

#include "zb1.h"

#if !defined ZB_ED_ROLE
#error Define ZB_ED_ROLE in idf.py menuconfig to compile light (End Device) source code.
#endif

#ifdef V4_LOG
const char *TAG = V4TAG;
#endif

bool light_state = 0;   // светодиод на плате
bool connected = false; // подключен ли Zigbee

bool motion_state = false;  // УЗ датчик движения 
bool motion_state_act = false;  // УЗ датчик движения

#if  defined USE_TEMP_CHIP
int16_t temperature = -100;
float temp = 0;
bool temp_change = false;
#endif


void get_current_state()
{
#ifdef USE_ZIGBEE
    set_attribute();
#endif
}


void app_main(void)
{

#ifdef USE_ZIGBEE
    esp_zb_platform_config_t config = {
        .radio_config = {
            .radio_mode = ZB_RADIO_MODE_NATIVE,
        },
        .host_config = {
            .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE,
        },
    };
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));

    xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);
    xTaskCreate(update_attribute, "Update_attribute_value", 4096, NULL, 5, NULL);

#endif

    gpio_pad_select_gpio(ECHO_PIN_NUM);
    gpio_set_direction(ECHO_PIN_NUM, GPIO_MODE_INPUT); // ECHO pin - input

    ESP_LOGI(TAG, "Deferred driver initialization %s", deferred_driver_init() ? "failed" : "successful");


#ifdef USE_TEMP_CHIP
    xTaskCreate(temp_chip_task, "temp_chip_task", 4096, NULL, 3, NULL);
#endif

    light_driver_init(LIGHT_DEFAULT_ON);
    light_driver_set_green(1);
    light_driver_set_red(1);
    light_driver_set_blue(1);
    light_driver_set_power(true);

}
