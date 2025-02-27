// 2025 GSB zb1 v5.0.3
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


// Инициализация шины. Должна быть одна для всех подключенных устройств.
static esp_err_t main_i2c_init()
{
    // Don't initialize twice
    if (i2c_semaphore != NULL)
        return ESP_FAIL;

    i2c_semaphore = xSemaphoreCreateMutex();
    if (i2c_semaphore == NULL)
        return ESP_FAIL;

    ESP_LOGI(TAG, "New i2c driver is used");

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

                    esp_err_t err = bmx280_readoutFloat(&bmp280_dev, &temp, &press, NULL);
                    if (err == ESP_OK)
                    {
                        ESP_LOGI(TAG, "Read Values: temp = %.1f press = %0.2f (%0.2f)", temp, press, press * 0.00750062);
                        int16_t tempInt16 = (int16_t)(temp * 100); // temp в сотых долях градуса
                        if ((tempInt16 > temperature && tempInt16 - temperature > 49) ||
                            (tempInt16 < temperature && temperature - tempInt16 > 49))
                        {
                            temperature = tempInt16;
#ifdef USE_ZIGBEE
                            reportAttribute(ZB1_ENDPOINT_1, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID, &temperature, 2);
#endif
                        }
                    }
                    else
                    {
                        ESP_LOGI(TAG, "Read Values error");
                    }
                }
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            } // while
        }
    }
    ESP_LOGI(TAG, "Device BMP280 - error");
    vTaskDelete(NULL);
}


void get_current_state()
{
#ifdef USE_ZIGBEE
    set_attribute();
#endif
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

    light_driver_init(LIGHT_DEFAULT_ON);
    light_driver_set_green(45);
    light_driver_set_red(10);
    light_driver_set_blue(20);
    light_driver_set_power(true);
}
