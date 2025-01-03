#include "settings.h"
#ifdef USE_ZIGBEE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "ha/esp_zigbee_ha_standard.h"

#include "zb1.h"

static const char *TAG = "GSB_ZB_2";

static char manufacturer[16], model[16], firmware_version[16];
extern bool connected;

extern bool light_state;
extern bool all_actual;
extern bool coridor_state;     // bit 2
extern bool hall_state;        // bit 3
extern bool coridor_state_act; // реле света в коридоре bit  6
extern bool hall_state_act;    // реле света в прихожей bit  7
extern uint16_t PresentValue;
#if defined USE_TEMP_CHIP
extern int16_t temperature;
#endif

typedef struct device_params_s
{
    esp_zb_ieee_addr_t ieee_addr;
    uint8_t endpoint;
    uint16_t short_addr;
} device_params_t;

device_params_t client_params; // ??

#if defined USE_TEMP_CHIP
static void report_temperature()
{
#ifdef USE_ZIGBEE
    if (connected)
    {
        esp_zb_lock_acquire(portMAX_DELAY);
        esp_zb_zcl_set_attribute_val(ZB2_ENDPOINT_1,
                                     ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
                                     ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                     ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
                                     &temperature,
                                     false);
 //      esp_zb_lock_release();
        //        ESP_LOGI(TAG, "Set attribute");

        esp_zb_zcl_report_attr_cmd_t report_attr_cmd = {0};
        report_attr_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
        report_attr_cmd.attributeID = ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID;
        report_attr_cmd.direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI;
        report_attr_cmd.clusterID = ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT;
        report_attr_cmd.zcl_basic_cmd.src_endpoint = ZB2_ENDPOINT_1;
        report_attr_cmd.zcl_basic_cmd.dst_addr_u.addr_short = 0;
        report_attr_cmd.zcl_basic_cmd.dst_endpoint = 1;

//      esp_zb_lock_acquire(portMAX_DELAY);
        esp_err_t err = esp_zb_zcl_report_attr_cmd_req(&report_attr_cmd);
        esp_zb_lock_release();

        if (err != ESP_OK)
            ESP_LOGW(TAG, "Reporting error");
          
    }
#endif
}
#endif

// Task for update attribute value
// Автоматически буду отправлять редко, чисто для информации
// Реальную отправку делать при изменении параметра сразу.
void update_attribute()
{
    while (true)
    {
        if (connected)
        {
            get_current_state();
        }

        vTaskDelay(60000 / portTICK_PERIOD_MS); // 1 раз в 60 секунд
    }
}

void set_attribute()
{

#ifdef USE_ZIGBEE
    if (connected)
    {
        PresentValue = hall_state_act << 6 | hall_state << 2 | coridor_state_act << 5 | coridor_state << 1;

#ifndef V2
        ESP_LOGI(TAG, "PresentValue 0x%04X", PresentValue);
#endif

        esp_zb_lock_acquire(portMAX_DELAY);
        esp_zb_zcl_set_attribute_val(ZB2_ENDPOINT_1,
                                     ESP_ZB_ZCL_CLUSTER_ID_MULTI_VALUE,
                                     ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                     ESP_ZB_ZCL_ATTR_MULTI_VALUE_PRESENT_VALUE_ID,
                                     &PresentValue,
                                     false);
//        esp_zb_lock_release();
        //        ESP_LOGI(TAG, "Set attribute");

        esp_zb_zcl_report_attr_cmd_t report_attr_cmd = {0};
        report_attr_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
        report_attr_cmd.attributeID = ESP_ZB_ZCL_ATTR_MULTI_VALUE_PRESENT_VALUE_ID;
        report_attr_cmd.direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI;
        report_attr_cmd.clusterID = ESP_ZB_ZCL_CLUSTER_ID_MULTI_VALUE;
        report_attr_cmd.zcl_basic_cmd.src_endpoint = ZB2_ENDPOINT_1;
        report_attr_cmd.zcl_basic_cmd.dst_addr_u.addr_short = 0;
        report_attr_cmd.zcl_basic_cmd.dst_endpoint = 1;

//        esp_zb_lock_acquire(portMAX_DELAY);
        esp_err_t err = esp_zb_zcl_report_attr_cmd_req(&report_attr_cmd);
        esp_zb_lock_release();

        if (err != ESP_OK)
            ESP_LOGW(TAG, "Reporting error");

#ifdef USE_TEMP_CHIP
        report_temperature();
#endif
    }
#endif
}

