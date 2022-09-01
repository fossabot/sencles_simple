// Copyright 2022 JeongYeham
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*
    SDK配置MQTT参数并建立连接, 之后创建2个线程
 *
 * + 一个线程用于保活长连接
 * + 一个线程用于接收消息, 并在有消息到达时进入默认的数据回调, 在连接状态变化时进入事件回调
 *
 * 注意: 本文件中的MQTT连接, 已经用 aiot_mqtt_setopt() 接口配置成了使用X509双向认证的方式进行底层TLS握手
 *
 *
 */

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_dm_api.h"
#include "core_string.h"
#include "cJSON.h"

#define TAG "Aliyun_service"
#define TAG_LOG "Aliyun_LOG"

extern QueueHandle_t xQueueHumi;
extern QueueHandle_t xQueueTemp;
extern EventGroupHandle_t all_event;

extern const uint8_t client_cert_pem_start[] asm("_binary_client_crt_start");
extern const uint8_t client_cert_pem_end[] asm("_binary_client_crt_end");
extern const uint8_t client_key_pem_start[] asm("_binary_client_key_start");
extern const uint8_t client_key_pem_end[] asm("_binary_client_key_end");

/* 位于portfiles/aiot_port文件夹下的系统适配函数集合 */
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

/* 位于external/ali_ca_cert.c中的服务器证书 */
extern const char *ali_ca_cert;

static TaskHandle_t process_Handle = NULL;
static TaskHandle_t recv_Handle = NULL;
static uint8_t g_mqtt_process_task_running = 0;
static uint8_t g_mqtt_recv_task_running = 0;
static char *g_product_key = NULL;
static char *g_device_name = NULL;

int32_t state_logcb(int32_t code, char *message)
{
    ESP_LOGI(TAG_LOG, "%s", message);
    // printf("%s", message);
    return 0;
}

static inline char *cJSON_phase(float temp, float humi, float temp_body)
{
    cJSON *cjson_data = NULL;
    cJSON *cjson_humi = NULL;
    cJSON *cjson_temp = NULL;
    cJSON *cjson_temp_body = NULL;
    char *str = NULL;

    cjson_data = cJSON_CreateObject();
    cjson_temp = cJSON_CreateObject();
    cjson_humi = cJSON_CreateObject();
    cjson_temp_body = cJSON_CreateObject();

    /* 添加一个嵌套的JSON数据（添加一个链表节点） */

    cJSON_AddNumberToObject(cjson_temp, "value", temp);
    cJSON_AddItemToObject(cjson_data, "CurrentTemperature", cjson_temp);

    cJSON_AddNumberToObject(cjson_humi, "value", humi);
    cJSON_AddItemToObject(cjson_data, "CurrentHumidity", cjson_humi);

    cJSON_AddNumberToObject(cjson_temp_body, "value", temp_body);
    cJSON_AddItemToObject(cjson_data, "CurrentBodytemp", cjson_temp_body);

    /* 打印JSON对象(整条链表)的所有数据 */
    str = cJSON_Print(cjson_data);
    return str;
}

/* 获取云端下发的设备信息, 包括 productKey 和 deviceName */

/* TODO: 用户需要在这个回调函数中把云端下推的 productKey 和 deviceName 做持久化的保存, 后面使用SDK的其它接口都会用到 */

