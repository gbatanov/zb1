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

static char manufacturer[16], model[16], firmware_version[16];
static const char *TAG = "GSB_ZB_1";
extern bool connected;
#ifdef USE_BMP280
extern int16_t temperature;
#endif
extern int8_t luminocity;
extern bool lum_change;
#ifdef USE_DISPLAY
extern int lcd_timeout;
extern uint8_t screen_number;
#endif
extern bool light_state;
extern bool luster_state;
extern bool coridor_state;
extern bool hall_state;
extern int8_t motion_state;

typedef struct device_params_s
{
    esp_zb_ieee_addr_t ieee_addr;
    uint8_t endpoint;
    uint16_t short_addr;
} device_params_t;

device_params_t client_params; // ??

static void bind_cb(esp_zb_zdp_status_t zdo_status, void *user_ctx)
{
    ESP_LOGI(TAG, "bind_cb status 0x%02x", zdo_status); // ESP_ZB_ZDP_STATUS_INVALID_EP  = 0x82,
    if (zdo_status == ESP_ZB_ZDP_STATUS_SUCCESS)
    {
        ESP_LOGI(TAG, "Bind successful");

        esp_zb_zcl_config_report_cmd_t report_cmd;
        bool report_change = 0;
        report_cmd.zcl_basic_cmd.dst_addr_u.addr_short = 0; // client_params.short_addr;
        report_cmd.zcl_basic_cmd.dst_endpoint = 1;          // client_params.endpoint;
        report_cmd.zcl_basic_cmd.src_endpoint = ZB1_ENDPOINT_1;
        report_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
        report_cmd.clusterID = ESP_ZB_ZCL_CLUSTER_ID_ON_OFF;

        esp_zb_zcl_config_report_record_t record = {{ESP_ZB_ZCL_REPORT_DIRECTION_SEND,
                                                     ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID,
                                                     ESP_ZB_ZCL_ATTR_TYPE_BOOL,
                                                     0,
                                                     30,
                                                     &report_change}};
        report_cmd.record_number = 1; // sizeof(records) / sizeof(esp_zb_zcl_config_report_record_t);
        report_cmd.record_field = &record;
        esp_zb_lock_acquire(portMAX_DELAY);
        esp_zb_zcl_config_report_cmd_req(&report_cmd);
        esp_zb_lock_release();
    }
}

static void bind_cb2(esp_zb_zdp_status_t zdo_status, void *user_ctx)
{
    ESP_LOGI(TAG, "bind_cb2 status 0x%02x", zdo_status); // ESP_ZB_ZDP_STATUS_INVALID_EP  = 0x82,
    if (zdo_status == ESP_ZB_ZDP_STATUS_SUCCESS)
    {
        ESP_LOGI(TAG, "Bind 2 successful");

        esp_zb_zcl_config_report_cmd_t report_cmd;
        bool report_change = 0;
        report_cmd.zcl_basic_cmd.dst_addr_u.addr_short = 0; // client_params.short_addr;
        report_cmd.zcl_basic_cmd.dst_endpoint = 1;          // client_params.endpoint;
        report_cmd.zcl_basic_cmd.src_endpoint = ZB1_ENDPOINT_2;
        report_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
        report_cmd.clusterID = ESP_ZB_ZCL_CLUSTER_ID_ON_OFF;

        esp_zb_zcl_config_report_record_t record = {
            {ESP_ZB_ZCL_REPORT_DIRECTION_SEND,
             ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID,
             ESP_ZB_ZCL_ATTR_TYPE_BOOL,
             0,
             30,
             &report_change},
        };
        report_cmd.record_number = 1; // sizeof(records) / sizeof(esp_zb_zcl_config_report_record_t);
        report_cmd.record_field = &record;

        esp_zb_lock_acquire(portMAX_DELAY);
        esp_zb_zcl_config_report_cmd_req(&report_cmd);
        esp_zb_lock_release();
    }
}