void set_zcl_string(char *buffer, char *value)
{
    buffer[0] = (char)strlen(value);
    memcpy(buffer + 1, value, buffer[0]);
}

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    ESP_ERROR_CHECK(esp_zb_bdb_start_top_level_commissioning(mode_mask));
}

// Обработчик сообщений от ZBOSS библиотеки
void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    uint32_t *p_sg_p = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;
    switch (sig_type)
    {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        // Устройство уже инициализировано, стартовая инициализация пропускается
        ESP_LOGI(TAG, "Zigbee stack initialized");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        connected = false;
        break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        // Мягкий или аппаратный сброс устройства
        if (err_status == ESP_OK)
        {
            ESP_LOGI(TAG, "Device started up in %s factory-reset mode", esp_zb_bdb_is_factory_new() ? "" : "non");
            if (esp_zb_bdb_is_factory_new())
            {
                ESP_LOGI(TAG, "Start network steering");
                esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
            }
            else
            {
                ESP_LOGI(TAG, "Device rebooted");
                esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
            }
        }
        else
        {
            // commissioning failed
            ESP_LOGW(TAG, "Failed to initialize Zigbee stack (status: %s)", esp_err_to_name(err_status));
        }
        break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
        // Результат поиска координатора и подключения к сети
        if (err_status == ESP_OK)
        {
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);
            connected = true;
            ESP_LOGI(TAG, "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d, Short Address: 0x%04hx)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                     extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                     esp_zb_get_pan_id(), esp_zb_get_current_channel(), esp_zb_get_short_address());
        }
        else
        {
            connected = false;
            ESP_LOGI(TAG, "Network steering was not successful (status: %s)", esp_err_to_name(err_status));
            esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
        }
        break;
        //   case ESP_ZB_NLME_STATUS_INDICATION:
        //   {
        //        printf("%s, status: 0x%x\n", esp_zb_zdo_signal_to_string(sig_type), *(uint8_t *)esp_zb_app_signal_get_params(p_sg_p));
        //   }
        //    break;
    default:
        // Необслуживаемое сообщение
        if (err_status != ESP_OK)
            ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: 0x%x %s",
                     esp_zb_zdo_signal_to_string(sig_type),
                     sig_type,
                     *(uint8_t *)esp_zb_app_signal_get_params(p_sg_p),
                     esp_err_to_name(err_status));
        break;
    }
}

// обработка команд установки аттрибутов от координатора
// Если функция возвращает ESP_OK, то сообщение дальше обрабатывается библиотекой с фиксацией аттрибутов (???)
esp_err_t zb_set_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *message)
{
    esp_err_t ret = ESP_OK;

    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG, TAG, "Received message: error status(%d)", message->info.status);
    ESP_LOGI(TAG, "Received message: endpoint(%d), cluster(0x%x), attribute(0x%x), data size(%d)",
             message->info.dst_endpoint,
             message->info.cluster,
             message->attribute.id,
             message->attribute.data.size);

    if (message->info.dst_endpoint == ZB2_ENDPOINT_1)
    {
        if (message->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_MULTI_VALUE)
        {
            if (message->attribute.id == ESP_ZB_ZCL_ATTR_MULTI_VALUE_PRESENT_VALUE_ID)
            {
                uint16_t value16 = *(uint16_t *)message->attribute.data.value;
                uint8_t value = (value16 & 0xff00) >> 8;
                // в 4-х старших битах старшего байта - какие младшие биты использовать для установки аттрибутов и смены состояния реле
#ifndef V2
                ESP_LOGI(TAG, "CurrentValue 0x%04x  sets to 0x%02x by coordinator", value16, value);
#endif
                uint8_t cmd = 0;

                if ((value & 0x20) == 0x20)
                { // коридор подсветка
                    cmd = (value >> 1) & 0x01;
                    coridor_light_control(cmd);
                }
                if ((value & 0x40) == 0x40)
                { // прихожая подсветка
                    cmd = (value >> 2) & 0x01;
                    hall_light_control(value & 0x04);
                }
            }
        }
    }

    return ret;
}

