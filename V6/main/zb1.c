// 2024 GSB zb1 v6.1.2
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

SemaphoreHandle_t blinkMutex = NULL;
static QueueHandle_t blink_evt_queue = NULL;

#define I2C_NUM I2C_NUM_0

SemaphoreHandle_t i2c_semaphore = NULL;
i2c_master_bus_handle_t bus_handle;
Dev_PAJ7620 devPaj7620;
#define INT_PIN_NUM GPIO_NUM_1

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

// count, red, green, blue
// count 0 - 3, 0 - просто включаем нужный цвет, защищенный аналог set_RGB()
void blink_task(void *arg)
{
    uint32_t crgb = 0;
    while (true)
    {
        if (xQueueReceive(blink_evt_queue, (void *)&crgb, portMAX_DELAY))
        {
            ESP_LOGI(TAG, "count %lu", crgb);
            if (xSemaphoreTake(blinkMutex, portMAX_DELAY) == pdTRUE)
            {
                // Здесь происходит защищенный доступ к ресурсу.
                uint32_t oldColor = get_RGB();

                set_RGB(crgb);
                uint8_t count = (uint8_t)(crgb >> 24);
                ESP_LOGI(TAG, "count %d", count);
                if (count > 0)
                {
                    light_driver_set_power(false);
                    vTaskDelay(250 / portTICK_PERIOD_MS);

                    for (int i = 0; i < count; i++)
                    {

                        light_driver_set_power(true);
                        vTaskDelay(250 / portTICK_PERIOD_MS);
                        light_driver_set_power(false);
                        vTaskDelay(250 / portTICK_PERIOD_MS);
                    }

                    set_RGB(oldColor);
                    light_driver_set_power(true);
                }
                // Освобождаем мьютекс.
                xSemaphoreGive(blinkMutex);
            }
        }
    }
}

void app_main(void)
{

    main_i2c_init();

    blinkMutex = xSemaphoreCreateMutex();
    if (blinkMutex == NULL)
        return;

    // создаем очередь для сообщений для мигания светодиодом
    blink_evt_queue = xQueueCreate(8, sizeof(uint32_t));
    if (blink_evt_queue == 0)
    {
        // ESP_LOGE(TAG, "Queue was not created and must not be used");
        return;
    }
    xTaskCreate(blink_task, "blink_task", 2048, NULL, 10, NULL);
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

    uint8_t mode = GESTURE_MODE;
    uint8_t speed = NORMAL_SPEED_MODE;
    i2c_bus_add_paj7620(&devPaj7620);
    devPaj7620.mode = mode;
    devPaj7620.speed = speed;
    devPaj7620.intPin = INT_PIN_NUM;
    esp_err_t ret = paj7620_init(&devPaj7620);
    if (ret != ESP_OK)
        return;

    if (true)
    {

        vTaskDelay(2000 / portTICK_PERIOD_MS);
        uint32_t tColor = (1 << 24) + (255 << 16); // 1 red
        xQueueSendToBack(blink_evt_queue, (void *)&tColor, (TickType_t)0);
    
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        tColor = (2 << 24) + (255 << 8); // 2 green
        xQueueSendToBack(blink_evt_queue, (void *)&tColor, (TickType_t)0);

        vTaskDelay(2000 / portTICK_PERIOD_MS);
        tColor = (3 << 24) + 255; // 3 blue
        xQueueSendToBack(blink_evt_queue, (void *)&tColor, (TickType_t)10);

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
