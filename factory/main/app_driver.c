/* Beispiel: app_driver.c
 * Definiert _app_driver[] und _app_driver_count an einer Stelle (eine TU).
 * Passe die user_data‑Objekte an, oder entferne sie, falls du sie bereits anderswo definiert hast.
 */

#include "app.h"         /* enthält typedef user_app_t und die extern Deklarationen */
#include "info/info_types.h" /* falls du info user_data benutzt (optional) */

/* Beispiel statische user_data (nur wenn du sie noch nicht anderswo definiert hast) */
const info_app_data_t s_info_chip_data = {
    .page_id = INFO_PAGE_CHIP,
    .icon_name = "esp_logo_small.jpg",
    .default_font = &Font12,
    .line_height = 14,
    .lines = NULL,
    .lines_count = 0,
};
const info_app_data_t s_info_sd_data = {
    .page_id = INFO_PAGE_SD,
    .icon_name = "icon_flash_small.jpg",
    .default_font = &Font12,
    .line_height = 14,
    .lines = NULL,
    .lines_count = 0,
};
const info_app_data_t s_info_wlan_data = {
    .page_id = INFO_PAGE_WLAN,
    .icon_name = "icon_router_small.jpg",
    .default_font = &Font12,
    .line_height = 14,
    .lines = NULL,
    .lines_count = 0,
};

/* Das eigentliche App‑Array (einzige Definition im Projekt!) */
const user_app_t _app_driver[] = {
    {
        .app_name = "app menu",
        .icon_name = "icon_flash.jpg",
        .init = app_menu_init,
        .deinit = app_menu_deinit,
        .p_queue_hdl = &g_app_menu_queue_hdl,
        .user_data = NULL,
    },
    {
        .app_name = "Info SD",
        .icon_name = "icon_flash_small.jpg",
        .init = info_app_init,             /* deine einheitliche Info‑App */
        .deinit = info_app_deinit,
        .p_queue_hdl = &g_info_queue_hdl,
        .user_data = &s_info_sd_data,
        .flags.restart_after_deinit = true,
    },
    {
        .app_name = "Info Esp",
        .icon_name = "esp_logo_small.jpg",
        .init = info_app_init,
        .deinit = info_app_deinit,
        .p_queue_hdl = &g_info_queue_hdl,
        .user_data = &s_info_chip_data,
        .flags.restart_after_deinit = true,
    },
    {
        .app_name = "Info Wlan",
        .icon_name = "icon_router_small.jpg",
        .init = info_app_init,
        .deinit = info_app_deinit,
        .p_queue_hdl = &g_info_queue_hdl,
        .user_data = &s_info_wlan_data,
        .flags.restart_after_deinit = true,
    },
    /* weitere Einträge ... */
};

/* Einzige Stelle, an der die Anzahl berechnet wird */
const int _app_driver_count = sizeof(_app_driver) / sizeof(_app_driver[0]);