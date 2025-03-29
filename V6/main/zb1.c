// 2024 GSB zb1 v6.1.1
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
#include "driver/i2c_master.h"
#include "paj7620.h"

#define I2C_NUM I2C_NUM_0

SemaphoreHandle_t i2c_semaphore = NULL;
i2c_master_bus_handle_t bus_handle;
Dev_PAJ7620 devPaj7620;
#define INT_PIN_NUM GPIO_NUM_1

// #include "gsbtimer.h"

#include "zb1.h"

#if !defined ZB_ED_ROLE
#error Define ZB_ED_ROLE in idf.py menuconfig to compile light (End Device) source code.
#endif

#ifdef VLOG
const char *TAG = VTAG;
#endif

bool light_state = 0;   // светодиод на плате
bool connected = false; // подключен ли Zigbee

#if defined USE_TEMP_CHIP
int16_t temperature = -100;
float temp = 0;
bool temp_change = false;
#endif

// Инициализация шины. Должна быть одна для всех подключенных устройств.
static esp_err_t main_i2c_init()
{
    // Don't initialize twice
    if (i2c_semaphore != NULL)
        return ESP_FAIL;

    i2c_semaphore = xSemaphoreCreateMutex();
    if (i2c_semaphore == NULL)
        return ESP_FAIL;

    i2c_master_bus_config_t i2c_master_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT, // это не CONFIG_I2C_MASTER_FREQ_HZ !
        .glitch_ignore_cnt = 7,
        .i2c_port = I2C_NUM,
        .scl_io_num = CONFIG_SCL_GPIO,
        .sda_io_num = CONFIG_SDA_GPIO,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_master_config, &bus_handle));
    return ESP_OK;
}

void app_main(void)
{

    main_i2c_init();

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

    // xTaskCreate(TaskFunction, NameFunction, StackDepth, void* Parameters, Priority, TaskHandle)

#ifdef USE_TEMP_CHIP
    xTaskCreate(temp_chip_task, "temp_chip_task", 4096, NULL, 3, NULL);
#endif

    light_driver_init(LIGHT_ON);

    uint8_t mode = 1;  // 0-gesture, 1-proximity
    uint8_t speed = 0; // 0-normal, 1-gamiing
    i2c_bus_add_paj7620(&devPaj7620);
    devPaj7620.mode = mode;
    devPaj7620.speed = speed;
    devPaj7620.intPin = INT_PIN_NUM;
    esp_err_t ret = paj7620_init(&devPaj7620);
    if (ret == ESP_OK)
    {
    //    xTaskCreate(gesture_task, "gesture_task", 4096, &devPaj7620, 6, NULL);

        light_driver_set_green(45);
        light_driver_set_red(0);
        light_driver_set_blue(10);
    }
    else
    {
        light_driver_set_green(0);
        light_driver_set_red(45);
        light_driver_set_blue(10);
    }
    light_driver_set_power(true);
}
/*
// Обработчик кнопки BOOT (одиночный клик)
void button_single_click_cb(void *arg, void *usr_data)
{
    ESP_LOGI("Button boot", "Single click");
    take_photo();
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
}
    */