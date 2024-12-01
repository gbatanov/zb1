#include "settings.h"
#ifdef USE_DISPLAY
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
#include "ssd1306.h"
#include "zb1.h"

#ifdef USE_BMP280
extern int16_t temperature;
#endif

extern bool motion_state;
extern int lcd_timeout;
extern SSD1306_t ssd1306_dev;
extern bool connected;

// Задача вывода на диплей
void lcd_task(void *pvParameters)
{
    char line3[16] = {0};
    // Start lcd
    i2c_bus_add_ssd1306(&ssd1306_dev, I2C_NUM_0);
    ssd1306_init(&ssd1306_dev, 128, 64);
    ssd1306_clear_screen(&ssd1306_dev, false);
    ssd1306_contrast(&ssd1306_dev, 0xff);

    vTaskDelay(1500 / portTICK_PERIOD_MS);

    while (1)
    {
        bool reverse = false;
        if (connected)
        {
            ssd1306_clear_line(&ssd1306_dev, 3, false);
            strcpy(line3, "Connected");
        }
        else
        {
            strcpy(line3, "Not connected");
            reverse = true;
        }
        ssd1306_display_text(&ssd1306_dev, 3, line3, strlen(line3), reverse);

#ifdef USE_BMP280
        if (temperature > 0)
        {
            char temp_data_str[16] = {0};
            sprintf(temp_data_str, "Temp: %.1f", (float)temperature / 100);

            ssd1306_display_text(&ssd1306_dev, 4, temp_data_str, strlen(temp_data_str), false);
        }
#endif

        if (motion_state)
            ssd1306_display_text(&ssd1306_dev, 5, "Motion", strlen("No motion"), false);
        else
            ssd1306_display_text(&ssd1306_dev, 5, "No motion   ", strlen("No motion"), false);

        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}
#endif