static void bind_cb3(esp_zb_zdp_status_t zdo_status, void *user_ctx)
{
    ESP_LOGI(TAG, "bind_cb3 status 0x%02x", zdo_status); // ESP_ZB_ZDP_STATUS_INVALID_EP  = 0x82,
    if (zdo_status == ESP_ZB_ZDP_STATUS_SUCCESS)
    {
        ESP_LOGI(TAG, "Bind 3 successful");

        esp_zb_zcl_config_report_cmd_t report_cmd;
        bool report_change = 0;
        report_cmd.zcl_basic_cmd.dst_addr_u.addr_short = 0; // client_params.short_addr;
        report_cmd.zcl_basic_cmd.dst_endpoint = 1;          // client_params.endpoint;
        report_cmd.zcl_basic_cmd.src_endpoint = ZB1_ENDPOINT_3;
        report_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
        report_cmd.clusterID = ESP_ZB_ZCL_CLUSTER_ID_ON_OFF;

        esp_zb_zcl_config_report_record_t record = {
            {ESP_ZB_ZCL_REPORT_DIRECTION_SEND,
             ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID,
             ESP_ZB_ZCL_ATTR_TYPE_BOOL,
             0,
             30,
             &report_change},
        };
        report_cmd.record_number = 1; // sizeof(records) / sizeof(esp_zb_zcl_config_report_record_t);
        report_cmd.record_field = &record;

        esp_zb_lock_acquire(portMAX_DELAY);
        esp_zb_zcl_config_report_cmd_req(&report_cmd);
        esp_zb_lock_release();
    }
}

static void bind_cb4(esp_zb_zdp_status_t zdo_status, void *user_ctx)
{
    ESP_LOGI(TAG, "bind_cb4 status 0x%02x", zdo_status); // ESP_ZB_ZDP_STATUS_INVALID_EP  = 0x82,
    if (zdo_status == ESP_ZB_ZDP_STATUS_SUCCESS)
    {
        ESP_LOGI(TAG, "Bind 4 successful");

        esp_zb_zcl_config_report_cmd_t report_cmd;
        bool report_change = 0;
        report_cmd.zcl_basic_cmd.dst_addr_u.addr_short = 0; // client_params.short_addr;
        report_cmd.zcl_basic_cmd.dst_endpoint = 1;          // client_params.endpoint;
        report_cmd.zcl_basic_cmd.src_endpoint = ZB1_ENDPOINT_4;
        report_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
        report_cmd.clusterID = ESP_ZB_ZCL_CLUSTER_ID_ON_OFF;

        esp_zb_zcl_config_report_record_t record = {
            {ESP_ZB_ZCL_REPORT_DIRECTION_SEND,
             ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID,
             ESP_ZB_ZCL_ATTR_TYPE_BOOL,
             0,
             30,
             &report_change},
        };
        report_cmd.record_number = 1; // sizeof(records) / sizeof(esp_zb_zcl_config_report_record_t);
        report_cmd.record_field = &record;

        esp_zb_lock_acquire(portMAX_DELAY);
        esp_zb_zcl_config_report_cmd_req(&report_cmd);
        esp_zb_lock_release();
    }
}

