/* Esptouch example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "smart_config.h"

extern EventGroupHandle_t all_event;

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t s_wifi_event_group;
static bool wifi_flag ;
//static int s_retry_num = 0;
static const char *NVS_CUSTOMER = "customer data";
static const char *DATA1 = "param 1";

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const char *TAG = "smartconfig";

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    nvs_handle handle;
    esp_err_t ret;

    //static const char *NVS_CUSTOMER = "customer data";
    //static const char *DATA1 = "param 1";

    wifi_config_t wifi_config_stored;
    memset(&wifi_config_stored, 0x0, sizeof(wifi_config_stored));
    uint32_t len = sizeof(wifi_config_stored);
    ret = nvs_open(NVS_CUSTOMER, NVS_READWRITE, &handle);

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
        ret = nvs_get_blob(handle, DATA1, &wifi_config_stored, (size_t *)&len);
        if (ret == ESP_ERR_NVS_NOT_FOUND){
            wifi_flag = false;
            xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 3, NULL);
        }else if (ret == ESP_OK){
            wifi_flag = true;
            esp_wifi_connect();
            nvs_close(handle);
            }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if(wifi_flag == true)
        {
            /*if (s_retry_num < 5) {
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGI(TAG, "retry to connect to the AP");
            }*/
            ESP_ERROR_CHECK(nvs_open( NVS_CUSTOMER, NVS_READWRITE, &handle));
            ESP_ERROR_CHECK(nvs_erase_key(handle,DATA1));
            vTaskDelay(5000/portTICK_PERIOD_MS);
            ESP_ERROR_CHECK(nvs_erase_all(handle));
            vTaskDelay(5000/portTICK_PERIOD_MS);
            ESP_ERROR_CHECK(nvs_commit(handle));
            nvs_close(handle);
            esp_restart();
        }else{  
            esp_wifi_connect();
            xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        if(wifi_flag == true){
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            //s_retry_num = 0;
            xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
            xEventGroupSetBits(all_event, CONNECTED_BIT);
        }else{
            xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
            xEventGroupSetBits(all_event, CONNECTED_BIT);
            }
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan done");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "Found channel");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "Got SSID and password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        uint8_t ssid[33] = { 0 };
        uint8_t password[65] = { 0 };

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));

        ESP_LOGI(TAG, "SSID:%s", ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", password);

        ESP_ERROR_CHECK(esp_wifi_disconnect() );
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
        ESP_ERROR_CHECK(nvs_open( NVS_CUSTOMER, NVS_READWRITE, &handle) );
        ESP_ERROR_CHECK(nvs_set_blob( handle, DATA1, &wifi_config, sizeof(wifi_config)) );
        ESP_ERROR_CHECK(nvs_commit(handle) );
        nvs_close(handle);

        ESP_ERROR_CHECK(esp_wifi_connect());
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}

void initialise_wifi_task(void* arg)
{
    (void) arg;
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK( nvs_flash_init() );
    
    
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );

    if(wifi_flag == true){
        nvs_handle handle;
        wifi_config_t wifi_config_stored;

        memset(&wifi_config_stored, 0x0, sizeof(wifi_config_stored));
        
        uint32_t len = sizeof(wifi_config_stored);
        
        ESP_ERROR_CHECK( nvs_open(NVS_CUSTOMER, NVS_READWRITE, &handle) );
        ESP_ERROR_CHECK ( nvs_get_blob(handle, DATA1, &wifi_config_stored, (size_t *)&len) );

        nvs_close(handle);
        ESP_ERROR_CHECK(esp_wifi_set_config((wifi_interface_t) ESP_IF_WIFI_STA, &wifi_config_stored) );

        wifi_flag = true;
    }

    ESP_ERROR_CHECK( esp_wifi_start() );
    vTaskDelete(NULL);
}

void smartconfig_task(void* parm)
{
    (void) parm;
    EventBits_t uxBits;
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT()
    ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
    while (1) {
        uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        if(uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
            xEventGroupSetBits(all_event, CONNECTED_BIT);
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();
            xEventGroupSetBits(all_event, ESPTOUCH_DONE_BIT);

        }
    }
}
