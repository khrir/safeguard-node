#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "bus/zbus_channels.h"

LOG_MODULE_REGISTER(sg_sensor, LOG_LEVEL_INF);

void sg_sensor_thread(void *a, void *b, void *c) {
    struct sg_accel dados = {0, 0, 256}; // ~1g no eixo Z - valor fixo

    while (1) {
        zbus_chan_pub(&accel_data, &dados, K_MSEC(10));
        k_sleep(K_MSEC(50));
    }
}

K_THREAD_DEFINE(sensor_tid, 1024, sg_sensor_thread, NULL, NULL, NULL, 7, 0, 0);