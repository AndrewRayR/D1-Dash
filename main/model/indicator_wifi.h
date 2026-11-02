#ifndef INDICATOR_WIFI_H
#define INDICATOR_WIFI_H

#include "config.h"
#include "view_data.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

int indicator_wifi_init(void);
esp_err_t indicator_wifi_disconnect_user(void);

#ifdef __cplusplus
}
#endif

#endif
