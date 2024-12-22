## Проект ZB1

### v0.1
Управление тремя светодиодными лампами через бесконтактное реле.  
Включение одной лампы через выключатель, двух по датчикам движения через координатор.
Минимальный состав (исключены датчик движения, датчик температуры, дисплей).  
Они есть в коде, исключаются по константе.

## Прочая информация

Detecting chip type... ESP32-H2  
Chip is ESP32-H2 (revision v0.1)  
Features: BLE, IEEE802.15.4  
Crystal is 32MHz  

ZB1  
MAC: 74:4d:bd:ff:fe:63:71:47  

ZB2  
MAC: 74:4d:bd:ff:fe:63:82:17   

Реле включаются подачей логической 1 на вывод.

ESP_ERROR_CHECK. Макрос ESP_ERROR_CHECK используется для тех же целей, что и assert, за исключением того, что ESP_ERROR_CHECK проверяет свое значение как esp_err_t, а не bool. Если аргумент ESP_ERROR_CHECK не равен ESP_OK, то в консоль печатается сообщение об ошибке, и вызывается abort().
SP_ERROR_CHECK_WITHOUT_ABORT. Макрос ESP_ERROR_CHECK_WITHOUT_ABORT работает так же, как и ESP_ERROR_CHECK, за исключением того, что не вызывает abort().

ESP_RETURN_ON_ERROR. Макрос ESP_RETURN_ON_ERROR проверяет код ошибки, и если он не равен ESP_OK, то печатает сообщение об ошибке и выполняет возврат из вызвавшей макрос функции.

ESP_GOTO_ON_ERROR. Макрос ESP_GOTO_ON_ERROR проверяет код ошибки, и если он не равен ESP_OK, то печатает сообщение, установит локальную переменную ret в err_code, и затем выполнит переход по метки goto_tag.

ESP_RETURN_ON_FALSE. Макрос ESP_RETURN_ON_FALSE проверяет условие, и если оно не равно true, то печатает сообщение и делает возврат с предоставленным err_code.

ESP_GOTO_ON_FALSE. Макрос ESP_GOTO_ON_FALSE проверяет условие, и если оно не равно true, то печатает сообщение, установит локальную переменную ret в предоставленный err_code, и затем выполнит переход по метке goto_tag.

В одном эндпойнте может быть несколько разных кластеров.

Пины:
https://docs.espressif.com/projects/esp-idf/en/v5.3.1/esp32/api-reference/peripherals/gpio.html

Установка направления ввода/вывода пина  
esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode)
GPIO_MODE_INPUT  
GPIO_MODE_OUTPUT ...

Чтение пина, настроенного на ввод  
int gpio_get_level(gpio_num_t gpio_num)

Вывод в пин, настроенный на вывод  
esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level)

Завершение задачи самой задачей:  
vTaskDelete( NULL )
 