static void ieee_cb(esp_zb_zdp_status_t zdo_status, esp_zb_ieee_addr_t ieee_addr, void *user_ctx)
{

    ESP_LOGI(TAG, "ieee_cb  status: 0x%02x", zdo_status); // ESP_ZB_ZDP_STATUS_TIMEOUT = 0x85,

    if (zdo_status == ESP_ZB_ZDP_STATUS_SUCCESS)
    {
        memcpy(&(client_params.ieee_addr), ieee_addr, sizeof(esp_zb_ieee_addr_t));
        ESP_LOGI(TAG, "IEEE address: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                 ieee_addr[7], ieee_addr[6], ieee_addr[5], ieee_addr[4],
                 ieee_addr[3], ieee_addr[2], ieee_addr[1], ieee_addr[0]);

        esp_zb_zdo_bind_req_param_t bind_req;
        memcpy(&(bind_req.src_address), client_params.ieee_addr, sizeof(esp_zb_ieee_addr_t));
        bind_req.src_endp = ZB1_ENDPOINT_1;
        bind_req.cluster_id = ESP_ZB_ZCL_CLUSTER_ID_ON_OFF;
        bind_req.dst_addr_mode = ESP_ZB_ZDO_BIND_DST_ADDR_MODE_64_BIT_EXTENDED;
        esp_zb_get_long_address(bind_req.dst_address_u.addr_long);
        bind_req.dst_endp = 1; // client_params.endpoint;
        bind_req.req_dst_addr = client_params.short_addr;
        esp_zb_lock_acquire(portMAX_DELAY);
        esp_zb_zdo_device_bind_req(&bind_req, bind_cb, NULL);
        esp_zb_lock_release();

        esp_zb_zdo_bind_req_param_t bind_req2;
        memcpy(&(bind_req2.src_address), client_params.ieee_addr, sizeof(esp_zb_ieee_addr_t));
        bind_req2.src_endp = ZB1_ENDPOINT_2;
        bind_req2.cluster_id = ESP_ZB_ZCL_CLUSTER_ID_ON_OFF;
        bind_req2.dst_addr_mode = ESP_ZB_ZDO_BIND_DST_ADDR_MODE_64_BIT_EXTENDED;
        esp_zb_get_long_address(bind_req2.dst_address_u.addr_long);
        bind_req2.dst_endp = 1; // client_params.endpoint;
        bind_req2.req_dst_addr = client_params.short_addr;
        esp_zb_lock_acquire(portMAX_DELAY);
        esp_zb_zdo_device_bind_req(&bind_req2, bind_cb2, NULL);
        esp_zb_lock_release();


        esp_zb_zdo_bind_req_param_t bind_req3;
        memcpy(&(bind_req3.src_address), client_params.ieee_addr, sizeof(esp_zb_ieee_addr_t));
        bind_req3.src_endp = ZB1_ENDPOINT_3;
        bind_req3.cluster_id = ESP_ZB_ZCL_CLUSTER_ID_ON_OFF;
        bind_req3.dst_addr_mode = ESP_ZB_ZDO_BIND_DST_ADDR_MODE_64_BIT_EXTENDED;
        esp_zb_get_long_address(bind_req3.dst_address_u.addr_long);
        bind_req3.dst_endp = 1; // client_params.endpoint;
        bind_req3.req_dst_addr = client_params.short_addr;
        esp_zb_lock_acquire(portMAX_DELAY);
        esp_zb_zdo_device_bind_req(&bind_req3, bind_cb3, NULL);
        esp_zb_lock_release();

        esp_zb_zdo_bind_req_param_t bind_req4;
        memcpy(&(bind_req4.src_address), client_params.ieee_addr, sizeof(esp_zb_ieee_addr_t));
        bind_req4.src_endp = ZB1_ENDPOINT_4;
        bind_req4.cluster_id = ESP_ZB_ZCL_CLUSTER_ID_ON_OFF;
        bind_req4.dst_addr_mode = ESP_ZB_ZDO_BIND_DST_ADDR_MODE_64_BIT_EXTENDED;
        esp_zb_get_long_address(bind_req4.dst_address_u.addr_long);
        bind_req4.dst_endp = 1; // client_params.endpoint;
        bind_req4.req_dst_addr = client_params.short_addr;
        esp_zb_lock_acquire(portMAX_DELAY);
        esp_zb_zdo_device_bind_req(&bind_req4, bind_cb4, NULL);
        esp_zb_lock_release();
    }
}
// Task for update attribute value
// Автоматически буду отправлять редко, чисто для информации
// Реальную отправку делать при изменении параметра сразу.
void update_attribute()
{
    while (1)
    {
#ifdef USE_BMP280
        //       ESP_LOGI(TAG, "Temperature (update) %d", temperature); // здесь целое со знаком, умнженное на 100 (по стандарту)
#endif
        //       ESP_LOGI(TAG, "Luminocity (update) %d", luminocity);                 //
        //       ESP_LOGI(TAG, "Luster state (update) %d", (uint8_t)!luster_state);   //
        //       ESP_LOGI(TAG, "Coridor state (update) %d", (uint8_t)!coridor_state); //
        //       ESP_LOGI(TAG, "Hall state (update) %d", (uint8_t)!hall_state);       //
        //       ESP_LOGI(TAG, "Motion state (update) %d", motion);          //

        if (connected)
        {
            esp_zb_zcl_status_t state = ESP_ZB_ZCL_STATUS_SUCCESS;

            esp_zb_lock_acquire(portMAX_DELAY);
            state = esp_zb_zcl_set_attribute_val(ZB1_ENDPOINT_4, ESP_ZB_ZCL_CLUSTER_ID_ON_OFF, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, 0, &motion_state, false);
            esp_zb_lock_release();
            if (state != ESP_ZB_ZCL_STATUS_SUCCESS)
            {
                ESP_LOGI(TAG, "Setting attribute: failed! 0x%04x", state);
            }

            esp_zb_lock_acquire(portMAX_DELAY);
            state = esp_zb_zcl_set_attribute_val(ZB1_ENDPOINT_3, ESP_ZB_ZCL_CLUSTER_ID_ON_OFF, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, 0, &hall_state, false);
            esp_zb_lock_release();
            if (state != ESP_ZB_ZCL_STATUS_SUCCESS)
            {
                ESP_LOGI(TAG, "Setting attribute: failed! 0x%04x", state);
            }

           esp_zb_lock_acquire(portMAX_DELAY);
            state = esp_zb_zcl_set_attribute_val(ZB1_ENDPOINT_2, ESP_ZB_ZCL_CLUSTER_ID_ON_OFF, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, 0, &coridor_state, false);
            esp_zb_lock_release();
            if (state != ESP_ZB_ZCL_STATUS_SUCCESS)
            {
                ESP_LOGI(TAG, "Setting attribute: failed! 0x%04x", state);
            }

            esp_zb_lock_acquire(portMAX_DELAY);
            state = esp_zb_zcl_set_attribute_val(ZB1_ENDPOINT_1, ESP_ZB_ZCL_CLUSTER_ID_ON_OFF, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, 0, &luster_state, false);
            esp_zb_lock_release();
            if (state != ESP_ZB_ZCL_STATUS_SUCCESS)
            {
                ESP_LOGI(TAG, "Setting attribute: failed! 0x%04x", state);
            }
        }

        vTaskDelay(10000 / portTICK_PERIOD_MS); // 1 раз в 10 секунд, на продакшене увеличу до минуты
    }
}

