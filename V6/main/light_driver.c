// Управление RGB светодиодом на плате
#include "esp_log.h"
#include "led_strip.h"
#include "light_driver.h"

#define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)  // меньше 10 МГц вообще не работает!!!

static led_strip_handle_t s_led_strip;
static uint8_t s_red = 10, s_green = 10, s_blue = 10;

// Если power = false, светодиод выключается
// если true, светодиоды включается согласно текущим состояниям s_red , s_green , s_blue
void light_driver_set_power(bool power)
{
    ESP_ERROR_CHECK(led_strip_set_pixel(s_led_strip, 0, s_red * power, s_green * power, s_blue * power));
    if (power)
        ESP_ERROR_CHECK(led_strip_refresh(s_led_strip));
    else
        ESP_ERROR_CHECK(led_strip_clear(s_led_strip));
}

uint32_t get_RGB()
{
    return (s_red << 16) + (s_green << 8) + s_blue;
}
void set_RGB(uint32_t rgb)
{
    s_blue = (uint8_t)rgb;
    rgb = rgb >> 8;
    s_green = (uint8_t)rgb;
    rgb = rgb >> 8;
    s_red = (uint8_t)rgb;
}
void set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    s_red = r;
    s_green = g;
    s_blue = b;
}

void light_driver_set_green(uint8_t power)
{
    s_green = power;
}
void light_driver_set_red(uint8_t power)
{
    s_red = power;
}
void light_driver_set_blue(uint8_t power)
{
    s_blue = power;
}

void light_driver_init(bool power)
{
    led_strip_config_t led_strip_conf = {
        .max_leds = CONFIG_EXAMPLE_STRIP_LED_NUMBER,
        .strip_gpio_num = CONFIG_EXAMPLE_STRIP_LED_GPIO,
        .flags = {
            .invert_out = false, // don't invert the output signal
        }

    };
    led_strip_rmt_config_t rmt_conf = {
        .resolution_hz = LED_STRIP_RMT_RES_HZ, // 1MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&led_strip_conf, &rmt_conf, &s_led_strip));
    light_driver_set_power(power);
}
