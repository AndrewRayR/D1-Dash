#include "lvgl.h"
#include "ui.h"
#include "tech_settings.h"
#include "tech_http.h"
#include "lv_port.h"
#include "cJSON.h"
#include "view_data.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static lv_obj_t *home;
static lv_obj_t *active_page;
static lv_obj_t *settings_fields[5];
static lv_obj_t *settings_form;
static lv_obj_t *settings_keyboard;
static lv_obj_t *settings_status;
static lv_obj_t *brightness_slider;
static lv_obj_t *brightness_value;
static lv_obj_t *timeout_slider;
static lv_obj_t *timeout_value;
static lv_obj_t *network_page;
static lv_obj_t *network_ssid;
static lv_obj_t *network_signal;
static lv_obj_t *network_ip;
static lv_obj_t *network_gateway;
static lv_obj_t *pi_page;
static lv_obj_t *pi_status;
static lv_obj_t *pi_metrics;
static bool pi_request_active;
#define TECH_EVENT_PI_RESULT 0x4001
#define TECH_EVENT_WEB_RESULT 0x4002
#define TECH_EVENT_GITHUB_RESULT 0x4003
#define TECH_EVENT_F1_RESULT 0x4004
#define WEB_HISTORY_MAX 8
static lv_obj_t *web_page;
static lv_obj_t *web_url_field;
static lv_obj_t *web_content;
static lv_obj_t *web_status;
static lv_obj_t *web_keyboard;
static bool web_request_active;
static char web_history[WEB_HISTORY_MAX][160];
static uint8_t web_history_count;
static uint8_t web_history_index;
static lv_obj_t *github_page;
static lv_obj_t *github_status;
static lv_obj_t *github_details;
static bool github_request_active;
static char github_web_url[160];
static lv_obj_t *f1_page;
static lv_obj_t *f1_status;
static lv_obj_t *f1_details;
static bool f1_request_active;
static bool f1_preparing;
/* Fixed-size, RAM-only fallback: at most 900 bytes and valid for 30 minutes. */
static char f1_cache_details[900];
static time_t f1_cache_time;
typedef struct {
    bool ok;
    char hostname[48];
    char metrics[192];
    char message[96];
} pi_result_t;
typedef struct {
    bool ok;
    bool add_history;
    char url[160];
    char message[96];
    char text[2200];
} web_result_t;
typedef struct {
    bool ok;
    char title[80];
    char details[380];
    char html_url[160];
    char message[96];
} github_result_t;
typedef struct {
    bool ok;
    bool clock_valid;
    char title[80];
    char details[900];
    char message[96];
} f1_result_t;
static void show_network(lv_event_t *event);
static void open_wifi_manager(lv_event_t *event);
static void open_pi_page(void);
static void network_refresh_event(lv_event_t *event);
static void pi_refresh_event(lv_event_t *event);
static void pi_result_event(void *handler_args, esp_event_base_t base,
                            int32_t id, void *event_data);
static void open_web_page(void);
static void web_go_event(lv_event_t *event);
static void web_back_event(lv_event_t *event);
static void web_forward_event(lv_event_t *event);
static void web_field_event(lv_event_t *event);
static void web_keyboard_event(lv_event_t *event);
static void web_result_event(void *handler_args, esp_event_base_t base,
                             int32_t id, void *event_data);
static void open_github_page(void);
static void github_refresh_event(lv_event_t *event);
static void github_open_web_event(lv_event_t *event);
static void github_result_event(void *handler_args, esp_event_base_t base,
                                int32_t id, void *event_data);
static void open_f1_page(void);
static void f1_refresh_event(lv_event_t *event);
static void f1_fetch_task(void *arg);
static void f1_result_event(void *handler_args, esp_event_base_t base,
                            int32_t id, void *event_data);
static void settings_brightness_event(lv_event_t *event);
static void settings_timeout_event(lv_event_t *event);
ESP_EVENT_DECLARE_BASE(VIEW_EVENT_BASE);
extern esp_event_loop_handle_t view_event_handle;
static const char *module_names[] = {
    "WEB READER", "NETWORK MONITOR", "PI MONITOR", "GITHUB API", "F1 API"
};
static const char *module_info[] = {
    "Articles and saved pages", "Wi-Fi, signal and connectivity",
    "Raspberry Pi health and services", "Repositories and activity",
    "Race schedule and live timing"
};
static const uint32_t module_accent[] = {
    0x42D6C3, 0x55A8FF, 0xB28BFF, 0xF2A65A, 0xFF6685
};

static void show_home(lv_event_t *event);
static void open_module(lv_event_t *event);
static void open_settings(lv_event_t *event);
static void configure_wifi_screen(void);
static void settings_field_event(lv_event_t *event);
static void settings_keyboard_event(lv_event_t *event);
static void settings_hide_keyboard(void);
static void settings_save_event(lv_event_t *event);
static lv_obj_t *make_label(lv_obj_t *parent, const char *text, int size, uint32_t color);

