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
#include "temp_chip.h"

static const char *TAG = "GSB_ZB_1";

#if defined USE_BMP280 || defined USE_TEMP_CHIP
extern int16_t temperature;
#endif

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
        if ((tempInt16 > temperature && tempInt16 - temperature > 49) ||
            (tempInt16 < temperature && temperature - tempInt16 > 49))
        {
            temperature = tempInt16;
#ifdef USE_ZIGBEE
            reportAttribute(ZB1_ENDPOINT_1, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID, &temperature, 2);
#else
            ESP_LOGI(TAG, "Temperature value %.02f ℃", tsens_value);
#endif
        }

        vTaskDelay(30000 / portTICK_PERIOD_MS); // меряем раз в 30 секунд
    }
}
#endif