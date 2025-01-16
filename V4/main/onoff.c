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
#include "esp_mac.h"

#include "zb1.h"

static const char *TAG = V4TAG;

#ifdef USE_ISR
#include "switch_driver4.h"

static switch_func_pair_t button_func_pair[] = {
    //   {GPIO_NUM_9, SWITCH_ON_CONTROL},
    {GPIO_NUM_0, SWITCH_ONOFF_TOGGLE_CONTROL}};

static void zb_buttons_handler(switch_func_pair_t *button_func_pair)
{
    uint32_t pin = button_func_pair->pin;
    ESP_LOGI(TAG, "zb_buttons_handler %lu pin", pin);
    //    if (pin == GPIO_NUM_9)
    //        ESP_LOGI(TAG, "BOOT click");
    //   else if (pin == GPIO_NUM_0)
    {

        bool value = gpio_get_level(pin);
        if (value)
        {
            ESP_LOGI(TAG, "Sonar ON");
            light_driver_set_red(40);
            light_driver_set_green(0);
            light_driver_set_blue(0);
        }
        else
        {
            ESP_LOGI(TAG, "Sonar OFF");
            light_driver_set_red(0);
            light_driver_set_green(40);
            light_driver_set_blue(0);
        }
        light_driver_set_power(true);
    }
}

esp_err_t deferred_driver_init(void)
{
    static bool is_inited = false;
    if (!is_inited)
    {
        ESP_RETURN_ON_FALSE(
            switch_driver_init(button_func_pair, zb_buttons_handler),
            ESP_FAIL, TAG, "Failed to initialize switch driver");
        is_inited = true;
    }
    return is_inited ? ESP_OK : ESP_FAIL;
}

#endif