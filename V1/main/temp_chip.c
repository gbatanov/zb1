#include "settings.h"
#ifdef USE_TEMP_CHIP
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
#include "driver/gpio.h"               // для использования пинов на ввод/вывод
#include "driver/temperature_sensor.h" // встроенный сенсор температуры чипа
#include "zb1.h"
#include "temp_chip.h"

static const char *TAG = "GSB_ZB_1";

extern int16_t temperature;
extern bool connected;

void temp_chip_task(void *pvParameters)
{
    temperature_sensor_handle_t temp_sensor = NULL;
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(20, 100);
    temperature_sensor_install(&temp_sensor_config, &temp_sensor);

    temperature_sensor_enable(temp_sensor);

    float tsens_value;

    while (1)
    {
        temperature_sensor_get_celsius(temp_sensor, &tsens_value);
        int16_t tempInt16 = (int16_t)(tsens_value * 100);
        temperature = tempInt16;
        ESP_LOGI(TAG, "Temperature value %.02f ℃", tsens_value);

        vTaskDelay(30000 / portTICK_PERIOD_MS); // меряем раз в 30 секунд
    }
}
#endif