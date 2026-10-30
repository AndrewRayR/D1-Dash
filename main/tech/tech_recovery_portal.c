#include "tech_recovery_portal.h"
#include "tech_settings.h"
#include "../config.h"
#include "../view_data.h"

#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"

#define PORTAL_IP "192.168.4.1"
#define PORTAL_DNS_PORT 53
#define PORTAL_BODY_MAX 1024

static const char *TAG = "recovery_portal";
static httpd_handle_t s_server;
static TaskHandle_t s_dns_task;
static volatile bool s_dns_running;
static char s_pair_code[7];
static char s_session[17];
static int64_t s_session_expiry_us;
static int64_t s_login_window_start_us;
static int64_t s_login_lockout_until_us;
static uint8_t s_login_failures;
static bool s_require_wifi;
static SemaphoreHandle_t s_pending_mutex;
static bool s_pending_wifi;
static char s_pending_ssid[33];
static char s_pending_password[65];

static const char LOGIN_PAGE[] =
"<!doctype html><html lang='en'><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
"<meta name='theme-color' content='#0b1420'><title>D1 Dash | Sign in</title>"
"<style>*{box-sizing:border-box}body{font:16px/1.5 system-ui,-apple-system,sans-serif;"
"min-height:100vh;margin:0;padding:28px 18px;display:grid;place-items:center;"
"background:radial-gradient(ellipse at 50% 0%,#173447 0%,#0b1420 62%);color:#edf5fb}"
".card{width:100%;max-width:390px;padding:30px 26px;background:#111f2d;border:1px solid #294052;"
"border-radius:22px;box-shadow:0 24px 70px #0008}.mark{width:48px;height:48px;border-radius:14px;"
"display:grid;place-items:center;background:#123b3d;color:#55e0cc;font-size:25px;font-weight:800}"
"h1{font-size:25px;margin:20px 0 4px;letter-spacing:-.5px}.sub{color:#9db0c0;margin:0 0 26px}"
"label{display:block;font-size:13px;font-weight:650;color:#c5d3df;margin-bottom:10px}"
".pin{width:100%;height:62px;border:1px solid #496174;border-radius:13px;background:#091522;"
"color:#f3fbff;text-align:center;font:700 27px/1.2 ui-monospace,monospace;letter-spacing:13px;"
"padding:10px 12px 10px 24px;outline:none}.pin:focus{border-color:#50d9c6;box-shadow:0 0 0 3px #50d9c622}"
"button{width:100%;margin-top:18px;padding:15px;border:0;border-radius:12px;background:#4bd9c5;"
"color:#06201e;font-size:16px;font-weight:750;cursor:pointer}button:active{transform:translateY(1px)}"
".hint{font-size:12px;color:#8298aa;text-align:center;margin:18px 0 0}"
".error{padding:10px 12px;border-radius:10px;background:#4a252b;color:#ffc2c7;font-size:13px;margin:0 0 16px}"
"</style></head><body><main class='card'><div class='mark'>D</div><h1>Sign in to D1 Dash</h1>"
"<p class='sub'>Enter the 6-digit access code displayed on your device.</p>"
"<form method='post' action='/login'><label for='code'>DEVICE ACCESS CODE</label>"
"<input class='pin' id='code' name='code' type='text' inputmode='numeric' pattern='[0-9]{6}' "
"autocomplete='one-time-code' maxlength='6' minlength='6' placeholder='••••••' "
"aria-label='Six digit access code' required autofocus>"
"<button type='submit'>Unlock device</button></form>"
"<p class='hint'>Your code stays on your D1 screen. Access expires after inactivity.</p>"
"</main><script>const p=document.getElementById('code');p.addEventListener('input',()=>{"
"p.value=p.value.replace(/\\D/g,'').slice(0,6)});</script></body></html>";

