## V4.1 Работа с ультразвуковым датчиком напрямую без arduino с использованием аппаратного т]аймера

### Инфа по таймерам с 5-й версии
Источник: https://kotyara12.ru/iot/esp32_timers/

General Purpose Timer для версий ESP-IDF 5.0.0 и выше
Ссылка на описание данного API: [ESP32 – GPTimer](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gptimer.html).

GPTimer (General Purpose Timer) — это новый драйвер для аппаратных таймеров ESP32. 

Для работы с данной версией аппаратных таймеров подключите новую API-библиотеку: #include "driver/gptimer.h"

Общая схема работы с GP-таймерами похожа на предыдущий способ:

Инициализируйте дескриптор таймера с помощью функции gptimer_new_timer(). Здесь вы задаете источник тактового сигнала для счетчика, частоту счета (минимальный интервал времени) и направление счета (вверх или вниз). 
Настройте параметры счетчика таймера с помощью gptimer_set_alarm_action().
Создайте функцию – обработчик прерывания и подключите его к таймеру – gptimer_register_event_callbacks().
Запустите счетчик таймера с помощью функции gptimer_enable() и gptimer_start()
Рассмотрим все эти этапы поподробнее.

#### Инициализация таймера и настройка его параметров
Для инициализации таймера необходимо вызывать функцию gptimer_new_timer:
```
esp_err_t gptimer_new_timer(const gptimer_config_t *config, gptimer_handle_t* ret_timer)
```
Функция принимает два параметра – указатель на параметры конфигурации аппаратного таймера и указатель на переменную, в которую будет помещен дескриптор таймера при успешном его создании. Тут есть тонкий момент: аппаратных таймеров всего 4, но вы можете попытаться вызвать эту функцию, скажем, 5 или 7 раз – и если все таймеры уже заняты, будет возвращена ошибка – всегда проверяйте коды ошибок.

Параметры конфигурации GP-таймера, здесь необходимо указать:
 - источник тактового сигнала для таймера
 - направление счета – вверх или вниз
 - рабочую частоту таймера в герцах

В примере это выглядит так:
```
gptimer_handle_t gptimer = NULL;
// Настраиваем параметры таймера
gptimer_config_t timer_config = {
    .clk_src = GPTIMER_CLK_SRC_DEFAULT,            // Выбираем источник тактового сигнала для счетчиков
    .direction = GPTIMER_COUNT_UP,                 // Устанавливаем направление счета
    .resolution_hz = 1000000, // 1MHz, 1 tick=1us  // Устанавливаем частоту счета, то есть минимальный интервал времени на 1 тик
};

// Создаем дескриптор GP-таймера с указанными параметрами
gptimer_new_timer(&timer_config, &gptimer);
```

#### Создаем функцию обратного вызова
В терминологии GPTimer API обработчики “тревог” таймеров называются уже не обработчиками прерываний, а callback-функциями. Наверное это потому, что формально обработчик прерываний у всех таймеров, задействованных через GPTimer API один и тот же, и именно он вызывает в свою очередь настроенную callback-функцию. Хотя эти callback-и должны удовлетворять всем тем же самым требованиями, которые предъявляются и обработчикам прерываний – не расслабляйтесь.

Прототип callback-а выглядит уже несколько сложнее – в качестве аргументов передается указатель на сработавший таймер и дополнительные данные:
```
static bool IRAM_ATTR gptimer_alarm_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t * edata, void * user_data)
```
А вот подцепить этот callback к таймеру можно с помощью функции gptimer_register_event_callbacks:
```
esp_err_t gptimer_register_event_callbacks(gptimer_handle_t timer, const gptimer_event_callbacks_t *cbs, void *user_data)
```
Как видим, в функцию нужно передать не указатель на сам callback, а опять некую структуру конфигурации

Важно! Сделать это нужно до вызова gptimer_enable()! Оно и понятно – gptimer_enable по всей видимости разрешает прерывания для таймера.

Что ж, сделаем как просили:
```
// Подключаем функцию обратного вызова
gptimer_event_callbacks_t cb_config = {
    .on_alarm = gptimer_alarm_callback,
};
gptimer_register_event_callbacks(gptimer, &cb_config, NULL);
```

#### Настраиваем счетчик таймера и параметры перезапуска
После этого необходимо настроить параметры счетчика таймера. Сделать это можно с помощью функции gptimer_set_alarm_action:
```
esp_err_t gptimer_set_alarm_action(gptimer_handle_t timer, const gptimer_alarm_config_t *config)
```

