#ifndef ZB_1_H
#define ZB_1_H

#include "esp_zigbee_core.h"
#include "light_driver.h"

// Zigbee configuration
#define INSTALLCODE_POLICY_ENABLE false // enable the install code policy for security /
#define ED_AGING_TIMEOUT ESP_ZB_ED_AGING_TIMEOUT_64MIN
#define ED_KEEP_ALIVE 3000                                               // 10 second
#define ZB1_ENDPOINT_1 1                                                 //
#define ESP_ZB_PRIMARY_CHANNEL_MASK ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK // Zigbee primary channel mask
#define MANUFACTURER_NAME "GSB"
#define MODEL_NAME "ZB1"
#define FIRMWARE_VERSION "v0.1.10"

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

    void coridor_light_control(uint8_t val);
    void hall_light_control(uint8_t val);
    void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct);
    esp_err_t zb_set_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *message);
    esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message);
    void esp_zb_task(void *pvParameters);
    void update_attribute();

#ifdef __cplusplus
}
#endif

#endif