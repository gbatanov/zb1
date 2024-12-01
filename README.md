## Проект ZB1

Управление тремя светодиодными лампами через бесконтактное реле.  
Включение одной лампы через выключатель, двух через датчики движения,  
один встроенный, один внешний.

В версии 2 попробовал сделать 4 эндпойнта. Но зигби часть не вытягивает.  
Версию 4 делаю на 1 эндпойнте с использованием стандартного кластера Binary Output  
для приема/передачи состояния реле.

## Прочая информация

Detecting chip type... ESP32-H2  
Chip is ESP32-H2 (revision v0.1)  
Features: BLE, IEEE802.15.4  
Crystal is 32MHz  

ZB1  
MAC: 74:4d:bd:ff:fe:63:71:47  
BASE MAC: 74:4d:bd:63:71:47  
MAC_EXT: ff:fe

ZB2  
MAC: 74:4d:bd:ff:fe:63:82:17  
BASE MAC: 74:4d:bd:63:82:17  


ESP_ERROR_CHECK. Макрос ESP_ERROR_CHECK используется для тех же целей, что и assert, за исключением того, что ESP_ERROR_CHECK проверяет свое значение как esp_err_t, а не bool. Если аргумент ESP_ERROR_CHECK не равен ESP_OK, то в консоль печатается сообщение об ошибке, и вызывается abort().
SP_ERROR_CHECK_WITHOUT_ABORT. Макрос ESP_ERROR_CHECK_WITHOUT_ABORT работает так же, как и ESP_ERROR_CHECK, за исключением того, что не вызывает abort().

ESP_RETURN_ON_ERROR. Макрос ESP_RETURN_ON_ERROR проверяет код ошибки, и если он не равен ESP_OK, то печатает сообщение об ошибке и выполняет возврат из вызвавшей макрос функции.

ESP_GOTO_ON_ERROR. Макрос ESP_GOTO_ON_ERROR проверяет код ошибки, и если он не равен ESP_OK, то печатает сообщение, установит локальную переменную ret в err_code, и затем выполнит переход по метки goto_tag.

ESP_RETURN_ON_FALSE. Макрос ESP_RETURN_ON_FALSE проверяет условие, и если оно не равно true, то печатает сообщение и делает возврат с предоставленным err_code.

ESP_GOTO_ON_FALSE. Макрос ESP_GOTO_ON_FALSE проверяет условие, и если оно не равно true, то печатает сообщение, установит локальную переменную ret в предоставленный err_code, и затем выполнит переход по метке goto_tag.


    Autostart mode: It initializes, load some parameters from NVRAM and proceed with startup. 
    Startup means either Formation (for ZC), rejoin or discovery/association join. After startup complete,
    No-autostart mode: It initializes scheduler and buffers pool, but not MAC and upper layers. 
    Notifies the application that Zigbee framework (scheduler, buffer pool, etc.) has started, 
    but no join/rejoin/formation/BDB initialization has been done yet. 
    Typically esp_zb_start with no_autostart mode is used when application wants to do something before starting joining the network.

   For example, you can use this function if it is needed to enable leds, timers or any other devices on periphery to work with them before starting working in a network. 
   It's also useful if you want to run something locally during joining.
   Precondition: stack must be initialized by Zigbee stack is not looped in this routine. 
   Instead, it schedules callback and returns. Caller must run esp_zb_main_loop_iteration() after this routine.
   Application should later call Zigbee commissioning initiation - for instance, esp_zb_bdb_start_top_level_commissioning().

 В одном эндпойнте может быть несколько кластеров.


Пины:
https://docs.espressif.com/projects/esp-idf/en/v5.3.1/esp32/api-reference/peripherals/gpio.html

Установка направления ввода/вывода пина
esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode)
GPIO_MODE_INPUT GPIO_MODE_OUTPUT ...

Чтение пина, настроенного на ввод
 int gpio_get_level(gpio_num_t gpio_num)

Вывод в пин, настроенный на вывод
esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level)

Thus, you just need to set the attribute report callback using esp_zb_device_add_report_attr_cb. Currently, there isn't a context pointer in the parameters, so you'll need to handle that on your own.

To send temperature readings, you'll need to send an attribute report request using esp_zb_zcl_report_attr_cmd_req. Please refer to the documentation to get more details on this API.

 vTaskDelete( NULL )
 