В конфиге:

 - alarm_count – здесь указываем конечное значение счетчика, при котором будет вызвана “тревога”
 - reload_count – для цикличных (повторяющихся) таймеров указываем начальное значение счетчика, которое будет установлено сразу после генерации прерывания
 - flags.auto_reload_on_alarm – этот флаг отвечает за автоперезапуск таймера – при 0 это будет одноразовый таймер, при 1 – цикличный
 ```
// Задаем начальное значение счетчика таймера (не обязательно)
gptimer_set_raw_count(gptimer, 0);
// Задаем параметры счетчика таймера
gptimer_alarm_config_t alarm_config = {
    .alarm_count = 3000000,                   // Конечное значение счетчика = 3 секунды
    .reload_count = 0,                        // Значение счетчика при автосбросе
    .flags.auto_reload_on_alarm = true,       // Автоперезапуск счетчика таймера разрешен
};
gptimer_set_alarm_action(gptimer, &alarm_config);
```
На этом подготовку можно считать завершенной.

#### Старт!
Для запуска таймера нужно сделать две вещи: разрешить прерывания через gptimer_enable и собственно выполнить запуск посредством gptimer_start:
```
esp_err_t gptimer_enable(gptimer_handle_t timer)esp_err_t gptimer_start(gptimer_handle_t timer)
```
Ничего особо хитрого в них нет, поехали:
```
// Разрешаем прерывания для данного таймера
gptimer_enable(gptimer);
// Запускаем таймер
gptimer_start(gptimer);
ESP_LOGI("main", "Hardware timer stated");
```
Если все в порядке и не наделали ошибок, то компилируем, загружаем в ESP32 и получаем результат
Пример для работы с этим API вы найдете по ссылке: github.com/kotyara12/dzen/tree/master/timer_gptimer
```
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gptimer.h"
#include "esp_log.h"
// Функция обратного вызова таймера
static bool IRAM_ATTR gptimer_alarm_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
  // В обработчиках нельзя использовать ESP_LOGx(), вместо этого следует использовать ESP_DRAM_LOGx()!!!
  ESP_DRAM_LOGW("timer0", "Hardware timer alarm!");
  // Если вы вызывали функции FreeRTOS (например `xQueueSendFromISR`) из обработчика прерываний, 
  // вам необходимо вернуть значение true или false на основе возвращаемого значения аргумента pxHigherPriorityTaskWoken. 
  // Если возвращаемое значение `pxHigherPriorityTaskWoken` любых вызовов FreeRTOS равно pdTRUE, то вы должны вернуть true; в противном случае вернуть false.
  // ---
  // В данном простейшем случае мы не отправляли ничего в очереди, поэтому можно вернуть false
  return false;
}
void app_main() 
{
  gptimer_handle_t gptimer = NULL;
  // Настраиваем параметры таймера
  gptimer_config_t timer_config = {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,            // Выбираем источник тактового сигнала для счетчиков
      .direction = GPTIMER_COUNT_UP,                 // Устанавливаем направление счета
      .resolution_hz = 1000000, // 1MHz, 1 tick=1us  // Устанавливаем частоту счета, то есть минимальный интервал времени на 1 тик
  };
  
  // Создаем дексриптор GP-таймера с указанными параметрами
  gptimer_new_timer(&timer_config, &gptimer);
  // Подключаем функцию обратного вызова
  gptimer_event_callbacks_t cb_config = {
      .on_alarm = gptimer_alarm_callback,
  };
  gptimer_register_event_callbacks(gptimer, &cb_config, NULL);
  // Задаем начальное значение счетчика таймера (не обязательно)
  gptimer_set_raw_count(gptimer, 0);
  // Задаем параметры счетчика таймера
  gptimer_alarm_config_t alarm_config = {
      .alarm_count = 3000000,                   // Конечное значение счетчика = 3 секунды
      .reload_count = 0,                        // Значение счетчика при автосбросе
      .flags.auto_reload_on_alarm = true,       // Автоперезапуск счетчика таймера разрешен
  };
  gptimer_set_alarm_action(gptimer, &alarm_config);
  // Разрешаем прерывания для данного таймера
  gptimer_enable(gptimer);
  // Запускаем таймер
  gptimer_start(gptimer);
  ESP_LOGI("main", "Hardware timer stated");
  // Основной цикл
  while (1) {
    // Просто выводим сообщение в лог через каждые 5 секунд
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGI("main", "vTaskDelay(5000) timeout");
  }
}
```