static void get_save_device_info(const char *topic, uint16_t topic_len, const char *payload, uint32_t payload_len)
{
    const char *target_topic = "/ext/auth/identity/response";
    char *p_product_key = NULL;
    uint32_t product_key_len = 0;
    char *p_device_name = NULL;
    uint32_t device_name_len = 0;
    int32_t res = STATE_SUCCESS;

    if (topic_len != strlen(target_topic) || memcmp(topic, target_topic, topic_len) != 0)
    {
        return;
    }

    /* TODO: 此处为说明上的方便, 使用了SDK内部接口 core_json_value(), 这不是一个官方API, 未来有可能变化

             用户实际使用时, 需要换成用自己设备上可用的JSON解析函数库的接口处理payload, 比如流行的 cJSON 等
    */
    res = core_json_value(payload, payload_len, "productKey", strlen("productKey"), &p_product_key, &product_key_len);
    if (res < 0)
    {
        return;
    }
    res = core_json_value(payload, payload_len, "deviceName", strlen("deviceName"), &p_device_name, &device_name_len);
    if (res < 0)
    {
        return;
    }

    if (g_product_key == NULL)
    {
        g_product_key = malloc(product_key_len + 1);
        if (NULL == g_product_key)
        {
            return;
        }

        memset(g_product_key, 0, product_key_len + 1);
        memcpy(g_product_key, p_product_key, product_key_len);
    }
    if (g_device_name == NULL)
    {
        g_device_name = malloc(device_name_len + 1);
        if (NULL == g_device_name)
        {
            return;
        }

        memset(g_device_name, 0, device_name_len + 1);
        memcpy(g_device_name, p_device_name, device_name_len);
    }

    ESP_LOGI(TAG, "device productKey: %s", g_product_key);
    ESP_LOGI(TAG, "device deviceName: %s", g_device_name);
}

/* MQTT事件回调函数, 当网络连接/重连/断开时被触发, 事件定义见core/aiot_mqtt_api.h */
void mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{
    switch (event->type)
    {
    /* SDK因为用户调用了aiot_mqtt_connect()接口, 与mqtt服务器建立连接已成功 */
    case AIOT_MQTTEVT_CONNECT:
    {
        ESP_LOGI(TAG, "AIOT_MQTTEVT_CONNECT");
    }
    break;

    /* SDK因为网络状况被动断连后, 自动发起重连已成功 */
    case AIOT_MQTTEVT_RECONNECT:
    {
        ESP_LOGI(TAG, "AIOT_MQTTEVT_RECONNECT");
    }
    break;

    /* SDK因为网络的状况而被动断开了连接, network是底层读写失败, heartbeat是没有按预期得到服务端心跳应答 */
    case AIOT_MQTTEVT_DISCONNECT:
    {
        char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect") : ("heartbeat disconnect");
        ESP_LOGI(TAG, "AIOT_MQTTEVT_DISCONNECT: %s", cause);
    }
    break;

    default:
    {
    }
    }
}

/* MQTT默认消息处理回调, 当SDK从服务器收到MQTT消息时, 且无对应用户回调处理时被调用 */
void mqtt_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type)
    {
    case AIOT_MQTTRECV_HEARTBEAT_RESPONSE:
    {
        ESP_LOGI(TAG, "heartbeat response");
    }
    break;

    case AIOT_MQTTRECV_SUB_ACK:
    {
        ESP_LOGI(TAG, "suback, res: -0x%04lX, packet id: %d, max qos: %d",
                 -packet->data.sub_ack.res, packet->data.sub_ack.packet_id, packet->data.sub_ack.max_qos);
    }
    break;

    case AIOT_MQTTRECV_PUB:
    {
        ESP_LOGI(TAG, "pub, qos: %d, topic_len: %d, topic_name: %s", packet->data.pub.qos, packet->data.pub.topic_len, packet->data.pub.topic);
        ESP_LOGI(TAG, "pub, payload_len: %ld, payload_name: %s", packet->data.pub.payload_len, packet->data.pub.payload);
        /* TODO: 处理服务器下发的业务报文 */

        /* 处理云端下发的productKey和deviceName */
        get_save_device_info(packet->data.pub.topic, packet->data.pub.topic_len,
                             (char *)packet->data.pub.payload, packet->data.pub.payload_len);
    }
    break;

    case AIOT_MQTTRECV_PUB_ACK:
    {
        ESP_LOGI(TAG, "puback, packet id: %d", packet->data.pub_ack.packet_id);
    }
    break;

    default:
    {
    }
    }
}

