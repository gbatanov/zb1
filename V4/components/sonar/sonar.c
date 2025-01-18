#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "light_driver.h"
#include "sonar.h"

// Этот драйвер заточен под сигнал с УЗ датчика (имитация переключателя,
// но неполная, без устранения дребезга и немного другая логика)
static QueueHandle_t gpio_evt_queue = NULL;

// call back function pointer
static esp_switch_callback_t func_ptr;

static const char *TAG = "GSB_ZB_4_SONAR";

uint32_t echo_pin;
// обработчик прерываний
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    //  запрещаем прерывания
    gpio_intr_disable(echo_pin);
    // посылаем в очередь сообщений пару пин/функция
    xQueueSendFromISR(gpio_evt_queue, &echo_pin, NULL);
}

// Задача определения измения состояния пина - входа с УЗ датчика
static void echo_detect_task(void *arg)
{
    uint32_t io_num;

    while (true)
    {
        // check if there is any queue received, if yes read out the button_func_pair
        if (xQueueReceive(gpio_evt_queue, (void *)&io_num, portMAX_DELAY))
        {
            gpio_intr_disable(echo_pin); // запрещаем прерывание на пине
            (*func_ptr)(echo_pin);
            gpio_intr_enable(echo_pin); // разрешаем прерывание на пине
        }
    }
}

// Настраиваем прерывание на пине
static bool switch_driver_gpio_init(uint32_t pin)
{
    gpio_config_t io_conf = {};
    uint64_t pin_bit_mask = 0;

    pin_bit_mask = (1ULL << pin);

    // interrupt of any edge
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.pin_bit_mask = pin_bit_mask;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    // configure GPIO with the given settings
    gpio_config(&io_conf);

    // create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    if (gpio_evt_queue == 0)
    {
        ESP_LOGE(TAG, "Queue was not created and must not be used");
        return false;
    }
    // start gpio task
    xTaskCreate(echo_detect_task, "echo_detect_task", 4096, NULL, 10, NULL);
    // install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // добавляет обработчик прервания  gpio_isr_handler на пине pin
    gpio_isr_handler_add(pin, gpio_isr_handler, NULL);

    return true;
}

esp_err_t sonar_init(uint32_t echopin, esp_switch_callback_t cb)
{
    echo_pin = echopin;
    static bool is_inited = false;
    if (!is_inited)
    {
        ESP_RETURN_ON_FALSE(
            switch_driver_gpio_init(echo_pin),
            ESP_FAIL, TAG, "Failed to initialize switch driver");
        func_ptr = cb;
        is_inited = true;
    }
    return is_inited ? ESP_OK : ESP_FAIL;
}
