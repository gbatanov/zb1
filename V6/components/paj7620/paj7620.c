// PAJ7620 Gesture Sensor i2c esp32
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_check.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "driver/i2c_master.h"
#include "paj7620.h"

// Register values for sensor initialization.
int init_register_array[][2] = {
    {0xEF, 0x00},
    {0x37, 0x07},
    {0x38, 0x17},
    {0x39, 0x06},
    {0x41, 0x00},
    {0x42, 0x00},
    {0x46, 0x2D},
    {0x47, 0x0F},
    {0x48, 0x3C},
    {0x49, 0x00},
    {0x4A, 0x1E},
    {0x4C, 0x20},
    {0x51, 0x10},
    {0x5E, 0x10},
    {0x60, 0x27},
    {0x80, 0x42},
    {0x81, 0x44},
    {0x82, 0x04},
    {0x8B, 0x01},
    {0x90, 0x06},
    {0x95, 0x0A},
    {0x96, 0x0C},
    {0x97, 0x05},
    {0x9A, 0x14},
    {0x9C, 0x3F},
    {0xA5, 0x19},
    {0xCC, 0x19},
    {0xCD, 0x0B},
    {0xCE, 0x13},
    {0xCF, 0x64},
    {0xD0, 0x21},
    {0xEF, 0x01},
    {0x02, 0x0F},
    {0x03, 0x10},
    {0x04, 0x02},
    {0x25, 0x01},
    {0x27, 0x39},
    {0x28, 0x7F},
    {0x29, 0x08},
    {0x3E, 0xFF},
    {0x5E, 0x3D},
    {0x65, 0x96},
    {0x67, 0x97},
    {0x69, 0xCD},
    {0x6A, 0x01},
    {0x6D, 0x2C},
    {0x6E, 0x01},
    {0x72, 0x01},
    {0x73, 0x35},
    {0x74, 0x00},
    {0x77, 0x01},
};

// Register values for gesture mode initialization.
int init_gesture_array[][2] = {
    {0xEF, 0x00},
    {0x41, 0x00},
    {0x42, 0x00},
    {0xEF, 0x00},
    {0x48, 0x3C},
    {0x49, 0x00},
    {0x51, 0x10},
    {0x83, 0x20},
    {0x9F, 0xF9},
    {0xEF, 0x01},
    {0x01, 0x1E},
    {0x02, 0x0F},
    {0x03, 0x10},
    {0x04, 0x02},
    {0x41, 0x40},
    {0x43, 0x30},
    {0x65, 0x96},
    {0x66, 0x00},
    {0x67, 0x97},
    {0x68, 0x01},
    {0x69, 0xCD},
    {0x6A, 0x01},
    {0x6B, 0xB0},
    {0x6C, 0x04},
    {0x6D, 0x2C},
    {0x6E, 0x01},
    {0x74, 0x00},
    {0xEF, 0x00},
    {0x41, 0xFF},
    {0x42, 0x01},
};

// Register values for proximity mode initialization.
int init_ps_array[][2] = {
    {0xEF, 0x00},
    {0x41, 0x00},
    {0x42, 0x00},
    {0x48, 0x3C},
    {0x49, 0x00},
    {0x51, 0x13},
    {0x83, 0x20},
    {0x84, 0x20},
    {0x85, 0x00},
    {0x86, 0x10},
    {0x87, 0x00},
    {0x88, 0x05},
    {0x89, 0x18},
    {0x8A, 0x10},
    {0x9f, 0xf8},
    {0x69, 0x96},
    {0x6A, 0x02},
    {0xEF, 0x01},
    {0x01, 0x1E},
    {0x02, 0x0F},
    {0x03, 0x10},
    {0x04, 0x02},
    {0x41, 0x50},
    {0x43, 0x34},
    {0x65, 0xCE},
    {0x66, 0x0B},
    {0x67, 0xCE},
    {0x68, 0x0B},
    {0x69, 0xE9},
    {0x6A, 0x05},
    {0x6B, 0x50},
    {0x6C, 0xC3},
    {0x6D, 0x50},
    {0x6E, 0xC3},
    {0x74, 0x05},
};

