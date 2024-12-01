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

#include "zb1.h"

extern int lcd_timeout;
extern uint8_t screen_number;

// Обработчик кнопки BOOT (одиночный клик)
// Только переключение экранов
void button_single_click_cb(void *arg, void *usr_data)
{
    ESP_LOGI("Button boot", "Single click");
#ifdef USE_DISPLAY
    lcd_timeout = 30;
    screen_number = screen_number + 1;
    if (screen_number == 3)
    {
        screen_number = 0;
    }
#endif
}

// Регистрация кнопок
void register_buttons()
{
    // Кнопка BOOT
    // create gpio button
    button_config_t gpio_btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,   // 1500ms
        .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS, // 180ms
        .gpio_button_config = {
            .gpio_num = GPIO_NUM_9, //  кнопка BOOT
            .active_level = 0,
        },
    };

    button_handle_t gpio_btn9 = iot_button_create(&gpio_btn_cfg);
    if (NULL == gpio_btn9)
    {
        ESP_LOGE("Button boot", "Button create failed");
    }

    iot_button_register_cb(gpio_btn9, BUTTON_SINGLE_CLICK, button_single_click_cb, NULL);
    //	iot_button_register_cb(gpio_btn, BUTTON_LONG_PRESS_START, button_long_press_cb, NULL);

    //  Выключатель люстры
    button_config_t gpio_btn_cfg0 = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,                                // 500ms
        .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS, // 180ms
        .gpio_button_config = {
            .gpio_num = GPIO_NUM_0, // выключатель люстры
            .active_level = 0,
        },
    };
    button_handle_t gpio_btn0 = iot_button_create(&gpio_btn_cfg0);
    if (NULL == gpio_btn0)
    {
        ESP_LOGE("Button luster", "Button create failed");
    }

    iot_button_register_cb(gpio_btn0, BUTTON_LONG_PRESS_START, luster_control, NULL);
    iot_button_register_cb(gpio_btn0, BUTTON_LONG_PRESS_UP, luster_control, NULL);

    // Датчик освещенности
    button_config_t gpio_btn_cfg2 = {
        .type = BUTTON_TYPE_GPIO,
        .short_press_time = 180, // 180ms
        .long_press_time = 1500,  // 1500ms
        .gpio_button_config = {
            .gpio_num = GPIO_NUM_2, // датчик на GPIO2
            .active_level = 0,
        },
    };
    button_handle_t gpio_btn2 = iot_button_create(&gpio_btn_cfg2);
    if (NULL == gpio_btn2)
    {
        ESP_LOGE("Sensor luminocity", "Create failed");
    }

    iot_button_register_cb(gpio_btn2, BUTTON_LONG_PRESS_START, luminocity_cb, NULL);
    iot_button_register_cb(gpio_btn2, BUTTON_LONG_PRESS_UP, luminocity_cb, NULL);

    // Датчик движения
    button_config_t gpio_btn_cfg3 = {
        .type = BUTTON_TYPE_GPIO,
        .short_press_time = 5, // 150ms
        .long_press_time = 10, // 500ms
        .gpio_button_config = {
            .gpio_num = GPIO_NUM_1, // датчик на GPIO1
            .active_level = 0,      //
        },
    };
    button_handle_t gpio_btn3 = iot_button_create(&gpio_btn_cfg3);
    if (NULL == gpio_btn3)
    {
        ESP_LOGE("Sensor motion", "Create failed");
    }

    //  gpio_set_pull_mode(GPIO_NUM_1, GPIO_PULLUP_ONLY);
    iot_button_register_cb(gpio_btn3, BUTTON_LONG_PRESS_START, motion_cb, NULL);
    iot_button_register_cb(gpio_btn3, BUTTON_LONG_PRESS_UP, motion_cb, NULL);
   // iot_button_register_cb(gpio_btn3, BUTTON_SINGLE_CLICK, motion_short_cb, NULL);
 
}
