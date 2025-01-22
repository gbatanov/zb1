#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "light_driver.h"
#include "sonar.h"

// Этот драйвер заточен под сигнал с УЗ датчика
static QueueHandle_t gpio_evt_queue = NULL;

// коллбэк-функция обработки сигналов с датчика
sonar_callback_t func_ptr;

static const char *TAG = "GSB_ZB_4_SONAR";

uint32_t echo_pin; // пин, на который подключен вывод ECHO
uint32_t trig_pin; // пин, на который подключен вывод TRIG (сигнал инвертируется)
uint64_t counter = 0;

extern gptimer_handle_t gptimer;

// обработчик прерываний
// Обработчик прерывания должен постоянно находится в оперативной памяти (IRAM),
// поэтому его следует пометить соответствующим атрибутом IRAM_ATTR.
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint64_t curr_counter = 0;
    uint32_t value = 0;
    //  запрещаем прерывания
    gpio_intr_disable(echo_pin);
    if (counter == 0)
    {
        gptimer_get_raw_count(gptimer, &counter);
        gpio_intr_enable(echo_pin);
    }
    else
    {
        gptimer_get_raw_count(gptimer, &curr_counter);
        value = (uint32_t)(curr_counter - counter);
        counter = 0;

        // Переменные для переключения контекста
        BaseType_t xHigherPriorityTaskWoken, xResult;
        xHigherPriorityTaskWoken = pdFALSE;

        // посылаем в очередь сообщений пин (туда надо тупо что-то послать,
        // хотя у нас заранее известно на каком пине возникло прерывание)
        xResult = xQueueSendFromISR(gpio_evt_queue, &value, &xHigherPriorityTaskWoken);
        gpio_intr_enable(echo_pin);
        // После завершения прерывания можно выполнять переключение контекста
        // путем вызова portYIELD_FROM_ISR.
        // Зачем это нужно? Допустим, в текущий момент выполняется низкоприоритетная
        // задача, а высокоприоритетная ожидает наступления некоторого прерывания.
        // Далее происходит прерывание, но по окончании работы обработчика прерываний
        // выполнение возвращается к текущей низкоприоритетной задаче,
        // а высокоприоритетная ожидает, пока закончится текущий квант времени.
        // Однако если после выполнения обработчика прерывания передать управление
        // планировщику ( portYIELD_FROM_ISR ), то он передаст управление
        // высокоприоритетной задаче, что позволяет значительно сократить время реакции системы на прерывание,
        // связанное с внешним событием.

        if (xResult == pdPASS)
        {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        };
    }
}

// Задача определения длительности импульса на пине - входе с УЗ датчика
static void echo_detect_task(void *arg)
{
    uint32_t value;

    while (true)
    {
        gpio_set_level(trig_pin, 1); // set low
        esp_rom_delay_us(2);
        gpio_set_level(trig_pin, 0); // set high
        esp_rom_delay_us(10);
        gpio_set_level(trig_pin, 1); // set low

        // Ждем появления сообщения в очереди и читаем при появлении
        if (xQueueReceive(gpio_evt_queue, (void *)&value, portMAX_DELAY))
        {
            //   ESP_LOGI(TAG, "Sonar %0.1f", (double)value / 58.0);

 //           func_ptr(value); // выполняем функцию
            ESP_LOGI(TAG, "Sonar %0.1f", (double)value / 58.0);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

// Настраиваем прерывание на пине
static bool sonar_gpio_init(uint32_t pin)
{
    gpio_config_t io_conf = {};
    uint64_t pin_bit_mask = 0;

    pin_bit_mask = (1ULL << pin);

    // GPIO_INTR_DISABLE – отключено
    // GPIO_INTR_POSEDGE – по изменению с 0 до 1
    // GPIO_INTR_NEGEDGE – по изменению с 1 на 0
    // GPIO_INTR_ANYEDGE – по любому изменению
    // GPIO_INTR_LOW_LEVEL – по низкому уровню
    // GPIO_INTR_HIGH_LEVEL – по высокому уровню
    io_conf.intr_type = GPIO_INTR_ANYEDGE; // прерывание по любому фронту
    io_conf.pin_bit_mask = pin_bit_mask;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    // конфигурируем GPIO с заданными установками
    gpio_config(&io_conf);

    // создаем очередь для обработки GPIO событий от ISR
    gpio_evt_queue = xQueueCreate(4, sizeof(uint32_t));
    if (gpio_evt_queue == 0)
    {
        ESP_LOGE(TAG, "Queue was not created and must not be used");
        return false;
    }
    // стартуем задачу сонара
    xTaskCreate(echo_detect_task, "echo_detect_task", 4096, NULL, 10, NULL);
    // устанавливаем службу прерываний
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // добавляем обработчик прервания  gpio_isr_handler на пине pin
    gpio_isr_handler_add(pin, gpio_isr_handler, NULL);

    return true;
}

// Инициализация сонара номером пина и коллбэк-функцией
esp_err_t sonar_init(uint32_t echopin, uint32_t trigpin, sonar_callback_t cb)
{
    echo_pin = echopin;
    trig_pin = trigpin;
    static bool is_inited = false;
    if (!is_inited)
    {
        func_ptr = cb;

        ESP_RETURN_ON_FALSE(
            sonar_gpio_init(echo_pin),
            ESP_FAIL, TAG, "Failed to initialize switch driver");
        is_inited = true;
    }
    return is_inited ? ESP_OK : ESP_FAIL;
}
