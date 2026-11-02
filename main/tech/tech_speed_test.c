#include "tech_speed_test.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"

#define SPEED_DOWNLOAD_BYTES 500000
#define SPEED_UPLOAD_BYTES 200000
#define SPEED_TIMEOUT_MS 20000

typedef struct {
    size_t received;
} speed_context_t;

static esp_err_t speed_event_handler(esp_http_client_event_t *event)
{
    speed_context_t *context = event->user_data;
    if (event->event_id == HTTP_EVENT_ON_DATA && event->data_len > 0) {
        context->received += (size_t)event->data_len;
    }
    return ESP_OK;
}

static esp_err_t speed_request(const char *url, const uint8_t *post_data,
                              size_t post_length, size_t *transferred,
                              int64_t *elapsed_us)
{
    speed_context_t context = {0};
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = speed_event_handler,
        .user_data = &context,
        .timeout_ms = SPEED_TIMEOUT_MS,
        .buffer_size = 1024,
        .buffer_size_tx = 1024,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .disable_auto_redirect = false,
        .max_redirection_count = 2,
        .keep_alive_enable = false
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) return ESP_ERR_NO_MEM;

    if (post_data != NULL) {
        esp_http_client_set_method(client, HTTP_METHOD_POST);
        esp_http_client_set_header(client, "Content-Type", "application/octet-stream");
        esp_http_client_set_post_field(client, (const char *)post_data, (int)post_length);
    }

    int64_t start = esp_timer_get_time();
    esp_err_t err = esp_http_client_perform(client);
    *elapsed_us = esp_timer_get_time() - start;
    int status = esp_http_client_get_status_code(client);
    if (err == ESP_OK && (status < 200 || status >= 300)) err = ESP_FAIL;
    *transferred = post_data != NULL ? post_length : context.received;
    esp_http_client_cleanup(client);
    if (err == ESP_OK && *transferred == 0) err = ESP_FAIL;
    return err;
}

esp_err_t tech_speed_test_run(tech_speed_test_result_t *result)
{
    if (result == NULL) return ESP_ERR_INVALID_ARG;
    memset(result, 0, sizeof(*result));

    size_t transferred = 0;
    int64_t elapsed = 0;
    esp_err_t err = speed_request(
        "https://speed.cloudflare.com/__down?bytes=500000",
        NULL, 0, &transferred, &elapsed);
    if (err != ESP_OK || elapsed <= 0) return err != ESP_OK ? err : ESP_FAIL;
    result->download_mbps = ((float)transferred * 8.0f) / (float)elapsed;

    uint8_t *upload = heap_caps_malloc(SPEED_UPLOAD_BYTES,
                                      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (upload == NULL) upload = malloc(SPEED_UPLOAD_BYTES);
    if (upload == NULL) return ESP_ERR_NO_MEM;
    memset(upload, 0x5A, SPEED_UPLOAD_BYTES);
    err = speed_request("https://speed.cloudflare.com/__up",
                        upload, SPEED_UPLOAD_BYTES, &transferred, &elapsed);
    free(upload);
    if (err != ESP_OK || elapsed <= 0) return err != ESP_OK ? err : ESP_FAIL;
    result->upload_mbps = ((float)transferred * 8.0f) / (float)elapsed;
    return ESP_OK;
}
