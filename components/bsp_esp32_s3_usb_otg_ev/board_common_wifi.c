// Copyright 2020-2021 Espressif Systems (Shanghai) PTE LTD
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
// cange to config via spiff

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "bsp_esp32_s3_usb_otg_ev.h"
#include "esp_spiffs.h"
#include "cJSON.h"

/* --- Begin: Config Struct --- */
typedef struct {
    char mode[8];               // "sta", "ap", "off"
    char sta_ssid[32];
    char sta_pass[64];
    int sta_maximum_retry;
    char ap_ssid[32];
    char ap_pass[64];
    int ap_channel;
    int ap_max_sta_conn;
    char ap_ip[16];
} board_wifi_config_t;

static board_wifi_config_t g_wifi_cfg;
static int s_retry_num = 0;
static const char *TAG = "BOARD_WIFI";

/* --- End: Config Struct --- */

/* --- Begin: Load Config from SPIFFS --- */
static bool board_wifi_load_config_from_spiffs(board_wifi_config_t *cfg)
{
    FILE *f = fopen("/spiffs/wifi_config.json", "r");
    if (!f) {
        ESP_LOGW(TAG, "SPIFFS: wifi_config.json missing!");
        return false;
    }
    char buf[512];
    size_t len = fread(buf, 1, sizeof(buf)-1, f);
    fclose(f);
    buf[len] = 0;
    cJSON *json = cJSON_Parse(buf);
    if (!json) {
        ESP_LOGW(TAG, "Error parsing wifi_config.json");
        return false;
    }

    // Set defaults in case some fields are missing
    strcpy(cfg->mode, "ap");
    strcpy(cfg->ap_ssid, "smart-sdstick");
    strcpy(cfg->ap_pass, "");
    strcpy(cfg->ap_ip, "192.168.4.1");
    cfg->ap_channel = 6;
    cfg->ap_max_sta_conn = 4;
    strcpy(cfg->sta_ssid, "");
    strcpy(cfg->sta_pass, "");
    cfg->sta_maximum_retry = 7;

    cJSON *it;
    if ((it = cJSON_GetObjectItem(json, "mode"))) strncpy(cfg->mode, it->valuestring, sizeof(cfg->mode)-1);
    if ((it = cJSON_GetObjectItem(json, "sta_ssid"))) strncpy(cfg->sta_ssid, it->valuestring, sizeof(cfg->sta_ssid)-1);
    if ((it = cJSON_GetObjectItem(json, "sta_pass"))) strncpy(cfg->sta_pass, it->valuestring, sizeof(cfg->sta_pass)-1);
    if ((it = cJSON_GetObjectItem(json, "sta_maximum_retry"))) cfg->sta_maximum_retry = it->valueint;
    if ((it = cJSON_GetObjectItem(json, "ap_ssid"))) strncpy(cfg->ap_ssid, it->valuestring, sizeof(cfg->ap_ssid)-1);
    if ((it = cJSON_GetObjectItem(json, "ap_pass"))) strncpy(cfg->ap_pass, it->valuestring, sizeof(cfg->ap_pass)-1);
    if ((it = cJSON_GetObjectItem(json, "ap_channel"))) cfg->ap_channel = it->valueint;
    if ((it = cJSON_GetObjectItem(json, "ap_max_sta_conn"))) cfg->ap_max_sta_conn = it->valueint;
    if ((it = cJSON_GetObjectItem(json, "ap_ip"))) strncpy(cfg->ap_ip, it->valuestring, sizeof(cfg->ap_ip)-1);

    cJSON_Delete(json);
    return true;
}
/* --- End: Load Config from SPIFFS --- */


/* --- Begin: WiFi Event Handler --- */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base != WIFI_EVENT) return;

    switch (event_id) {
        case WIFI_EVENT_AP_STACONNECTED: {
            wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
            ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
            break;
        }
        case WIFI_EVENT_AP_STADISCONNECTED: {
            wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
            ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
            break;
        }
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            if (s_retry_num < g_wifi_cfg.sta_maximum_retry) {
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGI(TAG, "retry to connect to the AP");
            } else {
                ESP_LOGI(TAG, "giveup retry");
            }
            ESP_LOGI(TAG, "connect to the AP fail");
            break;
        case IP_EVENT_STA_GOT_IP: {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
            ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            s_retry_num = 0;
            // Hier kannst du z.B. die IP ins Display bringen!
        }
        break;
        default:
            break;
    }
}
/* --- End: WiFi Event Handler --- */


