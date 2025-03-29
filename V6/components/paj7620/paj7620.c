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
    {0xEF, 0x00}, // Bank 0
    {0x41, 0x00}, // Disable interrupts for first 8 gestures
    {0x42, 0x00}, // Disable wave (and other mode's) interrupt(s)
    {0x48, 0x3C},
    {0x49, 0x00},
    {0x51, 0x10},
    {0x83, 0x20},
    {0x9F, 0xF9},
    {0xEF, 0x01}, // Bank 1
    {0x01, 0x1E},
    {0x02, 0x0F},
    {0x03, 0x10},
    {0x04, 0x02},
    {0x41, 0x40},
    {0x43, 0x30},
    {0x65, 0x96}, // R_IDLE_TIME  - Normal mode LSB "120 fps" (supposedly)
    {0x66, 0x00},
    {0x67, 0x97},
    {0x68, 0x01},
    {0x69, 0xCD},
    {0x6A, 0x01},
    {0x6B, 0xB0},
    {0x6C, 0x04},
    {0x6D, 0x2C},
    {0x6E, 0x01},
    {0x74, 0x00}, // Set gesture mode
    {0xEF, 0x00}, // Bank 0
    {0x41, 0xFF}, // Re-enable interrupts for first 8 gestures
    {0x42, 0xFF}, // Re-enable interrupts for wave gesture
};

// Register values for proximity mode initialization.
int init_ps_array[][2] = {
    {0xEF, 0x00}, // Bank 0
    {0x41, 0x00},
    {0x42, 0x02}, // disable interrupts for approch
    {0x48, 0x20},
    {0x49, 0x00},
    {0x51, 0x13},
    {0x83, 0x00},
    {0x9F, 0xF8},
    {0x69, 0x96}, // PS High Thd def 0xc8
    {0x6A, 0x02}, // PS Low Thd def 0x40

    {0xEF, 0x01}, // Bank 1
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
    {0x74, 0x05}, // Set proximity mode
    {0x44, 0xA0}, // PS gain settings

    {0xEF, 0x00}, // Bank 0
    {0x42, 0xFF}, // Re-enable interrupts for approch

};

static QueueHandle_t gpio_evt_queue = NULL;
// коллбэк-функция обработки прерываний с сенсора
// static sonar_callback_t func_ptr;

SemaphoreHandle_t print_mux = NULL;

extern i2c_master_bus_handle_t bus_handle;

//   Read a sequence of bytes from a i2c device registers
static esp_err_t i2c_register_read(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

//   Write a byte to a i2c device registers
static esp_err_t i2c_register_write_byte(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

// обработчик прерываний
// Обработчик прерывания должен постоянно находится в оперативной памяти (IRAM),
// поэтому его следует пометить соответствующим атрибутом IRAM_ATTR.
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    Dev_PAJ7620 *dev = (Dev_PAJ7620 *)arg;
    //  запрещаем прерывания
    gpio_intr_disable(dev->intPin);
    // Переменные для переключения контекста
    BaseType_t xHigherPriorityTaskWoken, xResult;
    xHigherPriorityTaskWoken = pdFALSE;

    // посылаем в очередь сообщений пин (туда надо тупо что-то послать,
    // хотя у нас заранее известно на каком пине возникло прерывание)
    xResult = xQueueSendFromISR(gpio_evt_queue, dev, &xHigherPriorityTaskWoken);

    if (xResult == pdPASS)
    {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    };
}

esp_err_t i2c_register_write(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, const uint8_t *data, size_t size)
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
const char *gesture_str(uint16_t *ges)
{
    switch (*ges)
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
        //        ESP_LOGI(TAG, "no gesture: %#04x", *ges);
        return "none";
    }
}
// 0 - NORMAL_SPEED
// 1 - GAMING_SPEED
static esp_err_t gesture_set_speed(Dev_PAJ7620 *devPaj7620)
{
    i2c_master_dev_handle_t dev_handle = devPaj7620->dev_handle;
    uint8_t speed = devPaj7620->speed;
    if (speed > 1)
    {
        speed = 0;
        devPaj7620->speed = speed;
    }

    // select bank
    esp_err_t ret = i2c_register_write_byte(dev_handle, CHANGE_BANK_ADDR, BANK1);
    if (ret != ESP_OK)
        return ret;

    // set speed
    ret = i2c_register_write_byte(dev_handle, R_IDLE_TIME, NORMAL_SPEED); // GAMING_SPEED;
    if (ret != ESP_OK)
    {
        i2c_register_write_byte(dev_handle, CHANGE_BANK_ADDR, BANK0);
        return ret;
    }
    // select bank
    ret = i2c_register_write_byte(dev_handle, CHANGE_BANK_ADDR, BANK0);
    if (ret != ESP_OK)
        return ret;

    return ESP_OK;
}

