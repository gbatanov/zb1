#include "settings.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_private/esp_clk.h"
// #include "driver/mcpwm_cap.h"

#include "driver/gpio.h" // для использования пинов на ввод/вывод
#include "rom/gpio.h"

#include "driver/gptimer.h"

// gptimer сам выбирает группу и таймер в группе

const static char *TAG = "Timer";
// #define HC_SR04_TRIG_GPIO GPIO_NUM_12

// uint32_t level = 0;
gptimer_handle_t gptimer = NULL;
/*
// Функция обратного вызова таймера
static bool IRAM_ATTR gptimer_alarm_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    // В обработчиках нельзя использовать ESP_LOGx(), вместо этого следует использовать ESP_DRAM_LOGx()!!!
    //    ESP_DRAM_LOGW("timer0", "Hardware timer alarm!");
    gpio_set_level(HC_SR04_TRIG_GPIO, 1 - level); // set high
    esp_rom_delay_us(9);
    level = 1 - level;
    gpio_set_level(HC_SR04_TRIG_GPIO, 1 - level); // set low
    level = 1 - level;
    // Если вы вызывали функции FreeRTOS (например `xQueueSendFromISR`) из обработчика прерываний,
    // вам необходимо вернуть значение true или false на основе возвращаемого значения аргумента pxHigherPriorityTaskWoken.
    // Если возвращаемое значение `pxHigherPriorityTaskWoken` любых вызовов FreeRTOS равно pdTRUE, то вы должны вернуть true; в противном случае вернуть false.
    // ---
    // В данном простейшем случае мы не отправляли ничего в очереди, поэтому можно вернуть false
    return false;
}
*/
void timer_init()
{
    // Настраиваем параметры таймера
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT, // Выбираем источник тактового сигнала для счетчиков
        .direction = GPTIMER_COUNT_UP,      // Устанавливаем направление счета
        .resolution_hz = 1000000,           // 1MHz, 1 tick=1mks  // Устанавливаем частоту счета, то есть минимальный интервал времени на 1 тик
    };

    // Создаем дексриптор GP-таймера с указанными параметрами
    gptimer_new_timer(&timer_config, &gptimer);
    // Подключаем функцию обратного вызова
    //    gptimer_event_callbacks_t cb_config = {
    //        .on_alarm = gptimer_alarm_callback,
    //    };
    //    gptimer_register_event_callbacks(gptimer, &cb_config, NULL);
    // Задаем начальное значение счетчика таймера (не обязательно)
    gptimer_set_raw_count(gptimer, 0);
    // чтение из счетчика таймера
    // uint64_t cur_count
    // esp_err_t gptimer_get_raw_count(gptimer, &cur_count)
    //
    // Задаем параметры счетчика таймера
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = __UINT64_MAX__,      // Бесконечное фактически значение
        .reload_count = 0,                  // Значение счетчика при автосбросе
        .flags.auto_reload_on_alarm = true, // Автоперезапуск счетчика таймера разрешен
    };
    gptimer_set_alarm_action(gptimer, &alarm_config);
    // Разрешаем прерывания для данного таймера
    gptimer_enable(gptimer);
    // Запускаем таймер
    gptimer_start(gptimer);
    ESP_LOGI("main", "Hardware timer stated");
} /*
     while (1)
     {
         //   esp_err_t err;
         // Формируем импульс на триггер 10 мкс
         uint64_t prev_count = 0;
         uint64_t cur_count = 0;
         //        gptimer_get_raw_count(gptimer, &prev_count); // задержка 1,5 микросекунды
         gpio_set_level(HC_SR04_TRIG_GPIO, 1); // set low
         esp_rom_delay_us(2);
         gpio_set_level(HC_SR04_TRIG_GPIO, 0); // set high
         esp_rom_delay_us(10);
         gpio_set_level(HC_SR04_TRIG_GPIO, 1); // set low

         //        gptimer_get_raw_count(gptimer, &cur_count); // задержка 1,5 микросекунды
         //ESP_LOGI(TAG, "End 10mks  %llu", cur_count - prev_count);

         vTaskDelay(pdMS_TO_TICKS(2000));
     }
 }

 void app_main()
 {
     gpio_pad_select_gpio(HC_SR04_TRIG_GPIO);
     gpio_set_direction(HC_SR04_TRIG_GPIO, GPIO_MODE_OUTPUT);
     gpio_pad_select_gpio(HC_SR04_ECHO_GPIO);
     gpio_set_direction(HC_SR04_ECHO_GPIO, GPIO_MODE_INPUT);

     xTaskCreate(timer_task, "Timer_task", 4096, NULL, 5, NULL);
 }
 */