/* --- Begin: WiFi Init Functions --- */
static void wifi_init_softap(esp_netif_t *wifi_netif)
{
    int a, b, c, d;
    sscanf(g_wifi_cfg.ap_ip, "%d.%d.%d.%d", &a, &b, &c, &d);
    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, a, b, c, d);
    IP4_ADDR(&ip_info.gw, a, b, c, d);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(wifi_netif));
    ESP_ERROR_CHECK(esp_netif_set_ip_info(wifi_netif, &ip_info));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(wifi_netif));

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));
    uint8_t derived_mac_addr[6] = {0};
    esp_read_mac(derived_mac_addr, ESP_MAC_WIFI_SOFTAP);
    snprintf((char *)wifi_config.ap.ssid, sizeof(wifi_config.ap.ssid), "%s_%02X%02X%02X", g_wifi_cfg.ap_ssid, derived_mac_addr[3], derived_mac_addr[4], derived_mac_addr[5]);
    wifi_config.ap.ssid_len = strlen((char *)wifi_config.ap.ssid);
    snprintf((char *)wifi_config.ap.password, sizeof(wifi_config.ap.password), "%s", g_wifi_cfg.ap_pass);
    wifi_config.ap.max_connection = g_wifi_cfg.ap_max_sta_conn;
    wifi_config.ap.authmode = strlen(g_wifi_cfg.ap_pass) ? WIFI_AUTH_WPA_WPA2_PSK : WIFI_AUTH_OPEN;
    wifi_config.ap.channel = g_wifi_cfg.ap_channel;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d ip:%s",
             wifi_config.ap.ssid, wifi_config.ap.password, wifi_config.ap.channel, g_wifi_cfg.ap_ip);
}

static void wifi_init_sta(esp_netif_t *wifi_netif)
{
    (void) wifi_netif;
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));
    snprintf((char *)wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), "%s", g_wifi_cfg.sta_ssid);
    snprintf((char *)wifi_config.sta.password, sizeof(wifi_config.sta.password), "%s", g_wifi_cfg.sta_pass);

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s", g_wifi_cfg.sta_ssid, g_wifi_cfg.sta_pass);
}
/* --- End: WiFi Init Functions --- */


/* --- Main Entry API (replaces old iot_board_wifi_init) --- */

esp_err_t iot_board_wifi_init(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_netif_t *wifi_netif = NULL;
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // SPIFFS must be mounted before!
    if (!board_wifi_load_config_from_spiffs(&g_wifi_cfg)) {
        ESP_LOGW(TAG, "No wifi_config.json in SPIFFS. WLAN will NOT start.");
        return ESP_ERR_INVALID_STATE;
    }

    wifi_mode_t mode = WIFI_MODE_NULL;
    if (!strcmp(g_wifi_cfg.mode, "sta"))      mode = WIFI_MODE_STA;
    else if (!strcmp(g_wifi_cfg.mode, "ap"))  mode = WIFI_MODE_AP;
    else if (!strcmp(g_wifi_cfg.mode, "apsta")) mode = WIFI_MODE_APSTA;
    else {  // includes "off", unknown etc.
        ESP_LOGW(TAG, "WLAN Mode OFF or invalid in config. WLAN stays OFF.");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID,
                    &wifi_event_handler,
                    NULL,
                    NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));

    if (mode & WIFI_MODE_AP) {
        wifi_netif = esp_netif_create_default_wifi_ap();
        wifi_init_softap(wifi_netif);
    }
    if (mode & WIFI_MODE_STA) {
        wifi_netif = esp_netif_create_default_wifi_sta();
        wifi_init_sta(wifi_netif);
    }

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    return ESP_OK;
}
