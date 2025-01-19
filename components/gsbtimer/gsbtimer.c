//
// Пример программного таймера. Дискретность очень грубая - 10мс.
//
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"

#include "gsbtimer.h"

extern const char *TAG;

TimerHandle_t _timer = NULL;

// Функция обратного вызова таймера
void timer_callback(TimerHandle_t pxTimer)
{
    // Это не обработчик прерываний, поэтому можно использовать ESP_LOGx
    uint32_t tmrId = (uint32_t)pvTimerGetTimerID(pxTimer);
    ESP_LOGW(TAG, "Software FreeRTOS timer %lu signal!", tmrId);
}

// Создаем таймер
void create_timer(uint32_t *pvTimerID)
{
     // В настройках стоит 100Hz, значит один тик равен 10 миллисекундам.
    _timer = xTimerCreate("Timer",       // Просто текстовое имя для отладки
                          2000,        // Период таймера в тиках, pdMS_TO_TICKS(1000) - в миллисекундах, 10000 x 10 = 100sec
                          pdTRUE,        // Повторяющийся таймер
                          pvTimerID,     // void * const pvTimerID Идентификатор, присваиваемый создаваемому таймеру
                          timer_callback // Функция обратного вызова
    );
}

// Запускаем таймер
void start_timer()
{
    if (xTimerStart(
            _timer, // дескриптор таймера, полученный при создании
            0       // казывает время в тиках, в течение которого вызывающая задача должна удерживаться в состоянии «заблокировано»,
            // чтобы дождаться успешной отправки команды запуска в очередь команд таймера,
            // если очередь задачи таймера уже заполнена на момент вызова xTimerStart().
            // Это не время таймера!
            ) == pdPASS)
    {
        ESP_LOGI(TAG, "Software FreeRTOS timer started");
    };
}
