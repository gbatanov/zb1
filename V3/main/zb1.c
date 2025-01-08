// 2024 GSB zb1 v0.3.1
//

#include "settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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

#ifndef USE_ZIGBEE
extern const char *TAG ;
#endif

bool light_state = 0;   // светодиод на плате
bool connected = false; // подключен ли Zigbee

bool relay_state = false; // реле zb4
// актуальность состояния
bool relay_state_act = false; // реле zb4

#if defined USE_TEMP_CHIP
int16_t temperature = -100;
float temp = 0;
bool temp_change = false;
#endif

void get_current_state()
{
    relay_state = (bool)gpio_get_level(GPIO_NUM_12);
    relay_state_act = true;
#ifdef USE_ZIGBEE
    set_attribute();
#endif
}

// Управление реле ZB4 дежурного света на кухне
// Включается с координатора по датчику движения 
void relay_zb4_control(uint8_t value)
{
    gpio_set_level(GPIO_NUM_12, (uint32_t)value); //  Выводим его на GPIO12
    relay_state = (bool)value;
    relay_state_act = true;
#ifdef USE_ZIGBEE
    set_attribute();
#else
    ESP_LOGI(TAG, "Реле ZB4 %s", relay_state ? "включено" : "выключено");
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
    gpio_pad_select_gpio(GPIO_NUM_12);
    gpio_set_direction(GPIO_NUM_12, GPIO_MODE_OUTPUT); // GPIO12 - на Реле ZB4
 
    // xTaskCreate(TaskFunction, NameFunction, StackDepth, void* Parameters, Priority, TaskHandle)

#ifdef USE_TEMP_CHIP
    xTaskCreate(temp_chip_task, "temp_chip_task", 4096, NULL, 3, NULL);
#endif
    light_driver_init(LIGHT_DEFAULT_ON);
    light_driver_set_green(30);
    light_driver_set_red(6);
    light_driver_set_blue(12);
    light_driver_set_power(true);
}
