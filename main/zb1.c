// 2024 GSB zb1 v0.2.4
//

#define I2C_NUM I2C_NUM_0

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

#ifdef USE_I2C
#include "driver/i2c_master.h"
SemaphoreHandle_t i2c_semaphore = NULL;
i2c_master_bus_handle_t bus_handle;
#endif

#ifdef USE_DISPLAY
#include "ssd1306.h"
#endif

#ifdef USE_BMP280
#include "bmx280.h"
#endif

#include "zb1.h"

#if !defined ZB_ED_ROLE
#error Define ZB_ED_ROLE in idf.py menuconfig to compile light (End Device) source code.
#endif

static const char *TAG = "GSB_ZB_1";
bool light_state = 0;    // светодиод на плате
bool connected = false;  // подключен ли Zigbee
int8_t luminocity = -1;  // датчик освещенности
bool lum_change = false; // изменение состояния датчика освещенности

bool motion_state = false;  // датчик движения
bool mot_change = false;    // изменение состояния датчика движения
bool luster_state = false;  // реле люстры
bool coridor_state = false; // реле света в коридоре
bool hall_state = false;    // реле света в прихожей

SemaphoreHandle_t motion_semaphore = NULL;

#ifdef USE_DISPLAY
int lcd_timeout = 60;
uint8_t screen_number = 0;
SSD1306_t ssd1306_dev;
#endif

#ifdef USE_BMP280
BMP280_t bmp280_dev;
int16_t temperature = -100;
float temp = 0;
bool temp_change = false;
#endif

