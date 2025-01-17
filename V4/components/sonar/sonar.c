#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "light_driver.h"
#include "sonar.h"

// Этот драйвер заточен под сигнал с УЗ датчика (имитация переключателя, но неполная, без устранения дребезга и немного другая логика)
static QueueHandle_t gpio_evt_queue = NULL;

// call back function pointer
static esp_switch_callback_t func_ptr;

static const char *TAG = "GSB_ZB_4_SONAR";

static void switch_driver_gpios_intr_enabled(bool enabled);
static void echo_handler(uint32_t pin);

// обработчик прерываний
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    //  запрещаем прерывания
    switch_driver_gpios_intr_enabled(false);
    // посылаем в очередь сообщений пару пин/функция
    xQueueSendFromISR(gpio_evt_queue, NULL, NULL);
}

// Разрешение прерывания
static void switch_driver_gpios_intr_enabled(bool enabled)
{
    if (enabled)
        gpio_intr_enable(ECHO_PIN_NUM);
    else
        gpio_intr_disable(ECHO_PIN_NUM);
}

// Задача определения измения состояния пина - входа с УЗ датчика
static void echo_detect_task(void *arg)
{
    gpio_num_t io_num = GPIO_NUM_NC;
    bool evt_flag = false;

    while (true)
    {
        // check if there is any queue received, if yes read out the button_func_pair
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            switch_driver_gpios_intr_enabled(false); // запрещаем прерывание на пине
            evt_flag = true;
        }
        while (evt_flag)
        {
            // логику делаем такую - запоминаем текущее состояние,
            // через 100 мс проверяем уровень, если изменился - считаем помехой и игнорируем,
            // если остался - фиксируем в переменной для зигби и запрещаем прерывания на этом пине
            // еще на 1 минуту
            bool value1 = gpio_get_level(ECHO_PIN_NUM);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            bool value2 = gpio_get_level(ECHO_PIN_NUM);
            if (value1 == value2)
            {
                // событие
                // вызываем коллбэк-функцию для фиксации в аттрибуте зигби и зажигание цвета светодиода
                (*func_ptr)(ECHO_PIN_NUM);
                vTaskDelay(1000 * 60 / portTICK_PERIOD_MS);
                switch_driver_gpios_intr_enabled(true);
            }
            else
            {
                // помеха
                 switch_driver_gpios_intr_enabled(true);
            }
            evt_flag = false;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
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
    // добавляет обработчик прервания  gpio_isr_handler на пине button_func_pair->pin
    // в качестве аргумента передается структура с пином и функцией обработки
    gpio_isr_handler_add(pin, gpio_isr_handler, NULL);

    return true;
}

bool switch_driver_init(uint32_t pin, esp_switch_callback_t cb)
{
    if (!switch_driver_gpio_init(pin))
    {
        return false;
    }
    func_ptr = cb;
    return true;
}

esp_err_t deferred_driver_init(void)
{
    static bool is_inited = false;
    if (!is_inited)
    {
        ESP_RETURN_ON_FALSE(
            switch_driver_init(ECHO_PIN_NUM, echo_handler),
            ESP_FAIL, TAG, "Failed to initialize switch driver");
        is_inited = true;
    }
    return is_inited ? ESP_OK : ESP_FAIL;
}

static void echo_handler(uint32_t pin)
{

    ESP_LOGI(TAG, "zb_buttons_handler %lu pin", pin);

    bool value = gpio_get_level(pin);
    if (value)
    {
        ESP_LOGI(TAG, "Sonar ON");
        light_driver_set_red(40);
        light_driver_set_green(0);
        light_driver_set_blue(0);
    }
    else
    {
        ESP_LOGI(TAG, "Sonar OFF");
        light_driver_set_red(0);
        light_driver_set_green(40);
        light_driver_set_blue(0);
    }
    light_driver_set_power(true);
}