// Manual reporting atribute to coordinator
// Отправляю все состояния в одном сообщении
// Один кластер, один аттрибут
// аттрибуты внутри сообщения:
// 1 - состояние реле1 (люстра)
// 2 - состояние реле2 (коридор)
// 3 - состояние реле3 (прихожая)
// 4 - состояние датчика движения (команда уходит в кластере OnOff)
// Все типы булевы
void reportAttribute()
{

    uint8_t value = 0;
    if (connected)
    {

        value = (uint8_t)luster_state;
        //      value[1] = (uint8_t)coridor_state;
        //       value[2] = (uint8_t)hall_state;
        //     value[3] = (uint8_t)motion_state;

        esp_zb_zcl_report_attr_cmd_t cmd = {
            .zcl_basic_cmd = {
                .dst_addr_u.addr_short = 0x0000,
                .dst_endpoint = 1, // Здесь должен быть номер эндпойнта координатора!
                .src_endpoint = ZB1_ENDPOINT_1,
            },
            .address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
            .clusterID = ESP_ZB_ZCL_CLUSTER_ID_ON_OFF,
            .manuf_specific = 0,
            .direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_SRV,
            .dis_defalut_resp = 1,
            .manuf_code = 0xffff,
            .attributeID = 0,
        };
        esp_zb_zcl_status_t state = ESP_ZB_ZCL_STATUS_SUCCESS;

        // записать аттрибут (check = false)
        esp_zb_lock_acquire(portMAX_DELAY);
        state = esp_zb_zcl_set_attribute_val(ZB1_ENDPOINT_1, ESP_ZB_ZCL_CLUSTER_ID_ON_OFF, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, 0, &value, false);
        esp_zb_lock_release();
        if (state != ESP_ZB_ZCL_STATUS_SUCCESS)
        {
            ESP_LOGI(TAG, "Setting attribute :failed! 0x%04x", state);
        }
        // ESP_ZB_ZCL_STATUS_READ_ONLY             = 0x88U

        // установка значения аттрибута через область памяти аттрибута
        //        esp_zb_lock_acquire(portMAX_DELAY);
        //              esp_zb_zcl_attr_t *value_r = esp_zb_zcl_get_attribute(
        //               ZB1_ENDPOINT_1, ESP_ZB_ZCL_CLUSTER_ID_ON_OFF, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, 0);
        ///            memcpy(value_r->data_p, &value, 1); //  тут падает

        //       esp_zb_lock_acquire(portMAX_DELAY);
        //       esp_zb_zcl_report_attr_cmd_req(&cmd);
        //            esp_zb_lock_release();
    }
}