SemaphoreHandle_t print_mux = NULL;

extern i2c_master_bus_handle_t bus_handle;
i2c_master_dev_handle_t dev_handle;

//   Read a sequence of bytes from a gesture registers
static esp_err_t gesture_register_read(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

//   Write a byte to a gesture registers
static esp_err_t gesture_register_write_byte(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

esp_err_t gesture_register_write(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, const uint8_t *data, size_t size)
{
    uint8_t write_buf[256] = {0};
    write_buf[0] = reg_addr;

    esp_err_t err;
    for (int i = 0; i < size; i++)
    {
        write_buf[i + 1] = *(data + i);
    }
    err = i2c_master_transmit(dev_handle, write_buf, size + 1, I2C_TICKS_TO_WAIT);

    return err;
}

// Returns a string describing the gesture, given the numerical reading value as input.
const char *gesture_str(uint16_t ges)
{
    switch (ges)
    {
    case PAJ_UP:
        light_driver_set_green(90);
        light_driver_set_red(0);
        light_driver_set_blue(0);
        light_driver_set_power(true);

        return "up";
        break;

    case PAJ_DOWN:
        light_driver_set_green(45);
        light_driver_set_red(0);
        light_driver_set_blue(0);
        light_driver_set_power(true);

        return "down";
        break;

    case PAJ_LEFT:
        light_driver_set_green(0);
        light_driver_set_red(0);
        light_driver_set_blue(45);
        light_driver_set_power(true);

        return "left";
        break;

    case PAJ_RIGHT:
        light_driver_set_green(0);
        light_driver_set_red(0);
        light_driver_set_blue(90);
        light_driver_set_power(true);

        return "right";
        break;

    case PAJ_FORWARD:
        light_driver_set_green(0);
        light_driver_set_red(90);
        light_driver_set_blue(0);
        light_driver_set_power(true);

        return "forward";
        break;

    case PAJ_BACKWARD:
        light_driver_set_green(0);
        light_driver_set_red(10);
        light_driver_set_blue(0);
        light_driver_set_power(true);

        return "backward";
        break;

    case PAJ_CLOCKWISE:
        return "clockwise";
        break;

    case PAJ_COUNT_CLOCKWISE:
        return "counter-clockwise";
        break;

    case PAJ_WAVE:
        return "wave";
        break;

    default:
        ESP_LOGI(TAG, "no gesture: %#02x", ges);
        return "none";
    }
}

static esp_err_t i2c_master_sensor_test()
{
    esp_err_t ret;

    // Check if sensor is ready.
    uint8_t *ready = (uint8_t *)malloc(sizeof(uint8_t));
    while (ready == NULL || *ready != 0x20)
    {
        // Wait for sensor to stabilize.
        vTaskDelay(500 / portTICK_PERIOD_MS);

        ESP_LOGI(TAG, "Checking if sensor is ready...");

        ret = gesture_register_read(dev_handle, 0x00, ready, 1);

        if (ready == NULL)
        {
            ESP_LOGE(TAG, "Failed checking if sensor is ready (ready = NULL, ret = %s). Checking again...", esp_err_to_name(ret));
            continue;
        }

        if (ret != ESP_OK || *ready != 0x20)
            ESP_LOGW(TAG, "Sensor isn't ready yet (ready = %#02x, ret = %s). Checking again...", *ready, esp_err_to_name(ret));
    }

    free((void *)ready);

    ESP_LOGI(TAG, "Sensor is ready.");

    vTaskDelay(30 / portTICK_PERIOD_MS);

    // Initialize sensor.

    // Get length of array.
    size_t reg_arr_len = sizeof init_register_array / sizeof *init_register_array;

    for (int i = 0; i < reg_arr_len; i++)
    {
        ESP_LOGI(TAG, "Initializing sensor state: {%#02x, %#02x}", init_register_array[i][0], init_register_array[i][1]);
        ret = gesture_register_write_byte(dev_handle, init_register_array[i][0], init_register_array[i][1]);
        if (ret != ESP_OK)
            return ret;
    }

    ESP_LOGI(TAG, "Sensor is initialized.");

    vTaskDelay(30 / portTICK_PERIOD_MS);

    // Initialize gesture mode (as opposed to proximity mode).

    // Get length of array.
    size_t gest_arr_len = sizeof init_gesture_array / sizeof *init_gesture_array;

    for (int i = 0; i < gest_arr_len; i++)
    {
        ESP_LOGI(TAG, "Initializing gesture mode: {%#02x, %#02x}", init_gesture_array[i][0], init_gesture_array[i][1]);

        ret = gesture_register_write_byte(dev_handle, init_gesture_array[i][0], init_gesture_array[i][1]);
        if (ret != ESP_OK)
        {
            return ret;
        }
    }

    ESP_LOGI(TAG, "Gesture mode is initialized.");

    vTaskDelay(30 / portTICK_PERIOD_MS);

    // Enable Normal Mode.
    ESP_LOGI(TAG, "Enabling normal mode...");

    // Enable Gaming Mode.
    //  ESP_LOGI(TAG, "Enabling gaming mode...");

    // Set i2c write mode.
    uint8_t bank = BANK1;
    ret = gesture_register_write_byte(dev_handle, CHANGE_BANK_ADDR, bank);
    if (ret != ESP_OK)
        return ret;

    uint8_t mode = NORMAL_MODE; // GAMING_MODE;
    ret = gesture_register_write_byte(dev_handle, MODE_ADDR, mode);
    if (ret != ESP_OK)
        return ret;

    bank = BANK0;
    ret = gesture_register_write_byte(dev_handle, CHANGE_BANK_ADDR, bank);
    if (ret != ESP_OK)
        return ret;

    ESP_LOGI(TAG, "Normal mode is enabled.");
    //    ESP_LOGI(TAG, "Gaming mode is enabled.");

    vTaskDelay(30 / portTICK_PERIOD_MS);

    return ret;
}

// Добавление устройства на шину
void i2c_bus_add_gesture()
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_GESTURE_ADDRESS,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
}

void gesture_task(void *arg)
{
    int ret;
//    uint8_t sensor_data_h, sensor_data_l;

    i2c_bus_add_gesture();
    while (1)
    {
        ret = i2c_master_sensor_test();
        xSemaphoreTake(print_mux, portMAX_DELAY);
        if (ret == ESP_ERR_TIMEOUT)
        {
            ESP_LOGE(TAG, "I2C Timeout");
        }
        else if (ret == ESP_OK)
        {
            // Initialization succeeded.
            for (;;)
            {
                uint8_t *ges = (uint8_t *)malloc(sizeof(uint8_t) * 2);

                ret = gesture_register_read(dev_handle, PAJ_INT_FLAG1, ges, sizeof(uint8_t) * 2);

                if (ret != ESP_OK)
                {
                    if (ges != NULL)
                    {
                        free((void *)ges);
                    }

                    vTaskDelay((DELAY_TIME_BETWEEN_ITEMS_MS * 2) / portTICK_PERIOD_MS);
                    continue;
                }

                if (ges != NULL)
                {
                    const char *ges_str = gesture_str(*ges);

                    if (strcmp(ges_str, "none") != 0)
                    {
                        ESP_LOGI(TAG, "Gesture detected: %s", ges_str);
                    }

                    free((void *)ges);
                }

                vTaskDelay(GESTURE_DURATION / portTICK_PERIOD_MS);
            }
        }
        else
        {
            ESP_LOGW(TAG, "%s: No ack, sensor not connected...skip...", esp_err_to_name(ret));
        }
        xSemaphoreGive(print_mux);
        vTaskDelay((DELAY_TIME_BETWEEN_ITEMS_MS * 2) / portTICK_PERIOD_MS);
    }
}