// обработчик входных сообщений
esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message)
{
    esp_err_t ret = ESP_OK;

    switch (callback_id)
    {
    case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID: // 0
        ret = zb_set_attribute_handler((esp_zb_zcl_set_attr_value_message_t *)message);
        break;
    default:
        ESP_LOGI(TAG, "Receive Zigbee action(0x%x) callback", callback_id);
        break;
    }
    return ret;
}

// задача инициализации ZB
void esp_zb_task(void *pvParameters)
{
    // initialize Zigbee stack
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZED_CONFIG();
    esp_zb_init(&zb_nwk_cfg);

    // Объявления кластеров
    // basic cluster
    set_zcl_string(manufacturer, MANUFACTURER_NAME);
    set_zcl_string(model, MODEL_NAME);
    set_zcl_string(firmware_version, FIRMWARE_VERSION);
    esp_zb_attribute_list_t *esp_zb_basic_cluster = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_BASIC);
    esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, manufacturer);
    esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, model);
    esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_SW_BUILD_ID, firmware_version);
    uint8_t dc_power_source;
    dc_power_source = 4;
    esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_POWER_SOURCE_ID, &dc_power_source);

    // identify cluster
    uint8_t identyfi_id;
    identyfi_id = 0;
    esp_zb_attribute_list_t *esp_zb_identify_cluster = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_IDENTIFY);
    esp_zb_identify_cluster_add_attr(esp_zb_identify_cluster, ESP_ZB_ZCL_CMD_IDENTIFY_IDENTIFY_ID, &identyfi_id);

#if defined USE_TEMP_CHIP
    // Temperature cluster
    int16_t undefined_value, value_min, value_max;
    undefined_value = 0x8000;
    value_min = 0x954d;
    value_max = 0x7ffe;
    esp_zb_attribute_list_t *esp_zb_temperature_meas_cluster = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT);
    esp_zb_temperature_meas_cluster_add_attr(esp_zb_temperature_meas_cluster, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID, &undefined_value);
    esp_zb_temperature_meas_cluster_add_attr(esp_zb_temperature_meas_cluster, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_MIN_VALUE_ID, &value_min);
    esp_zb_temperature_meas_cluster_add_attr(esp_zb_temperature_meas_cluster, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_MAX_VALUE_ID, &value_max);