void send_onoff_cmd(uint8_t endpoint, uint8_t state)
{

    esp_zb_zcl_on_off_cmd_t cmd = {
        .zcl_basic_cmd = {
            .dst_addr_u.addr_short = 0x0000,
            .dst_endpoint = 1, // Здесь должен быть номер эндпойнта координатора!
            .src_endpoint = endpoint,
        },
        .address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
        .on_off_cmd_id = state,
    };
    esp_zb_lock_acquire(portMAX_DELAY);
    esp_zb_zcl_status_t res = esp_zb_zcl_on_off_cmd_req(&cmd);
    esp_zb_lock_release();
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

            esp_zb_zdo_ieee_addr_req_param_t ieee_req;
            ieee_req.addr_of_interest = 0;
            ieee_req.dst_nwk_addr = 0;
            ieee_req.request_type = 0;
            ieee_req.start_index = 0;
            esp_zb_zdo_ieee_addr_req(&ieee_req, ieee_cb, NULL);
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
    uint8_t light_level = 0;

    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG, TAG, "Received message: error status(%d)", message->info.status);
    ESP_LOGI(TAG, "Received message: endpoint(%d), cluster(0x%x), attribute(0x%x), data size(%d)",
             message->info.dst_endpoint,
             message->info.cluster,
             message->attribute.id,
             message->attribute.data.size);

    if (message->info.dst_endpoint == ZB1_ENDPOINT_1)
    {
        if (message->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_ON_OFF)
        {
            if (message->attribute.id == 0)
            {
                luster_control_remote();
                ESP_LOGI(TAG, "Luster relay sets to %s by coordinator", !luster_state ? "On" : "Off");
            }
        }
    }

   if (message->info.dst_endpoint == ZB1_ENDPOINT_2)
    {
        if (message->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_ON_OFF)
        {
            if (message->attribute.id == 0)
            {
                coridor_light_control(*(uint8_t *)message->attribute.data.value);
                ESP_LOGI(TAG, "Luster relay sets to %s by coordinator", !luster_state ? "On" : "Off");
            }
        }
    }
   if (message->info.dst_endpoint == ZB1_ENDPOINT_3)
    {
        if (message->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_ON_OFF)
        {
            if (message->attribute.id == 0)
            {
                hall_light_control(*(uint8_t *)message->attribute.data.value);
                ESP_LOGI(TAG, "Luster relay sets to %s by coordinator", !luster_state ? "On" : "Off");
            }
        }
    }

    return ret;
}

// обработка команд чтения аттрибутов от координатора
esp_err_t zb_read_attr_resp_handler(const esp_zb_zcl_cmd_read_attr_resp_message_t *message)
{
    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG, TAG, "Received message: error status(%d)",
                        message->info.status);
    ESP_LOGI(TAG, "Read attribute response: status(%d), cluster(0x%x), attribute(0x%x),  value(0x%x)",
             message->info.status,
             message->info.cluster,
             message->variables->attribute.id,
             *(uint8_t *)message->variables->attribute.data.value);
    if (message->info.dst_endpoint == ZB1_ENDPOINT_5)
    {
        switch (message->info.cluster)
        {
        case ESP_ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL:
            ESP_LOGI(TAG, "Server data recieved %d", *(uint8_t *)message->variables->attribute.data.value);
            break;
        default:
            ESP_LOGI(TAG, "Message data: cluster(0x%x), attribute(0x%x)  ", message->info.cluster, message->variables->attribute.id);
        }
    }
    return ESP_OK;
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
    case ESP_ZB_CORE_CMD_READ_ATTR_RESP_CB_ID: // 4096
        ret = zb_read_attr_resp_handler((esp_zb_zcl_cmd_read_attr_resp_message_t *)message);
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

    // Level cluster
    uint8_t level_id, min_level_id, max_level_id, options_id;
    level_id = 0;
    min_level_id = 0;
    max_level_id = 255;
    options_id = 0x01;
    esp_zb_attribute_list_t *esp_zb_level_cluster = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL);
    esp_zb_level_cluster_add_attr(esp_zb_level_cluster, ESP_ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID, &level_id);
    esp_zb_level_cluster_add_attr(esp_zb_level_cluster, ESP_ZB_ZCL_ATTR_LEVEL_CONTROL_MIN_LEVEL_ID, &min_level_id);
    esp_zb_level_cluster_add_attr(esp_zb_level_cluster, ESP_ZB_ZCL_ATTR_LEVEL_CONTROL_MAX_LEVEL_ID, &max_level_id);
    esp_zb_level_cluster_add_attr(esp_zb_level_cluster, ESP_ZB_ZCL_ATTR_LEVEL_CONTROL_OPTIONS_ID, &options_id);
