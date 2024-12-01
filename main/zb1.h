#ifndef ZB_1_H
#define ZB_1_H

#include "esp_zigbee_core.h"
#include "light_driver.h"

typedef struct esp_zb_zb1_cfg_s
{
    esp_zb_basic_cluster_cfg_t basic_cfg;       // Basic cluster configuration, @ref esp_zb_basic_cluster_cfg_s
    esp_zb_identify_cluster_cfg_t identify_cfg; // Identify cluster configuration, @ref esp_zb_identify_cluster_cfg_s
    esp_zb_on_off_cluster_cfg_t on_off_cfg;     //  On off cluster configuration, @ref esp_zb_on_off_cluster_cfg_s
    esp_zb_level_cluster_cfg_t level_cfg;       //  Level cluster configuration, @ref esp_zb_level_cluster_cfg_s
    //    esp_zb_temperature_meas_cluster_cfg_t temperature_cfg; // Temperature Measurement cluster configuration
} esp_zb_zb1_cfg_t;

// Zigbee configuration
// #define ESP_ZB_ZCL_CLUSTER_ID_GSB_ZB1 0xffee// Custom cluster (начинаются с FC00)
// #define ESP_ZB_ZCL_ATTR_GSB_ZB1_ID 0 //
#define INSTALLCODE_POLICY_ENABLE false // enable the install code policy for security /
#define ED_AGING_TIMEOUT ESP_ZB_ED_AGING_TIMEOUT_64MIN
#define ED_KEEP_ALIVE 3000                                               // 10 second
#define ZB1_ENDPOINT_1 1                                                 // Коридор люстра
#define ZB1_ENDPOINT_2 2                                                 // Коридор подсветка
#define ZB1_ENDPOINT_3 3                                                 // Прихожая подсветка
#define ZB1_ENDPOINT_4 4                                                 // Датчик движения
#define ZB1_ENDPOINT_5 5                                                 // RGB  Led
#define ESP_ZB_PRIMARY_CHANNEL_MASK ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK // Zigbee primary channel mask
#define MANUFACTURER_NAME "GSB"
#define MODEL_NAME "ZB1"
#define FIRMWARE_VERSION "v0.2.4"

#define ESP_ZB_ZED_CONFIG()                               \
    {                                                     \
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_ED,             \
        .install_code_policy = INSTALLCODE_POLICY_ENABLE, \
        .nwk_cfg.zed_cfg = {                              \
            .ed_timeout = ED_AGING_TIMEOUT,               \
            .keep_alive = ED_KEEP_ALIVE,                  \
        },                                                \
    }

/*
#define ZB1_EP_CONFIG()                                                                \
    {                                                                                  \
        .basic_cfg =                                                                   \
            {                                                                          \
                .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,             \
                .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE,           \
            },                                                                         \
        .identify_cfg =                                                                \
            {                                                                          \
                .identify_time = ESP_ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE,      \
            },                                                                         \
        .on_off_cfg =                                                                  \
            {                                                                          \
                .on_off = ESP_ZB_ZCL_ON_OFF_ON_OFF_DEFAULT_VALUE,                      \
            },                                                                         \
        .level_cfg =                                                                   \
            {                                                                          \
                .current_level = ESP_ZB_ZCL_LEVEL_CONTROL_CURRENT_LEVEL_DEFAULT_VALUE, \
            },                                                                         \
    }


#define ESP_ZB_DEFAULT_RADIO_CONFIG()    \
    {                                    \
        .radio_mode = RADIO_MODE_NATIVE, \
    }

#define ESP_ZB_DEFAULT_HOST_CONFIG()                       \
    {                                                      \
        .host_connection_mode = HOST_CONNECTION_MODE_NONE, \
    }
*/
#ifdef __cplusplus
extern "C"
{
#endif
    void lcd_task(void *pvParameters);
    void reportAttribute();
    void button_single_click_cb(void *arg, void *usr_data);
    void register_buttons();
    void luster_control(void *arg, void *usr_data);
    void luster_control_remote();
    void coridor_light_control(uint8_t val);
    void hall_light_control(uint8_t val);
    void luminocity_cb(void *arg, void *usr_data);
    void motion_cb(void *arg, void *usr_data);
    void motion_short_cb(void *arg, void *usr_data);
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