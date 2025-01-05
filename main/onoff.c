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

static const char *TAG = V0TAG;

#ifdef USE_ISR_BUTTON
#include "switch_driver0.h"

static switch_func_pair_t button_func_pair[] = {
    {GPIO_NUM_9, SWITCH_ON_CONTROL},
    {GPIO_NUM_0, SWITCH_ONOFF_TOGGLE_CONTROL}};

//    typedef struct
//    {
//        uint32_t pin;
//        switch_func_t func;
//    } switch_func_pair_t;
static void zb_buttons_handler(switch_func_pair_t *button_func_pair)
{
    uint32_t pin = button_func_pair->pin;
    ESP_LOGI(TAG, "zb_buttons_handler %lu pin", pin);
    if (pin == GPIO_NUM_9)
        ESP_LOGI(TAG, "BOOT click");
    else if (pin == GPIO_NUM_0)
    {
        bool value = gpio_get_level(pin);
        if (value) // логика инверсная, активный 0
            ESP_LOGI(TAG, "Switch to off");
        else
            ESP_LOGI(TAG, "Switch to on");
    }
}

esp_err_t deferred_driver_init(void)
{
    static bool is_inited = false;
    if (!is_inited)
    {
        ESP_RETURN_ON_FALSE(
            switch_driver_init(button_func_pair, PAIR_SIZE(button_func_pair), zb_buttons_handler),
            ESP_FAIL, TAG, "Failed to initialize switch driver");
        is_inited = true;
    }
    return is_inited ? ESP_OK : ESP_FAIL;
}

#else
#ifndef V1
// Обработчик кнопки BOOT (одиночный клик)
// Только переключение экранов
void button_single_click_cb(void *arg, void *usr_data)
{
    ESP_LOGI(TAG, "Single click");
#ifdef USE_DISPLAY
    lcd_timeout = 30;
    screen_number = screen_number + 1;
    if (screen_number == 3)
    {
        screen_number = 0;
    }
#endif
}
#endif

// Регистрация кнопок
void register_buttons()
{
    //  Выключатель люстры
    button_config_t gpio_btn_cfg0 = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,   // 1500ms
        .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS, // 180ms
        .gpio_button_config = {
            .gpio_num = GPIO_NUM_0, // выключатель люстры
            .active_level = 0,
        },
    };
    button_handle_t gpio_btn0 = iot_button_create(&gpio_btn_cfg0);
    if (NULL == gpio_btn0)
    {
        ESP_LOGE(TAG, "Button create failed");
    }

    iot_button_register_cb(gpio_btn0, BUTTON_LONG_PRESS_START, luster_control, NULL);
    iot_button_register_cb(gpio_btn0, BUTTON_LONG_PRESS_UP, luster_control, NULL);

#ifndef V1
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
        ESP_LOGE(TAG, "Button create failed");
    }

    iot_button_register_cb(gpio_btn9, BUTTON_SINGLE_CLICK, button_single_click_cb, NULL);
    //	iot_button_register_cb(gpio_btn, BUTTON_LONG_PRESS_START, button_long_press_cb, NULL);

    // Датчик освещенности
    button_config_t gpio_btn_cfg2 = {
        .type = BUTTON_TYPE_GPIO,
        .short_press_time = 180, // 180ms
        .long_press_time = 1500, // 1500ms
        .gpio_button_config = {
            .gpio_num = GPIO_NUM_2, // датчик на GPIO2
            .active_level = 0,
        },
    };
    button_handle_t gpio_btn2 = iot_button_create(&gpio_btn_cfg2);
    if (NULL == gpio_btn2)
    {
        ESP_LOGE(TAG, "Sensor /(motion) - create failed");
    }

    iot_button_register_cb(gpio_btn2, BUTTON_LONG_PRESS_START, motion_cb, NULL);
    iot_button_register_cb(gpio_btn2, BUTTON_LONG_PRESS_UP, motion_cb, NULL);

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
        ESP_LOGE(TAG, "Sensor motion - create failed");
    }

    //  gpio_set_pull_mode(GPIO_NUM_1, GPIO_PULLUP_ONLY);
    iot_button_register_cb(gpio_btn3, BUTTON_LONG_PRESS_START, motion_cb, NULL);
    iot_button_register_cb(gpio_btn3, BUTTON_LONG_PRESS_UP, motion_cb, NULL);
// iot_button_register_cb(gpio_btn3, BUTTON_SINGLE_CLICK, motion_short_cb, NULL);
#endif
}
#endif