#ifndef TECH_SPEED_TEST_H
#define TECH_SPEED_TEST_H

#include "esp_err.h"

typedef struct {
    float download_mbps;
    float upload_mbps;
} tech_speed_test_result_t;

esp_err_t tech_speed_test_run(tech_speed_test_result_t *result);

#endif