#endif

    // MultiState Output Cluster
    bool OutOfService = false;
    esp_zb_attribute_list_t *esp_zb_multistate_value_cluster = esp_zb_zcl_attr_list_create(
        ESP_ZB_ZCL_CLUSTER_ID_MULTI_VALUE);
    uint32_t application_type_value = ESP_ZB_ZCL_MV_SET_APP_TYPE_WITH_ID(0xffff, 0x111);
    esp_zb_multistate_value_cluster_add_attr(esp_zb_multistate_value_cluster,
                                             ESP_ZB_ZCL_ATTR_MULTI_VALUE_APPLICATION_TYPE_ID,
                                             &application_type_value);
    char desc[] = "zb1 value";
    esp_zb_multistate_value_cluster_add_attr(esp_zb_multistate_value_cluster,
                                             ESP_ZB_ZCL_ATTR_MULTI_VALUE_DESCRIPTION_ID,
                                             desc);

    esp_zb_multistate_value_cluster_add_attr(esp_zb_multistate_value_cluster,
                                             ESP_ZB_ZCL_ATTR_MULTI_VALUE_OUT_OF_SERVICE_ID,
                                             &OutOfService);
    esp_zb_multistate_value_cluster_add_attr(esp_zb_multistate_value_cluster,
                                             ESP_ZB_ZCL_ATTR_MULTI_VALUE_PRESENT_VALUE_ID,
                                             &PresentValue);

    // Объявление списка кластеров
    esp_zb_cluster_list_t *esp_zb_cluster_list = esp_zb_zcl_cluster_list_create();
    esp_zb_cluster_list_add_basic_cluster(esp_zb_cluster_list,
                                          esp_zb_basic_cluster,
                                          ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_identify_cluster(esp_zb_cluster_list,
                                             esp_zb_identify_cluster,
                                             ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
#ifdef USE_TEMP_CHIP
    esp_zb_cluster_list_add_temperature_meas_cluster(esp_zb_cluster_list,
                                                     esp_zb_temperature_meas_cluster,
                                                     ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
#endif
    esp_zb_cluster_list_add_multistate_value_cluster(esp_zb_cluster_list,
                                                     esp_zb_multistate_value_cluster,
                                                     ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    esp_zb_attribute_list_t *multi_value_cluster =
        esp_zb_cluster_list_get_cluster(esp_zb_cluster_list,
                                        ESP_ZB_ZCL_CLUSTER_ID_MULTI_VALUE,
                                        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_attribute_list_t *attr = multi_value_cluster;
    while (attr)
    {
        if (attr->attribute.id == ESP_ZB_ZCL_ATTR_MULTI_VALUE_PRESENT_VALUE_ID)
        {
            attr->attribute.access = multi_value_cluster->attribute.access |
                                     ESP_ZB_ZCL_ATTR_ACCESS_REPORTING |
                                     ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE;
            break;
        }
        attr = attr->next;
    }

    // Объявление эндпойнтов
    esp_zb_ep_list_t *esp_zb_ep_list = esp_zb_ep_list_create();

    esp_zb_endpoint_config_t endpoint_config = {
        .endpoint = ZB2_ENDPOINT_1,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_ON_OFF_SWITCH_DEVICE_ID,
        .app_device_version = 0};
    esp_zb_ep_list_add_ep(esp_zb_ep_list, esp_zb_cluster_list, endpoint_config);

    // Регистрация списка эндпойнтов
    esp_zb_device_register(esp_zb_ep_list);

    // Config the reporting info
    esp_zb_zcl_reporting_info_t reporting_info1 = {
        .direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_SRV,
        .ep = ZB2_ENDPOINT_1,
        .cluster_id = ESP_ZB_ZCL_CLUSTER_ID_MULTI_VALUE,
        .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        .dst.profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .u.send_info.min_interval = 1,
        .u.send_info.max_interval = 0,
        .u.send_info.def_min_interval = 1,
        .u.send_info.def_max_interval = 0,
        .u.send_info.delta.u16 = 1,
        .attr_id = ESP_ZB_ZCL_ATTR_MULTI_VALUE_PRESENT_VALUE_ID,
        .manuf_code = ESP_ZB_ZCL_ATTR_NON_MANUFACTURER_SPECIFIC,
    };
    esp_zb_zcl_update_reporting_info(&reporting_info1);

#if defined USE_TEMP_CHIP
    esp_zb_zcl_reporting_info_t reporting_info2 = {
        .direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_SRV,
        .ep = ZB2_ENDPOINT_1,
        .cluster_id = ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
        .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        .dst.profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .u.send_info.min_interval = 1,
        .u.send_info.max_interval = 0,
        .u.send_info.def_min_interval = 1,
        .u.send_info.def_max_interval = 0,
        .u.send_info.delta.u16 = 1,
        .attr_id = ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
        .manuf_code = ESP_ZB_ZCL_ATTR_NON_MANUFACTURER_SPECIFIC,
    };
    esp_zb_zcl_update_reporting_info(&reporting_info2);
#endif

    // регистрация обработчика действий (входные команды от координатора)
    esp_zb_core_action_handler_register(zb_action_handler);

    // маска на все каналы TODO: ограничить рабочими из проекта Zhub4
    esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);
    //   esp_zb_set_secondary_network_channel_set(ESP_ZB_SECONDARY_CHANNEL_MASK);
    // Запуск устройства
    ESP_ERROR_CHECK(esp_zb_start(false));

    esp_zb_stack_main_loop();
}
#endif