static const char SETUP_PAGE[] =
"<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>D1 Dash Configuration</title><style>body{font:16px system-ui;background:#0b1118;color:#eaf2f8;"
"max-width:480px;margin:5vh auto;padding:20px}section{background:#111c27;padding:22px;border-radius:16px}"
"input,button{box-sizing:border-box;width:100%;padding:13px;margin:8px 0;border-radius:9px;border:1px solid #405366}"
"input{background:#09121b;color:white}button{background:#42d6c3;color:#061318;font-weight:700}</style></head>"
"<body><section><h1>D1 Dash</h1><p>Device configuration</p><form method='post' action='/wifi'>"
"<h2>Wi-Fi</h2><label>Home Wi-Fi network name (leave blank to keep current connection)</label><input name='ssid' maxlength='32'>"
"<label>Home Wi-Fi password (only needed when changing networks)</label><input name='password' type='password' maxlength='64'>"
"<h2>Device settings</h2><p>Leave a field blank to keep its current value.</p>"
"<label>Screen brightness (1-100)</label><input name='brightness' type='number' min='1' max='100'>"
"<label>Screen timeout in minutes (0-120; 0 disables)</label><input name='timeout' type='number' min='0' max='120'>"
"<label>Refresh interval in minutes (1-60)</label><input name='refresh' type='number' min='1' max='60'>"
"<label>Browser home URL</label><input name='browser_home' maxlength='127'>"
"<label>GitHub owner</label><input name='github_owner' maxlength='63'>"
"<label>GitHub repository</label><input name='github_repo' maxlength='63'>"
"<label>Raspberry Pi status URL</label><input name='pi_url' maxlength='127'>"
"<label>Information location</label><input name='information_city' maxlength='47'>"
"<button>Save settings</button></form><p>Settings are saved directly on your D1. If you change Wi-Fi, the device will reconnect automatically.</p></section></body></html>";

static bool has_session(httpd_req_t *req)
{
    int64_t now = esp_timer_get_time();
    if (s_session[0] == '\\0' || now > s_session_expiry_us) return false;
    char cookie[96] = {0};
    if (httpd_req_get_hdr_value_str(req, "Cookie", cookie, sizeof(cookie)) != ESP_OK) return false;
    char expected[40];
    snprintf(expected, sizeof(expected), "d1session=%s", s_session);
    bool valid = strstr(cookie, expected) != NULL;
    if (valid) s_session_expiry_us = now + 10LL * 60LL * 1000000LL;
    return valid;
}

static int hex_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static void form_value(const char *body, const char *key, char *out, size_t cap)
{
    size_t key_len = strlen(key);
    const char *p = body;
    out[0] = '\0';
    while (*p) {
        const char *eq = strchr(p, '=');
        if (!eq) break;
        const char *end = strchr(eq + 1, '&');
        if (!end) end = p + strlen(p);
        if ((size_t)(eq - p) == key_len && strncmp(p, key, key_len) == 0) {
            size_t n = 0;
            for (const char *q = eq + 1; q < end && n + 1 < cap; q++) {
                if (*q == '+') out[n++] = ' ';
                else if (*q == '%' && q + 2 < end) {
                    int hi = hex_value(q[1]), lo = hex_value(q[2]);
                    if (hi >= 0 && lo >= 0) { out[n++] = (char)((hi << 4) | lo); q += 2; }
                    else out[n++] = *q;
                } else out[n++] = *q;
            }
            out[n] = '\0';
            return;
        }
        p = (*end == '&') ? end + 1 : end;
    }
}

static int optional_number(const char *body, const char *key,
                            long minimum, long maximum, long *value)
{
    char raw[24] = {0};
    form_value(body, key, raw, sizeof(raw));
    if (raw[0] == '\0') return 0;
    char *end = NULL;
    long parsed = strtol(raw, &end, 10);
    if (end == raw || *end != '\0' || parsed < minimum || parsed > maximum) return -1;
    *value = parsed;
    return 1;
}

static bool valid_http_url(const char *url)
{
    return url != NULL &&
        (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0) &&
        url[strncmp(url, "https://", 8) == 0 ? 8 : 7] != '\0';
}

static bool valid_identifier(const char *value, size_t maximum)
{
    size_t length = value ? strlen(value) : 0;
    if (length == 0 || length > maximum) return false;
    for (size_t i = 0; i < length; i++) {
        unsigned char c = (unsigned char)value[i];
        if (!isalnum(c) && c != '-' && c != '_' && c != '.') return false;
    }
    return true;
}

static esp_err_t portal_get_handler(httpd_req_t *req)
{
    if (has_session(req)) {
        httpd_resp_set_type(req, "text/html; charset=utf-8");
        return httpd_resp_sendstr(req, SETUP_PAGE);
    }
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_sendstr(req, LOGIN_PAGE);
}