// 0 - gesture
// 1 - proximity
static esp_err_t paj7620_set_mode(Dev_PAJ7620 *dev)
{
    i2c_master_dev_handle_t dev_handle = dev->dev_handle;
    uint8_t mode = dev->mode;
    esp_err_t ret = ESP_OK;
    if (mode == 0)
    {
        // Initialize gesture mode (as opposed to proximity mode).

        // Get length of array.
        size_t gest_arr_len = sizeof init_gesture_array / sizeof *init_gesture_array;

        for (int i = 0; i < gest_arr_len; i++)
        {
            ESP_LOGI(TAG, "Initializing gesture mode: {%#02x, %#02x}", init_gesture_array[i][0], init_gesture_array[i][1]);

            ret = i2c_register_write_byte(dev_handle, init_gesture_array[i][0], init_gesture_array[i][1]);
            if (ret != ESP_OK)
                return ret;
        }

        ESP_LOGI(TAG, "Gesture mode is initialized.");
    }
    else if (mode == 1)
    {
        // Initialize proximity mode

        // Get length of array.
        size_t ps_arr_len = sizeof init_ps_array / sizeof *init_ps_array;

        for (int i = 0; i < ps_arr_len; i++)
        {
            // ESP_LOGI(TAG, "Initializing proximity mode: {%#02x, %#02x}", init_ps_array[i][0], init_ps_array[i][1]);

            ret = i2c_register_write_byte(dev_handle, init_ps_array[i][0], init_ps_array[i][1]);
            if (ret != ESP_OK)
                return ret;
        }

        // ESP_LOGI(TAG, "Proximiy mode is initialized.");
    }

    vTaskDelay(30 / portTICK_PERIOD_MS);

    return ESP_OK;
}

// Настраиваем прерывание на пине
static bool paj762r_gpio_init(Dev_PAJ7620 *dev)
{
    gpio_config_t io_conf = {};
    uint64_t pin_bit_mask = 0;

    pin_bit_mask = (1ULL << dev->intPin);

    io_conf.intr_type = GPIO_INTR_NEGEDGE; // прерывание по спаду
    io_conf.pin_bit_mask = pin_bit_mask;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    // конфигурируем GPIO с заданными установками
    gpio_config(&io_conf);

    // создаем очередь для обработки GPIO событий от ISR
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    if (gpio_evt_queue == 0)
    {
        ESP_LOGE(TAG, "Queue was not created and must not be used");
        return false;
    }
    // стартуем задачу
    //   xTaskCreate(interrupt_detect_task, "interrupt_detect_task", 4096, NULL, 10, NULL);
    xTaskCreate(gesture_task, "gesture_task", 4096, dev, 6, NULL);
    // устанавливаем службу прерываний
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // добавляем обработчик прервания  gpio_isr_handler на пине pin
    gpio_isr_handler_add(dev->intPin, gpio_isr_handler, dev);
    //   funcfunc_ptr = cb;
    return true;
}

