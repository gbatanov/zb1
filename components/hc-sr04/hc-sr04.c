// Компонент работы с ультразвуковым датчиком HC-SR04
/* для большей точности установим значение LOW на пине Trig
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Теперь установим высокий уровень на пине Trig
  digitalWrite(trigPin, HIGH);
  // Подождем 10 μs
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Узнаем длительность высокого сигнала на пине Echo
  duration = pulseIn(echoPin, HIGH);
  // Рассчитаем расстояние
  distance = duration / 58;
*/
#include "hc-sr04.h"

extern const char *TAG;
HCSR04 sonar;
uint16_t distance = 0;

void create_sonar(int trigPin, int echoPin)
{
    sonar.ECHO_PIN = echoPin;
    sonar.TRIG_PIN = trigPin;
    gpio_pad_select_gpio(sonar.ECHO_PIN);
    gpio_set_direction(sonar.ECHO_PIN, GPIO_MODE_INPUT); // ECHO pin - input
    gpio_pad_select_gpio(sonar.TRIG_PIN);
    gpio_set_direction(sonar.TRIG_PIN, GPIO_MODE_OUTPUT); // TRIG pin - output
}

void sonar_task(void *pvParameters)
{
    create_sonar(TRIG_PIN_NUM, ECHO_PIN_NUM);
    uint32_t duration = 0;
    while (1)
    {
        duration = 0;
        // отправляем импульс длительностью 10 микросекунд
        gpio_set_level(sonar.TRIG_PIN, (uint32_t)0); // выводим на триггер 0
        esp_rom_delay_us(2);                         // задержка 2 микросекунды
        gpio_set_level(sonar.TRIG_PIN, (uint32_t)1); // выводим на триггер 1
        esp_rom_delay_us(10);                        // задержка 10 микросекунды
        gpio_set_level(sonar.TRIG_PIN, (uint32_t)0); // выводим на триггер 0
        bool state = false;
        uint8_t counter = 0;
        //           ESP_LOGI(TAG, "Включи!");
        do
        {
            //              vTaskDelay(100 / portTICK_PERIOD_MS);
            state = (bool)gpio_get_level(sonar.ECHO_PIN);
            esp_rom_delay_us(1);

            counter++;
            if (counter > 100)
            {
                ESP_LOGI(TAG, "Не успел!");
                break;
            }
        } while (!state);
        while (state && duration < 23200)
        {
            // 3,4 cm in microsecond
            state = (bool)gpio_get_level(sonar.ECHO_PIN);
            esp_rom_delay_us(1);
            duration++;
        }

        if (duration > 0 && duration < 23200)
        {
            distance = duration / 58;
            ESP_LOGI(TAG, "Duration %d cm", distance);
        }
        else
        {
            ESP_LOGI(TAG, "Duration no");
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}