#ifdef USE_I2C
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
        .scl_io_num = CONFIG_SCL_GPIO,
        .sda_io_num = CONFIG_SDA_GPIO,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));
    return ESP_OK;
}
#endif
// Задача получения температуры
#ifdef USE_BMP280
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
                        vTaskDelay(500 / portTICK_PERIOD_MS);
                    } while (bmx280_isSampling(&bmp280_dev));

                    esp_err_t err = bmx280_readoutFloat(&bmp280_dev, &temp, NULL, NULL);
                    if (err == ESP_OK)
                    {
                        // ESP_LOGI(TAG, "Read Values: temp = %.1f", temp);
                        int16_t tempInt16 = (int16_t)(temp * 100);
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
#endif

// Получение уровня освещения
// Это значение наружу не отдаем, используем только для принятия решения
// включить или нет дежурный свет
// TODO: закомментировать, ориентироваться на включение люстры
void luminocity_cb(void *arg, void *usr_data)
{
    // для тестов заменю датчик движения
    motion_cb(arg, usr_data);
    /*
    int8_t lum = gpio_get_level(GPIO_NUM_2);
    ESP_LOGI(TAG, "New luminicity %d", lum);
    if (lum != luminocity)
    {
        lum_change = true;
        luminocity = lum;
    }
    */
}
// Получение сигнала с датчика движения
void motion_cb(void *arg, void *usr_data)
{
    ///    int8_t mot = gpio_get_level(GPIO_NUM_1);// для тестов заменю датчик движения
    bool mot = (bool)gpio_get_level(GPIO_NUM_2);
    ESP_LOGI(TAG, "New motion state %d", mot);
    if (mot != motion_state)
    {
        mot_change = true;
        motion_state = mot;
        hall_light_control((uint8_t)motion_state);
#ifdef USE_ZIGBEE
        if (connected)
        {
            esp_zb_zcl_status_t state = ESP_ZB_ZCL_STATUS_SUCCESS;
            // состояние датчика движения будем учитывать с другими датчиками движения
            send_onoff_cmd(ZB1_ENDPOINT_4, (uint8_t)motion_state);
            esp_zb_lock_acquire(portMAX_DELAY);
            state = esp_zb_zcl_set_attribute_val(ZB1_ENDPOINT_4, ESP_ZB_ZCL_CLUSTER_ID_ON_OFF, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, 0, &motion_state, false);
            esp_zb_lock_release();
            if (state != ESP_ZB_ZCL_STATUS_SUCCESS)
            {
                ESP_LOGI(TAG, "Setting attribute :failed! 0x%04x", state);
            }
            esp_zb_lock_acquire(portMAX_DELAY);
            state = esp_zb_zcl_set_attribute_val(ZB1_ENDPOINT_3, ESP_ZB_ZCL_CLUSTER_ID_ON_OFF, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, 0, &hall_state, false);
            esp_zb_lock_release();
            if (state != ESP_ZB_ZCL_STATUS_SUCCESS)
            {
                ESP_LOGI(TAG, "Setting attribute :failed! 0x%04x", state);
            }
        }
#endif
    }
}

// Управлением реле люстры
void luster_control_remote()
{
    luster_control(NULL, NULL);
}
void luster_control(void *arg, void *usr_data)
{
    luster_state = !luster_state;                        // Инвертируем текущее состояние
    gpio_set_level(GPIO_NUM_12, (uint32_t)luster_state); //  Выводим его на GPIO12
#ifdef USE_ZIGBEE
    if (connected)
    {
        uint8_t value = (uint8_t)!luster_state;
        esp_zb_zcl_status_t state = ESP_ZB_ZCL_STATUS_SUCCESS;
        esp_zb_lock_acquire(portMAX_DELAY);
        state = esp_zb_zcl_set_attribute_val(ZB1_ENDPOINT_1, ESP_ZB_ZCL_CLUSTER_ID_ON_OFF, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, 0, &value, false);
        esp_zb_lock_release();
        if (state != ESP_ZB_ZCL_STATUS_SUCCESS)
        {
            ESP_LOGI(TAG, "Setting attribute :failed! 0x%04x", state);
        }
    }
#endif
    ESP_LOGI(TAG, "Реле люстры %s", luster_state ? "выключено" : "включено");
}
// Управление реле дежурного света в коридоре
// Включается с координатора по датчику движения в коридоре
void coridor_light_control(uint8_t val)
{
    uint8_t value = 1 - val;                      // Включение нулем, поэтому инвертируем
    gpio_set_level(GPIO_NUM_13, (uint32_t)value); //  Выводим его на GPIO13
    coridor_state = (bool)value;
#ifdef USE_ZIGBEE
    if (connected)
    {
        esp_zb_zcl_status_t state = ESP_ZB_ZCL_STATUS_SUCCESS;
        esp_zb_lock_acquire(portMAX_DELAY);
        state = esp_zb_zcl_set_attribute_val(ZB1_ENDPOINT_2, ESP_ZB_ZCL_CLUSTER_ID_ON_OFF, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, 0, &value, false);
        esp_zb_lock_release();
        if (state != ESP_ZB_ZCL_STATUS_SUCCESS)
        {
            ESP_LOGI(TAG, "Setting attribute :failed! 0x%04x", state);
        }
    }
#endif
    ESP_LOGI(TAG, "Реле света в коридоре %s", coridor_state ? "выключено" : "включено");
}

// Управление реле дежурного света в прихожей
// Включается по датчику движения в этом устройстве
void hall_light_control(uint8_t val)
{
    uint8_t value = 1 - val;                      // Включение нулем, поэтому инвертируем
    gpio_set_level(GPIO_NUM_14, (uint32_t)value); //  Выводим его на GPIO14
    hall_state = (bool)val;
    // команду не посылаем, состояние света получим через аттрибут
#ifdef USE_ZIGBEE
    if (connected)
    {
        esp_zb_zcl_status_t state = ESP_ZB_ZCL_STATUS_SUCCESS;
        esp_zb_lock_acquire(portMAX_DELAY);
        state = esp_zb_zcl_set_attribute_val(ZB1_ENDPOINT_3, ESP_ZB_ZCL_CLUSTER_ID_ON_OFF, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, 0, &value, false);
        esp_zb_lock_release();
        if (state != ESP_ZB_ZCL_STATUS_SUCCESS)
        {
            ESP_LOGI(TAG, "Setting attribute :failed! 0x%04x", state);
        }
    }
#endif

    ESP_LOGI(TAG, "Реле света в прихожей %s", hall_state ? "выключено" : "включено");
}

void app_main(void)
{
    register_buttons();
#ifdef USE_I2C
    main_i2c_init();
#endif
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
    gpio_set_direction(GPIO_NUM_12, GPIO_MODE_OUTPUT); // GPIO12 - на Реле1 Люстра
    gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT); // GPIO13 - на Реле2 Коридор
    gpio_set_direction(GPIO_NUM_14, GPIO_MODE_OUTPUT); // GPIO14 - на Реле3 Прихожая

// xTaskCreate(TaskFunction, NameFunction, StackDepth, void* Parameters, Priority, TaskHandle)
#ifdef USE_DISPLAY
    xTaskCreate(lcd_task, "lcd_task", 4096, NULL, 2, NULL);
#endif
#ifdef USE_BMP280
    xTaskCreate(bmx280_task, "bmx280_task", 4096, NULL, 3, NULL);
#endif
    light_driver_init(LIGHT_DEFAULT_ON);
    light_driver_set_green(45);
    light_driver_set_red(10);
    light_driver_set_blue(20);
    light_driver_set_power(true);
}