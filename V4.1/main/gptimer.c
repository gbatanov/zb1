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

gptimer_handle_t gptimer = NULL;

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
    // Задаем начальное значение счетчика таймера (не обязательно)
    gptimer_set_raw_count(gptimer, 0);
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
    ESP_LOGI("main", "Hardware timer started");
} 