static esp_err_t portal_post_handler(httpd_req_t *req)
{
    char body[PORTAL_BODY_MAX + 1] = {0};
    if (req->content_len <= 0 || req->content_len > PORTAL_BODY_MAX) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid request body");
        return ESP_FAIL;
    }
    int received = 0;
    while (received < req->content_len) {
        int chunk = httpd_req_recv(req, body + received, req->content_len - received);
        if (chunk <= 0) break;
        received += chunk;
    }
    if (received != req->content_len) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Request body unavailable");
        return ESP_FAIL;
    }
    body[received] = '\0';

    if (strcmp(req->uri, "/login") == 0) {
        int64_t now = esp_timer_get_time();
        if (now < s_login_lockout_until_us) {
            httpd_resp_set_status(req, "429 Too Many Requests");
            return httpd_resp_sendstr(req, "Too many attempts. Wait one minute and try again.");
        }
        if (now - s_login_window_start_us > 60LL * 1000000LL) {
            s_login_window_start_us = now;
            s_login_failures = 0;
        }
        char code[16] = {0};
        form_value(body, "code", code, sizeof(code));
        if (strcmp(code, s_pair_code) != 0) {
            s_login_failures++;
            if (s_login_failures >= 5) {
                s_login_lockout_until_us = now + 60LL * 1000000LL;
                s_login_failures = 0;
                httpd_resp_set_status(req, "429 Too Many Requests");
                return httpd_resp_sendstr(req, "Too many attempts. Wait one minute and try again.");
            }
            httpd_resp_set_status(req, "401 Unauthorized");
            httpd_resp_set_type(req, "text/html; charset=utf-8");
            return httpd_resp_sendstr(req, LOGIN_PAGE);
        }
        s_login_failures = 0;
        s_session_expiry_us = now + 10LL * 60LL * 1000000LL;
        snprintf(s_session, sizeof(s_session), "%08lx%08lx",
                 (unsigned long)esp_random(), (unsigned long)esp_random());
        char cookie[64];
        snprintf(cookie, sizeof(cookie), "d1session=%s; HttpOnly; SameSite=Strict; Path=/", s_session);
        httpd_resp_set_hdr(req, "Set-Cookie", cookie);
        httpd_resp_set_status(req, "303 See Other");
        httpd_resp_set_hdr(req, "Location", "/");
        return httpd_resp_send(req, NULL, 0);
    }

    if (strcmp(req->uri, "/wifi") == 0) {
        if (!has_session(req)) {
            httpd_resp_set_status(req, "401 Unauthorized");
            return httpd_resp_sendstr(req, "Sign-in required");
        }
        char ssid[33] = {0}, password[65] = {0};
        form_value(body, "ssid", ssid, sizeof(ssid));
        form_value(body, "password", password, sizeof(password));
        if ((s_require_wifi && ssid[0] == '\0') || strlen(ssid) > 32) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                                s_require_wifi ? "A Wi-Fi name is required in recovery mode" :
                                                 "Wi-Fi name is too long");
            return ESP_FAIL;
        }
        if (!s_require_wifi && ssid[0] != '\0') {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                                "Change Wi-Fi credentials in recovery mode, not LAN configuration mode");
            return ESP_FAIL;
        }

        tech_settings_t settings;
        if (tech_settings_get(&settings) != ESP_OK) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Settings are unavailable");
            return ESP_FAIL;
        }

        long value = 0;
        int brightness_status = optional_number(body, "brightness", 1, 100, &value);
        if (brightness_status < 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Brightness must be 1-100");
            return ESP_FAIL;
        }
        bool brightness_changed = brightness_status > 0;
        if (brightness_changed) settings.brightness = (uint8_t)value;

        int timeout_status = optional_number(body, "timeout", 0, 120, &value);
        if (timeout_status < 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Screen timeout must be 0-120 minutes");
            return ESP_FAIL;
        }
        if (timeout_status > 0) settings.screen_timeout_minutes = (uint16_t)value;

        int refresh_status = optional_number(body, "refresh", 1, 60, &value);
        if (refresh_status < 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Refresh interval must be 1-60 minutes");
            return ESP_FAIL;
        }
        if (refresh_status > 0) settings.refresh_seconds = (uint16_t)(value * 60);

        char field[128] = {0};
        form_value(body, "browser_home", field, sizeof(field));
        if (field[0]) {
            if (!valid_http_url(field)) {
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Browser home must be an HTTP or HTTPS URL");
                return ESP_FAIL;
            }
            strlcpy(settings.browser_home, field, sizeof(settings.browser_home));
        }

        form_value(body, "github_owner", field, sizeof(field));
        if (field[0]) {
            if (!valid_identifier(field, 63)) {
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "GitHub owner contains invalid characters");
                return ESP_FAIL;
            }
            strlcpy(settings.github_owner, field, sizeof(settings.github_owner));
        }

        form_value(body, "github_repo", field, sizeof(field));
        if (field[0]) {
            if (!valid_identifier(field, 63)) {
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "GitHub repository contains invalid characters");
                return ESP_FAIL;
            }
            strlcpy(settings.github_repos[0], field, sizeof(settings.github_repos[0]));
            if (settings.github_repo_count == 0) settings.github_repo_count = 1;
        }

        form_value(body, "pi_url", field, sizeof(field));
        if (field[0]) {
            if (!valid_http_url(field)) {
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Pi status URL must be an HTTP or HTTPS URL");
                return ESP_FAIL;
            }
            strlcpy(settings.pi_devices[0].base_url, field, sizeof(settings.pi_devices[0].base_url));
            if (settings.pi_devices[0].name[0] == '\0') {
                strlcpy(settings.pi_devices[0].name, "Raspberry Pi", sizeof(settings.pi_devices[0].name));
            }
            if (settings.pi_count == 0) settings.pi_count = 1;
        }

        form_value(body, "information_city", field, sizeof(field));
        if (field[0]) strlcpy(settings.information_city, field, sizeof(settings.information_city));

        esp_err_t settings_err = tech_settings_set(&settings);
        if (settings_err != ESP_OK) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not save device settings");
            return ESP_FAIL;
        }
        if (brightness_changed) {
            int brightness_value = settings.brightness;
            esp_event_post_to(view_event_handle, VIEW_EVENT_BASE, VIEW_EVENT_BRIGHTNESS_UPDATE,
                              &brightness_value, sizeof(brightness_value), portMAX_DELAY);
        }
        esp_event_post_to(view_event_handle, VIEW_EVENT_BASE, VIEW_EVENT_REMOTE_SETTINGS_UPDATED,
                          NULL, 0, portMAX_DELAY);

        if (ssid[0] != '\0') {
            xSemaphoreTake(s_pending_mutex, portMAX_DELAY);
            strlcpy(s_pending_ssid, ssid, sizeof(s_pending_ssid));
            strlcpy(s_pending_password, password, sizeof(s_pending_password));
            s_pending_wifi = true;
            xSemaphoreGive(s_pending_mutex);
        }
        httpd_resp_set_type(req, "text/html; charset=utf-8");
        return httpd_resp_sendstr(req,
            "<!doctype html><meta name='viewport' content='width=device-width,initial-scale=1'>"
            "<body style='font:18px system-ui;background:#0b1118;color:white;padding:30px'>"
            "<h2>Settings saved</h2><p>Your D1 settings have been saved. You can close this page.</p></body>");
    }

    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
    return ESP_FAIL;
}

