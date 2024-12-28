// Управление RGB светодиодом на плате
#include "esp_log.h"
#include "led_strip.h"
#include "light_driver.h"

static led_strip_handle_t s_led_strip;
static uint8_t s_red = 255, s_green = 255, s_blue = 255;

void light_driver_set_power(bool power)
{
    ESP_ERROR_CHECK(led_strip_set_pixel(s_led_strip, 0, s_red * power, s_green * power, s_blue * power));
    ESP_ERROR_CHECK(led_strip_refresh(s_led_strip));
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
    };
    led_strip_rmt_config_t rmt_conf = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&led_strip_conf, &rmt_conf, &s_led_strip));
    light_driver_set_power(power);
}
