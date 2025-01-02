// 2024 GSB zb1 v0.1.17
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
static const char *TAG = "GSB_ZB_1";
#endif

bool light_state = 0;   // светодиод на плате
bool connected = false; // подключен ли Zigbee

bool coridor_state = false; // реле света в коридоре bit 2
bool hall_state = false;    // реле света в прихожей bit 3

// актуальность состояния
bool all_actual = false;
bool coridor_state_act = false; // реле света в коридоре bit  6
bool hall_state_act = false;    // реле света в прихожей bit  7

uint16_t PresentValue = 0; // итоговое значение для отправки аттрибута в координатор

#if defined USE_TEMP_CHIP
int16_t temperature = -100;
float temp = 0;
bool temp_change = false;
#endif

void get_current_state()
{
    coridor_state = (bool)gpio_get_level(GPIO_NUM_13);
    hall_state = (bool)gpio_get_level(GPIO_NUM_14);
    coridor_state_act = true;
    hall_state_act = true;
#ifdef USE_ZIGBEE
    set_attribute();
#endif
}

// Управление реле дежурного света в коридоре
// Включается с координатора по датчику движения в коридоре
void coridor_light_control(uint8_t value)
{
    gpio_set_level(GPIO_NUM_13, (uint32_t)value); //  Выводим его на GPIO13
    coridor_state = (bool)value;
    coridor_state_act = true;
#ifdef USE_ZIGBEE
    set_attribute();
#else
    ESP_LOGI(TAG, "Реле света в коридоре %s", coridor_state ? "включено" : "выключено");
#endif
}

// Управление реле дежурного света в прихожей
// Включается по датчику движения в прихожей
void hall_light_control(uint8_t value)
{
    gpio_set_level(GPIO_NUM_14, (uint32_t)value); //  Выводим его на GPIO14
    hall_state = (bool)value;
    hall_state_act = true;
#ifdef USE_ZIGBEE
    set_attribute();
#else
    ESP_LOGI(TAG, "Реле света в прихожей %s", hall_state ? "включено" : "выключено");
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
    gpio_pad_select_gpio(GPIO_NUM_13);
    gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT); // GPIO13 - на Реле2 Коридор
    gpio_pad_select_gpio(GPIO_NUM_14);
    gpio_set_direction(GPIO_NUM_14, GPIO_MODE_OUTPUT); // GPIO14 - на Реле3 Прихожая

// xTaskCreate(TaskFunction, NameFunction, StackDepth, void* Parameters, Priority, TaskHandle)

#ifdef USE_TEMP_CHIP
    xTaskCreate(temp_chip_task, "temp_chip_task", 4096, NULL, 3, NULL);
#endif
    light_driver_init(LIGHT_DEFAULT_ON);
    light_driver_set_green(45);
    light_driver_set_red(10);
    light_driver_set_blue(20);
    light_driver_set_power(true);
}
