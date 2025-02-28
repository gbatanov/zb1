// 2025 GSB zb1 v5.1.1
// Датчик температуры и давления на балкон

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
#include "bmx280.h"

#ifdef USE_TEMP_CHIP
#include "temp_chip.h"
#endif

#include "zb1.h"

SemaphoreHandle_t i2c_semaphore = NULL;
i2c_master_bus_handle_t bus_handle;

#if !defined ZB_ED_ROLE
#error Define ZB_ED_ROLE in idf.py menuconfig to compile light (End Device) source code.
#endif

#ifdef V0_LOG
const char *TAG = V0TAG;
#endif

bool light_state = 0;   // светодиод на плате
bool connected = false; // подключен ли Zigbee

BMP280_t bmp280_dev;
int16_t temperature = -100;
float temp = 0;
bool temp_change = false;
int16_t pressure = 0;
float press = 0;
bool press_change = false;

#if defined USE_TEMP_CHIP
int16_t chip_temperature = -100;
float chip_temp = 0;
bool chip_temp_change = false;
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
#ifdef V0_LOG
    ESP_LOGI(TAG, "New i2c driver is used");
#endif
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .i2c_port = I2C_NUM,
        .scl_io_num = CONFIG_BMX280_SCL_GPIO,
        .sda_io_num = CONFIG_BMX280_SDA_GPIO,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));
    return ESP_OK;
}

// Задача получения температуры и давления
static void bmx280_task(void *pvParameters)
{

    i2c_bus_add_bmp280(&bmp280_dev, I2C_NUM_0);

    esp_err_t res = bmx280_init(&bmp280_dev);
    if (res == ESP_OK)
    {

        bmx280_config_t bmx_cfg = BMX280_DEFAULT_CONFIG;
        res = bmx280_configure(&bmp280_dev, &bmx_cfg);
        if (res == ESP_OK)
        {

            while (1)
            {
                esp_err_t res = bmx280_setMode(&bmp280_dev, BMX280_MODE_FORCE);
                if (res == ESP_OK)
                {
                    do
                    {
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                    } while (bmx280_isSampling(&bmp280_dev));

                    temp_change = false;
                    press_change = false;
                    esp_err_t err = bmx280_readoutFloat(&bmp280_dev, &temp, &press, NULL);
                    if (err == ESP_OK)
                    {
                        int16_t tempInt16 = (int16_t)(temp * 100);   // temp в сотых долях градуса
                        int16_t pressInt16 = (int16_t)(press / 100); // по спецификации передаем в 0.1kPa (1003, например)
#ifdef V0_LOG
                        ESP_LOGI(TAG, "Read Values: temperature = %.1f pressure = %.1f Pa, %d kPa*10", temp, press, pressInt16);
#endif
                        if (tempInt16 != temperature)
                        {
                            temperature = tempInt16;
                            temp_change = true;
                        }
                        if (pressInt16 != pressure)
                        {
                            pressure = pressInt16; // на приеме умножить на 100 и на коэффициентперевода паскалей в мм.рт.столба
                            press_change = true;
                        }
#ifdef USE_ZIGBEE
//                        if (temp_change || press_change)
//                            set_attribute();
#endif
                    }
                    else
                    {
#ifdef V0_LOG
                        ESP_LOGI(TAG, "Read Values error");
#endif
                    }
                }
                res = bmx280_setMode(&bmp280_dev, BMX280_MODE_SLEEP);
                vTaskDelay(30000 / portTICK_PERIOD_MS);
            } // while
        }
    }
#ifdef V0_LOG
    ESP_LOGI(TAG, "Device BMP280 - error");
#endif
    vTaskDelete(NULL);
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

    xTaskCreate(bmx280_task, "bmx280_task", 4096, NULL, 3, NULL);

#ifdef USE_TEMP_CHIP
    xTaskCreate(temp_chip_task, "temp_chip_task", 4096, NULL, 3, NULL);
#endif

    light_driver_init(LIGHT_DEFAULT_ON);
    light_driver_set_green(45);
    light_driver_set_red(10);
    light_driver_set_blue(20);
    light_driver_set_power(true);
}