static void dns_server_task(void *arg)
{
    (void)arg;
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) { s_dns_running = false; s_dns_task = NULL; vTaskDelete(NULL); return; }
    struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = htons(PORTAL_DNS_PORT),
                               .sin_addr.s_addr = htonl(INADDR_ANY)};
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock); s_dns_running = false; s_dns_task = NULL; vTaskDelete(NULL); return;
    }
    uint8_t packet[512];
    while (s_dns_running) {
        struct sockaddr_in peer;
        socklen_t peer_len = sizeof(peer);
        int len = recvfrom(sock, packet, sizeof(packet), 0, (struct sockaddr *)&peer, &peer_len);
        if (len < 17) continue;
        size_t qend = 12;
        while (qend < (size_t)len && packet[qend] != 0) {
            uint8_t label = packet[qend++];
            if ((label & 0xC0) || qend + label >= (size_t)len) { qend = 0; break; }
            qend += label;
        }
        if (!qend || qend + 5 > (size_t)len) continue;
        qend += 5; /* root terminator, QTYPE, QCLASS */
        if (qend + 16 > sizeof(packet)) continue;
        packet[2] = 0x81; packet[3] = 0x80;
        packet[4] = 0; packet[5] = 1;
        packet[6] = 0; packet[7] = 1;
        packet[8] = 0; packet[9] = 0; packet[10] = 0; packet[11] = 0;
        packet[qend++] = 0xC0; packet[qend++] = 0x0C;
        packet[qend++] = 0; packet[qend++] = 1;
        packet[qend++] = 0; packet[qend++] = 1;
        packet[qend++] = 0; packet[qend++] = 0; packet[qend++] = 0; packet[qend++] = 30;
        packet[qend++] = 0; packet[qend++] = 4;
        packet[qend++] = 192; packet[qend++] = 168; packet[qend++] = 4; packet[qend++] = 1;
        sendto(sock, packet, qend, 0, (struct sockaddr *)&peer, peer_len);
    }
    close(sock);
    s_dns_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t tech_recovery_portal_start(const char *pair_code,
                                      bool captive_dns, bool require_wifi)
{
    if (pair_code == NULL || strlen(pair_code) != 6) return ESP_ERR_INVALID_ARG;
    if (s_server != NULL) return ESP_OK;
    if (s_pending_mutex == NULL) s_pending_mutex = xSemaphoreCreateMutex();
    if (s_pending_mutex == NULL) return ESP_ERR_NO_MEM;
    xSemaphoreTake(s_pending_mutex, portMAX_DELAY);
    s_pending_wifi = false;
    s_pending_ssid[0] = '\0';
    s_pending_password[0] = '\0';
    xSemaphoreGive(s_pending_mutex);
    strlcpy(s_pair_code, pair_code, sizeof(s_pair_code));
    s_require_wifi = require_wifi;
    s_session[0] = '\0';
    s_session_expiry_us = 0;
    s_login_window_start_us = esp_timer_get_time();
    s_login_lockout_until_us = 0;
    s_login_failures = 0;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_uri_handlers = 8;
    config.stack_size = 6144;
    config.uri_match_fn = httpd_uri_match_wildcard;
    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) return err;
    httpd_uri_t get_uri = {.uri = "/*", .method = HTTP_GET, .handler = portal_get_handler};
    httpd_uri_t post_uri = {.uri = "/*", .method = HTTP_POST, .handler = portal_post_handler};
    err = httpd_register_uri_handler(s_server, &get_uri);
    if (err == ESP_OK) err = httpd_register_uri_handler(s_server, &post_uri);
    if (err != ESP_OK) {
        httpd_stop(s_server); s_server = NULL; return err;
    }
    if (captive_dns) {
        s_dns_running = true;
        if (xTaskCreate(dns_server_task, "portal_dns", 3072, NULL, 4, &s_dns_task) != pdPASS) {
            s_dns_running = false; httpd_stop(s_server); s_server = NULL; return ESP_ERR_NO_MEM;
        }
    }
    ESP_LOGI(TAG, "Configuration portal started (captive DNS=%s)", captive_dns ? "yes" : "no");
    return ESP_OK;
}

void tech_recovery_portal_stop(void)
{
    s_dns_running = false;
    if (s_server != NULL) { httpd_stop(s_server); s_server = NULL; }
    s_pair_code[0] = '\0';
    s_session[0] = '\0';
    s_session_expiry_us = 0;
    if (s_pending_mutex != NULL) {
        xSemaphoreTake(s_pending_mutex, portMAX_DELAY);
        s_pending_wifi = false;
        s_pending_ssid[0] = '\0';
        s_pending_password[0] = '\0';
        xSemaphoreGive(s_pending_mutex);
    }
}

bool tech_recovery_portal_take_wifi(char *ssid, size_t ssid_cap,
                                    char *password, size_t password_cap)
{
    if (s_pending_mutex == NULL || ssid == NULL || password == NULL) return false;
    bool ready = false;
    xSemaphoreTake(s_pending_mutex, portMAX_DELAY);
    if (s_pending_wifi) {
        strlcpy(ssid, s_pending_ssid, ssid_cap);
        strlcpy(password, s_pending_password, password_cap);
        s_pending_wifi = false;
        s_pending_ssid[0] = '\0';
        s_pending_password[0] = '\0';
        ready = true;
    }
    xSemaphoreGive(s_pending_mutex);
    return ready;
}