#ifdef USE_BMP280
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
    // OnOff cluster отправка команд состояния датчика движения
    esp_zb_attribute_list_t *esp_zb_onoff_cluster = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_ON_OFF);
    esp_zb_on_off_cluster_add_attr(esp_zb_onoff_cluster, 0, &luster_state);
    /*
        // кастомный кластер
        uint8_t undefined_value_zb1 = 0xff;
        const uint16_t attr_id = 0;
        const uint8_t attr_type = ESP_ZB_ZCL_ATTR_TYPE_8BIT;
        const uint8_t attr_access = ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE | ESP_ZB_ZCL_ATTR_ACCESS_REPORTING;

        esp_zb_attribute_list_t *gsb_zb1_cluster = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_GSB_ZB1);
        esp_zb_custom_cluster_add_custom_attr(gsb_zb1_cluster, attr_id, attr_type, attr_access, &undefined_value_zb1);
    */
    // Объявление эндпойнтов
    esp_zb_ep_list_t *esp_zb_ep_list = esp_zb_ep_list_create();

    // Объявление списка кластеров
    esp_zb_cluster_list_t *esp_zb_cluster_list = esp_zb_zcl_cluster_list_create();
    esp_zb_cluster_list_add_basic_cluster(esp_zb_cluster_list, esp_zb_basic_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_identify_cluster(esp_zb_cluster_list, esp_zb_identify_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_on_off_cluster(esp_zb_cluster_list, esp_zb_onoff_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
#ifdef USE_BMP280
    esp_zb_cluster_list_add_temperature_meas_cluster(esp_zb_cluster_list, esp_zb_temperature_meas_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
#endif
    //   esp_zb_cluster_list_add_custom_cluster(esp_zb_cluster_list, gsb_zb1_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    esp_zb_endpoint_config_t endpoint_config = {
        .endpoint = ZB1_ENDPOINT_1,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_COMBINED_INTERFACE_DEVICE_ID,
        .app_device_version = 0};
    esp_zb_ep_list_add_ep(esp_zb_ep_list, esp_zb_cluster_list, endpoint_config);

    // ZB_ENDPOINT_2 - Коридор подсветка
    esp_zb_attribute_list_t *esp_zb_onoff_cluster_2 = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_ON_OFF);
    esp_zb_on_off_cluster_add_attr(esp_zb_onoff_cluster_2, 0, &coridor_state);
    esp_zb_cluster_list_t *esp_zb_cluster_list_2 = esp_zb_zcl_cluster_list_create();
    esp_zb_cluster_list_add_on_off_cluster(esp_zb_cluster_list_2, esp_zb_onoff_cluster_2, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    esp_zb_endpoint_config_t endpoint_config_2 = {
        .endpoint = ZB1_ENDPOINT_2,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_ON_OFF_OUTPUT_DEVICE_ID,
        .app_device_version = 0};
    esp_zb_ep_list_add_ep(esp_zb_ep_list, esp_zb_cluster_list_2, endpoint_config_2);


    // ZB_ENDPOINT_3 - Прихожая подсветка
    esp_zb_attribute_list_t *esp_zb_onoff_cluster_3 = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_ON_OFF);
    esp_zb_on_off_cluster_add_attr(esp_zb_onoff_cluster_3, 0, &hall_state);
    esp_zb_cluster_list_t *esp_zb_cluster_list_3 = esp_zb_zcl_cluster_list_create();
    esp_zb_cluster_list_add_on_off_cluster(esp_zb_cluster_list_3, esp_zb_onoff_cluster_3, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    esp_zb_endpoint_config_t endpoint_config_3 = {
        .endpoint = ZB1_ENDPOINT_3,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_ON_OFF_OUTPUT_DEVICE_ID,
        .app_device_version = 0};
    esp_zb_ep_list_add_ep(esp_zb_ep_list, esp_zb_cluster_list_3, endpoint_config_3);

    // ZB_ENDPOINT_4 - датчик движения
    // OnOff cluster отправка команд состояния датчика движения
    esp_zb_attribute_list_t *esp_zb_onoff_cluster_4 = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_ON_OFF);
    esp_zb_on_off_cluster_add_attr(esp_zb_onoff_cluster_4, 0, &motion_state);
    esp_zb_cluster_list_t *esp_zb_cluster_list_4 = esp_zb_zcl_cluster_list_create();
    esp_zb_cluster_list_add_on_off_cluster(esp_zb_cluster_list_4, esp_zb_onoff_cluster_4, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    esp_zb_endpoint_config_t endpoint_config_4 = {
        .endpoint = ZB1_ENDPOINT_4,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_ON_OFF_OUTPUT_DEVICE_ID,
        .app_device_version = 0};
    esp_zb_ep_list_add_ep(esp_zb_ep_list, esp_zb_cluster_list_4, endpoint_config_4);

    // TODO: восстановить level_control на EP5

    // Регистрация списка эндпойнтов
    esp_zb_device_register(esp_zb_ep_list);

    // Config the reporting info
    esp_zb_zcl_reporting_info_t reporting_info1 = {
        .direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI,
        .ep = ZB1_ENDPOINT_1,
        .cluster_id = ESP_ZB_ZCL_CLUSTER_ID_ON_OFF,
        .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        .dst.profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .u.send_info.min_interval = 1,
        .u.send_info.max_interval = 20,
        .u.send_info.def_min_interval = 1,
        .u.send_info.def_max_interval = 30,
        .u.send_info.delta.u16 = 1,
        .attr_id = 0,
        //        .manuf_code = 0,
    };
    esp_zb_zcl_update_reporting_info(&reporting_info1);

    esp_zb_zcl_reporting_info_t reporting_info2 = {
        .direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI,
        .ep = ZB1_ENDPOINT_2,
        .cluster_id = ESP_ZB_ZCL_CLUSTER_ID_ON_OFF,
        .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        .dst.profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .u.send_info.min_interval = 1,
        .u.send_info.max_interval = 20,
        .u.send_info.def_min_interval = 1,
        .u.send_info.def_max_interval = 30,
        .u.send_info.delta.u16 = 1,
        .attr_id = 0,
        //        .manuf_code = 0,
    };
    esp_zb_zcl_update_reporting_info(&reporting_info2);

    esp_zb_zcl_reporting_info_t reporting_info3 = {
        .direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI,
        .ep = ZB1_ENDPOINT_3,
        .cluster_id = ESP_ZB_ZCL_CLUSTER_ID_ON_OFF,
        .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        .dst.profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .u.send_info.min_interval = 1,
        .u.send_info.max_interval = 20,
        .u.send_info.def_min_interval = 1,
        .u.send_info.def_max_interval = 30,
        .u.send_info.delta.u16 = 1,
        .attr_id = 0,
        //        .manuf_code = 0,
    };
    esp_zb_zcl_update_reporting_info(&reporting_info3);

    esp_zb_zcl_reporting_info_t reporting_info4 = {
        .direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI,
        .ep = ZB1_ENDPOINT_4,
        .cluster_id = ESP_ZB_ZCL_CLUSTER_ID_ON_OFF,
        .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        .dst.profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .u.send_info.min_interval = 1,
        .u.send_info.max_interval = 20,
        .u.send_info.def_min_interval = 1,
        .u.send_info.def_max_interval = 30,
        .u.send_info.delta.u16 = 1,
        .attr_id = 0,
        //        .manuf_code = 0,
    };
    esp_zb_zcl_update_reporting_info(&reporting_info4);

    // регистрация обработчика действий (входные команды от координатора)
    esp_zb_core_action_handler_register(zb_action_handler);

    // маска на все каналы TODO: ограничить рабочими из проекта Zhub4
    esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);
    // Запуск устройства
    ESP_ERROR_CHECK(esp_zb_start(false));

    esp_zb_stack_main_loop();
}
#endif