/* 执行aiot_mqtt_process的线程, 包含心跳发送和QoS1消息重发 */
void mqtt_process_task(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_process_task_running)
    {
        res = aiot_mqtt_process(args);
        if (res == STATE_USER_INPUT_EXEC_DISABLED)
        {
            break;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/* 执行aiot_mqtt_recv的线程, 包含网络自动重连和从服务器收取MQTT消息 */
void mqtt_recv_task(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_recv_task_running)
    {
        res = aiot_mqtt_recv(args);
        if (res < STATE_SUCCESS)
        {
            if (res == STATE_USER_INPUT_EXEC_DISABLED)
            {
                break;
            }
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}

static void dm_recv_generic_reply(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    ESP_LOGI(TAG, "dm_recv_generic_reply msg_id = %ld, code = %ld, data_len = %ld, data = %s, message_len = %ld, message = %s",
             recv->data.generic_reply.msg_id,
             recv->data.generic_reply.code,
             recv->data.generic_reply.data_len,
             recv->data.generic_reply.data,
             recv->data.generic_reply.message_len,
             recv->data.generic_reply.message);
}

static void dm_recv_property_set(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    ESP_LOGI(TAG, "dm_recv_property_set msg_id = %ld, params_len = %ld, params = %s",
             (unsigned long)recv->data.property_set.msg_id,
             recv->data.property_set.params_len,
             recv->data.property_set.params);

    /* TODO: 以下代码演示如何对来自云平台的属性设置指令进行应答, 用户可取消注释查看演示效果 */
    /*
    {
        aiot_dm_msg_t msg;

        memset(&msg, 0, sizeof(aiot_dm_msg_t));
        msg.type = AIOT_DMMSG_PROPERTY_SET_REPLY;
        msg.data.property_set_reply.msg_id = recv->data.property_set.msg_id;
        msg.data.property_set_reply.code = 200;
        msg.data.property_set_reply.data = "{}";
        int32_t res = aiot_dm_send(dm_handle, &msg);
        if (res < 0) {
            printf("aiot_dm_send failed\r\n");
        }
    }
    */
}

static void dm_recv_async_service_invoke(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    ESP_LOGI(TAG, "dm_recv_async_service_invoke msg_id = %ld, service_id = %s, params_len = %ld, params = %s",
             (unsigned long)recv->data.async_service_invoke.msg_id,
             recv->data.async_service_invoke.service_id,
             recv->data.async_service_invoke.params_len,
             recv->data.async_service_invoke.params);

    /* TODO: 以下代码演示如何对来自云平台的异步服务调用进行应答, 用户可取消注释查看演示效果
     *
     * 注意: 如果用户在回调函数外进行应答, 需要自行保存msg_id, 因为回调函数入参在退出回调函数后将被SDK销毁, 不可以再访问到
     */

    /*
    {
        aiot_dm_msg_t msg;

        memset(&msg, 0, sizeof(aiot_dm_msg_t));
        msg.type = AIOT_DMMSG_ASYNC_SERVICE_REPLY;
        msg.data.async_service_reply.msg_id = recv->data.async_service_invoke.msg_id;
        msg.data.async_service_reply.code = 200;
        msg.data.async_service_reply.service_id = "ToggleLightSwitch";
        msg.data.async_service_reply.data = "{\"dataA\": 20}";
        int32_t res = aiot_dm_send(dm_handle, &msg);
        if (res < 0) {
            printf("aiot_dm_send failed\r\n");
        }
    }
    */
}

static void dm_recv_sync_service_invoke(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    ESP_LOGI(TAG_LOG, "dm_recv_sync_service_invoke msg_id = %ld, rrpc_id = %s, service_id = %s, params_len = %ld, params = %s",
             (unsigned long)recv->data.sync_service_invoke.msg_id,
             recv->data.sync_service_invoke.rrpc_id,
             recv->data.sync_service_invoke.service_id,
             recv->data.sync_service_invoke.params_len,
             recv->data.sync_service_invoke.params);

    /* TODO: 以下代码演示如何对来自云平台的同步服务调用进行应答, 用户可取消注释查看演示效果
     *
     * 注意: 如果用户在回调函数外进行应答, 需要自行保存msg_id和rrpc_id字符串, 因为回调函数入参在退出回调函数后将被SDK销毁, 不可以再访问到
     */

    /*
    {
        aiot_dm_msg_t msg;

        memset(&msg, 0, sizeof(aiot_dm_msg_t));
        msg.type = AIOT_DMMSG_SYNC_SERVICE_REPLY;
        msg.data.sync_service_reply.rrpc_id = recv->data.sync_service_invoke.rrpc_id;
        msg.data.sync_service_reply.msg_id = recv->data.sync_service_invoke.msg_id;
        msg.data.sync_service_reply.code = 200;
        msg.data.sync_service_reply.service_id = "SetLightSwitchTimer";
        msg.data.sync_service_reply.data = "{}";
        int32_t res = aiot_dm_send(dm_handle, &msg);
        if (res < 0) {
            printf("aiot_dm_send failed\r\n");
        }
    }
    */
}

static void dm_recv_raw_data(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    ESP_LOGI(TAG_LOG, "dm_recv_raw_data raw data len = %ld", recv->data.raw_data.data_len);
    /* TODO: 以下代码演示如何发送二进制格式数据, 若使用需要有相应的数据透传脚本部署在云端 */
    /*
    {
        aiot_dm_msg_t msg;
        uint8_t raw_data[] = {0x01, 0x02};

        memset(&msg, 0, sizeof(aiot_dm_msg_t));
        msg.type = AIOT_DMMSG_RAW_DATA;
        msg.data.raw_data.data = raw_data;
        msg.data.raw_data.data_len = sizeof(raw_data);
        aiot_dm_send(dm_handle, &msg);
    }
    */
}

static void dm_recv_raw_sync_service_invoke(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    ESP_LOGI(TAG_LOG, "dm_recv_raw_sync_service_invoke raw sync service rrpc_id = %s, data_len = %ld",
             recv->data.raw_service_invoke.rrpc_id,
             recv->data.raw_service_invoke.data_len);
}

static void dm_recv_raw_data_reply(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    ESP_LOGI(TAG_LOG, "demo_dm_recv_raw_data_reply receive reply for up_raw msg, data len = %ld", recv->data.raw_data.data_len);
    /* TODO: 用户处理下行的二进制数据, 位于recv->data.raw_data.data中 */
}

/* 用户数据接收处理回调函数 */
static void dm_recv_handler(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    ESP_LOGI(TAG_LOG, "dm_recv_handler, type = %d", recv->type);

    switch (recv->type)
    {

    /* 属性上报, 事件上报, 获取期望属性值或者删除期望属性值的应答 */
    case AIOT_DMRECV_GENERIC_REPLY:
    {
        dm_recv_generic_reply(dm_handle, recv, userdata);
    }
    break;

    /* 属性设置 */
    case AIOT_DMRECV_PROPERTY_SET:
    {
        dm_recv_property_set(dm_handle, recv, userdata);
    }
    break;

    /* 异步服务调用 */
    case AIOT_DMRECV_ASYNC_SERVICE_INVOKE:
    {
        dm_recv_async_service_invoke(dm_handle, recv, userdata);
    }
    break;

    /* 同步服务调用 */
    case AIOT_DMRECV_SYNC_SERVICE_INVOKE:
    {
        dm_recv_sync_service_invoke(dm_handle, recv, userdata);
    }
    break;

    /* 下行二进制数据 */
    case AIOT_DMRECV_RAW_DATA:
    {
        dm_recv_raw_data(dm_handle, recv, userdata);
    }
    break;

    /* 二进制格式的同步服务调用, 比单纯的二进制数据消息多了个rrpc_id */
    case AIOT_DMRECV_RAW_SYNC_SERVICE_INVOKE:
    {
        dm_recv_raw_sync_service_invoke(dm_handle, recv, userdata);
    }
    break;

    /* 上行二进制数据后, 云端的回复报文 */
    case AIOT_DMRECV_RAW_DATA_REPLY:
    {
        dm_recv_raw_data_reply(dm_handle, recv, userdata);
    }
    break;

    default:
        break;
    }
}

/* 属性上报函数 */
int32_t send_property_post(void *dm_handle, char *params)
{
    aiot_dm_msg_t msg;

    memset(&msg, 0, sizeof(aiot_dm_msg_t));
    msg.type = AIOT_DMMSG_PROPERTY_POST;
    msg.data.property_post.params = params;

    return aiot_dm_send(dm_handle, &msg);
}

int32_t send_property_batch_post(void *dm_handle, char *params)
{
    aiot_dm_msg_t msg;

    memset(&msg, 0, sizeof(aiot_dm_msg_t));
    msg.type = AIOT_DMMSG_PROPERTY_BATCH_POST;
    msg.data.property_post.params = params;

    return aiot_dm_send(dm_handle, &msg);
}

/* 事件上报函数 */
int32_t send_event_post(void *dm_handle, char *event_id, char *params)
{
    aiot_dm_msg_t msg;

    memset(&msg, 0, sizeof(aiot_dm_msg_t));
    msg.type = AIOT_DMMSG_EVENT_POST;
    msg.data.event_post.event_id = event_id;
    msg.data.event_post.params = params;

    return aiot_dm_send(dm_handle, &msg);
}

void link_main(void *args)
{
    (void)args;
    int32_t res = STATE_SUCCESS;
    void *mqtt_handle = NULL;
    void *dm_handle = NULL;
    uint8_t post_reply = 0;
    float temp_rx = 0;
    float humi_rx = 0;
    float temp_body = 0;

    /* TODO: 使用X509双向认证时, MQTT连接的服务器域名与通常情况不同 */
    char *host = "x509.itls.cn-shanghai.aliyuncs.com";

    uint16_t port = 1883;            /* X.509服务器目的端口是1883 */
    aiot_sysdep_network_cred_t cred; /* 安全凭据结构体, 如果要用TLS, 这个结构体中配置CA证书等参数 */

    /* TODO: 使用X.509认证模式建连的设备, 三元组信息都需用空字符串进行配置 */
    char *product_key = "";
    char *device_name = "";
    char *device_secret = "";

    /* 配置SDK的底层依赖 */
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    /* 配置SDK的日志输出 */
    aiot_state_set_logcb(state_logcb);

    /* 创建SDK的安全凭据, 用于建立TLS连接 */
    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA; /* 使用RSA证书校验MQTT服务端 */
    cred.max_tls_fragment = 16384;                     /* 最大的分片长度为16K, 其它可选值还有4K, 2K, 1K, 0.5K */
    cred.sni_enabled = 1;                              /* TLS建连时, 支持Server Name Indicator */
    cred.x509_server_cert = ali_ca_cert;               /* 用来验证MQTT服务端的RSA根证书 */
    cred.x509_server_cert_len = strlen(ali_ca_cert);   /* 用来验证MQTT服务端的RSA根证书长度 */

    /* TODO: 留意以下4行, 使用X509双向认证时, 用户对安全凭据的设置就只要增加这一部分 */
    cred.x509_client_cert = (const char *)client_cert_pem_start;
    cred.x509_client_cert_len = strlen((const char *)client_cert_pem_start);
    cred.x509_client_privkey = (const char *)client_key_pem_start;
    cred.x509_client_privkey_len = strlen((const char *)client_key_pem_start);

    /* 创建1个MQTT客户端实例并内部初始化默认参数 */
    mqtt_handle = aiot_mqtt_init();
    if (mqtt_handle == NULL)
    {
        ESP_LOGE(TAG, "aiot_mqtt_init failed");
        goto error_loop;
    }

    /* 配置MQTT服务器地址 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_HOST, (void *)host);
    /* 配置MQTT服务器端口 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&port);
    /* 配置设备productKey */
    /* TODO: 与不使用X509时不同的是, productKey 是从云平台推下来, 此处先用""配置给SDK */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)product_key);
    /* 配置设备deviceName */
    /* TODO: 与不使用X509时不同的是, deviceName 是从云平台推下来, 此处先用""配置给SDK */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)device_name);
    /* 配置设备deviceSecret */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)device_secret);
    /* 配置网络连接的安全凭据, 上面已经创建好了 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_NETWORK_CRED, (void *)&cred);
    /* 配置MQTT默认消息接收回调函数 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)mqtt_recv_handler);
    /* 配置MQTT事件回调函数 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)mqtt_event_handler);

    /* 创建DATA-MODEL实例 */
    dm_handle = aiot_dm_init();
    if (dm_handle == NULL)
    {
        ESP_LOGE(TAG, "aiot_dm_init failed");
        goto error_loop;
    }
    /* 配置MQTT实例句柄 */
    aiot_dm_setopt(dm_handle, AIOT_DMOPT_MQTT_HANDLE, mqtt_handle);
    /* 配置消息接收处理回调函数 */
    aiot_dm_setopt(dm_handle, AIOT_DMOPT_RECV_HANDLER, (void *)dm_recv_handler);

    /* 配置是云端否需要回复post_reply给设备. 如果为1, 表示需要云端回复, 否则表示不回复 */
    aiot_dm_setopt(dm_handle, AIOT_DMOPT_POST_REPLY, (void *)&post_reply);

    /* 与服务器建立MQTT连接 */
    res = aiot_mqtt_connect(mqtt_handle);
    if (res < STATE_SUCCESS)
    {
        /* 尝试建立连接失败, 销毁MQTT实例, 回收资源 */
        aiot_mqtt_deinit(&mqtt_handle);
        ESP_LOGE(TAG, "aiot_mqtt_connect failed: -0x%04lX", -res);
        goto error_loop;
    }

    /* 创建一个单独的线程, 专用于执行aiot_mqtt_process, 它会自动发送心跳保活, 以及重发QoS1的未应答报文 */
    g_mqtt_process_task_running = 1;
    xTaskCreatePinnedToCore(mqtt_process_task, "mqtt_process", 2048, mqtt_handle, 0, &process_Handle, 1);
    while (process_Handle == NULL)
    {
        ESP_LOGE(TAG, "create mqtt_process_task failed: %ld", res);
    }

    /* 创建一个单独的线程用于执行aiot_mqtt_recv, 它会循环收取服务器下发的MQTT消息, 并在断线时自动重连 */
    g_mqtt_recv_task_running = 1;
    xTaskCreatePinnedToCore(mqtt_recv_task, "recv_process", 4096, mqtt_handle, 0, &recv_Handle, 1);
    if (recv_Handle == NULL)
    {
        ESP_LOGE(TAG, "create mqtt_recv_task failed: %ld", res);
        aiot_dm_deinit(&dm_handle);
        aiot_mqtt_disconnect(mqtt_handle);
        aiot_mqtt_deinit(&mqtt_handle);
        goto error_loop;
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)g_product_key);
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)g_device_name);
    xEventGroupSetBits(all_event, BIT3);

    while (1)
    {

        send_property_post(dm_handle, cJSON_phase(temp_rx, humi_rx, temp_body));

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    /* 断开MQTT连接, 一般不会运行到这里 */
    res = aiot_mqtt_disconnect(mqtt_handle);
    if (res < STATE_SUCCESS)
    {
        aiot_mqtt_deinit(&mqtt_handle);
        ESP_LOGE(TAG, "aiot_mqtt_disconnect failed: -0x%04lX", -res);
    }

    /* 销毁MQTT实例, 一般不会运行到这里 */
    res = aiot_mqtt_deinit(&mqtt_handle);
    if (res < STATE_SUCCESS)
    {
        ESP_LOGE(TAG, "aiot_mqtt_deinit failed: -0x%04lX", -res);
    }

    g_mqtt_process_task_running = 0;
    g_mqtt_recv_task_running = 0;
    vTaskDelete(process_Handle);
    vTaskDelete(recv_Handle);

error_loop:
    ESP_LOGE(TAG, "Aliyun_ERROR");
    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