//
esp_err_t paj7620_init(Dev_PAJ7620 *dev)
{
    esp_err_t ret = ESP_OK;

    // Check if sensor is ready.
    uint8_t ready = 0;
    i2c_master_dev_handle_t dev_handle = dev->dev_handle;
    if (!paj762r_gpio_init(dev))
        return ESP_FAIL;

    while (ready != 0x20)
    {
        // Wait for sensor to stabilize.
        vTaskDelay(500 / portTICK_PERIOD_MS);

        ESP_LOGI(TAG, "Checking if sensor is ready...");
        //  пробуем прочитать из банка 0 ChipId младший байт (default 0x20)
        ret = i2c_register_read(dev_handle, 0x00, &ready, 1);
        if (ret != ESP_OK || ready != 0x20)
            ESP_LOGW(TAG, "Sensor isn't ready yet (ready = 0x%#02x, ret = %s). Checking again...", ready, esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "Sensor is ready.");

    vTaskDelay(30 / portTICK_PERIOD_MS);

    // Initialize sensor.

    // Get length of array.
    size_t reg_arr_len = sizeof init_register_array / sizeof *init_register_array;

    for (int i = 0; i < reg_arr_len; i++)
    {
        ESP_LOGI(TAG, "Initializing sensor state: {addr: 0x%#02x, val: 0x%#02x}", init_register_array[i][0], init_register_array[i][1]);
        ret = i2c_register_write_byte(dev_handle, init_register_array[i][0], init_register_array[i][1]);
        if (ret != ESP_OK)
            return ret;
    }

    ESP_LOGI(TAG, "Sensor is initialized.");

    vTaskDelay(30 / portTICK_PERIOD_MS);
    ret = paj7620_set_mode(dev);

    if (dev->mode == 0)
        ret = gesture_set_speed(dev);

    vTaskDelay(30 / portTICK_PERIOD_MS);

    return ret;
}

// Добавление устройства на шину
void i2c_bus_add_paj7620(Dev_PAJ7620 *devPaj7620)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_GESTURE_ADDRESS,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &(devPaj7620->dev_handle)));
    devPaj7620->available = true;
}

void gesture_task(void *arg)
{
    esp_err_t ret = ESP_OK;

    Dev_PAJ7620 *devPaj7620 = (Dev_PAJ7620 *)arg;
    i2c_master_dev_handle_t dev_handle = devPaj7620->dev_handle;

    uint8_t *ges = (uint8_t *)malloc(sizeof(uint8_t) * 2);
    uint8_t state = 0;
    uint8_t level = 0;

    while (1)
    {
        *ges = 0;
        ges++;
        *ges = 0;
        ges--;
        // Ждем появления сообщения в очереди и читаем при появлении
        if (xQueueReceive(gpio_evt_queue, (void *)devPaj7620, portMAX_DELAY))
        {
            gpio_intr_disable(devPaj7620->intPin); // запрещаем прерывание на пине
            ESP_LOGI(TAG, "Interrupt detected");
            // res = (*func_ptr)();            // выполняем функцию
            // gpio_intr_enable(devPaj7620->intPin);  // разрешаем прерывание на пине

            if (devPaj7620->mode == 0)
            {
                ret = i2c_register_read(dev_handle, PAJ_INT_FLAG1, ges, sizeof(uint8_t) * 2);
                if (ret == ESP_OK)
                {
                    const char *ges_str = gesture_str((uint16_t *)ges);
                    if (strcmp(ges_str, "none") != 0)
                        ESP_LOGI(TAG, "Gesture detected: %s", ges_str);
                }
                //            vTaskDelay(GESTURE_DURATION / portTICK_PERIOD_MS);
            }
            else if (devPaj7620->mode == 1)
            {
                ret = i2c_register_read(dev_handle, PAJ7620_ADDR_PS_APPROACH_STATE, &state, 1);
                if (ret == ESP_OK)
                    ESP_LOGI(TAG, "proximity state: %#02x", state & 0x01);
                vTaskDelay(30 / portTICK_PERIOD_MS);

                ret = i2c_register_read(dev_handle, PAJ7620_ADDR_S_AVE_Y_BRIGHTNESS, &level, 1);
                if (ret == ESP_OK)
                    ESP_LOGI(TAG, "proximity level: %#02x", level);
                //           vTaskDelay(200 / portTICK_PERIOD_MS);
            }
            gpio_intr_enable(devPaj7620->intPin); // разрешаем прерывание на пине
        }
    }
}
