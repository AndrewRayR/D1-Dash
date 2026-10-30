#ifndef TECH_RECOVERY_PORTAL_H
#define TECH_RECOVERY_PORTAL_H

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

esp_err_t tech_recovery_portal_start(const char *pair_code,
                                      bool captive_dns, bool require_wifi);
void tech_recovery_portal_stop(void);
bool tech_recovery_portal_take_wifi(char *ssid, size_t ssid_cap,
                                    char *password, size_t password_cap);

#endif
