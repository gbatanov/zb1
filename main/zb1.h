#ifndef ZB_1_H
#define ZB_1_H

#include "esp_zigbee_core.h"
#include "light_driver.h"

// Zigbee configuration
#define INSTALLCODE_POLICY_ENABLE false // enable the install code policy for security /
#define ED_AGING_TIMEOUT ESP_ZB_ED_AGING_TIMEOUT_64MIN
#define ED_KEEP_ALIVE 3000 // 10 second
#define ZB1_ENDPOINT_1 1   // Коридор люстра - бит 1
// #define ZB1_ENDPOINT_2 2                                                 // Коридор подсветка - бит 2
// #define ZB1_ENDPOINT_3 3                                                 // Прихожая подсветка - бит 3
// #define ZB1_ENDPOINT_4 4                                                 // Датчик движения - бит 4
// #define ZB1_ENDPOINT_5 5                                                 // RGB  Led - только внутреннее управление
#define ESP_ZB_PRIMARY_CHANNEL_MASK ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK // Zigbee primary channel mask
#define MANUFACTURER_NAME "GSB"
#define MODEL_NAME "ZB1"
#define FIRMWARE_VERSION "v0.4.1"

#define ESP_ZB_ZED_CONFIG()                               \
    {                                                     \
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_ED,             \
        .install_code_policy = INSTALLCODE_POLICY_ENABLE, \
        .nwk_cfg.zed_cfg = {                              \
            .ed_timeout = ED_AGING_TIMEOUT,               \
            .keep_alive = ED_KEEP_ALIVE,                  \
        },                                                \
    }

#ifdef __cplusplus
extern "C"
{
#endif
    void get_current_state();
    void set_attribute();
    void lcd_task(void *pvParameters);
    void button_single_click_cb(void *arg, void *usr_data);
    void register_buttons();
    void luster_control(void *arg, void *usr_data);
    void luster_control_remote(uint8_t);
    void coridor_light_control(uint8_t val);
    void hall_light_control(uint8_t val);
    void motion_cb(void *arg, void *usr_data);
    void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct);
    esp_err_t zb_set_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *message);
    esp_err_t zb_read_attr_resp_handler(const esp_zb_zcl_cmd_read_attr_resp_message_t *message);
    esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message);
    void esp_zb_task(void *pvParameters);
    void send_onoff_cmd(uint8_t endpoint, uint8_t state);
    void update_attribute();
#ifdef __cplusplus
}
#endif

#endif