void tech_terminal_ui_init(void)
{
    ESP_ERROR_CHECK(esp_event_handler_register_with(view_event_handle,
        VIEW_EVENT_BASE, TECH_EVENT_PI_RESULT, pi_result_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register_with(view_event_handle,
        VIEW_EVENT_BASE, TECH_EVENT_WEB_RESULT, web_result_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register_with(view_event_handle,
        VIEW_EVENT_BASE, TECH_EVENT_GITHUB_RESULT, github_result_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register_with(view_event_handle,
        VIEW_EVENT_BASE, TECH_EVENT_F1_RESULT, f1_result_event, NULL));
    home = lv_obj_create(NULL);
    lv_obj_clear_flag(home, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(home, lv_color_hex(0x0B1118), 0);
    lv_obj_set_style_bg_opa(home, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(home, 0, 0);
    lv_obj_set_style_pad_all(home, 20, 0);

    lv_obj_t *top = lv_obj_create(home);
    lv_obj_set_size(top, 440, 42);
    lv_obj_align(top, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(top, lv_color_hex(0x111C27), 0);
    lv_obj_set_style_border_width(top, 0, 0);
    lv_obj_set_style_radius(top, 12, 0);
    lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);
    make_label(top, "D1  /  TECH TERMINAL", 17, 0xEAF2F8);
    lv_obj_t *status = make_label(top, "ESP32-S3  •  ONLINE", 12, 0x42D6C3);
    lv_obj_align(status, LV_ALIGN_RIGHT_MID, -10, 0);

    lv_obj_t *eyebrow = make_label(home, "YOUR SYSTEMS", 12, 0x8295A7);
    lv_obj_align(eyebrow, LV_ALIGN_TOP_LEFT, 0, 48);
    lv_obj_t *title = make_label(home, "Command Center", 30, 0xF2F6FA);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 66);
    lv_obj_t *subtitle = make_label(home, "Choose a module to get started", 14, 0x91A3B4);
    lv_obj_align(subtitle, LV_ALIGN_TOP_LEFT, 0, 106);

    lv_obj_t *section = lv_obj_create(home);
    lv_obj_set_size(section, 440, 268);
    lv_obj_align(section, LV_ALIGN_TOP_MID, 0, 130);
    lv_obj_set_style_bg_opa(section, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(section, 0, 0);
    lv_obj_set_style_pad_all(section, 0, 0);
    lv_obj_set_style_pad_row(section, 8, 0);
    lv_obj_set_style_pad_column(section, 8, 0);
    lv_obj_set_layout(section, LV_LAYOUT_GRID);
    static lv_coord_t cols[] = {210, 210, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t rows[] = {82, 82, 82, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(section, cols, rows);

    for (int i = 0; i < 5; i++) {
        lv_obj_t *card = lv_btn_create(section);
        lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, i % 2, 1,
                             LV_GRID_ALIGN_STRETCH, i / 2, 1);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x14212D), 0);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x1B2D3B), LV_STATE_PRESSED);
        lv_obj_set_style_border_color(card, lv_color_hex(0x2A3B49), 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_radius(card, 12, 0);
        lv_obj_set_style_shadow_width(card, 0, 0);
        lv_obj_set_style_pad_all(card, 10, 0);
        lv_obj_add_event_cb(card, open_module, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_t *accent = lv_obj_create(card);
        lv_obj_set_size(accent, 4, 44);
        lv_obj_align(accent, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_style_bg_color(accent, lv_color_hex(module_accent[i]), 0);
        lv_obj_set_style_border_width(accent, 0, 0);
        lv_obj_set_style_radius(accent, 2, 0);
        lv_obj_clear_flag(accent, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t *name = make_label(card, module_names[i], 13, 0xEDF4F8);
        lv_obj_align(name, LV_ALIGN_TOP_LEFT, 12, 8);
        lv_obj_t *desc = make_label(card, module_info[i], 10, 0x91A3B4);
        lv_obj_set_width(desc, 180);
        lv_label_set_long_mode(desc, LV_LABEL_LONG_DOT);
        lv_obj_align(desc, LV_ALIGN_BOTTOM_LEFT, 12, -7);
    }

    lv_obj_t *footer = lv_obj_create(home);
    lv_obj_set_size(footer, 440, 38);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);
    make_label(footer, "MODULES", 12, 0x42D6C3);
    lv_obj_t *settings = lv_btn_create(footer);
    lv_obj_set_size(settings, 92, 34);
    lv_obj_align(settings, LV_ALIGN_RIGHT_MID, -124, 0);
    lv_obj_set_style_bg_color(settings, lv_color_hex(0x1A2632), 0);
    lv_obj_set_style_border_width(settings, 0, 0);
    lv_obj_set_style_radius(settings, 9, 0);
    lv_obj_add_event_cb(settings, open_settings, LV_EVENT_CLICKED, NULL);
    lv_obj_t *settings_text = make_label(settings, "SETTINGS", 11, 0xD0DCE5);
    lv_obj_center(settings_text);

    lv_obj_t *network = lv_btn_create(footer);
    lv_obj_set_size(network, 112, 34);
    lv_obj_align(network, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(network, lv_color_hex(0x17304A), 0);
    lv_obj_set_style_border_width(network, 0, 0);
    lv_obj_set_style_radius(network, 9, 0);
    lv_obj_add_event_cb(network, open_module, LV_EVENT_CLICKED, (void *)(intptr_t)1);
    lv_obj_t *network_text = make_label(network, "NETWORK", 11, 0x55A8FF);
    lv_obj_center(network_text);
    configure_wifi_screen();
    /* Build this screen while startup heap is available; screen creation can fail later under pressure. */
    f1_preparing = true;
    open_f1_page();
    f1_preparing = false;
    lv_scr_load(home);
}

static void configure_wifi_screen(void)
{
    lv_obj_set_style_bg_color(ui_screen_wifi, lv_color_hex(0x0B1118), LV_PART_MAIN);
    lv_label_set_text(ui_wifi_title, "Network Monitor");
    lv_obj_remove_event_cb(ui_back3, ui_event_back3);
    lv_obj_set_style_bg_img_src(ui_back3, NULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_back3, lv_color_hex(0x17304A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_back3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_back3, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *home_label = lv_label_create(ui_back3);
    lv_label_set_text(home_label, "<  NETWORK");
    lv_obj_center(home_label);
    lv_obj_set_style_text_color(home_label, lv_color_hex(0x55A8FF), 0);
    lv_obj_set_style_text_font(home_label, &lv_font_montserrat_14, 0);
    lv_obj_add_event_cb(ui_back3, show_network, LV_EVENT_CLICKED, NULL);
}

static void network_refresh(void)
{
    wifi_ap_record_t ap = {0};
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
        char ssid[sizeof(ap.ssid) + 1] = {0};
        memcpy(ssid, ap.ssid, sizeof(ap.ssid));
        lv_label_set_text_fmt(network_ssid, "Wi-Fi  /  %s", ssid);
        lv_label_set_text_fmt(network_signal, "Signal  /  %d dBm", ap.rssi);
    } else {
        lv_label_set_text(network_ssid, "Wi-Fi  /  Not connected");
        lv_label_set_text(network_signal, "Signal  /  --");
    }

    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t info = {0};
    if (sta && esp_netif_get_ip_info(sta, &info) == ESP_OK && info.ip.addr != 0) {
        char ip[16], gateway[16];
        snprintf(ip, sizeof(ip), IPSTR, IP2STR(&info.ip));
        snprintf(gateway, sizeof(gateway), IPSTR, IP2STR(&info.gw));
        lv_label_set_text_fmt(network_ip, "IP address  /  %s", ip);
        lv_label_set_text_fmt(network_gateway, "Gateway  /  %s", gateway);
    } else {
        lv_label_set_text(network_ip, "IP address  /  --");
        lv_label_set_text(network_gateway, "Gateway  /  --");
    }
}

static void open_network_page(void)
{
    if (network_page == NULL) {
        network_page = lv_obj_create(NULL);
        active_page = network_page;
        lv_obj_set_style_bg_color(network_page, lv_color_hex(0x0B1118), 0);
        lv_obj_set_style_border_width(network_page, 0, 0);

        lv_obj_t *top = lv_obj_create(network_page);
        lv_obj_set_size(top, 440, 42);
        lv_obj_align(top, LV_ALIGN_TOP_MID, 0, 14);
        lv_obj_set_style_bg_color(top, lv_color_hex(0x111C27), 0);
        lv_obj_set_style_border_width(top, 0, 0);
        lv_obj_set_style_radius(top, 12, 0);
        lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);
        make_label(top, "D1  /  TECH TERMINAL", 17, 0xEAF2F8);

        lv_obj_t *back = lv_btn_create(network_page);
        lv_obj_set_size(back, 96, 38);
        lv_obj_align(back, LV_ALIGN_TOP_LEFT, 20, 72);
        lv_obj_set_style_bg_color(back, lv_color_hex(0x17304A), 0);
        lv_obj_set_style_border_width(back, 0, 0);
        lv_obj_add_event_cb(back, show_home, LV_EVENT_CLICKED, NULL);
        lv_obj_t *back_text = make_label(back, "< HOME", 12, 0x55A8FF);
        lv_obj_center(back_text);

        lv_obj_t *title = make_label(network_page, "NETWORK MONITOR", 22, 0xF2F6FA);
        lv_obj_align(title, LV_ALIGN_TOP_LEFT, 126, 78);
        lv_obj_t *panel = lv_obj_create(network_page);
        lv_obj_set_size(panel, 436, 222);
        lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, 132);
        lv_obj_set_style_bg_color(panel, lv_color_hex(0x14212D), 0);
        lv_obj_set_style_border_color(panel, lv_color_hex(0x2A3B49), 0);
        lv_obj_set_style_radius(panel, 14, 0);
        lv_obj_set_style_pad_all(panel, 18, 0);
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
        network_ssid = make_label(panel, "Wi-Fi  /  --", 16, 0xEDF4F8);
        lv_obj_set_pos(network_ssid, 4, 8);
        network_signal = make_label(panel, "Signal  /  --", 14, 0x91A3B4);
        lv_obj_set_pos(network_signal, 4, 50);
        network_ip = make_label(panel, "IP address  /  --", 14, 0x91A3B4);
        lv_obj_set_pos(network_ip, 4, 90);
        network_gateway = make_label(panel, "Gateway  /  --", 14, 0x91A3B4);
        lv_obj_set_pos(network_gateway, 4, 130);

        lv_obj_t *manage = lv_btn_create(network_page);
        lv_obj_set_size(manage, 210, 48);
        lv_obj_align(manage, LV_ALIGN_BOTTOM_LEFT, 22, -28);
        lv_obj_set_style_bg_color(manage, lv_color_hex(0x17304A), 0);
        lv_obj_set_style_border_width(manage, 0, 0);
        lv_obj_set_style_radius(manage, 10, 0);
        lv_obj_add_event_cb(manage, open_wifi_manager, LV_EVENT_CLICKED, NULL);
        lv_obj_t *manage_text = make_label(manage, "MANAGE WI-FI", 13, 0x55A8FF);
        lv_obj_center(manage_text);

        lv_obj_t *refresh = lv_btn_create(network_page);
        lv_obj_set_size(refresh, 210, 48);
        lv_obj_align(refresh, LV_ALIGN_BOTTOM_RIGHT, -22, -28);
        lv_obj_set_style_bg_color(refresh, lv_color_hex(0x174336), 0);
        lv_obj_set_style_border_width(refresh, 0, 0);
        lv_obj_set_style_radius(refresh, 10, 0);
        lv_obj_add_event_cb(refresh, network_refresh_event, LV_EVENT_CLICKED, NULL);
        lv_obj_t *refresh_text = make_label(refresh, "REFRESH", 13, 0x42D6C3);
        lv_obj_center(refresh_text);
    }
    active_page = network_page;
    network_refresh();
    lv_scr_load(network_page);
}

static void show_network(lv_event_t *event)
{
    (void)event;
    open_network_page();
}

static void open_wifi_manager(lv_event_t *event)
{
    (void)event;
    lv_disp_load_scr(ui_screen_wifi);
}

static void network_refresh_event(lv_event_t *event)
{
    (void)event;
    network_refresh();
}

static void pi_fetch_task(void *arg)
{
    char configured_url[128];
    pi_result_t result = {0};
    tech_settings_t settings;
    if (tech_settings_get(&settings) != ESP_OK || settings.pi_count == 0 ||
        settings.pi_devices[0].base_url[0] == '\0') {
        strlcpy(result.message, "Set a Pi status URL in Settings", sizeof(result.message));
        goto finished;
    }

    strlcpy(configured_url, settings.pi_devices[0].base_url, sizeof(configured_url));
    size_t url_len = strlen(configured_url);
    while (url_len > 0 && configured_url[url_len - 1] == '/') {
        configured_url[--url_len] = '\0';
    }
    char url[160];
    if (url_len >= 14 && strcmp(configured_url + url_len - 14, "/api/v1/status") == 0) {
        snprintf(url, sizeof(url), "%s", configured_url);
    } else {
        snprintf(url, sizeof(url), "%s/api/v1/status", configured_url);
    }

    tech_http_response_t *response = calloc(1, sizeof(*response));
    if (response == NULL) {
        strlcpy(result.message, "Not enough memory for Pi response", sizeof(result.message));
        goto finished;
    }
    esp_err_t err = tech_http_get(url, "application/json", response);
    if (err != ESP_OK) {
        free(response);
        strlcpy(result.message, "Request failed; check Wi-Fi and Pi URL", sizeof(result.message));
        goto finished;
    }
    if (response->status_code != 200) {
        snprintf(result.message, sizeof(result.message), "Pi returned HTTP %d", response->status_code);
        free(response);
        goto finished;
    }

    cJSON *root = cJSON_Parse(response->body);
    free(response);
    if (root == NULL) {
        strlcpy(result.message, "Pi response is not valid JSON", sizeof(result.message));
        goto finished;
    }
    cJSON *hostname = cJSON_GetObjectItemCaseSensitive(root, "hostname");
    cJSON *cpu = cJSON_GetObjectItemCaseSensitive(root, "cpu");
    cJSON *memory = cJSON_GetObjectItemCaseSensitive(root, "memory");
    cJSON *disk = cJSON_GetObjectItemCaseSensitive(root, "disk");
    cJSON *temperature = cJSON_GetObjectItemCaseSensitive(root, "temperature");
    cJSON *uptime = cJSON_GetObjectItemCaseSensitive(root, "uptime");
    if (cJSON_IsString(hostname) && hostname->valuestring != NULL) {
        strlcpy(result.hostname, hostname->valuestring, sizeof(result.hostname));
    } else {
        strlcpy(result.hostname, settings.pi_devices[0].name, sizeof(result.hostname));
    }
    if (cJSON_IsNumber(cpu) && cJSON_IsNumber(memory) &&
        cJSON_IsNumber(disk) && cJSON_IsNumber(temperature) && cJSON_IsNumber(uptime)) {
        unsigned long up = (unsigned long)uptime->valuedouble;
        snprintf(result.metrics, sizeof(result.metrics),
            "CPU       %.0f%%\nMemory  %.0f%%\nDisk        %.0f%%\nTemp       %.1f C\nUptime    %lud %luh",
            cpu->valuedouble, memory->valuedouble, disk->valuedouble,
            temperature->valuedouble, up / 86400UL, (up / 3600UL) % 24UL);
        result.ok = true;
    } else {
        strlcpy(result.message, "Missing metrics in Pi status JSON", sizeof(result.message));
    }
    cJSON_Delete(root);

finished:
    pi_request_active = false;
    esp_event_post_to(view_event_handle, VIEW_EVENT_BASE, TECH_EVENT_PI_RESULT,
                      &result, sizeof(result), pdMS_TO_TICKS(1000));
    vTaskDelete(NULL);
}

static void pi_refresh_event(lv_event_t *event)
{
    (void)event;
    if (pi_request_active) return;
    pi_request_active = true;
    lv_label_set_text(pi_status, "CONTACTING PI...");
    lv_label_set_text(pi_metrics, "Waiting for status response");
    /* HTTPS/TLS plus JSON parsing can exceed the original 6 KiB stack. */
    if (xTaskCreate(pi_fetch_task, "pi_status", 12288, NULL, 4, NULL) != pdPASS) {
        pi_request_active = false;
        lv_label_set_text(pi_status, "COULD NOT START REQUEST");
    }
}

static void pi_result_event(void *handler_args, esp_event_base_t base,
                            int32_t id, void *event_data)
{
    (void)handler_args;
    (void)base;
    if (id != TECH_EVENT_PI_RESULT || event_data == NULL) return;
    pi_result_t *result = event_data;
    lv_port_sem_take();
    if (pi_page != NULL && active_page == pi_page && pi_status != NULL && pi_metrics != NULL) {
        if (result->ok) {
            lv_label_set_text_fmt(pi_status, "ONLINE  /  %s", result->hostname);
            lv_label_set_text(pi_metrics, result->metrics);
            lv_obj_set_style_text_color(pi_status, lv_color_hex(0x42D6C3), 0);
        } else {
            lv_label_set_text(pi_status, "STATUS UNAVAILABLE");
            lv_label_set_text(pi_metrics, result->message);
            lv_obj_set_style_text_color(pi_status, lv_color_hex(0xFF6685), 0);
        }
    }
    lv_port_sem_give();
}

static void open_pi_page(void)
{
    if (pi_page == NULL) {
        pi_page = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(pi_page, lv_color_hex(0x0B1118), 0);
        lv_obj_set_style_border_width(pi_page, 0, 0);

        lv_obj_t *top = lv_obj_create(pi_page);
        lv_obj_set_size(top, 440, 42);
        lv_obj_align(top, LV_ALIGN_TOP_MID, 0, 14);
        lv_obj_set_style_bg_color(top, lv_color_hex(0x111C27), 0);
        lv_obj_set_style_border_width(top, 0, 0);
        lv_obj_set_style_radius(top, 12, 0);
        lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);
        make_label(top, "D1  /  TECH TERMINAL", 17, 0xEAF2F8);

        lv_obj_t *back = lv_btn_create(pi_page);
        lv_obj_set_size(back, 96, 38);
        lv_obj_align(back, LV_ALIGN_TOP_LEFT, 20, 72);
        lv_obj_set_style_bg_color(back, lv_color_hex(0x17304A), 0);
        lv_obj_set_style_border_width(back, 0, 0);
        lv_obj_add_event_cb(back, show_home, LV_EVENT_CLICKED, NULL);
        lv_obj_t *back_text = make_label(back, "< HOME", 12, 0x55A8FF);
        lv_obj_center(back_text);

        lv_obj_t *title = make_label(pi_page, "RASPBERRY PI MONITOR", 20, 0xF2F6FA);
        lv_obj_align(title, LV_ALIGN_TOP_LEFT, 126, 78);
        lv_obj_t *panel = lv_obj_create(pi_page);
        lv_obj_set_size(panel, 436, 252);
        lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, 132);
        lv_obj_set_style_bg_color(panel, lv_color_hex(0x14212D), 0);
        lv_obj_set_style_border_color(panel, lv_color_hex(0x2A3B49), 0);
        lv_obj_set_style_radius(panel, 14, 0);
        lv_obj_set_style_pad_all(panel, 18, 0);
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
        pi_status = make_label(panel, "READY", 15, 0x42D6C3);
        lv_obj_set_pos(pi_status, 4, 6);
        pi_metrics = make_label(panel, "CPU       --\nMemory  --\nDisk        --\nTemp       --\nUptime    --", 14, 0xC1CFD9);
        lv_obj_set_pos(pi_metrics, 4, 44);

        lv_obj_t *refresh = lv_btn_create(pi_page);
        lv_obj_set_size(refresh, 210, 48);
        lv_obj_align(refresh, LV_ALIGN_BOTTOM_MID, 0, -28);
        lv_obj_set_style_bg_color(refresh, lv_color_hex(0x174336), 0);
        lv_obj_set_style_border_width(refresh, 0, 0);
        lv_obj_set_style_radius(refresh, 10, 0);
        lv_obj_add_event_cb(refresh, pi_refresh_event, LV_EVENT_CLICKED, NULL);
        lv_obj_t *refresh_text = make_label(refresh, "REFRESH STATUS", 13, 0x42D6C3);
        lv_obj_center(refresh_text);
    }
    active_page = pi_page;
    lv_scr_load(pi_page);
    pi_refresh_event(NULL);
}

typedef struct {
    char url[160];
    bool add_history;
} web_request_t;

static void html_add_char(char *out, size_t cap, size_t *length, char value)
{
    if (*length + 1 >= cap) return;
    if (value == '\n' && (*length == 0 || out[*length - 1] == '\n')) return;
    out[(*length)++] = value;
    out[*length] = '\0';
}

static bool html_match_ci(const char *text, const char *match)
{
    while (*match && *text) {
        char a = *text++, b = *match++;
        if (a >= 'A' && a <= 'Z') a = (char)(a + ('a' - 'A'));
        if (b >= 'A' && b <= 'Z') b = (char)(b + ('a' - 'A'));
        if (a != b) return false;
    }
    return *match == '\0';
}

static void web_extract_text(const char *html, char *out, size_t capacity)
{
    bool in_tag = false;
    const char *skip_end = NULL;
    size_t written = 0;
    out[0] = '\0';
    for (size_t i = 0; html[i] && written + 1 < capacity; i++) {
        if (skip_end != NULL) {
            if (html[i] == '<' && html_match_ci(html + i, skip_end)) {
                in_tag = true;
                skip_end = NULL;
            }
            continue;
        }
        if (html[i] == '<') {
            const char *tag = html + i + 1;
            if (html_match_ci(tag, "script")) {
                skip_end = "</script";
            } else if (html_match_ci(tag, "style")) {
                skip_end = "</style";
            }
            if (html_match_ci(tag, "br") || html_match_ci(tag, "br/") ||
                html_match_ci(tag, "/p") || html_match_ci(tag, "p") ||
                html_match_ci(tag, "/div") || html_match_ci(tag, "div") ||
                html_match_ci(tag, "li") || html_match_ci(tag, "/li") ||
                html_match_ci(tag, "/h1") || html_match_ci(tag, "/h2") ||
                html_match_ci(tag, "/h3")) {
                html_add_char(out, capacity, &written, '\n');
            }
            in_tag = true;
            continue;
        }
        if (in_tag) {
            if (html[i] == '>') in_tag = false;
            continue;
        }
        if (html[i] == '&') {
            const char *entity = html + i;
            if (html_match_ci(entity, "&amp;")) { html_add_char(out, capacity, &written, '&'); i += 4; continue; }
            if (html_match_ci(entity, "&lt;")) { html_add_char(out, capacity, &written, '<'); i += 3; continue; }
            if (html_match_ci(entity, "&gt;")) { html_add_char(out, capacity, &written, '>'); i += 3; continue; }
            if (html_match_ci(entity, "&quot;")) { html_add_char(out, capacity, &written, '"'); i += 5; continue; }
            if (html_match_ci(entity, "&#39;")) { html_add_char(out, capacity, &written, '\''); i += 4; continue; }
            if (html_match_ci(entity, "&nbsp;")) { html_add_char(out, capacity, &written, ' '); i += 5; continue; }
        }
        if (html[i] == '\r' || html[i] == '\t') continue;
        html_add_char(out, capacity, &written, html[i]);
    }
}

static void web_fetch_task(void *arg)
{
    web_request_t *request = arg;
    web_result_t result = {0};
    strlcpy(result.url, request->url, sizeof(result.url));
    result.add_history = request->add_history;
    tech_http_response_t *response = calloc(1, sizeof(*response));
    free(request);
    if (response == NULL) {
        strlcpy(result.message, "Not enough memory to load page", sizeof(result.message));
        goto finished;
    }
    esp_err_t err = tech_http_get(result.url, "text/html,text/plain;q=0.9,*/*;q=0.5", response);
    if (err != ESP_OK) {
        free(response);
        strlcpy(result.message, "Request failed; check URL and Wi-Fi", sizeof(result.message));
        goto finished;
    }
    if (response->status_code < 200 || response->status_code >= 300) {
        snprintf(result.message, sizeof(result.message), "Server returned HTTP %d", response->status_code);
        free(response);
        goto finished;
    }
    web_extract_text(response->body, result.text, sizeof(result.text));
    free(response);
    if (result.text[0] == '\0') {
        strlcpy(result.message, "Page had no readable text", sizeof(result.message));
        goto finished;
    }
    result.ok = true;

finished:
    web_request_active = false;
    esp_event_post_to(view_event_handle, VIEW_EVENT_BASE, TECH_EVENT_WEB_RESULT,
                      &result, sizeof(result), pdMS_TO_TICKS(1000));
    vTaskDelete(NULL);
}

static bool web_url_valid(const char *url)
{
    if (url == NULL) return false;
    const char *authority = NULL;
    if (strncmp(url, "https://", 8) == 0) authority = url + 8;
    else if (strncmp(url, "http://", 7) == 0) authority = url + 7;
    else return false;

    if (*authority == '\0' || *authority == '/' || *authority == '?' ||
        *authority == '#') return false;
    const char *end = authority;
    while (*end && *end != '/' && *end != '?' && *end != '#') {
        unsigned char ch = (unsigned char)*end;
        if (ch <= 0x20 || ch >= 0x7f || ch == '\\' || ch == '@') return false;
        end++;
    }
    return end > authority;
}

static void web_start_fetch(const char *url, bool add_history)
{
    if (web_request_active || !web_url_valid(url)) {
        lv_label_set_text(web_status, "Enter a valid HTTP(S) URL with a host");
        return;
    }
    web_request_t *request = calloc(1, sizeof(*request));
    if (request == NULL) {
        lv_label_set_text(web_status, "Not enough memory to start request");
        return;
    }
    strlcpy(request->url, url, sizeof(request->url));
    request->add_history = add_history;
    web_request_active = true;
    lv_label_set_text(web_status, "LOADING...");
    lv_label_set_text(web_content, "Fetching page text");
    /* HTTPS/TLS and response processing can exceed the original 6 KiB stack. */
    if (xTaskCreate(web_fetch_task, "web_reader", 12288, request, 4, NULL) != pdPASS) {
        web_request_active = false;
        free(request);
        lv_label_set_text(web_status, "Could not start page request");
    }
}

static void web_go_event(lv_event_t *event)
{
    (void)event;
    if (web_keyboard != NULL) lv_obj_add_flag(web_keyboard, LV_OBJ_FLAG_HIDDEN);
    web_start_fetch(lv_textarea_get_text(web_url_field), true);
}

static void web_back_event(lv_event_t *event)
{
    (void)event;
    if (web_request_active || web_history_index == 0) return;
    web_history_index--;
    lv_textarea_set_text(web_url_field, web_history[web_history_index]);
    web_start_fetch(web_history[web_history_index], false);
}

static void web_forward_event(lv_event_t *event)
{
    (void)event;
    if (web_request_active || web_history_index + 1 >= web_history_count) return;
    web_history_index++;
    lv_textarea_set_text(web_url_field, web_history[web_history_index]);
    web_start_fetch(web_history[web_history_index], false);
}

static void web_field_event(lv_event_t *event)
{
    (void)event;
    lv_keyboard_set_textarea(web_keyboard, web_url_field);
    lv_obj_clear_flag(web_keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(web_keyboard);
}

static void web_keyboard_event(lv_event_t *event)
{
    (void)event;
    lv_keyboard_set_textarea(web_keyboard, NULL);
    lv_obj_add_flag(web_keyboard, LV_OBJ_FLAG_HIDDEN);
}

static void web_result_event(void *handler_args, esp_event_base_t base,
                             int32_t id, void *event_data)
{
    (void)handler_args;
    (void)base;
    if (id != TECH_EVENT_WEB_RESULT || event_data == NULL) return;
    web_result_t *result = event_data;
    lv_port_sem_take();
    if (web_page != NULL && active_page == web_page && web_status != NULL && web_content != NULL) {
        if (result->ok) {
            lv_label_set_text(web_status, result->url);
            lv_label_set_text(web_content, result->text);
            if (result->add_history) {
                while (web_history_count > web_history_index + 1) web_history_count--;
                if (web_history_count == WEB_HISTORY_MAX) {
                    memmove(web_history[0], web_history[1], (WEB_HISTORY_MAX - 1) * sizeof(web_history[0]));
                    web_history_count--;
                }
                strlcpy(web_history[web_history_count], result->url, sizeof(web_history[0]));
                web_history_index = web_history_count++;
            }
        } else {
            lv_label_set_text(web_status, "LOAD FAILED");
            lv_label_set_text(web_content, result->message);
        }
    }
    lv_port_sem_give();
}

static void open_web_page(void)
{
    if (web_page == NULL) {
        web_page = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(web_page, lv_color_hex(0x0B1118), 0);
        lv_obj_set_style_border_width(web_page, 0, 0);
        lv_obj_t *top = lv_obj_create(web_page);
        lv_obj_set_size(top, 440, 42);
        lv_obj_align(top, LV_ALIGN_TOP_MID, 0, 10);
        lv_obj_set_style_bg_color(top, lv_color_hex(0x111C27), 0);
        lv_obj_set_style_border_width(top, 0, 0);
        lv_obj_set_style_radius(top, 12, 0);
        lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);
        make_label(top, "D1  /  TECH TERMINAL", 17, 0xEAF2F8);

        lv_obj_t *back = lv_btn_create(web_page);
        lv_obj_set_size(back, 88, 36);
        lv_obj_align(back, LV_ALIGN_TOP_LEFT, 18, 60);
        lv_obj_set_style_bg_color(back, lv_color_hex(0x17304A), 0);
        lv_obj_set_style_border_width(back, 0, 0);
        lv_obj_add_event_cb(back, show_home, LV_EVENT_CLICKED, NULL);
        lv_obj_t *back_label = make_label(back, "< HOME", 12, 0x55A8FF);
        lv_obj_center(back_label);

        web_url_field = lv_textarea_create(web_page);
        lv_obj_set_size(web_url_field, 292, 36);
        lv_obj_align(web_url_field, LV_ALIGN_TOP_LEFT, 112, 60);
        lv_textarea_set_one_line(web_url_field, true);
        lv_textarea_set_max_length(web_url_field, 159);
        lv_obj_set_style_bg_color(web_url_field, lv_color_hex(0x14212D), 0);
        lv_obj_set_style_text_color(web_url_field, lv_color_hex(0xEDF4F8), 0);
        lv_obj_add_event_cb(web_url_field, web_field_event, LV_EVENT_CLICKED, NULL);

        lv_obj_t *go = lv_btn_create(web_page);
        lv_obj_set_size(go, 46, 36);
        lv_obj_align(go, LV_ALIGN_TOP_RIGHT, -18, 60);
        lv_obj_set_style_bg_color(go, lv_color_hex(0x174336), 0);
        lv_obj_set_style_border_width(go, 0, 0);
        lv_obj_add_event_cb(go, web_go_event, LV_EVENT_CLICKED, NULL);
        lv_obj_t *go_text = make_label(go, "GO", 12, 0x42D6C3);
        lv_obj_center(go_text);

        web_status = make_label(web_page, "Enter a URL", 11, 0x8295A7);
        lv_obj_set_width(web_status, 444);
        lv_label_set_long_mode(web_status, LV_LABEL_LONG_DOT);
        lv_obj_align(web_status, LV_ALIGN_TOP_LEFT, 20, 104);

        lv_obj_t *content_panel = lv_obj_create(web_page);
        lv_obj_set_size(content_panel, 440, 270);
        lv_obj_align(content_panel, LV_ALIGN_TOP_MID, 0, 130);
        lv_obj_set_style_bg_color(content_panel, lv_color_hex(0x14212D), 0);
        lv_obj_set_style_border_color(content_panel, lv_color_hex(0x2A3B49), 0);
        lv_obj_set_style_radius(content_panel, 12, 0);
        lv_obj_set_style_pad_all(content_panel, 12, 0);
        lv_obj_set_scrollbar_mode(content_panel, LV_SCROLLBAR_MODE_AUTO);
        web_content = make_label(content_panel, "Readable page text will appear here", 13, 0xC1CFD9);
        lv_obj_set_width(web_content, 396);
        lv_label_set_long_mode(web_content, LV_LABEL_LONG_WRAP);

        lv_obj_t *prev = lv_btn_create(web_page);
        lv_obj_set_size(prev, 118, 36);
        lv_obj_align(prev, LV_ALIGN_BOTTOM_LEFT, 20, -8);
        lv_obj_set_style_bg_color(prev, lv_color_hex(0x17304A), 0);
        lv_obj_set_style_border_width(prev, 0, 0);
        lv_obj_add_event_cb(prev, web_back_event, LV_EVENT_CLICKED, NULL);
        lv_obj_t *prev_label = make_label(prev, "< BACK", 12, 0x55A8FF);
        lv_obj_center(prev_label);

        lv_obj_t *next = lv_btn_create(web_page);
        lv_obj_set_size(next, 118, 36);
        lv_obj_align(next, LV_ALIGN_BOTTOM_RIGHT, -20, -8);
        lv_obj_set_style_bg_color(next, lv_color_hex(0x17304A), 0);
        lv_obj_set_style_border_width(next, 0, 0);
        lv_obj_add_event_cb(next, web_forward_event, LV_EVENT_CLICKED, NULL);
        lv_obj_t *next_label = make_label(next, "FORWARD >", 12, 0x55A8FF);
        lv_obj_center(next_label);

        web_keyboard = lv_keyboard_create(web_page);
        lv_obj_set_size(web_keyboard, 480, 190);
        lv_obj_align(web_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_add_flag(web_keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_event_cb(web_keyboard, web_keyboard_event, LV_EVENT_READY, NULL);
        lv_obj_add_event_cb(web_keyboard, web_keyboard_event, LV_EVENT_CANCEL, NULL);
        tech_settings_t settings;
        if (tech_settings_get(&settings) == ESP_OK) {
            lv_textarea_set_text(web_url_field, settings.browser_home);
        }
    }
    active_page = web_page;
    lv_scr_load(web_page);
}

static bool github_component_valid(const char *text)
{
    if (text == NULL || text[0] == '\0') return false;
    for (size_t i = 0; text[i]; i++) {
        char c = text[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.')) return false;
    }
    return true;
}

static void github_append_latest_workflow(const char *owner, const char *repo,
                                             char *details, size_t capacity)
{
    char url[320];
    snprintf(url, sizeof(url),
             "https://api.github.com/repos/%s/%s/actions/runs?per_page=1", owner, repo);
    tech_http_response_t *response = calloc(1, sizeof(*response));
    if (response == NULL) return;

    esp_err_t err = tech_http_get(url, "application/vnd.github+json", response);
    size_t used = strlen(details);
    if (used >= capacity) {
        free(response);
        return;
    }
    if (err != ESP_OK) {
        snprintf(details + used, capacity - used, "\n\nWorkflow status unavailable (network error)");
        free(response);
        return;
    }
    if (response->status_code == 403 || response->status_code == 429) {
        snprintf(details + used, capacity - used,
                 "\n\nWorkflow status unavailable (GitHub rate limit or permissions)");
        free(response);
        return;
    }
    if (response->status_code != 200) {
        snprintf(details + used, capacity - used,
                 "\n\nWorkflow API returned HTTP %d", response->status_code);
        free(response);
        return;
    }

    cJSON *root = cJSON_Parse(response->body);
    free(response);
    cJSON *runs = cJSON_GetObjectItemCaseSensitive(root, "workflow_runs");
    cJSON *run = cJSON_IsArray(runs) ? cJSON_GetArrayItem(runs, 0) : NULL;
    if (run == NULL) {
        snprintf(details + used, capacity - used, "\n\nNo workflow runs found");
        cJSON_Delete(root);
        return;
    }
    cJSON *name = cJSON_GetObjectItemCaseSensitive(run, "name");
    cJSON *status = cJSON_GetObjectItemCaseSensitive(run, "status");
    cJSON *conclusion = cJSON_GetObjectItemCaseSensitive(run, "conclusion");
    cJSON *updated = cJSON_GetObjectItemCaseSensitive(run, "updated_at");
    const char *run_name = cJSON_IsString(name) && name->valuestring ?
                           name->valuestring : "Workflow";
    const char *run_status = cJSON_IsString(status) && status->valuestring ?
                             status->valuestring : "unknown";
    const char *run_conclusion = cJSON_IsString(conclusion) && conclusion->valuestring ?
                                 conclusion->valuestring : "pending";
    const char *date = cJSON_IsString(updated) && updated->valuestring ?
                       updated->valuestring : "unknown";
    snprintf(details + used, capacity - used,
             "\n\nLatest workflow\n%.48s: %.16s / %.16s\n%.19s",
             run_name, run_status, run_conclusion, date);
    cJSON_Delete(root);
}

static void github_fetch_task(void *arg)
{
    github_result_t result = {0};
    tech_settings_t settings;
    if (tech_settings_get(&settings) != ESP_OK || !settings.github_owner[0] ||
        !settings.github_repo_count || !settings.github_repos[0][0]) {
        strlcpy(result.message, "Set GitHub owner and repository in Settings", sizeof(result.message));
        goto finished;
    }
    const char *owner = settings.github_owner;
    const char *repo = settings.github_repos[0];
    if (!github_component_valid(owner) || !github_component_valid(repo)) {
        strlcpy(result.message, "Owner/repository may use letters, digits, . _ -", sizeof(result.message));
        goto finished;
    }
    char url[320];
    snprintf(url, sizeof(url), "https://api.github.com/repos/%s/%s", owner, repo);
    tech_http_response_t *response = calloc(1, sizeof(*response));
    if (response == NULL) {
        strlcpy(result.message, "Not enough memory for GitHub response", sizeof(result.message));
        goto finished;
    }
    esp_err_t err = tech_http_get(url, "application/vnd.github+json", response);
    if (err != ESP_OK) {
        free(response);
        strlcpy(result.message, "GitHub request failed; check Wi-Fi", sizeof(result.message));
        goto finished;
    }
    if (response->status_code != 200) {
        if (response->status_code == 403 || response->status_code == 429) {
            strlcpy(result.message, "GitHub API rate limit or access restriction; retry later",
                    sizeof(result.message));
        } else {
            snprintf(result.message, sizeof(result.message), "GitHub returned HTTP %d", response->status_code);
        }
        free(response);
        goto finished;
    }
    cJSON *root = cJSON_Parse(response->body);
    free(response);
    if (root == NULL) {
        strlcpy(result.message, "GitHub response was not valid JSON", sizeof(result.message));
        goto finished;
    }
    cJSON *full_name = cJSON_GetObjectItemCaseSensitive(root, "full_name");
    cJSON *description = cJSON_GetObjectItemCaseSensitive(root, "description");
    cJSON *stars = cJSON_GetObjectItemCaseSensitive(root, "stargazers_count");
    cJSON *issues = cJSON_GetObjectItemCaseSensitive(root, "open_issues_count");
    cJSON *updated = cJSON_GetObjectItemCaseSensitive(root, "updated_at");
    cJSON *html_url = cJSON_GetObjectItemCaseSensitive(root, "html_url");
    if (cJSON_IsString(full_name) && cJSON_IsNumber(stars) && cJSON_IsNumber(issues)) {
        strlcpy(result.title, full_name->valuestring, sizeof(result.title));
        const char *desc = cJSON_IsString(description) && description->valuestring ?
                           description->valuestring : "No description";
        const char *date = cJSON_IsString(updated) && updated->valuestring ?
                           updated->valuestring : "Unknown";
        snprintf(result.details, sizeof(result.details),
            "Stars  %lld\nOpen issues  %lld\nUpdated  %.24s\n\n%.150s",
            (long long)stars->valuedouble, (long long)issues->valuedouble, date, desc);
        if (cJSON_IsString(html_url) && html_url->valuestring) {
            strlcpy(result.html_url, html_url->valuestring, sizeof(result.html_url));
        }
        result.ok = true;
    } else {
        strlcpy(result.message, "GitHub response is missing repository fields", sizeof(result.message));
    }
    cJSON_Delete(root);
    if (result.ok) {
        github_append_latest_workflow(owner, repo, result.details, sizeof(result.details));
    }

finished:
    github_request_active = false;
    esp_event_post_to(view_event_handle, VIEW_EVENT_BASE, TECH_EVENT_GITHUB_RESULT,
                      &result, sizeof(result), pdMS_TO_TICKS(1000));
    vTaskDelete(NULL);
}

static void github_refresh_event(lv_event_t *event)
{
    (void)event;
    if (github_request_active) return;
    github_request_active = true;
    lv_label_set_text(github_status, "LOADING REPOSITORY...");
    lv_label_set_text(github_details, "Contacting api.github.com");
    /* HTTPS/TLS plus cJSON parsing needs more than the original 6 KiB stack. */
    if (xTaskCreate(github_fetch_task, "github_repo", 12288, NULL, 4, NULL) != pdPASS) {
        github_request_active = false;
        lv_label_set_text(github_status, "COULD NOT START REQUEST");
    }
}

static void github_result_event(void *handler_args, esp_event_base_t base,
                                int32_t id, void *event_data)
{
    (void)handler_args;
    (void)base;
    if (id != TECH_EVENT_GITHUB_RESULT || event_data == NULL) return;
    github_result_t *result = event_data;
    lv_port_sem_take();
    if (github_page != NULL && active_page == github_page &&
        github_status != NULL && github_details != NULL) {
        if (result->ok) {
            lv_label_set_text(github_status, result->title);
            lv_label_set_text(github_details, result->details);
            lv_obj_set_style_text_color(github_status, lv_color_hex(0x42D6C3), 0);
            strlcpy(github_web_url, result->html_url, sizeof(github_web_url));
        } else {
            lv_label_set_text(github_status, "REPOSITORY UNAVAILABLE");
            lv_label_set_text(github_details, result->message);
            lv_obj_set_style_text_color(github_status, lv_color_hex(0xFF6685), 0);
        }
    }
    lv_port_sem_give();
}

static void github_open_web_event(lv_event_t *event)
{
    (void)event;
    if (github_web_url[0] == '\0') return;
    open_web_page();
    lv_textarea_set_text(web_url_field, github_web_url);
    web_start_fetch(github_web_url, true);
}

static void open_github_page(void)
{
    if (github_page == NULL) {
        github_page = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(github_page, lv_color_hex(0x0B1118), 0);
        lv_obj_set_style_border_width(github_page, 0, 0);
        lv_obj_t *top = lv_obj_create(github_page);
        lv_obj_set_size(top, 440, 42);
        lv_obj_align(top, LV_ALIGN_TOP_MID, 0, 14);
        lv_obj_set_style_bg_color(top, lv_color_hex(0x111C27), 0);
        lv_obj_set_style_border_width(top, 0, 0);
        lv_obj_set_style_radius(top, 12, 0);
        lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);
        make_label(top, "D1  /  TECH TERMINAL", 17, 0xEAF2F8);

        lv_obj_t *back = lv_btn_create(github_page);
        lv_obj_set_size(back, 96, 38);
        lv_obj_align(back, LV_ALIGN_TOP_LEFT, 20, 72);
        lv_obj_set_style_bg_color(back, lv_color_hex(0x17304A), 0);
        lv_obj_set_style_border_width(back, 0, 0);
        lv_obj_add_event_cb(back, show_home, LV_EVENT_CLICKED, NULL);
        lv_obj_t *back_text = make_label(back, "< HOME", 12, 0x55A8FF);
        lv_obj_center(back_text);
        lv_obj_t *title = make_label(github_page, "GITHUB REPOSITORY", 20, 0xF2F6FA);
        lv_obj_align(title, LV_ALIGN_TOP_LEFT, 126, 78);

        lv_obj_t *panel = lv_obj_create(github_page);
        lv_obj_set_size(panel, 436, 250);
        lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, 132);
        lv_obj_set_style_bg_color(panel, lv_color_hex(0x14212D), 0);
        lv_obj_set_style_border_color(panel, lv_color_hex(0x2A3B49), 0);
        lv_obj_set_style_radius(panel, 14, 0);
        lv_obj_set_style_pad_all(panel, 16, 0);
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
        github_status = make_label(panel, "READY", 15, 0x42D6C3);
        lv_obj_set_pos(github_status, 4, 4);
        lv_obj_set_width(github_status, 390);
        lv_label_set_long_mode(github_status, LV_LABEL_LONG_DOT);
        github_details = make_label(panel, "Public repository metrics will appear here", 13, 0xC1CFD9);
        lv_obj_set_width(github_details, 396);
        lv_label_set_long_mode(github_details, LV_LABEL_LONG_WRAP);
        lv_obj_set_pos(github_details, 4, 42);

        lv_obj_t *refresh = lv_btn_create(github_page);
        lv_obj_set_size(refresh, 196, 46);
        lv_obj_align(refresh, LV_ALIGN_BOTTOM_LEFT, 24, -22);
        lv_obj_set_style_bg_color(refresh, lv_color_hex(0x174336), 0);
        lv_obj_set_style_border_width(refresh, 0, 0);
        lv_obj_add_event_cb(refresh, github_refresh_event, LV_EVENT_CLICKED, NULL);
        lv_obj_t *refresh_text = make_label(refresh, "REFRESH", 13, 0x42D6C3);
        lv_obj_center(refresh_text);

        lv_obj_t *open_web = lv_btn_create(github_page);
        lv_obj_set_size(open_web, 196, 46);
        lv_obj_align(open_web, LV_ALIGN_BOTTOM_RIGHT, -24, -22);
        lv_obj_set_style_bg_color(open_web, lv_color_hex(0x17304A), 0);
        lv_obj_set_style_border_width(open_web, 0, 0);
        lv_obj_add_event_cb(open_web, github_open_web_event, LV_EVENT_CLICKED, NULL);
        lv_obj_t *open_text = make_label(open_web, "OPEN IN WEB", 13, 0x55A8FF);
        lv_obj_center(open_text);
    }
    active_page = github_page;
    lv_scr_load(github_page);
    github_refresh_event(NULL);
}

static size_t f1_format_sessions(const char *body, char *details, size_t capacity,
                                 const char *today, bool upcoming_only)
{
    cJSON *root = cJSON_Parse(body);
    if (root == NULL || !cJSON_IsArray(root)) {
        cJSON_Delete(root);
        return 0;
    }

    char recent[5][96] = {{0}};
    size_t total = 0;
    size_t shown = 0;
    cJSON *session = NULL;
    cJSON_ArrayForEach(session, root) {
        cJSON *date = cJSON_GetObjectItemCaseSensitive(session, "date_start");
        cJSON *circuit = cJSON_GetObjectItemCaseSensitive(session, "circuit_short_name");
        if (!cJSON_IsString(date) || !cJSON_IsString(circuit) ||
            date->valuestring == NULL || circuit->valuestring == NULL) {
            continue;
        }
        if (upcoming_only && strncmp(date->valuestring, today, 10) < 0) {
            continue;
        }
        char row[96];
        snprintf(row, sizeof(row), "%.10s  %s", date->valuestring, circuit->valuestring);
        if (upcoming_only) {
            size_t used = strlen(details);
            if (shown < 5 && used < capacity) {
                int written = snprintf(details + used, capacity - used, "%s%s",
                                       shown == 0 ? "" : "\n", row);
                if (written > 0 && (size_t)written < capacity - used) {
                    shown++;
                }
            }
            if (shown >= 5) break;
        } else {
            strlcpy(recent[total % 5], row, sizeof(recent[0]));
            total++;
        }
    }

    if (!upcoming_only && total > 0) {
        size_t count = total < 5 ? total : 5;
        size_t start = total > 5 ? total % 5 : 0;
        details[0] = '\0';
        for (size_t i = 0; i < count; i++) {
            const char *row = recent[(start + i) % 5];
            size_t used = strlen(details);
            if (used >= capacity) break;
            int written = snprintf(details + used, capacity - used, "%s%s",
                                   i == 0 ? "" : "\n", row);
            if (written < 0 || (size_t)written >= capacity - used) break;
            shown++;
        }
    }

    cJSON_Delete(root);
    return shown;
}

static void f1_append_latest_result(tech_http_response_t *response,
                                     char *details, size_t capacity,
                                     const char *today, int year)
{
    char url[192];
    snprintf(url, sizeof(url),
             "https://api.openf1.org/v1/sessions?year=%d&session_name=Race", year);
    if (tech_http_get(url, "application/json", response) != ESP_OK ||
        response->status_code != 200) return;

    cJSON *sessions = cJSON_Parse(response->body);
    if (!cJSON_IsArray(sessions)) {
        cJSON_Delete(sessions);
        return;
    }

    int latest_key = 0;
    char latest_circuit[48] = {0};
    char latest_date[16] = {0};
    cJSON *session = NULL;
    cJSON_ArrayForEach(session, sessions) {
        cJSON *date = cJSON_GetObjectItemCaseSensitive(session, "date_start");
        cJSON *key = cJSON_GetObjectItemCaseSensitive(session, "session_key");
        cJSON *circuit = cJSON_GetObjectItemCaseSensitive(session, "circuit_short_name");
        if (!cJSON_IsString(date) || !cJSON_IsNumber(key) ||
            !cJSON_IsString(circuit) || date->valuestring == NULL ||
            circuit->valuestring == NULL || strncmp(date->valuestring, today, 10) >= 0) {
            continue;
        }
        if (latest_date[0] == '\0' || strncmp(date->valuestring, latest_date, 10) > 0) {
            strlcpy(latest_date, date->valuestring, sizeof(latest_date));
            strlcpy(latest_circuit, circuit->valuestring, sizeof(latest_circuit));
            latest_key = (int)key->valuedouble;
        }
    }
    cJSON_Delete(sessions);
    if (latest_key <= 0) return;

    snprintf(url, sizeof(url),
             "https://api.openf1.org/v1/session_result?session_key=%d", latest_key);
    if (tech_http_get(url, "application/json", response) != ESP_OK ||
        response->status_code != 200) return;

    int podium_numbers[3] = {-1, -1, -1};
    cJSON *results = cJSON_Parse(response->body);
    if (!cJSON_IsArray(results)) {
        cJSON_Delete(results);
        return;
    }
    cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, results) {
        cJSON *position = cJSON_GetObjectItemCaseSensitive(entry, "position");
        cJSON *number = cJSON_GetObjectItemCaseSensitive(entry, "driver_number");
        cJSON *dnf = cJSON_GetObjectItemCaseSensitive(entry, "dnf");
        cJSON *dns = cJSON_GetObjectItemCaseSensitive(entry, "dns");
        cJSON *dsq = cJSON_GetObjectItemCaseSensitive(entry, "dsq");
        if (!cJSON_IsNumber(position) || !cJSON_IsNumber(number) ||
            (cJSON_IsTrue(dnf)) || (cJSON_IsTrue(dns)) || (cJSON_IsTrue(dsq))) continue;
        int pos = (int)position->valuedouble;
        if (pos >= 1 && pos <= 3) podium_numbers[pos - 1] = (int)number->valuedouble;
    }
    cJSON_Delete(results);

    char names[3][40] = {{0}};
    snprintf(url, sizeof(url),
             "https://api.openf1.org/v1/drivers?session_key=%d", latest_key);
    if (tech_http_get(url, "application/json", response) == ESP_OK &&
        response->status_code == 200) {
        cJSON *drivers = cJSON_Parse(response->body);
        if (cJSON_IsArray(drivers)) {
            cJSON *driver = NULL;
            cJSON_ArrayForEach(driver, drivers) {
                cJSON *number = cJSON_GetObjectItemCaseSensitive(driver, "driver_number");
                cJSON *name = cJSON_GetObjectItemCaseSensitive(driver, "name_acronym");
                if (!cJSON_IsNumber(number) || !cJSON_IsString(name) ||
                    name->valuestring == NULL) continue;
                for (int i = 0; i < 3; i++) {
                    if (podium_numbers[i] == (int)number->valuedouble) {
                        strlcpy(names[i], name->valuestring, sizeof(names[i]));
                    }
                }
            }
        }
        cJSON_Delete(drivers);
    }

    size_t used = strlen(details);
    if (used >= capacity) return;
    int written = snprintf(details + used, capacity - used,
                           "%sLAST RACE - %.10s %s",
                           used == 0 ? "" : "\n\n", latest_date, latest_circuit);
    if (written < 0 || (size_t)written >= capacity - used) return;
    used += (size_t)written;
    for (int i = 0; i < 3; i++) {
        if (podium_numbers[i] < 0) continue;
        written = snprintf(details + used, capacity - used, "\nP%d  %s%s#%d",
                           i + 1, names[i][0] ? names[i] : "CAR ",
                           names[i][0] ? "  " : "", podium_numbers[i]);
        if (written < 0 || (size_t)written >= capacity - used) break;
        used += (size_t)written;
    }
}

static void f1_fetch_task(void *arg)
{
    (void)arg;
    f1_result_t result = {0};
    time_t now = time(NULL);
    struct tm utc = {0};
    char today[16] = "2026-10-27";
    int year = 2026;
    bool clock_valid = now > 1767225600;
    if (clock_valid && gmtime_r(&now, &utc) != NULL) {
        year = utc.tm_year + 1900;
        strftime(today, sizeof(today), "%Y-%m-%d", &utc);
    }
    result.clock_valid = clock_valid;
    tech_http_response_t *response = calloc(1, sizeof(*response));
    if (response == NULL) {
        strlcpy(result.message, "Not enough memory for the F1 response", sizeof(result.message));
        goto finished;
    }

    char url[224];
    if (clock_valid) {
        snprintf(url, sizeof(url),
                 "https://api.openf1.org/v1/sessions?year=%d&session_name=Race&date_start%%3E%%3D%.10sT00%%3A00%%3A00Z",
                 year, today);
    } else {
        snprintf(url, sizeof(url),
                 "https://api.openf1.org/v1/sessions?year=%d&session_name=Race", year);
    }

    esp_err_t err = tech_http_get(url, "application/json", response);
    if (err != ESP_OK || response->status_code != 200) {
        snprintf(result.message, sizeof(result.message),
                 "OpenF1 request failed (HTTP %d); check Wi-Fi", response->status_code);
        free(response);
        goto finished;
    }

    size_t count = f1_format_sessions(response->body, result.details,
                                       sizeof(result.details), today, clock_valid);
    if (count == 0 && clock_valid) {
        /* If the season has ended, show its latest races instead of a blank page. */
        snprintf(url, sizeof(url),
                 "https://api.openf1.org/v1/sessions?year=%d&session_name=Race", year);
        memset(response, 0, sizeof(*response));
        err = tech_http_get(url, "application/json", response);
        if (err == ESP_OK && response->status_code == 200) {
            count = f1_format_sessions(response->body, result.details,
                                       sizeof(result.details), today, false);
            result.clock_valid = false;
        }
    }
    if (count == 0) {
        free(response);
        strlcpy(result.message, "No race sessions found for this season", sizeof(result.message));
        goto finished;
    }
    if (clock_valid) {
        f1_append_latest_result(response, result.details, sizeof(result.details), today, year);
    }
    free(response);
    strlcpy(result.title, result.clock_valid ? "UPCOMING RACES" : "RECENT RACES",
            sizeof(result.title));
    result.ok = true;
    strlcpy(f1_cache_details, result.details, sizeof(f1_cache_details));
    f1_cache_time = time(NULL);

finished:
    /* If the network is down, preserve a recent successful schedule rather than
       replacing useful data with an error. Cache is fixed-size and expires. */
    if (!result.ok && f1_cache_time > 0 && now >= f1_cache_time &&
        difftime(now, f1_cache_time) <= 1800.0 && f1_cache_details[0] != '\0') {
        result.ok = true;
        strlcpy(result.title, "CACHED RACES (OFFLINE)", sizeof(result.title));
        strlcpy(result.details, f1_cache_details, sizeof(result.details));
        ESP_LOGW("f1_ui", "OpenF1 unavailable; displaying cached schedule");
    }
    f1_request_active = false;
    esp_event_post_to(view_event_handle, VIEW_EVENT_BASE, TECH_EVENT_F1_RESULT,
                      &result, sizeof(result), pdMS_TO_TICKS(1000));
    vTaskDelete(NULL);
}

static void f1_refresh_event(lv_event_t *event)
{
    (void)event;
    if (f1_request_active) return;
    f1_request_active = true;
    lv_label_set_text(f1_status, "LOADING RACE SCHEDULE...");
    lv_label_set_text(f1_details, "Contacting api.openf1.org");
    if (xTaskCreate(f1_fetch_task, "f1_schedule", 8192, NULL, 4, NULL) != pdPASS) {
        f1_request_active = false;
        lv_label_set_text(f1_status, "COULD NOT START REQUEST");
    }
}

static void f1_result_event(void *handler_args, esp_event_base_t base,
                            int32_t id, void *event_data)
{
    (void)handler_args;
    (void)base;
    if (id != TECH_EVENT_F1_RESULT || event_data == NULL) return;
    f1_result_t *result = event_data;
    ESP_LOGI("f1_ui", "Schedule result: ok=%d, status=%s", result->ok,
             result->ok ? result->title : result->message);
    lv_port_sem_take();
    if (f1_page != NULL && active_page == f1_page &&
        f1_status != NULL && f1_details != NULL) {
        if (result->ok) {
            lv_label_set_text(f1_status, result->title);
            lv_label_set_text(f1_details, result->details);
            lv_obj_set_style_text_color(f1_status, lv_color_hex(0x42D6C3), 0);
        } else {
            lv_label_set_text(f1_status, "SCHEDULE UNAVAILABLE");
            lv_label_set_text(f1_details, result->message);
            lv_obj_set_style_text_color(f1_status, lv_color_hex(0xFF6685), 0);
        }
    }
    lv_port_sem_give();
}

static void open_f1_page(void)
{
    if (f1_page == NULL) {
        /* Use a hidden overlay on the home screen rather than a dynamically loaded LVGL screen. */
        f1_page = lv_obj_create(home);
        if (f1_page == NULL) {
            ESP_LOGE("f1_ui", "Could not allocate F1 overlay; internal=%u psram=%u bytes free",
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
            return;
        }
        lv_obj_set_size(f1_page, LV_PCT(100), LV_PCT(100));
        lv_obj_align(f1_page, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_add_flag(f1_page, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(f1_page, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(f1_page, lv_color_hex(0x0B1118), 0);
        lv_obj_set_style_bg_opa(f1_page, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(f1_page, 0, 0);
        lv_obj_t *top = lv_obj_create(f1_page);
        lv_obj_set_size(top, 440, 42);
        lv_obj_align(top, LV_ALIGN_TOP_MID, 0, 14);
        lv_obj_set_style_bg_color(top, lv_color_hex(0x111C27), 0);
        lv_obj_set_style_border_width(top, 0, 0);
        lv_obj_set_style_radius(top, 12, 0);
        lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);
        make_label(top, "D1  /  TECH TERMINAL", 17, 0xEAF2F8);

        lv_obj_t *back = lv_btn_create(f1_page);
        lv_obj_set_size(back, 96, 38);
        lv_obj_align(back, LV_ALIGN_TOP_LEFT, 20, 72);
        lv_obj_set_style_bg_color(back, lv_color_hex(0x17304A), 0);
        lv_obj_set_style_border_width(back, 0, 0);
        lv_obj_add_event_cb(back, show_home, LV_EVENT_CLICKED, NULL);
        lv_obj_t *back_text = make_label(back, "< HOME", 12, 0x55A8FF);
        lv_obj_center(back_text);
        lv_obj_t *title = make_label(f1_page, "FORMULA 1", 22, 0xF2F6FA);
        lv_obj_align(title, LV_ALIGN_TOP_LEFT, 136, 78);

        lv_obj_t *panel = lv_obj_create(f1_page);
        lv_obj_set_size(panel, 436, 270);
        lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, 132);
        lv_obj_set_style_bg_color(panel, lv_color_hex(0x14212D), 0);
        lv_obj_set_style_border_color(panel, lv_color_hex(0x2A3B49), 0);
        lv_obj_set_style_radius(panel, 14, 0);
        lv_obj_set_style_pad_all(panel, 16, 0);
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
        f1_status = make_label(panel, "READY", 15, 0x42D6C3);
        lv_obj_align(f1_status, LV_ALIGN_TOP_LEFT, 4, 4);
        f1_details = make_label(panel, "Race calendar\nTap REFRESH to load upcoming races.", 14, 0xC1CFD9);
        lv_obj_set_width(f1_details, 390);
        lv_label_set_long_mode(f1_details, LV_LABEL_LONG_WRAP);
        lv_obj_align(f1_details, LV_ALIGN_TOP_LEFT, 4, 42);

        lv_obj_t *refresh = lv_btn_create(f1_page);
        lv_obj_set_size(refresh, 120, 40);
        lv_obj_align(refresh, LV_ALIGN_BOTTOM_RIGHT, -20, -18);
        lv_obj_set_style_bg_color(refresh, lv_color_hex(0x17304A), 0);
        lv_obj_set_style_border_width(refresh, 0, 0);
        lv_obj_add_event_cb(refresh, f1_refresh_event, LV_EVENT_CLICKED, NULL);
        lv_obj_t *refresh_text = make_label(refresh, "REFRESH", 12, 0x55A8FF);
        lv_obj_center(refresh_text);
    }
    if (f1_preparing) return;
    if (f1_page == NULL) {
        ESP_LOGE("f1_ui", "F1 screen is unavailable; refusing to load a null screen");
        return;
    }
    ESP_LOGI("f1_ui", "Opening F1 schedule overlay");
    active_page = f1_page;
    lv_obj_move_foreground(f1_page);
    lv_obj_clear_flag(f1_page, LV_OBJ_FLAG_HIDDEN);
    f1_refresh_event(NULL);
}

static void open_module(lv_event_t *event)
{
    int index = (int)(intptr_t)lv_event_get_user_data(event);
    if (index == 1) {
        open_network_page();
        return;
    }
    if (index == 0) {
        open_web_page();
        return;
    }
    if (index == 2) {
        open_pi_page();
        return;
    }
    if (index == 3) {
        open_github_page();
        return;
    }
    if (index == 4) {
        open_f1_page();
        return;
    }

    lv_obj_t *page = lv_obj_create(NULL);
    active_page = page;
    lv_obj_set_style_bg_color(page, lv_color_hex(0x0B1118), 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_t *back = lv_btn_create(page);
    lv_obj_set_size(back, 95, 42);
    lv_obj_align(back, LV_ALIGN_TOP_LEFT, 18, 18);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x17304A), 0);
    lv_obj_add_event_cb(back, show_home, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_label = make_label(back, "<  HOME", 13, 0x55A8FF);
    lv_obj_center(back_label);
    lv_obj_t *title = make_label(page, module_names[index], 24, 0xF2F6FA);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 22, 88);
    lv_obj_t *desc = make_label(page, module_info[index], 15, 0x91A3B4);
    lv_obj_set_width(desc, 420);
    lv_obj_align(desc, LV_ALIGN_TOP_LEFT, 22, 133);
    lv_obj_t *panel = lv_obj_create(page);
    lv_obj_set_size(panel, 436, 160);
    lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, 190);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x14212D), 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(0x2A3B49), 0);
    lv_obj_set_style_radius(panel, 14, 0);
    lv_obj_t *state = make_label(panel, "MODULE READY FOR INTEGRATION", 14, 0x42D6C3);
    lv_obj_align(state, LV_ALIGN_TOP_LEFT, 12, 12);
    lv_obj_t *detail = make_label(panel,
        "Home UI  /  Navigation  /  Settings\nWi-Fi  •  HTTP  •  HTTPS  •  DNS\nESP32-S3  /  480 x 480  /  Touch",
        13, 0xC1CFD9);
    lv_obj_set_width(detail, 390);
    lv_obj_align(detail, LV_ALIGN_TOP_LEFT, 12, 50);
    lv_scr_load(page);
}

static void open_settings(lv_event_t *event)
{
    (void)event;
    tech_settings_t settings;
    if (tech_settings_get(&settings) != ESP_OK) return;

    lv_obj_t *page = lv_obj_create(NULL);
    active_page = page;
    lv_obj_set_style_bg_color(page, lv_color_hex(0x0B1118), 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_t *back = lv_btn_create(page);
    lv_obj_set_size(back, 90, 40);
    lv_obj_align(back, LV_ALIGN_TOP_LEFT, 16, 14);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x17304A), 0);
    lv_obj_add_event_cb(back, show_home, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_label = make_label(back, "< HOME", 13, 0x55A8FF);
    lv_obj_center(back_label);

    lv_obj_t *title = make_label(page, "SETTINGS", 22, 0xF2F6FA);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 22);
    lv_obj_t *save = lv_btn_create(page);
    lv_obj_set_size(save, 78, 40);
    lv_obj_align(save, LV_ALIGN_TOP_RIGHT, -16, 14);
    lv_obj_set_style_bg_color(save, lv_color_hex(0x174336), 0);
    lv_obj_add_event_cb(save, settings_save_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *save_label = make_label(save, "SAVE", 13, 0x42D6C3);
    lv_obj_center(save_label);

    lv_obj_t *form = lv_obj_create(page);
    settings_form = form;
    lv_obj_set_size(settings_form, 448, 390);
    lv_obj_align(settings_form, LV_ALIGN_TOP_MID, 0, 64);
    lv_obj_set_style_bg_opa(settings_form, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(settings_form, 0, 0);
    lv_obj_set_style_pad_all(settings_form, 4, 0);
    lv_obj_set_style_pad_row(settings_form, 8, 0);
    lv_obj_add_flag(settings_form, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(settings_form, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(settings_form, LV_SCROLLBAR_MODE_AUTO);

    lv_obj_t *brightness_label = make_label(form, "Brightness", 12, 0x91A3B4);
    lv_obj_set_pos(brightness_label, 4, 0);
    brightness_slider = lv_slider_create(form);
    lv_obj_set_size(brightness_slider, 330, 12);
    lv_obj_set_pos(brightness_slider, 4, 24);
    lv_slider_set_range(brightness_slider, 1, 100);
    lv_slider_set_value(brightness_slider, settings.brightness, LV_ANIM_OFF);
    lv_obj_add_event_cb(brightness_slider, settings_brightness_event, LV_EVENT_VALUE_CHANGED, NULL);
    brightness_value = make_label(form, "", 12, 0x42D6C3);
    lv_obj_set_pos(brightness_value, 350, 20);
    lv_label_set_text_fmt(brightness_value, "%u%%", settings.brightness);

    lv_obj_t *timeout_label = make_label(form, "Screen timeout (0 = off)", 12, 0x91A3B4);
    lv_obj_set_pos(timeout_label, 4, 53);
    timeout_slider = lv_slider_create(form);
    lv_obj_set_size(timeout_slider, 330, 12);
    lv_obj_set_pos(timeout_slider, 4, 78);
    lv_slider_set_range(timeout_slider, 0, 120);
    lv_slider_set_value(timeout_slider, settings.screen_timeout_minutes, LV_ANIM_OFF);
    lv_obj_add_event_cb(timeout_slider, settings_timeout_event, LV_EVENT_VALUE_CHANGED, NULL);
    timeout_value = make_label(form, "", 12, 0x42D6C3);
    lv_obj_set_pos(timeout_value, 350, 74);
    if (settings.screen_timeout_minutes == 0) {
        lv_label_set_text(timeout_value, "Off");
    } else {
        lv_label_set_text_fmt(timeout_value, "%u min", settings.screen_timeout_minutes);
    }

    char values[5][128] = {{0}};
    strlcpy(values[0], settings.browser_home, sizeof(values[0]));
    strlcpy(values[1], settings.github_owner, sizeof(values[1]));
    if (settings.github_repo_count) strlcpy(values[2], settings.github_repos[0], sizeof(values[2]));
    if (settings.pi_count) strlcpy(values[3], settings.pi_devices[0].base_url, sizeof(values[3]));
    strlcpy(values[4], settings.information_city, sizeof(values[4]));
    const char *names[5] = {"Browser home URL", "GitHub owner", "GitHub repository",
                            "Pi status URL", "Information location"};

    for (int i = 0; i < 5; i++) {
        lv_obj_t *label = make_label(form, names[i], 12, 0x91A3B4);
        lv_obj_set_pos(label, 4, 112 + i * 52);
        settings_fields[i] = lv_textarea_create(form);
        lv_obj_set_size(settings_fields[i], 424, 32);
        lv_obj_set_pos(settings_fields[i], 4, 130 + i * 52);
        lv_textarea_set_one_line(settings_fields[i], true);
        lv_textarea_set_max_length(settings_fields[i], 127);
        lv_textarea_set_text(settings_fields[i], values[i]);
        lv_obj_set_style_bg_color(settings_fields[i], lv_color_hex(0x14212D), 0);
        lv_obj_set_style_text_color(settings_fields[i], lv_color_hex(0xEDF4F8), 0);
        lv_obj_add_event_cb(settings_fields[i], settings_field_event, LV_EVENT_CLICKED, NULL);
    }

    settings_status = make_label(page, "Saved locally on this device", 11, 0x8295A7);
    lv_obj_align(settings_status, LV_ALIGN_BOTTOM_LEFT, 22, -8);
    settings_keyboard = lv_keyboard_create(page);
    lv_obj_set_size(settings_keyboard, 480, 190);
    lv_obj_align(settings_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(settings_keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(settings_keyboard, settings_keyboard_event, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(settings_keyboard, settings_keyboard_event, LV_EVENT_CANCEL, NULL);
    lv_scr_load(page);
}

static void show_home(lv_event_t *event)
{
    (void)event;
    if (f1_page != NULL) lv_obj_add_flag(f1_page, LV_OBJ_FLAG_HIDDEN);
    lv_scr_load(home);
    if (active_page != NULL && active_page != network_page &&
        active_page != pi_page && active_page != web_page &&
        active_page != github_page && active_page != f1_page) {
        lv_obj_del_async(active_page);
    }
    active_page = NULL;
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text, int size, uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, size >= 20 ? &lv_font_montserrat_24 :
                                size >= 14 ? &lv_font_montserrat_14 :
                                &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    return label;
}
static void settings_brightness_event(lv_event_t *event)
{
    (void)event;
    int brightness = (int)lv_slider_get_value(brightness_slider);
    lv_label_set_text_fmt(brightness_value, "%d%%", brightness);
    esp_event_post_to(view_event_handle, VIEW_EVENT_BASE,
                      VIEW_EVENT_BRIGHTNESS_UPDATE, &brightness,
                      sizeof(brightness), portMAX_DELAY);
}

static void settings_timeout_event(lv_event_t *event)
{
    (void)event;
    int minutes = (int)lv_slider_get_value(timeout_slider);
    if (minutes == 0) lv_label_set_text(timeout_value, "Off");
    else lv_label_set_text_fmt(timeout_value, "%d min", minutes);
}

static void settings_field_event(lv_event_t *event)
{
    if (settings_keyboard == NULL || settings_form == NULL) return;
    lv_obj_t *field = lv_event_get_target(event);

    /* Reserve the lower part of the screen for the keyboard, then scroll the
       selected field into the remaining visible area. */
    lv_obj_set_height(settings_form, 210);
    lv_keyboard_set_textarea(settings_keyboard, field);
    lv_obj_clear_flag(settings_keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(settings_keyboard);
    lv_obj_scroll_to_view(field, LV_ANIM_OFF);
}

static void settings_hide_keyboard(void)
{
    if (settings_keyboard != NULL) {
        lv_keyboard_set_textarea(settings_keyboard, NULL);
        lv_obj_add_flag(settings_keyboard, LV_OBJ_FLAG_HIDDEN);
    }
    if (settings_form != NULL) {
        lv_obj_set_height(settings_form, 390);
        lv_obj_scroll_to_y(settings_form, 0, LV_ANIM_OFF);
    }
}

static void settings_keyboard_event(lv_event_t *event)
{
    (void)event;
    settings_hide_keyboard();
}

static void settings_save_event(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    tech_settings_t settings;
    if (tech_settings_get(&settings) != ESP_OK) return;

    strlcpy(settings.browser_home, lv_textarea_get_text(settings_fields[0]),
            sizeof(settings.browser_home));
    strlcpy(settings.github_owner, lv_textarea_get_text(settings_fields[1]),
            sizeof(settings.github_owner));
    memset(settings.github_repos, 0, sizeof(settings.github_repos));
    const char *repo = lv_textarea_get_text(settings_fields[2]);
    if (repo[0]) {
        strlcpy(settings.github_repos[0], repo, sizeof(settings.github_repos[0]));
        settings.github_repo_count = 1;
    } else {
        settings.github_repo_count = 0;
    }

    memset(settings.pi_devices, 0, sizeof(settings.pi_devices));
    const char *pi_url = lv_textarea_get_text(settings_fields[3]);
    if (pi_url[0]) {
        strlcpy(settings.pi_devices[0].name, "Pi #1",
                sizeof(settings.pi_devices[0].name));
        strlcpy(settings.pi_devices[0].base_url, pi_url,
                sizeof(settings.pi_devices[0].base_url));
        settings.pi_count = 1;
    } else {
        settings.pi_count = 0;
    }
    strlcpy(settings.information_city, lv_textarea_get_text(settings_fields[4]),
            sizeof(settings.information_city));
    settings.brightness = (uint8_t)lv_slider_get_value(brightness_slider);
    settings.screen_timeout_minutes = (uint16_t)lv_slider_get_value(timeout_slider);

    esp_err_t err = tech_settings_set(&settings);
    if (err == ESP_OK) {
        struct view_data_display display = {
            .brightness = settings.brightness,
            .sleep_mode_en = settings.screen_timeout_minutes > 0,
            .sleep_mode_time_min = settings.screen_timeout_minutes
        };
        err = esp_event_post_to(view_event_handle, VIEW_EVENT_BASE,
                                VIEW_EVENT_DISPLAY_CFG_APPLY, &display,
                                sizeof(display), portMAX_DELAY);
    }
    lv_label_set_text(settings_status, err == ESP_OK ?
                      "Saved locally on this device" : "Save failed; check storage");
    lv_obj_set_style_text_color(settings_status,
        err == ESP_OK ? lv_color_hex(0x42D6C3) : lv_color_hex(0xFF6685), 0);
    settings_hide_keyboard();
}
